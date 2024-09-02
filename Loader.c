#include "loader.h"
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
Elf32_Phdr *virt_mem;

// Macros for error handling, cleanup, and seeking
#define ERROR(msg) do { printf("%s\n", msg); } while(0)
#define ERROR_CLEANUP_EXIT(msg) do { printf("%s\n", msg); loader_cleanup(); exit(1); } while(0)
#define ERROR_CLEANUP(msg) do { printf("%s\n", msg); loader_cleanup(); } while(0)
#define SEEK(fd, offset, whence) do { if (lseek(fd, offset, whence) == -1) { ERROR_CLEANUP_EXIT("Failed in lseek"); } } while(0)

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
    if (fd > 2){
        close(fd);
    }
    if (ehdr != NULL){
        free(ehdr);
    }
    if (phdr != NULL){
        free(phdr);
    }
    if (virt_mem != MAP_FAILED){
        munmap(virt_mem, phdr->p_memsz);
    }
}
// Function to verify if the file is a valid ELF file
int verify_elf(const char *filename) {
    fd = open(filename, O_RDONLY);
    if (fd == -1){
        ERROR("Error in opening the ELF file ...");
        return 0;
    }
    const char elf_chk[] = {0x7F, 'E', 'L', 'F'};
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (!ehdr){
        ERROR_CLEANUP_EXIT("Error in memory allocation for ELF header");
    }
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)){
        free(ehdr);
        ERROR_CLEANUP_EXIT("Invalid ELF header");
    }
    if (memcmp(elf_chk, ehdr->e_ident, 4) != 0){
        free(ehdr);
        ERROR("Invalid or unsupported file type");
        close(fd);
        return 0;
    }
    return 1;
}
/*
Load and run the ELF executable file
*/
void load_and_run_elf(char **exe){

    // 1. Load entire binary content into the memory from the ELF file.

    SEEK(fd, 0, SEEK_SET);
    long elf_s = lseek(fd, 0, SEEK_END);
    if (elf_s == -1){
        ERROR_CLEANUP_EXIT("Failed in lseek");
    }

    SEEK(fd, 0, SEEK_SET);

    // Allocating memory to hold the elf file 
    ehdr = (Elf32_Ehdr *)realloc(ehdr, elf_s);
    phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));

    // Reading the elf file header into memory

    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        ERROR_CLEANUP_EXIT("Error in loading ELF data into memory ... ");
    }

   // 2. Iterate through the PHDR table and find the section of PT_LOAD 
   //    type that contains the address of the entrypoint method in fib.c

    SEEK(fd, ehdr->e_phoff, SEEK_SET);
    int k=0;
    while(k<ehdr->e_phnum){
    

        ssize_t phdr_R = read(fd, phdr, sizeof(Elf32_Phdr));
        if (phdr_R != sizeof(Elf32_Phdr)){
            ERROR_CLEANUP_EXIT("Error in loading phdr header ...");
        k++;
        }

   // 3. Allocate memory of the size "p_memsz" using mmap function 
   //    and then copy the segment content

        if (phdr->p_type == PT_LOAD){

            if (phdr->p_vaddr < ehdr->e_entry){

                if (ehdr->e_entry < phdr->p_memsz + phdr->p_vaddr){

                    // Map a region of memory to load the program segment
                    virt_mem = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, phdr->p_offset);
                    
                    if (virt_mem == MAP_FAILED){

                        ERROR_CLEANUP_EXIT("Error in mapping a region of memory to load program segment ...");
                    }
                    break;
                }
            }
        }
    }
    // End of the loop


    // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step

    char *entry_p = (char *)virt_mem + (ehdr->e_entry - phdr->p_vaddr);

    // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.

    typedef int (*start_method)();
    start_method _start = (start_method)entry_p;

    // 6. Call the "_start" method and print the value returned from the "_start"

    int result = _start();
    printf("User _start return value = %d\n", result);
}
int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }
    // 1. carry out necessary checks on the input ELF file [verify_elf() defined above from line 30-52]
    if (!verify_elf(argv[1])) {
        ERROR_CLEANUP_EXIT("Invalid ELF");
    }

    // Passing it to the loader for carrying out the loading/execution
    load_and_run_elf(argv);

    // Invoke the cleanup routine inside the loader  
    loader_cleanup();

    return 0;
}
