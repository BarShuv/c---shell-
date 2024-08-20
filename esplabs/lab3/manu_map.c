#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function descriptor structure
struct fun_desc {
    char *name;
    char (*fun)(char);
};

// Function implementations
char my_get(char c) {
    return fgetc(stdin);
}

char cprt(char c) {
    if (c >= 0x20 && c <= 0x7E) {
        printf("%c\n", c);
    } else {
        printf(".\n");
    }
    return c;
}

char encrypt(char c) {
    if (c >= 0x20 && c <= 0x4E) {
        return c + 0x20;
    }
    return c;
}

char decrypt(char c) {
    if (c >= 0x40 && c <= 0x7E) {
        return c - 0x20;
    }
    return c;
}

char xoprt(char c) {
    printf("%02X %03o\n", c, c);
    return c;
}

char* map(char *array, int array_length, char (*f)(char)) {
    char* mapped_array = (char*)malloc(array_length * sizeof(char));
    for (int i = 0; i < array_length; i++) {
        mapped_array[i] = f(array[i]);
    }
    return mapped_array;
}

int main() {
    // Array of function descriptors
    struct fun_desc menu[] = {
            {"Get string", my_get},
            {"Print string", cprt},
            {"Encrypt", encrypt},
            {"Decrypt", decrypt},
            {"Print Hex and Octal", xoprt},
            {NULL, NULL}
    };
    char arr[5];
    // Character array
    char *carray = arr;
    char *copy = arr;

    // Display menu
    printf("Select operation from the following menu (Ctrl^D for exit):\n");
    for (int i = 0; menu[i].name != NULL; i++) {
        printf("%d) %s\n", i, menu[i].name);
    }

    // Buffer for input
    char input[100];

    // Read user choice
    printf("Option: ");
    while (fgets(input, sizeof(input), stdin) != NULL) {
        // Convert input to numeric value
        int choice = atoi(input);

        // Validate choice
        int valid = 0;
        for (int i = 0; menu[i].name != NULL; i++) {
            if (i == choice) {
                valid = 1;
                break;
            }
        }

        if (!valid) {
            // Print error message if choice is out of bounds
            printf("Not within bounds\n");
            return 0;
        }

        // Print within bounds message
        printf("Within bounds\n");

        // Apply the chosen function to the character array
        copy = map(carray, 5, menu[choice].fun);
        carray = copy;

        //memcpy(carray, new_carray, 5); // Copy the new array to the original array
        free(copy); // Free the temporary array

        // Prompt for next option
        printf("DONE.\n");
        printf("Select operation from the following menu (Ctrl^D for exit):\n");
        for (int i = 0; menu[i].name != NULL; i++) {
            printf("%d) %s\n", i, menu[i].name);
        }
        printf("Option: ");
    }
    free(carray);
    return 0;
}
