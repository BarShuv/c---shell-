#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct virus {
unsigned short SigSize;
char virusName[16];
unsigned char* sig;
} virus;

 char signatureFileName[256] = "signatures-L";
  FILE *fileSig = NULL;


void printHex(unsigned char *buffer, int length) {
    for (int i = 0; i < length; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}
int getFileSize(FILE* file) {
    int size;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return size;
}

void SetSigFileName() {
    printf("Enter sig file name: \n");
    char input[256];
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // Remove the newline character if it exists
        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) > 0) {
            strcpy(signatureFileName, input);
        }
    }
   // free(fileSig);
    if ((fileSig = fopen(input, "r")) == NULL) {
        printf ("No such file\n");
        return;
    }
}

virus *readVirus(FILE *file, int isBigEndian)
{
    virus *virusPtr = malloc(sizeof(virus));
	if (virusPtr == NULL) {
        perror("Failed to allocate memory for virus");
        return NULL;
    }
	// read the virus size
    if (fread(&virusPtr->SigSize, 2, 1, file) != 1)
    {
        free(virusPtr);
        return NULL;
    }
    if (isBigEndian) {
        virusPtr->SigSize = (virusPtr->SigSize << 8) | (virusPtr->SigSize >> 8);
    }
	// read the virus name
    if (fread(virusPtr->virusName, sizeof(char), 16, file) != 16) {
        free(virusPtr);
        return NULL;
    }
	//creating space for sig and reading it
    // virusPtr->sig = malloc(virusPtr->SigSize );
     virusPtr->sig = malloc(virusPtr->SigSize*sizeof(char) );
    fread(virusPtr->sig, virusPtr->SigSize, 1, file);
    return virusPtr;
}

void printVirus(virus *virus)
{
    if (virus == NULL) {
        return;
    }

    printf("name: %s\n", virus->virusName);
    printf("length: %d\n", virus->SigSize);
   printHex(virus->sig, virus->SigSize);
}

virus *virusMemoryFree(virus *virus)
{
    free(virus->sig);
    free(virus);
    return NULL;
}
typedef struct link link;
struct link {
    link *nextVirus;
    virus *vir;
};
void list_print(link *virusList){
    while (virusList != NULL)
    {
        printVirus(virusList->vir);
        virusList = virusList->nextVirus;
    }
}
link* list_append(link* virusList, virus* data){
    //If the list is null - create a new entry and return a pointer to the entry
    if (virusList == NULL)
    {
        virusList = malloc(sizeof(link));
        virusList->nextVirus = NULL;
        virusList->vir = data;
        return virusList;
    }
    //new link at the beging
    link *temp = malloc(sizeof(link));
    temp->nextVirus = virusList;
    temp->vir = data;
    return temp;

}
list_free(link *virusList){
    while (virusList != NULL)
    {
        virusMemoryFree(virusList->vir);
        link *temp = virusList; //this is to move to next and then free what is in virusList
        virusList = virusList->nextVirus;
        free(temp);
    }
}
struct fun_desc
{
    char *name;
    link *(*fun)(link *, FILE *);
};

link *ldSig(link *virusList, FILE * file)
{
    if(virusList != NULL) {
        list_free(virusList);
        virusList = NULL;
    }
    if(fileSig==NULL){
        printf( "This is not the right file\n");
        list_free(virusList);
        exit(1);
    }
    //check magic number, check correct length and viral or vrib
    char magic[5] = {0}; 
    fread(magic, 1, 4, fileSig);
    if (strcmp(magic, "VIRL") != 0 && strcmp(magic, "VIRB") != 0) {
        printf("Invalid magic number: %s\n", magic);
        fclose(fileSig);
        return;
    }
    int isBigEndian =0;
    if (strcmp(magic, "VIRB") == 0) {
        printf("inside");
        isBigEndian = 1;
    }
    virus *virusPtr;

    while ((virusPtr = readVirus(fileSig, isBigEndian)) != NULL)
    {
        virusList = list_append(virusList, virusPtr);
    }
  //  fclose(fileSig); todo
    return virusList;
}

