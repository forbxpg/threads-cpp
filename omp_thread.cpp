/*
M25-514 Kirgizov Temurmalik Kutbiddin ugli.

Task 3. OpenMP thread impl.
*/

#include <fstream>
#include <iostream>
#include <omp.h>
#include <vector>

using namespace std;

int calculate_threads_count(int A, int B) {
  int X = 2 * A + B;
  return X % 5 + 2;
}

unsigned int global_seed;

void my_srand(unsigned int a) { global_seed = a; }

unsigned int my_rand() {
  global_seed = 214013 * global_seed + 2531011;
  return (global_seed >> 16) & 0x7fff;
}

void generate_binarnik(const string &filename, long size) {
  cout << "Generating binary file..." << endl;

  ofstream file(filename, ios::binary);

  my_srand(2025);

  const int BUFFER = 1024 * 1024;
  vector<unsigned char> buffer(BUFFER);

  long remaining = size;
  while (remaining > 0) {

    int to_write = (remaining > BUFFER) ? BUFFER : int(remaining);
    for (int i = 0; i < to_write; i++) {
      buffer[i] = static_cast<unsigned char>(my_rand() & 0xff);
    }
    file.write(reinterpret_cast<char *>(buffer.data()), to_write);
    remaining -= to_write;
  }
  cout << "File generated." << endl;
  file.close();
}

void calculate_local_maximum(unsigned char *mas, int thread_id,
                             int threads_count, long arr_size,
                             long long *results) {
  long long local_maximum_count = 0;

  // Dividing the array between threads
  long long batch_size = arr_size / threads_count;
  long long start = thread_id * batch_size;
  long long end = (thread_id == threads_count - 1)
                      ? arr_size
                      : (thread_id + 1) * batch_size; // If the last thread,
                                                      // take the rest of the
                                                      // array
                                                      // Otherwise, take the
                                                      // next batch of the array
  #pragma omp parallel num_threads(threads_count) reduction(+:local_maximum_count)
  for (long long i = start; i < end; i++) {
    if (i == 0) {

      // First element of the array (can be processed only by the first thread)
      if (thread_id == 0 && mas[0] >= mas[1]) {
        local_maximum_count++;
      }
    } else if (i == arr_size - 1) {
      // Last element of the array (can be processed only by the last thread)
      if (thread_id == threads_count - 1 &&
          mas[arr_size - 1] >= mas[arr_size - 2]) {
        local_maximum_count++;
      }
    } else {
      // Ordinary elements (check the boundaries of our part)
      bool is_local_max = true;

      if (i > start) {
        is_local_max = is_local_max && (mas[i] >= mas[i - 1]);
      }

      if (i < end - 1) {
        is_local_max = is_local_max && (mas[i] >= mas[i + 1]);
      } else if (i == end - 1 && thread_id < threads_count - 1) {
        is_local_max = false;
      }

      if (is_local_max) {
        local_maximum_count++;
      }
    }
  }

  results[thread_id] = local_maximum_count;
}

int main() {
  const int A = 514;
  const int B = 8;
  const long ARR_SIZE = 1000000000;
  const string FILENAME = "data.bin";

  int threads_count = calculate_threads_count(A, B);

  generate_binarnik(FILENAME, ARR_SIZE);

  unsigned char *mas = new unsigned char[ARR_SIZE];

  ifstream file(FILENAME, ios::binary);
  file.read(reinterpret_cast<char *>(mas), ARR_SIZE);
  file.close();

  cout << "Calculating local maximum using OpenMP with " << threads_count
       << " threads" << endl;

  vector<long long> thread_results(threads_count, 0);

  for (int i = 0; i < threads_count; i++) {
    calculate_local_maximum(mas, i, threads_count, ARR_SIZE,
                            thread_results.data());
  }

  long long total_maximum = 0;
  for (int i = 0; i < threads_count; i++) {
    cout << "Thread " << i + 1 << " counted " << thread_results[i]
         << " local maximum" << endl;
    total_maximum += thread_results[i];
  }

  cout << "Total local maximum count (OpenMP): " << total_maximum << endl;
}