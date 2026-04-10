#include <iostream>
#include <unistd.h>    // ← Linux system calls live here
#include <sys/types.h>

int main() {
    std::cout << "Before fork — only one process\n";

    pid_t childPid = fork();  // ← OS clones this process RIGHT HERE
    //    ^^^^^^^^
    //    pid_t is just a typedef for int, Linux's type for PIDs

    // After fork() — TWO processes run from this exact line!
    if (childPid < 0) {
        // fork failed
        std::cerr << "Fork failed!\n";
        return 1;

    } else if (childPid == 0) {
        // ✅ We are INSIDE THE CHILD
        // fork() returns 0 to the child
        std::cout << "I am the CHILD, my PID: " << getpid() << "\n";

    } else {
        // ✅ We are INSIDE THE PARENT
        // fork() returns the child's PID to the parent
        std::cout << "I am the PARENT, my PID: " << getpid() << "\n";
        std::cout << "I created child with PID: " << childPid << "\n";
    }

    return 0;
}

/*
	~/Coding/CPP/os/process
	❯ g++ proc_api/fork.cpp -o app && ./app
	Before fork — only one process
	I am the PARENT, my PID: 83448
	I created child with PID: 83449
	I am the CHILD, my PID: 83449
 */