/*
M25-514 Kirgizov Temurmalik Kutbiddin ugli.

Task 7. OpenMPI impl. (MPI_2)
*/

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mpi.h>
#include <vector>

using namespace std;

const int A = 514;
const int B = 8;
const long ARR_SIZE = 1000000000;
const unsigned int SEED = 2025;

unsigned int global_seed;

int calculate_processes_count(int A, int B)
{
  int X = 2 * A + B;
  return (X % 5) + 2;
}

int calculate_distribution_type(int A, int B)
{
  int X = 2 * A + B;
  return X % 4;
}

void my_srand(unsigned int seed_value) { global_seed = seed_value; }

unsigned int my_rand()
{
  global_seed = 214013u * global_seed + 2531011u;
  return (global_seed >> 16) & 0x7fffu;
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  int process_rank, total_processes;
  MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &total_processes);

  int required_processes = calculate_processes_count(A, B);
  int distribution_type = calculate_distribution_type(A, B);

  if (process_rank == 0)
  {
    cout << "X = " << (2 * A + B) << ", order = " << distribution_type
         << ", procs = " << total_processes << endl;
  }

  if (total_processes != required_processes)
  {
    if (process_rank == 0)
    {
      cerr << "Error: run with -np " << required_processes << " processes.\n";
    }
    MPI_Finalize();
    return 1;
  }

  // Только процесс 0 генерирует полный массив
  unsigned char *mas = nullptr;

  if (process_rank == 0)
  {
    mas = new unsigned char[ARR_SIZE];
    my_srand(SEED);
    for (long i = 0; i < ARR_SIZE; i++)
    {
      mas[i] = static_cast<unsigned char>(my_rand() & 0xFF);
    }
  }

  // Вычисляем границы для каждого процесса
  long chunk_size = ARR_SIZE / total_processes;
  long remainder = ARR_SIZE % total_processes;

  long start, local_size;
  if (process_rank < remainder)
  {
    local_size = chunk_size + 1;
    start = process_rank * local_size;
  }
  else
  {
    local_size = chunk_size;
    start = remainder * (chunk_size + 1) + (process_rank - remainder) * chunk_size;
  }

  // Выделяем память для локальных данных + ghost cells
  unsigned char *local_data = new unsigned char[local_size + 2];

  // Процесс 0 раздает данные
  if (process_rank == 0)
  {
    // Копируем свою часть
    for (long i = 0; i < local_size; i++)
      local_data[i + 1] = mas[i];

    // Отправляем данные остальным процессам
    long offset = local_size;
    for (int proc = 1; proc < total_processes; proc++)
    {
      long proc_chunk = (proc < remainder) ? chunk_size + 1 : chunk_size;
      MPI_Send(&mas[offset], proc_chunk, MPI_UNSIGNED_CHAR,
               proc, 0, MPI_COMM_WORLD);
      offset += proc_chunk;
    }

    delete[] mas;
  }
  else
  {
    // Получаем свою часть от процесса 0
    MPI_Recv(&local_data[1], local_size, MPI_UNSIGNED_CHAR,
             0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  // Обмен граничными элементами с левым соседом
  if (process_rank > 0)
  {
    unsigned char my_first = local_data[1];
    unsigned char left_ghost;
    MPI_Sendrecv(&my_first, 1, MPI_UNSIGNED_CHAR, process_rank - 1, 0,
                 &left_ghost, 1, MPI_UNSIGNED_CHAR, process_rank - 1, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    local_data[0] = left_ghost;
  }
  else
  {
    local_data[0] = 0; // У процесса 0 нет левого соседа
  }

  // Обмен граничными элементами с правым соседом
  if (process_rank < total_processes - 1)
  {
    unsigned char my_last = local_data[local_size];
    unsigned char right_ghost;
    MPI_Sendrecv(&my_last, 1, MPI_UNSIGNED_CHAR, process_rank + 1, 0,
                 &right_ghost, 1, MPI_UNSIGNED_CHAR, process_rank + 1, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    local_data[local_size + 1] = right_ghost;
  }
  else
  {
    local_data[local_size + 1] = 0; // У последнего процесса нет правого соседа
  }

  // Подсчет локальных максимумов
  unsigned long long local_count = 0;

  for (long i = 1; i <= local_size; i++)
  {
    long global_i = start + (i - 1);

    // Проверяем граничные случаи
    if (global_i == 0)
    {
      // Первый элемент массива
      if (ARR_SIZE > 1 && local_data[i] >= local_data[i + 1])
        local_count++;
      else if (ARR_SIZE == 1)
        local_count++;
    }
    else if (global_i == ARR_SIZE - 1)
    {
      // Последний элемент массива
      if (local_data[i] >= local_data[i - 1])
        local_count++;
    }
    else
    {
      // Внутренний элемент
      if (local_data[i] >= local_data[i - 1] &&
          local_data[i] >= local_data[i + 1])
        local_count++;
    }
  }

  // Сбор результатов
  if (process_rank == 0)
  {
    vector<unsigned long long> all_counts(total_processes);
    all_counts[0] = local_count;

    // Получаем результаты от остальных процессов
    for (int src = 1; src < total_processes; src++)
    {
      MPI_Recv(&all_counts[src], 1, MPI_UNSIGNED_LONG_LONG, src, 0,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Выводим результаты в нужном формате
    unsigned long long total = 0;
    for (int i = 0; i < total_processes; i++)
    {
      if (i > 0)
        cout << " ";
      cout << all_counts[i];
      total += all_counts[i];
    }
    cout << " " << total << endl;
  }
  else
  {
    // Отправляем результат процессу 0
    MPI_Send(&local_count, 1, MPI_UNSIGNED_LONG_LONG, 0, 0, MPI_COMM_WORLD);
  }

  delete[] local_data;
  MPI_Finalize();
  return 0;
}
