#include <iostream>
#include <unistd.h>
#include <sys/wait.h>   // ← wait() lives here

int main() {
    pid_t childPid = fork();

    if (childPid == 0) {
        // Child process
        std::cout << "Child: doing some work...\n";
        // child finishes and exits
        return 0;

    } else {
        // Parent process
        std::cout << "Parent: waiting for child...\n";

        int status;
        wait(&status);  // ← Parent BLOCKS here until child finishes
        //   ^^^^^^^
        //   fills this with child's exit info

        std::cout << "Parent: child is done!\n";
    }

    return 0;
}

/*
  	 ❯ with wait() 
	 Parent: waiting for child...
	 Parent: child is done!
	 Child: doing some work...
	
	 ❯ without wait() 
	 Parent: waiting for child...
	 Child: doing some work...
	 Parent: child is done!
 */