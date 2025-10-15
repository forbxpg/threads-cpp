/*
M25-514 Kirgizov Temurmalik Kutbiddin ugli.

Task 5. POSIX threads - pthreads impl.
*/

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <pthread.h>
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

struct Task {
  long long start;
  long long end;
  long long step;
  int thread_id;
  unsigned long long *out_result;
  unsigned char *mas;
};

bool is_local_maximum(const unsigned char *mas, long long arr_size,
                      long long i) {
  if (i == 0) {
    return mas[i] >= mas[i + 1];
  } else if (i + 1 == arr_size) {
    return mas[i] >= mas[i - 1];
  }
  return ((mas[i] >= mas[i - 1]) && (mas[i] >= mas[i + 1]));
}

void *ThreadFunc(void *param) {
  Task *task = static_cast<Task *>(param);
  unsigned long long local_maximum_count = 0;

  for (long long i = task->start; i < task->end; i += task->step) {
    if (is_local_maximum(task->mas, task->end, i)) {
      local_maximum_count++;
    }
  }

  *(task->out_result) = local_maximum_count;
  return nullptr;
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

  cout << "Calculating local maximum using POSIX threads with " << threads_count
       << " threads" << endl;

  vector<unsigned long long> thread_results(threads_count, 0);
  vector<Task> tasks(threads_count);
  vector<pthread_t> threads(threads_count);

  long long batch_size = ARR_SIZE / threads_count;
  for (int t = 0; t < threads_count; t++) {
    long long start = t * batch_size;
    long long end = (t == threads_count - 1) ? ARR_SIZE : (t + 1) * batch_size;

    tasks[t] = Task{start, end, 1, t, &thread_results[t], mas};

    int result = pthread_create(&threads[t], nullptr, ThreadFunc, &tasks[t]);
    if (result != 0) {
      cerr << "Error creating thread " << t << ": " << strerror(result) << endl;
      delete[] mas;
      return 1;
    }
  }

  for (int t = 0; t < threads_count; t++) {
    pthread_join(threads[t], nullptr);
  }

  unsigned long long total_maximum = 0;
  for (int t = 0; t < threads_count; t++) {
    cout << "Thread " << t + 1 << " counted " << thread_results[t]
         << " local maximum" << endl;
    total_maximum += thread_results[t];
  }

  cout << "Total local maximum count (POSIX threads): " << total_maximum
       << endl;

  delete[] mas;
  return 0;
}
