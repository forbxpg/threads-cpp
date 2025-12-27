#include <iostream>
#include <windows.h>
#include <cstdlib>
#include <vector>

using namespace std;

// Create Process with Relocs Disable
const int length = 1000000000;
const int A = 514;
const int B = 8;


int calculate_threads_count(int A, int B) {
    int X = 2 * A + B;
    return X % 5 + 2;
}

int calculate_distribution_type(int A, int B) {
    return (2 * A + B) % 4;
}

// Max processes for memory allocation
const int MAX_PROCESSES = 10;

int main(int argc, char* argv[]) {
    const int num_processes = calculate_threads_count(A, B);
    const int distribution_type = calculate_distribution_type(A, B);

    // Child process implementation
    if (argc == 4) {
        int processID = atoi(argv[1]);
        int start = atoi(argv[2]);
        int end = atoi(argv[3]);

        cout << "Процесс " << processID << ": запущен, PID=" << GetCurrentProcessId() << endl;
        cout << "Процесс " << processID << ": адрес main = " << hex << (void*)main << dec << endl;

        int chunk = end - start;
        int buffer_size = chunk + 2;
        unsigned char* local_data = new unsigned char[buffer_size];

        srand(2025);
        // Skip numbers until our range
        for (int i = 0; i < start - 1; i++) {
            rand();
        }

        // Left neighbor
        if (processID > 0) {
            local_data[0] = rand() & 0xff;
        } else {
            local_data[0] = 0;
        }

        // Main data
        for (int i = 0; i < chunk; i++) {
            local_data[i + 1] = rand() & 0xff;
        }

        // Right neighbor
        if (processID < num_processes - 1) {
            local_data[buffer_size - 1] = rand() & 0xff;
        } else {
            local_data[buffer_size - 1] = 0;
        }

        // Count local maximum
        int local_count = 0;

        for (int i = 1; i <= chunk; i++) {
            int global_i = start + (i - 1);

            if (global_i == 0) {
                if (local_data[i] >= local_data[i + 1]) {
                    local_count++;
                }
            }
            else if (global_i == length - 1) {
                if (local_data[i] >= local_data[i - 1]) {
                    local_count++;
                }
            }
            else {
                if (local_data[i] >= local_data[i - 1] &&
                    local_data[i] >= local_data[i + 1]) {
                    local_count++;
                }
            }
        }

        delete[] local_data;
        return local_count;
    }

    // Parent process
    cout << "A = " << A << ", B = " << B << endl;
    cout << endl;

    cout << "Количество процессов = (X % 5) + 2 = (" << (2*A + B) << " % 5) + 2 = "
         << num_processes << endl;
    cout << "Тип распределения = (X % 4) = (" << (2*A + B) << " % 4) = "
         << distribution_type << endl;
    cout << endl;

    const char* distribution_names[] = {
        "Последовательное (процесс i → диапазон [i*chunk, (i+1)*chunk))",
        "Обратное последовательное (процесс 0 → последний диапазон)",
        "Чередующееся (чётные - начало, нечётные - конец)",
        "Перемешанное (по простому правилу)"
    };

    cout << "Используется распределение типа " << distribution_type << ": "
         << distribution_names[distribution_type] << endl;
    cout << endl;

    cout << "PID родителя: " << GetCurrentProcessId() << endl;
    cout << "Адрес main в родителе: " << hex << (void*)main << dec << endl;
    cout << endl;

    // Exe path
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    // Proccesses arrays
    PROCESS_INFORMATION processInfo[MAX_PROCESSES];
    STARTUPINFOA startupInfo[MAX_PROCESSES];

    // Base batch size
    int chunk_size = length / num_processes;

    // Store for processes ranges
    vector<pair<int, int>> ranges(num_processes);

    for (int i = 0; i < num_processes; i++) {
        int logical_index;  // Which chunk of data this process handles

        switch (distribution_type) {
            case 0:
                logical_index = i;
                break;

            case 1:
                logical_index = num_processes - 1 - i;
                break;

            case 2:
                if (i % 2 == 0) {
                    logical_index = i / 2;
                } else {
                    logical_index = num_processes - 1 - (i / 2);
                }
                break;
            case 3:
                logical_index = (i * 3) % num_processes;
                break;

            default:
                logical_index = i;
        }

        ranges[i].first = logical_index * chunk_size;
        ranges[i].second = (logical_index + 1) * chunk_size;
    }

    cout << "Создаю " << num_processes << " дочерних процессов" << endl;
    cout << endl;

    // Create child processes
    for (int i = 0; i < num_processes; i++) {
        int start = ranges[i].first;
        int end = ranges[i].second;

        char cmdLine[512];
        sprintf(cmdLine, "\"%s\" %d %d %d", exePath, i, start, end);

        ZeroMemory(&startupInfo[i], sizeof(STARTUPINFOA));
        startupInfo[i].cb = sizeof(STARTUPINFOA);
        ZeroMemory(&processInfo[i], sizeof(PROCESS_INFORMATION));

        BOOL success = CreateProcessA(
            NULL, cmdLine, NULL, NULL, FALSE, 0,
            NULL, NULL, &startupInfo[i], &processInfo[i]
        );

        if (!success) {
            cerr << "ОШИБКА при создании процесса " << i << ": " << GetLastError() << endl;
            return 1;
        }

        cout << "Процесс " << i << " (PID " << processInfo[i].dwProcessId
             << "): диапазон [" << start << ", " << end << ")" << endl;
    }

    cout << endl;

    // Wait for all processes to finish
    HANDLE handles[MAX_PROCESSES];
    for (int i = 0; i < num_processes; i++) {
        handles[i] = processInfo[i].hProcess;
    }
    WaitForMultipleObjects(num_processes, handles, TRUE, INFINITE);

    // Get the results
    int total_count = 0;
    cout << "Результаты:" << endl;
    for (int i = 0; i < num_processes; i++) {
        DWORD exitCode;
        GetExitCodeProcess(processInfo[i].hProcess, &exitCode);

        cout << "Процесс " << i << " (диапазон [" << ranges[i].first << ", "
             << ranges[i].second << ")): " << exitCode << " максимумов" << endl;
        total_count += exitCode;

        CloseHandle(processInfo[i].hProcess);
        CloseHandle(processInfo[i].hThread);
    }
    cout << endl;
    cout << "Total maximum count: " << total_count << endl;
    cout << endl;

    cout << "Программа скомпилирована с флагами:" << endl;
    cout << "-Wl,--image-base=0x400000" << endl;
    cout << "-Wl,--disable-dynamicbase" << endl;

    return 0;
}
