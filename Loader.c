#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
Elf32_Phdr *virt_mem;

// Macros for error handling, cleanup, and seeking
#define ERROR(msg) do { printf("%s\n", msg); } while(0)
#define ERROR_CLEANUP_EXIT(msg) do { printf("%s\n", msg); loader_cleanup(); exit(1); } while(0)
#define SEEK(fd, offset, whence) do { if (lseek(fd, offset, whence) == -1) { ERROR_CLEANUP_EXIT("Failed in lseek"); } } while(0)

// Function to release memory and other cleanups
void loader_cleanup() {
    if (fd > 2) close(fd);
    if (ehdr != NULL) free(ehdr);
    if (phdr != NULL) free(phdr);
    if (virt_mem != MAP_FAILED) munmap(virt_mem, phdr->p_memsz);
}

// Function to verify if the file is a valid ELF file
int verify_elf(const char *filename) {
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        ERROR("Error in opening ELF file ...");
        return 0;
    }

    const char elf_chk[] = {0x7F, 'E', 'L', 'F'};
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (!ehdr) {
        ERROR_CLEANUP_EXIT("Error allocating memory for ELF header");
    }

    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        free(ehdr);
        ERROR_CLEANUP_EXIT("Error in reading ELF header");
    }

    if (memcmp(elf_chk, ehdr->e_ident, 4) != 0) {
        free(ehdr);
        ERROR("Invalid or unsupported file type");
        close(fd);
        return 0;
    }

    return 1;
}

// Load and run the ELF executable file
void load_and_run_elf(char **exe) {
    SEEK(fd, 0, SEEK_SET);

    long elf_s = lseek(fd, 0, SEEK_END);
    if (elf_s == -1) {
        ERROR_CLEANUP_EXIT("Failed in lseek, seeking to end of file ...");
    }

    SEEK(fd, 0, SEEK_SET);

    ehdr = (Elf32_Ehdr *)realloc(ehdr, elf_s);
    phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));

    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        ERROR_CLEANUP_EXIT("Error in reading ELF content into memory ... ");
    }

    SEEK(fd, ehdr->e_phoff, SEEK_SET);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        ssize_t phdr_R = read(fd, phdr, sizeof(Elf32_Phdr));
        if (phdr_R != sizeof(Elf32_Phdr)) {
            ERROR_CLEANUP_EXIT("Error in reading phdr header ...");
        }

        if (phdr->p_type == PT_LOAD && phdr->p_vaddr < ehdr->e_entry && ehdr->e_entry < phdr->p_memsz + phdr->p_vaddr) {
            virt_mem = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, phdr->p_offset);
            if (virt_mem == MAP_FAILED) {
                ERROR_CLEANUP_EXIT("Error in mapping a region of memory to load program segment ...");
            }
            break;
        }
    }

    char *function_entry_point = (char *)virt_mem + (ehdr->e_entry - phdr->p_vaddr);
    typedef int (*start_func_t)();
    start_func_t _start = (start_func_t)function_entry_point;

    int result = _start();
    printf("User _start return value = %d\n", result);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    if (!verify_elf(argv[1])) {
        exit(1);
    }

    load_and_run_elf(argv);

    loader_cleanup();

    return 0;
}
