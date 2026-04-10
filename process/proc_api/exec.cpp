#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t childPid = fork();

    if (childPid == 0) {
        // Child: replace myself with the "ls" program
        std::cout << "Child: about to become ls!\n";

        char *args[] = {
            (char*)"ls",    // program name
            (char*)"-la",   // argument
            nullptr         // must end with null!
        };

        execvp("ls", args); // ← child's memory is REPLACED with ls
        //     ^^^
        //     if this line succeeds, nothing below runs
        //     because this process IS now ls

        // only reaches here if exec FAILED
        std::cerr << "exec failed!\n";

    } else {
        // Parent: wait for child (ls) to finish
        wait(nullptr);  // nullptr = don't care about exit status
        std::cout << "Parent: ls finished!\n";
    }

    return 0;
}