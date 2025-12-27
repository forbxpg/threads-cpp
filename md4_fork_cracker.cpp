/*
 * M25-514 Kirgizov Temurmalik Kutbiddin ugli
 *
 * MD4 Brute Force with fork impl.
 */

 #include <iostream>
 #include <iomanip>
 #include <sstream>
 #include <cstring>
 #include <cstdint>
 #include <unistd.h>
 #include <sys/wait.h>
 #include <signal.h>
 #include <openssl/md4.h>

 using namespace std;

 const int PASSWORD_LENGTH = 3;
 const int ALPHABET_SIZE = 26;
 const int A = 514;
 const int B = 8;

 const long long TOTAL_PASSWORDS = 208827064576LL;

 int calculate_processes_count(int A, int B) {
     int X = 2 * A + B;
     return (X % 5) + 2;
 }

 int calculate_distribution_type(int A, int B) {
     int X = 2 * A + B;
     return X % 4;
 }

 // OpenSSL MD4 computation
 string computeMD4(const char* data, size_t len) {
     unsigned char digest[MD4_DIGEST_LENGTH];  // 16 bytes
     MD4((unsigned char*)data, len, digest);

     // Convert binary digest to hex string
     stringstream ss;
     for (int i = 0; i < MD4_DIGEST_LENGTH; i++) {
         ss << hex << setw(2) << setfill('0') << (int)digest[i];
     }
     return ss.str();
 }

 // OpenSSL MD4 computation
 void number_to_password(long long num, char* password) {
     for (int i = 0; i < PASSWORD_LENGTH; i++) {
         password[i] = 'a' + (num % ALPHABET_SIZE);
         num /= ALPHABET_SIZE;
     }
     password[PASSWORD_LENGTH] = '\0';
 }

