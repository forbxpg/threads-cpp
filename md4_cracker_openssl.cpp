/*
M25-514 Kirgizov Temurmalik Kutbiddin ugli.

BDZ-2. MD4 Brute Force (OpenSSL) impl.
*/

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <chrono>
#include <openssl/md4.h>
using namespace std;


const int PASSWORD_LENGTH = 3;
const int ALPHABET_SIZE = 26;
const int A = 514;
const int B = 8;
const long long TOTAL_PASSWORDS = 208827064576LL;


atomic<bool> password_found(false);
mutex output_mutex;
string found_password = "";


int calculate_threads_count(int A, int B) {
    int X = 2 * A + B;
    return X % 5 + 2;
}

int calculate_distribution_type(int A, int B) {
    return (2 * A + B) % 4;
}

// OpenSSL MD4 computation
string computeMD4(const char* data, size_t len) {
    unsigned char digest[MD4_DIGEST_LENGTH];  // 16 bytes
    MD4((unsigned char*)data, len, digest);

    // Convert to hex string
    stringstream ss;
    for (int i = 0; i < MD4_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)digest[i];
    }
    return ss.str();
}

// Convert number to password string
// Base-26 encoding: aaaaaaaa, baaaaaaa, caaaaaaa, etc.
void number_to_password(long long num, char* password) {
    for (int i = 0; i < PASSWORD_LENGTH; i++) {
        password[i] = 'a' + (num % ALPHABET_SIZE);
        num /= ALPHABET_SIZE;
    }
    password[PASSWORD_LENGTH] = '\0';
}

/**
 * Block distribution worker
 * Each thread gets a continuous chunk of passwords
 */
void crack_block(int thread_id, int num_threads, const string& target_hash,
                 long long start_offset = 0) {
    long long chunk_size = TOTAL_PASSWORDS / num_threads;
    long long start = thread_id * chunk_size + start_offset;
    long long end = (thread_id == num_threads - 1) ?
                    TOTAL_PASSWORDS + start_offset :
                    (thread_id + 1) * chunk_size + start_offset;

    char password[PASSWORD_LENGTH + 1];
    long long checked = 0;

    {
        lock_guard<mutex> lock(output_mutex);
        cout << "Thread " << thread_id << ": range [" << start << ", " << end << ")" << endl;
    }

    for (long long i = start; i < end && !password_found; i++) {
        number_to_password(i, password);

        if (computeMD4(password, PASSWORD_LENGTH) == target_hash) {
            password_found = true;
            found_password = password;

            lock_guard<mutex> lock(output_mutex);
            cout << "Thread " << thread_id << " FOUND IT! XDDDDDDDD lox-pwd" << endl;
            cout << "Password: " << password << endl;
            return;
        }

        if (++checked % 10000000 == 0) {
            lock_guard<mutex> lock(output_mutex);
            cout << "Thread " << thread_id << ": " << checked << " checked" << endl;
        }
    }
}

/**
 * Cyclic distribution worker
 * Thread i checks passwords i, i+N, i+2N, i+3N, ...
 */
void crack_cyclic(int thread_id, int num_threads, const string& target_hash,
                  bool reverse = false) {
    char password[PASSWORD_LENGTH + 1];
    long long checked = 0;

    long long start = reverse ? (TOTAL_PASSWORDS - 1 - thread_id) : thread_id;
    long long step = reverse ? -num_threads : num_threads;
    long long limit = reverse ? -1 : TOTAL_PASSWORDS;

    for (long long i = start;
         (reverse ? i > limit : i < limit) && !password_found;
         i += step) {

        number_to_password(i, password);

        if (computeMD4(password, PASSWORD_LENGTH) == target_hash) {
            password_found = true;
            found_password = password;

            lock_guard<mutex> lock(output_mutex);
            cout << "\nThread " << thread_id << " FOUND IT!" << endl;
            cout << "Password: " << password << endl;
            return;
        }

        if (++checked % 10000000 == 0) {
            lock_guard<mutex> lock(output_mutex);
            cout << "Thread " << thread_id << ": " << checked << " checked" << endl;
        }
    }
}


int main(int argc, char* argv[]) {
    // Default: MD4 hash of "aaa"
    string target_hash = "918d7099b77c7a06634c62ccaf5ebac7";
    if (argc > 1) target_hash = argv[1];

    int num_threads = calculate_threads_count(A, B);
    int dist_type = calculate_distribution_type(A, B);

    cout << "Threads: " << num_threads << endl;
    cout << "Distribution: " << dist_type;
    switch(dist_type) {
        case 0: cout << " (Block L->R)"; break;
        case 1: cout << " (Block R->L)"; break;
        case 2: cout << " (Cyclic)"; break;
        case 3: cout << " (Reverse Cyclic)"; break;
    }
    cout << endl;
    cout << "Target: " << target_hash << endl;

    // Test
    cout << "\nTest: MD4('aaa') = " << computeMD4("aaa", 3) << endl << endl;

    vector<thread> threads;
    auto start_time = chrono::high_resolution_clock::now();

    // Launch threads based on distribution type
    switch(dist_type) {
        case 0:  // Block L->R
            for (int i = 0; i < num_threads; i++)
                threads.emplace_back(crack_block, i, num_threads, ref(target_hash), 0);
            break;
        case 1:  // Block R->L
            for (int i = 0; i < num_threads; i++)
                threads.emplace_back(crack_block, i, num_threads, ref(target_hash),
                    TOTAL_PASSWORDS - (TOTAL_PASSWORDS / num_threads) * num_threads);
            break;
        case 2:  // Cyclic
            for (int i = 0; i < num_threads; i++)
                threads.emplace_back(crack_cyclic, i, num_threads, ref(target_hash), false);
            break;
        case 3:  // Reverse Cyclic
            for (int i = 0; i < num_threads; i++)
                threads.emplace_back(crack_cyclic, i, num_threads, ref(target_hash), true);
            break;
    }

    for (auto& t : threads) t.join();

    auto duration = chrono::duration_cast<chrono::seconds>(
        chrono::high_resolution_clock::now() - start_time);
    if (password_found) {
        cout << "SUCCESS! Password: " << found_password << endl;
        cout << "Verification: " << computeMD4(found_password.c_str(), PASSWORD_LENGTH) << endl;
    } else {
        cout << "Not found" << endl;
    }
    cout << "Time bruteforcing: " << duration.count() << "s" << endl;
    return 0;
}
