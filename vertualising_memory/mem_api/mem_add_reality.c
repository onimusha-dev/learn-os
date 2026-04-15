#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	printf("location of code : %p\n", (void *) main);
	printf("location of heap : %p\n", (void *) malloc(1));
	int x = 3;
	printf("location of stack : %p\n", (void *) &x);
	return x;
}

/*
	 ❯ ./app
	 location of code : 0x556678dfe159
	 location of heap : 0x5566b0125430
	 location of stack : 0x7ffcd1e063d4
 */