link *printSig(link *virusList)
{
    list_print(virusList);
    return virusList;
}
void detect_virus(char *buffer, unsigned int fileSize, link *virusList)
{   //the buf is the file we want to fix and the  virus->sig is the respective virus signature 
for (int i = 0 ; i < fileSize ; i++) {//we are going over the index and every time compering it to all the respective virus signature 
        link *list=virusList;
        while (list != NULL) {
            virus *v = list->vir;
            int len = v->SigSize;
            if (memcmp(buffer + i, v->sig, len) == 0) { // if it is 0 meaning it is the same as one of our viruses
                printf("from index in the suspected file %d \n", i);
                printf("virus name: %s \n", v->virusName);
                printf("size of the virus signature: %d \n\n", v->SigSize);
            }
            list=list->nextVirus;
        }
    }
}
 link *detectViruses(link * virusList)
    {
    printf("Enter the file you want to check: \n");
    char input[512];
    
    if (fgets(input, 512, stdin) == NULL) {
        printf ("Error loading the file\n");
    }

    input[strcspn(input, "\n")] = '\0';
    FILE *file;
    if ((file = fopen(input, "r")) == NULL) {
        printf ("No such file\n");
        return;
    }
    char buffer[10000];
    int minSize = fread(buffer, 1, 10000, file); //reading the file into buffer
    detect_virus(buffer, minSize, virusList);
    fclose(file); 
    return virusList;
    }

link *quit(link * virusList, FILE * file){
    list_free(virusList);
    fclose(fileSig);
    exit(0);
}
void neutralize_virus(char *fileName, int signatureOffset){
    char ret[] = {0xC3};
    FILE *file = fopen(fileName, "r+");
    fseek(file, signatureOffset, SEEK_SET);
    fwrite(ret, 1, 1, file);
    fclose(file);
}

link *fixFile(link * virusList) {
    printf("Enter file to fix: \n");
    char input[512];
    
    if (fgets(input, 512, stdin) == NULL) {
        printf ("Error loading the file\n");
    }

    input[strcspn(input, "\n")] = '\0';
    FILE *file;
    if ((file = fopen(input, "r")) == NULL) {
        printf ("No such file\n");
        return;
    }
    else {// same as in detecet but here we fix virus insted of printing it
        char buffer[10000];
        int minSize = fread(buffer, 1, 10000, file); 
            for (int i = 0 ; i < minSize ; i++) {
                link *list=virusList;
                while (list != NULL) {
                    virus *v = list->vir;
                    int len = v->SigSize;
                    if (memcmp(buffer + i, v->sig, len) == 0) {
                        neutralize_virus(input, i);
                    }
                    list=list->nextVirus;
                }        
        }
    }
    fclose(file);
    return virusList;
}
struct fun_desc menu[] = {{"Set signatures file name", &SetSigFileName}, {"Load signatures", &ldSig}, {"Print signatures", &printSig}, {"Detect viruses", &detectViruses}, {"Fix file", &fixFile}, {"Quit", &quit}, {NULL, NULL}};
void printManu(int bound){
        printf("Select operation from the following menu:\n"); 
        for (int i = 0; i < bound - 1; i++)
        {
            printf("%d) %s\n", i, menu[i].name);
        }
}

int main(int argc, char* argv[]) {

        if (argc != 2)
        {
            printf("please write the file name");
            return 1;
        }

    link* virusList = NULL;
    int bound = sizeof(menu) / sizeof(struct fun_desc);
    char input[128]; 
    fileSig = fopen(argv[1], "rb+");

    printManu(bound);
    while (fgets(input, sizeof(input), stdin) != NULL) {

        char ch = input[0];
        if (ch >= '0' && ch <= '5') { 
            printf("Within bounds\n");
            virusList = (*menu[ch - '0'].fun)(virusList, fileSig);
        } else {
            printf("Not within bounds\n");
        }
        printManu(bound);
    }
    fclose(fileSig);
}


