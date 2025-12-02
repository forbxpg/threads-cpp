/*
M25-514 Kirgizov Temurmalik Kutbiddin ugli.

Task 8. Collective MPI impl.
*/
#include <cstdint>
#include <iostream>
#include <mpi.h>
#include <vector>

using namespace std;

const int A = 514;
const int B = 8;
const long ARR_SIZE = 1000000000;
const unsigned int SEED = 2025;

unsigned int global_seed;

int calculate_processes_count(int A, int B) {
  int X = 2 * A + B;
  return (X % 5) + 2;
}

int calculate_distribution_type(int A, int B) {
  int X = 2 * A + B;
  return X % 4;
}

void my_srand(unsigned int seed_value) { global_seed = seed_value; }

unsigned int my_rand() {
  global_seed = 214013u * global_seed + 2531011u;
  return (global_seed >> 16) & 0x7FFFu;
}

bool is_local_maximum(const unsigned char *data, long index, long size) {
  if (index == 0) {
    return (size == 1) || (data[0] >= data[1]);
  } else if (index == size - 1) {
    return data[size - 1] >= data[size - 2];
  } else {
    return (data[index] >= data[index - 1]) && (data[index] >= data[index + 1]);
  }
}

unsigned long long count_local_maximum(const unsigned char *data,
                                       int process_id, int total_processes,
                                       int distribution_type, long total_size) {
  unsigned long long count = 0;

  auto calculate_block_start = [&](int rank) -> long {
    return static_cast<long>(
        (static_cast<unsigned long long>(rank) * total_size) / total_processes);
  };

  auto calculate_block_end = [&](int rank) -> long {
    return static_cast<long>(
        (static_cast<unsigned long long>(rank + 1) * total_size) /
        total_processes);
  };

  if (distribution_type == 0) {
    // left -> right
    long start = calculate_block_start(process_id);
    long end = calculate_block_end(process_id);

    for (long i = start; i < end; i++) {
      if (is_local_maximum(data, i, total_size)) {
        count++;
      }
    }
  } else if (distribution_type == 1) {
    // right -> left
    long start =
        total_size - calculate_block_end(total_processes - 1 - process_id);
    long end =
        total_size - calculate_block_start(total_processes - 1 - process_id);

    for (long i = start; i < end; i++) {
      if (is_local_maximum(data, i, total_size)) {
        count++;
      }
    }
  } else if (distribution_type == 2) {
    // cyclic
    for (long i = process_id; i < total_size; i += total_processes) {
      if (is_local_maximum(data, i, total_size)) {
        count++;
      }
    }
  } else {
    // reverse cyclic
    for (long i = total_size - 1 - process_id; i >= 0; i -= total_processes) {
      if (is_local_maximum(data, i, total_size)) {
        count++;
      }
    }
  }

  return count;
}

void generate_local_data(unsigned char *buffer, long size) {
  my_srand(SEED);
  for (long i = 0; i < size; i++) {
    buffer[i] = static_cast<unsigned char>(my_rand() & 0xFF);
  }
}

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int process_rank, total_processes;
  MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &total_processes);

  int required_processes = calculate_processes_count(A, B);
  int distribution_type = calculate_distribution_type(A, B);

  MPI_Bcast(&distribution_type, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&required_processes, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (process_rank == 0) {
    cout << "A = " << A << ", B = " << B << endl;
    cout << "Required processes = " << required_processes << endl;
    cout << "Distribution type = " << distribution_type << endl;
    cout << "Array size = " << ARR_SIZE << endl;
  }

  if (total_processes != required_processes) {
    if (process_rank == 0) {
      cerr << "Error: This program requires exactly " << required_processes
           << " processes!" << endl;
    }
    MPI_Finalize();
    return 1;
  }

  vector<unsigned char> data(ARR_SIZE);
  generate_local_data(data.data(), ARR_SIZE);

  MPI_Barrier(MPI_COMM_WORLD);

  unsigned long long local_maximum_count = count_local_maximum(
      data.data(), process_rank, total_processes, distribution_type, ARR_SIZE);

  vector<unsigned long long> individual_maximum_counts;
  if (process_rank == 0) {
    individual_maximum_counts.resize(total_processes);
  }

  MPI_Gather(&local_maximum_count, 1, MPI_UNSIGNED_LONG_LONG,
             individual_maximum_counts.data(), 1, MPI_UNSIGNED_LONG_LONG, 0,
             MPI_COMM_WORLD);

  unsigned long long total_maximum_count = 0;
  MPI_Reduce(&local_maximum_count, &total_maximum_count, 1,
             MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

  unsigned long long global_maximum_count = 0;
  MPI_Allreduce(&local_maximum_count, &global_maximum_count, 1,
                MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);

  if (process_rank == 0) {
    cout << "process counts:";
    for (int i = 0; i < total_processes; i++) {
      cout << " " << individual_maximum_counts[i];
    }

    cout << endl;
    cout << " Reduce: " << total_maximum_count;
    cout << endl;
    cout << "All_Reduce: " << global_maximum_count << endl;
  }

  MPI_Finalize();
  return 0;
}