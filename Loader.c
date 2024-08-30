#include "loader.h"
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
int *data;
Elf32_Phdr *virt_mem;

// Macros for error handling, cleanup, and seeking
#define ERROR(msg) do { printf("%s\n", msg);} while(0)
#define ERROR_CLEANUP_EXIT(msg) do { printf("%s\n", msg); loader_cleanup(); exit(1); } while (0)
#define SEEK(fd, offset, whence) do { if (lseek(fd, offset, whence) == -1) { ERROR_CLEANUP_EXIT("Failed in lseek"); } } while (0)
#define CLEANUP_AND_EXIT() do { loader_cleanup(); exit(1); } while (0)

// Function to release memory and other cleanups
void loader_cleanup(){
    if (fd > 2){
        close(fd);
    }
    if (ehdr != NULL){
        free(ehdr);
    }
    if (phdr != NULL){
        free(phdr);
    }
}

// Function to verify if the file is a valid ELF file
int verify_elf(const char *filename){
    // Open the file
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        ERROR_CLEANUP_EXIT("Error in opening ELF file ...\n");
    }

    const char elf_chk[] = {0x7F, 'E', 'L', 'F'};
    // Read the ELF header
    Elf32_Ehdr *e_hdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (!e_hdr){
        ERROR_CLEANUP_EXIT("Error allocating memory for ELF header");
    }

    // Corrected: Read the ELF header
    if (read(fd, e_hdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)){
        free(e_hdr);
        ERROR_CLEANUP_EXIT("Error in reading ELF header\n");
    }

    // Validate the ELF magic number
    if (memcmp(elf_chk, e_hdr->e_ident, 4) != 0) {
        free(e_hdr);
        ERROR_CLEANUP_EXIT("Invalid or unsupported file type\n");
    }
    close(fd);
    return 1;
}

// Load and run the ELF executable file
void load_and_run_elf(char **exe){
    fd = open(exe[1], O_RDONLY);
    if (fd == -1) {
        ERROR_CLEANUP_EXIT("Failed in opening ELF file ...");
    }

    // 1. Load entire binary content into the memory from the ELF file.
    long elf_s = lseek(fd, 0, SEEK_END);
    if (elf_s == -1){
        ERROR_CLEANUP_EXIT("Failed in lseek, seeking to end of file ...");
    }

    // Rewind to the beginning of the file
    SEEK(fd, 0, SEEK_SET);

    // Allocate memory to hold the ELF file
    ehdr = (Elf32_Ehdr *)malloc(elf_s);
    phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));

    // Read the ELF file header into memory
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)){
        ERROR_CLEANUP_EXIT("Error in reading ELF content into memory ... ");
    }

    // 2. Iterate through the PHDR table and find the section of PT_LOAD type
    // that contains the address of the entry point method in fib.c
    SEEK(fd, ehdr->e_phoff, SEEK_SET);
    for (int i = 0; i < ehdr->e_phnum; i++){
        ssize_t phdr_R = read(fd, phdr, sizeof(Elf32_Phdr));
        if (phdr_R != sizeof(Elf32_Phdr)) {
            ERROR_CLEANUP_EXIT("Error in reading phdr header ...");
        }

        // 3. Allocate memory of the size "p_memsz" using mmap function
        // and then copy the segment content
        if (phdr->p_type == PT_LOAD && phdr->p_vaddr < ehdr->e_entry && ehdr->e_entry < phdr->p_memsz + phdr->p_vaddr){
            // Map a region of memory to load the program segment
            virt_mem = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
            SEEK(fd, phdr->p_offset, SEEK_SET);
            if (virt_mem == MAP_FAILED){
                ERROR_CLEANUP_EXIT("Error in mapping a region of memory to load program segment ...");
            }

            // Read the program data and copy it to the mapped memory
            ssize_t data_R = read(fd, virt_mem, phdr->p_memsz);
            if (data_R != phdr->p_memsz){
                ERROR("Error reading program data into virtual memory...");
                loader_cleanup();
                munmap(virt_mem, phdr->p_memsz);
                exit(1);
            }
            break;
        }
    }

    // End of for loop
    // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
    char *function_entry_point = (char *)virt_mem + (ehdr->e_entry - phdr->p_vaddr);

    // 5. Define a function pointer type and assign the address
    typedef int (*start_func_t)();  // Define a function pointer type that returns int
    start_func_t _start = (start_func_t)function_entry_point;

    // 6. Call the "_start" method and print the value returned from the "_start"
    int result = _start();  
    printf("User _start return value = %d\n", result);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable> \n", argv[0]);
        exit(1);
    }

    // Verify the ELF file
    if (!verify_elf(argv[1])) {
        exit(1);
    }

    // Passing it to the loader for carrying out the loading/execution
    load_and_run_elf(argv);

    // Invoke the cleanup routine inside the loader  
    loader_cleanup();

    return 0;
}
