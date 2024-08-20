#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>



typedef struct {
  char debug_mode;
  char display_mode;
  char file_name[128];
  int unit_size;
  unsigned char mem_buf[10000];
  size_t mem_count;
  /*
   .
   .
   Any additional fields you deem necessary
  */
} state;


void toggleDebugMode(state* s) {
    if (s->debug_mode == '0'){
        s->debug_mode = '1'; 
        printf ("Debug flag now on\n");
    }
    else{
        s->debug_mode = '0'; 
        printf("Debug flag now off\n");
    }
}  

void setFileName(state* s) {
    printf("Please enter File Name\n");
    if (fgets(s->file_name, 100, stdin) != NULL) {
        // Remove the newline character if it was read
        size_t len = strlen(s->file_name);
        if (len > 0 && s->file_name[len - 1] == '\n') {
            s->file_name[len - 1] = '\0';
        }
        if(s->debug_mode == '1') {
            printf("Debug: file name set to %s\n", s->file_name);
        }
        return;
    }
    printf("error while reading");
}

void setUnitSize(state* s) {
    char *size;
    //Prompt the user for a number.
    printf("please enter Size\n");
    char buffer[1024];
    size = fgets(buffer, sizeof(buffer), stdin);
    //If the value is valid (1, 2, or 4), set the size variable accordingly.
    if (*size == '1' || *size == '2' || *size == '4') { 
        s->unit_size = atoi(size);
        //If debug mode is on, print "Debug: set size to x", with x the appropriate size.
        if(s->debug_mode == '1') {
            printf("Debug: set size to %c\n", *size);
        }
        return;
    }
    //If not valid, print an error message and leave size unchanged.
    printf("error size is not valid\n");
}

void quit(state* s) {
    if(s->debug_mode == '1') {
            printf("Debug: quitting");
    }
    free(s);
    exit(0);
}

void loadIntoMemory(state* s) {
    //Check if file_name is empty (i.e. equals to ""), and if it is print an error message and return.
    if (strcmp(s->file_name, "") == 0){
        printf("File name is Empty\n");
        return;
    }
    //Open file_name for reading. If this fails, print an error message and return.
    FILE* toOpen = fopen(s->file_name, "rb");

    if (toOpen == NULL) {
        printf("Error opening the file\n");
        fclose(toOpen);
        return;
    }
    //Prompt the user for location in hexadecimal, and length (in decimal).
    int length, location;
    char buffer[1024];
    printf("Please enter <location> <length> (length in decimal and location in hexadecimal)\n");
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%x %d", &location, &length);
    //If debug flag is on, print the file_name, as well as location, and length.
    if (s->debug_mode == '1') {
        printf("File name: %s\n", s->file_name);
        printf("Location: %x\n", location);
        printf("Length: %d\n", length);
    }
    //checking that we got an ok location
    if (fseek(toOpen, location, SEEK_SET) != 0) {
        printf("Error got incorrect location\n");
        fclose(toOpen);
        return;
    }
    //Copy length * unit_size bytes from file_name starting at position location into mem_buf.
    size_t bytes_to_read = length * s->unit_size;

    size_t bytes_read = fread(s->mem_buf, 1, bytes_to_read, toOpen);
    if (bytes_read != bytes_to_read) {
        printf("Error reading from the file\n");
        fclose(toOpen);
        return;
    }
    s->mem_count = bytes_read / s->unit_size;
    //Close the file.
    fclose(toOpen);
}

void toggleDisplayMode(state* s) {
    if (s->display_mode == '0'){
        s->display_mode = '1'; 
        printf ("Display flag now on, hexadecimal representation\n");
    }
    else{
        s->display_mode = '0'; 
        printf("Display flag now off, decimal representation\n");
    }
}  

