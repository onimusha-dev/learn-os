#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("allocating memory for the array\n");

    int size = 10;
    char *pointer_to_array = malloc(sizeof(char) * size);

    // assign values
    for (int i = 0; i < size; i++) {
        pointer_to_array[i] = 'A' + i;
    }

    // print values
    for (int i = 0; i < size; i++) {
        printf("Index %d - %c\n", i, pointer_to_array[i]);
    }

    free(pointer_to_array);
    return 0;
}