#include <stdio.h>

int count_digits (char* s) {//why do I need hexdilpuse
    int counter = 0, i = 0;
    while (s[i] != '\0') {//not end
        if(s[i] >= '0' && s[i] <= '9') {
            counter ++;
        }
        i++;
    }
    return counter;
}

int main (int argc, char** argv) {
    printf("String contains %d\n digits", count_digits(argv[1]));
    return 0;
}