/*
M25-514 Kirgizov Temurmalik Kutbiddin ugli.

Task 2. std::thread impl.
*/

#include <cassert>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;

unsigned int global_seed;

int calculate_threads_count(int A, int B) {
  int X = 2 * A + B;
  return X % 5 + 2;
}

void my_srand(unsigned int a) { global_seed = a; }

unsigned int my_rand() {
  global_seed = 214013u * global_seed + 2531011u;
  return (global_seed >> 16) & 0x7fffu;
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
    for (int i = 0; i < to_write; ++i) {
      buffer[i] = static_cast<unsigned char>(my_rand() & 0xff);
    }
    file.write(reinterpret_cast<char *>(buffer.data()), to_write);
    remaining -= to_write;
  }
  cout << "File generated." << endl;
  file.close();
}

void calculate_local_maximum(unsigned char *mas, int thread_id,
                             int threads_count, long *out_result,
                             long arr_size) {

  long local_maximum_count = 0;

  unsigned long long ARR_SIZE_LONG = static_cast<unsigned long long>(arr_size);
  unsigned long long start =
      ((unsigned long long)thread_id * ARR_SIZE_LONG) / threads_count;
  unsigned long long end =
      ((unsigned long long)(thread_id + 1) * ARR_SIZE_LONG) / threads_count;

  if (start >= ARR_SIZE_LONG) {
    *out_result = 0;
    return;
  }

  if (end > ARR_SIZE_LONG) {
    end = ARR_SIZE_LONG;
  }

  for (unsigned long long idx = start; idx < end; idx++) {
    if (idx == 0) {
      if (ARR_SIZE_LONG > 1 && mas[0] >= mas[1])
        local_maximum_count++;
      else if (ARR_SIZE_LONG == 1)
        local_maximum_count++;
    } else if (idx + 1 == ARR_SIZE_LONG) {
      if (mas[idx] >= mas[idx - 1])
        local_maximum_count++;
    } else {
      if (mas[idx] >= mas[idx - 1] && mas[idx] >= mas[idx + 1])
        local_maximum_count++;
    }
  }
  *out_result = static_cast<long>(local_maximum_count);
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

  cout << "Calculating local maximum with " << threads_count << endl;

  vector<thread> threads;
  vector<long> results(threads_count, 0);

  for (int i = 0; i < threads_count; ++i) {
    threads.push_back(thread(calculate_local_maximum, mas, i, threads_count,
                             &results[i], ARR_SIZE));
  }
  for (auto &t : threads) {
    t.join();
  }

  long long total = 0;
  for (int i = 0; i < threads_count; ++i) {
    cout << "Thread " << i + 1 << " counted " << results[i] << " local maxima"
         << endl;
    total += results[i];
  }
  cout << "Total local maxima: " << total << endl;

  delete[] mas;
  return 0;
}
