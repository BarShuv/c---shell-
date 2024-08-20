#include <stdio.h>
#include <unistd.h>
#include <elf.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

Elf32_Ehdr *header;
extern int startup(int argc, char **argv, void (*start)());

//function to load ELF program headers into memory and print information
void load_phdr(Elf32_Phdr *phdr, int fd) {
    void *vaddr_aligned = (void *)(phdr->p_vaddr & 0xfffff000);
    int offset_aligned = phdr->p_offset & 0xfffff000;
    int padding = phdr->p_vaddr & 0xfff;

    //convert to mmap flags
    int mmap_flags = (phdr->p_flags == 0x000) ? 0 :
                     (phdr->p_flags == 0x001) ? PROT_READ :
                     (phdr->p_flags == 0x002) ? PROT_WRITE :
                     (phdr->p_flags == 0x003) ? (PROT_READ | PROT_WRITE) :
                     (phdr->p_flags == 0x004) ? PROT_EXEC :
                     (phdr->p_flags == 0x005) ? (PROT_READ | PROT_EXEC) :
                     (phdr->p_flags == 0x006) ? (PROT_EXEC | PROT_WRITE) :
                     (phdr->p_flags == 0x007) ? (PROT_READ | PROT_WRITE | PROT_EXEC) : -1;

    //perform mmap - place to map in, size, protections,
    void *map_start = mmap(vaddr_aligned, phdr->p_memsz + padding, mmap_flags, MAP_FIXED | MAP_PRIVATE, fd, offset_aligned);

    if (map_start == MAP_FAILED) {
        perror("mmap failed");
        exit(-4);
    }

    //convert p_type to string representation
    const char *type_string = (phdr->p_type == PT_NULL) ? "NULL" :
                           (phdr->p_type == PT_LOAD) ? "LOAD" :
                           (phdr->p_type == PT_DYNAMIC) ? "DYNAMIC" :
                           (phdr->p_type == PT_INTERP) ? "INTERP" :
                           (phdr->p_type == PT_NOTE) ? "NOTE" :
                           (phdr->p_type == PT_SHLIB) ? "SHLIB" :
                           (phdr->p_type == PT_PHDR) ? "PHDR" :
                           (phdr->p_type == PT_TLS) ? "TLS" : "UNKNOWN";

    //print ELF program header information
    printf("%-10s 0x%06x 0x%08x 0x%08x 0x%05x 0x%05x %s 0x%x\n",
           type_string, phdr->p_offset, phdr->p_vaddr, phdr->p_paddr,
           phdr->p_filesz, phdr->p_memsz,
           (phdr->p_flags == 0x000) ? "" :
           (phdr->p_flags == 0x001) ? "E" :
           (phdr->p_flags == 0x002) ? "W" :
           (phdr->p_flags == 0x003) ? "WE" :
           (phdr->p_flags == 0x004) ? "R" :
           (phdr->p_flags == 0x005) ? "RE" :
           (phdr->p_flags == 0x006) ? "RW" :
           (phdr->p_flags == 0x007) ? "RWE" : "Unknown",
           phdr->p_align);
}
//function to iterate over ELF program headers and apply a callback function
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int fd) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) map_start;

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Not an ELF file.\n");
        return -1;
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "Not a 32-bit ELF file.\n");
        return -1;
    }
    //apply function to each header
    Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        func(&phdr[i], fd);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        perror("error : expected more arguments");
        exit(1);
    }

    const char *file_name = argv[1];
    int fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        perror("error in open file");
        return 1;
    }

    struct stat stat1;
    if (fstat(fd, &stat1) == -1) {
        perror("error in get file size");
        close(fd);
        return 1;
    }

    void *map_start = mmap(NULL, stat1.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("Failed to map file");
        close(fd);
        return 1;
    }

    header = (Elf32_Ehdr *) map_start;

    printf("Program Headers:\n");
    printf("%-10s %-8s %-10s %-10s %-6s %-6s %-3s %-5s\n",
           "Type", "Offset", "VirtAddr", "PhysAddr", "FileSiz", "MemSiz", "Flags", "Align");
    //load headers
    foreach_phdr(map_start, load_phdr, fd);

    //call startup function with adjusted arguments
    startup(argc - 1, argv + 1, (void *)(header->e_entry));

    munmap(map_start, stat1.st_size);
    close(fd);

    return 0;
}
