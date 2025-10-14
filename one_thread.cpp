/*
M25-514 Kirgizov Temurmalik Kutbiddin ugli.

Task 1. One thread simple impl.
*/

#include <iostream>

using namespace std;

unsigned int global_seed;

void my_srand(unsigned int a) { global_seed = a; }

unsigned int my_rand() {
  global_seed = 214013 * global_seed + 2531011;
  return (global_seed >> 16) & 0x7fff;
}

long calculate_local_maximum(unsigned char *mas, long size) {
  my_srand(2025);

  for (long i = 0; i < size; i++) {
    mas[i] = my_rand() & 0xff;
  }

  long counter = 0;
  for (long i = 1; i < size - 1; i++) {
    if (mas[i] >= mas[i - 1] && mas[i] >= mas[i + 1])
      counter++;
  }

  if (mas[0] >= mas[1])
    counter++;
  if (mas[size - 1] >= mas[size - 2])
    counter++;

  return counter;
}

int main() {
  const long MAS_SIZE = 1000000000;
  unsigned char *mas = new unsigned char[MAS_SIZE];
  long count = calculate_local_maximum(mas, MAS_SIZE);

  cout << "count: " << count << endl;

  delete[] mas;
  return 0;
}