void memDisplay(state* s) {
    int u, addr;
    char buffer[1024];
    printf("Enter address and length\n");
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%x %d", &addr, &u);

    static char *hex_formats[] = {"%#hhx\n", "%#hx\n", "No such unit", "%#x\n"};
    static char *dec_formats[] = {"%#hhd\n", "%#hd\n", "No such unit", "%#d\n"};

    void *ptr, *end;
    if (addr == 0) {
        ptr = &(s->mem_buf);
    }
    else {
        ptr = &addr;
    }
    end = ptr + (u * s->unit_size);

    if(s->display_mode == '1') {
        printf("Hexadecimal\n=======\n");
    }
    else {
        printf("Decimal\n=======\n");
    }

    while (ptr < end) {
        int value = *(int*)ptr;
        if (s->display_mode == '1') { //print in hexadecimal
            printf(hex_formats[s->unit_size - 1],value);
        }
        else { //print in decimal
            printf(dec_formats[s->unit_size - 1],value);
        }
        ptr = ptr + s->unit_size;
    }
}

void saveInfoFile(state* s) {
    if (strcmp(s->file_name, "") == 0){
        printf("File name is Empty\n");
        return;
    }
    FILE* toChange = fopen(s->file_name, "r+");

    if (toChange == NULL) {
        printf("Error opening the file\n");
        return;
    }

    int src_addr, target_location, length;
    char buffer[1024];
    //source-address (source virtual memory address, in hexadecimal), source-address can be set to 0, in which case, the source address is start of mem_buf, in any other case, use source-address as an address in (virtual) memory
    printf("Please enter <source-address> <target-location> <length>\n");
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%x %x %d", &src_addr, &target_location, &length);

    void *ptr;
    if (src_addr == 0) {
        ptr = s->mem_buf;
    }
    else {
        ptr = &src_addr;
    }
    //target-location (target file offset, in hexadecimal),
    fseek(toChange,0,SEEK_END);

    if (target_location > ftell(toChange)) {
        printf("Target location is greater than the size of the file\n");
        fclose(toChange);
        return;
    }

    fseek(toChange,0,SEEK_SET);
    fseek(toChange, target_location, SEEK_SET);
    fwrite(ptr, s->unit_size, length, toChange);

    //  todo do I need to debug?  

    fclose(toChange);        
}

void memModify(state* s) {
    int location, value;
    char buffer[1024];
    //Prompt the user for location and val (all in hexadecimal).
    printf("Please enter <location> <val>\n");
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%x %x", &location, &value);
    //If debug mode is on, print the location and val given by the user.
    if (s->debug_mode == '1') {
        printf("Location: %x\n", location);
        printf("Value: %d\n", value);
    }
    //Replace a unit at location in the memory with the value given by val.
    memcpy(&s->mem_buf[location],&value,s->unit_size);
}

struct fun_desc
{
      char *name;
    void (*fun)(state *);
};

int main(int argc, char **argv){

    state *s = (state *)(malloc(sizeof(state)));
    //initializing s
    s->unit_size = 1;
    s->debug_mode = '1';
    s->display_mode = '0';
    struct fun_desc menu[] = {{"Toggle Debug Mode", toggleDebugMode}, {"Set File Name", setFileName}, {"Set Unit Size", setUnitSize}, {"Load Into Memory", loadIntoMemory}, {"Toggle Display Mode",toggleDisplayMode}, {"Memory Display", memDisplay}, {"Save Into File", saveInfoFile}, {"Memory Modify", memModify}, {"Quit", quit}, {NULL, NULL}};
    int bound = sizeof(menu) / sizeof(menu[0]) - 1;
    while (1) {

        //print menu
        printf("%s", "Select operation from the following menu:\n");
        for (int i = 0 ; i < bound && menu[i].name != NULL ; i++) {
            printf("%i-%s\n", i, menu[i].name);
        }
        
        char buffer[1024];
        char* input = fgets(buffer, sizeof(buffer), stdin);

        if (input == NULL) { 
            break;
        }

        int option = atoi(input);

        if (option < 0 || option > bound - 1) {
            printf("Not within bounds\n");
            break;    
        }
        printf("Within bounds\n");
        menu[option].fun(s);
    }
    
    return 0;
}