// child process worker with his distribution type
 void crack_password_process(int process_id, int num_processes,
                            const string& target_hash,
                            int distribution_type,
                            int pipe_write) {

     char password[PASSWORD_LENGTH + 1];
     long long checked = 0;
     long long start, end, step;
     if (distribution_type == 0) {
         long long chunk_size = TOTAL_PASSWORDS / num_processes;
         start = process_id * chunk_size;
         end = (process_id == num_processes - 1) ? TOTAL_PASSWORDS : (process_id + 1) * chunk_size;
         step = 1;
         cout << "Process " << process_id << " (PID " << getpid()
              << "): Block L->R [" << start << ", " << end << ")" << endl;
     } else if (distribution_type == 1) {
         long long chunk_size = TOTAL_PASSWORDS / num_processes;
         long long reverse_id = num_processes - 1 - process_id;
         start = reverse_id * chunk_size;
         end = (reverse_id == num_processes - 1) ? TOTAL_PASSWORDS : (reverse_id + 1) * chunk_size;
         step = 1;
         cout << "Process " << process_id << " (PID " << getpid()
              << "): Block R->L [" << start << ", " << end << ")" << endl;
     } else if (distribution_type == 2) {
         start = process_id;
         end = TOTAL_PASSWORDS;
         step = num_processes;
         cout << "Process " << process_id << " (PID " << getpid()
              << "): Cyclic (every " << num_processes << "th from " << start << ")" << endl;
     } else {
         start = TOTAL_PASSWORDS - 1 - process_id;
         end = -1;
         step = -num_processes;
         cout << "Process " << process_id << " (PID " << getpid()
              << "): Reverse Cyclic (from " << start << " backwards)" << endl;
     }
     // Brute force ...
     if (step > 0) {
         for (long long i = start; i < end; i += step) {
             number_to_password(i, password);
             if (computeMD4(password, PASSWORD_LENGTH) == target_hash) {
                 cout << "\nProcess " << process_id << " (PID " << getpid()
                      << ") FOUND THE PASSWORD!" << endl;
                 cout << "Password: " << password << endl;
                 cout << "Checked: " << checked << " combinations" << endl;

                 // Send password to parent via pipe
                 write(pipe_write, password, PASSWORD_LENGTH);
                 return;
             }
             checked++;
             if (checked % 10000000 == 0) {
                 cout << "Process " << process_id << ": checked "
                      << checked << " passwords..." << endl;
             }
         }
     } else {
         // Back search
         for (long long i = start; i > end; i += step) {
             number_to_password(i, password);
             if (computeMD4(password, PASSWORD_LENGTH) == target_hash) {
                 cout << "\nProcess " << process_id << " (PID " << getpid()
                      << ") FOUND THE PASSWORD!" << endl;
                 cout << "Password: " << password << endl;
                 cout << "Checked: " << checked << " combinations" << endl;
                 write(pipe_write, password, PASSWORD_LENGTH);
                 return;
             }
             checked++;
             if (checked % 10000000 == 0) {
                 cout << "Process " << process_id << ": checked "
                      << checked << " passwords..." << endl;
             }
         }
     }
     cout << "Process " << process_id << " finished: checked "
          << checked << " passwords (not found)" << endl;

     // not found -> notify parent
     char empty[PASSWORD_LENGTH] = {0};
     write(pipe_write, empty, PASSWORD_LENGTH);
 }

 int main(int argc, char* argv[]) {
     // Default test hash for "aaa"
     string target_hash = "918d7099b77c7a06634c62ccaf5ebac7";

     if (argc > 1) {target_hash = argv[1];}
     int num_processes = calculate_processes_count(A, B);
     int distribution_type = calculate_distribution_type(A, B);

     cout << "A = " << A << ", B = " << B << endl;
     cout << "Number of processes: " << num_processes << endl;
     cout << "Distribution type: " << distribution_type;
     switch(distribution_type) {
         case 0: cout << " (Block: Left→Right)"; break;
         case 1: cout << " (Block: Right→Left)"; break;
         case 2: cout << " (Cyclic)"; break;
         case 3: cout << " (Reverse Cyclic)"; break;
     }
     cout << "Target hash: " << target_hash << endl;
     cout << "Password length: " << PASSWORD_LENGTH << " characters" << endl;
     cout << "Total combinations: " << TOTAL_PASSWORDS << endl;
     cout << "Parent PID: " << getpid() << endl;

     cout << "Testing MD4 implementation:" << endl;
     cout << "MD4('aaa') = " << computeMD4("aaa", 3) << endl;

     // Create pipes for inter-process communication
     // Each child process will send result through its pipe
     int pipes[num_processes][2];
     pid_t pids[num_processes];

     cout << "Creating " << num_processes << " child processes..." << endl;

     // Fork child processes
     for (int i = 0; i < num_processes; i++) {
         if (pipe(pipes[i]) == -1) {
             cerr << "Error creating pipe for process " << i << endl;
             return 1;
         }

         pid_t pid = fork();
         if (pid == -1) {
             cerr << "Error forking process " << i << endl;
             return 1;
         } else if (pid == 0) {
             // child only can write
             close(pipes[i][0]);
             crack_password_process(i, num_processes, target_hash,
                                  distribution_type, pipes[i][1]);
             close(pipes[i][1]);
             exit(0);

         } else {
             pids[i] = pid;
             // parent only can read
             close(pipes[i][1]);
         }
     }

     string found_password = "";
     cout << "Waiting for results from child processes..." << endl;

     // check for all pipes
     for (int i = 0; i < num_processes; i++) {
         char buffer[PASSWORD_LENGTH + 1] = {0};
         int bytes_read = read(pipes[i][0], buffer, PASSWORD_LENGTH);
         if (bytes_read > 0 && buffer[0] != '\0' && found_password.empty()) {
             buffer[PASSWORD_LENGTH] = '\0';
             found_password = string(buffer);
             cout << "Received password from process " << i
                  << " (PID " << pids[i] << ")" << endl;
             for (int j = 0; j < num_processes; j++) {
                 if (j != i) {
                     kill(pids[j], SIGTERM);
                 }
             }
         }
         close(pipes[i][0]);
     }

     for (int i = 0; i < num_processes; i++) {
         int status;
         waitpid(pids[i], &status, 0);
     }
     if (!found_password.empty()) {
         cout << "Password found: " << found_password << endl;
         cout << "Verification: " << computeMD4(found_password.c_str(), found_password.length()) << endl;
     } else {
         cout << "Password not found in search space" << endl;
     }
     return 0;
 }
