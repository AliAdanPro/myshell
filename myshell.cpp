#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

using namespace std;

#define MAX_ARGS 64

// Helper to split string by delimiter
vector<string> split(const string &s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        // Trim whitespace
        size_t first = token.find_first_not_of(" \t");
        if (first == string::npos) continue;
        size_t last = token.find_last_not_of(" \t");
        tokens.push_back(token.substr(first, (last - first + 1)));
    }
    return tokens;
}

// Helper to convert vector<string> to char* array for execvp
void get_args(const vector<string> &parts, char *args[]) {
    for (size_t i = 0; i < parts.size(); ++i) {
        args[i] = (char *)parts[i].c_str();
    }
    args[parts.size()] = nullptr;
}

// Execute a single command
// This function handles the creation of a child process and the execution of a command within it.
// It uses the fork-exec-wait pattern fundamental to process management in Unix-like systems.
void execute_command(const vector<string> &cmd_tokens) {
    if (cmd_tokens.empty()) return;

    if (cmd_tokens[0] == "quit") {
        exit(0);
    }

    // fork() creates a new process by duplicating the calling process.
    // The new process is the child, and the calling process is the parent.
    // It returns 0 to the child process, and the child's PID to the parent.
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        return;
    }

    if (pid == 0) {
        // Child process
        char *args[MAX_ARGS];
        get_args(cmd_tokens, args);
        
        // execvp() replaces the current process image with a new process image.
        // It searches for the command in the PATH environment variable.
        // If successful, this function never returns because the process memory is replaced.
        execvp(args[0], args);
        perror("Execvp failed"); // Only reached if execvp fails
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        // waitpid() suspends execution of the calling process until a child specified by pid argument creates state changes.
        // This ensures the parent waits for the command to finish before prompting again.
        waitpid(pid, &status, 0);
    }
}

// Execute a pipeline of commands
// This function handles commands connected by pipes (e.g., cmd1 | cmd2 | cmd3).
// It sets up IPC channels using pipe() and redirects I/O using dup2().
void execute_pipeline(const vector<string> &commands) {
    int num_cmds = commands.size();
    int pipefds[2 * (num_cmds - 1)];

    // Create all necessary pipes
    // pipe() creates a unidirectional data channel that can be used for interprocess communication.
    // It returns two file descriptors: pipefds[0] refers to the read end, pipefds[1] refers to the write end.
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("Pipe failed");
            return;
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            return;
        }

        if (pid == 0) {
            // Child process
            
            // If not the first command, handle input from previous pipe
            if (i > 0) {
                // dup2() duplicates a file descriptor. Here it redirects STDIN (0) to read from the previous pipe's read end.
                // This allows the process to read its input from the output of the previous command in the pipeline.
                if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) < 0) {
                    perror("dup2 stdin failed");
                    exit(EXIT_FAILURE);
                }
            }

            // If not the last command, handle output to current pipe
            if (i < num_cmds - 1) {
                // dup2() redirects STDOUT (1) to write to the current pipe's write end.
                // This passes the output of the process to the input of the next command in the pipeline.
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0) {
                    perror("dup2 stdout failed");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe fds in child
            for (int k = 0; k < 2 * (num_cmds - 1); k++) {
                close(pipefds[k]);
            }

            // Parse command arguments
            vector<string> tokens = split(commands[i], ' '); 
            char *args[MAX_ARGS];
            get_args(tokens, args);

            execvp(args[0], args);
            perror("Execvp failed");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process: Close all pipes
    for (int i = 0; i < 2 * (num_cmds - 1); i++) {
        close(pipefds[i]);
    }

    // Wait for all children
    for (int i = 0; i < num_cmds; i++) {
        wait(NULL);
    }
}

int main() {
    string input;

    while (true) {
        cout << "myshell> ";
        if (!getline(cin, input)) break; // Handle EOF

        if (input.empty()) continue;

        // Check for quit before parsing pipelines (simple case)
        if (input == "quit") break;

        // Check for pipeline
        vector<string> commands = split(input, '|');

        if (commands.empty()) continue;

        if (commands.size() == 1) {
             vector<string> tokens = split(commands[0], ' ');
             execute_command(tokens);
        } else {
            execute_pipeline(commands);
        }
    }

    return 0;
}
