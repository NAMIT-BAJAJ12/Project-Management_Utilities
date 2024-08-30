#include "loader.h"
#include "stdint.h"
#define ERROR(msg) do { perror(msg); exit(1); } while(0)
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
void *virt_mem; 
void loader_cleanup(){
    if (fd > 2){
        close(fd);
    }
    if (ehdr != NULL){
        loader_cleanup(ehdr);
    }
    if (phdr != NULL){
        loader_cleanup(phdr);
    }
    if (virt_mem != NULL){
        munmap(virt_mem, phdr->p_memsz);  // This will unmap only if virt_mem was successfully mapped
    }
}
void chk_off(off_t offset, const char *error_msg){
    if (lseek(fd, offset, SEEK_SET) == (off_t) -1) {
        ERROR(error_msg);
    }
}
void verify_ELF(const char* filename) {
    fd = open(filename, O_RDONLY);  // Open file for reading
    if (fd < 0) {
        ERROR("Error opening ELF file");
    }
    Elf32_Ehdr t_hdr;
    chk_off(0, "Error seeking to the beginning of the file");
    if (read(fd, &t_hdr, sizeof(t_hdr)) != sizeof(t_hdr)) {
        close(fd);
        ERROR("Error reading ELF header");
    }
    // Valid ELF magic number and other fields
    if (t_hdr.e_ident[0] != 0x7f || t_hdr.e_ident[1] != 'E' || t_hdr.e_ident[2] != 'L' || t_hdr.e_ident[3] != 'F' ||
        t_hdr.e_ident[EI_CLASS] != ELFCLASS32 || t_hdr.e_ident[EI_DATA] != ELFDATA2LSB ||
        t_hdr.e_machine != EM_386 || t_hdr.e_ident[EI_VERSION] != EV_CURRENT ||
        (t_hdr.e_type != ET_EXEC && t_hdr.e_type != ET_REL)) {
        close(fd);
        ERROR("Invalid or unsupported ELF file");
    }
    ERROR("ELF file is valid and supported\n");
    close(fd);  // Closing the file descriptor after validation
}
void load_and_run_elf(char **argv){
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        ERROR("Failed to open ELF file");
        exit(1);
    }
    // 1. Load entire binary content into the memory from the ELF file.
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1){
        ERROR("Failed in lseek, seeking to end of file");
        loader_cleanup();
        exit(1);
    }
    // Rewind to the beginning of the file
    if (lseek(fd, 0, SEEK_SET) == -1){
        ERROR("Failed in lseek, seeking to beginning of file   ...");
        loader_cleanup();
        exit(1);
    }

    // Allocate memory to hold the ELF file
    ehdr = (Elf32_Ehdr *)malloc(file_size);
    if (!ehdr){
        ERROR("Error allocating memory for ELF header");
        loader_cleanup();
        exit(1);
    }
    phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));
    if (!phdr){
        ERROR("Error allocating memory for program header");
        loader_cleanup();
        exit(1);
    }
    // Read the ELF file header into memory
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)){
        ERROR("Error reading ELF header");
        loader_cleanup();
        exit(1);
    }
    // 2. Iterate through the PHDR table and find the section of PT_LOAD 
   //     type that contains the address of the entrypoint method in fib.c
    chk_off(ehdr->e_phoff, "Error seeking to program header table");
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (read(fd, phdr, sizeof(Elf32_Phdr)) != sizeof(Elf32_Phdr)){
            ERROR("Error reading program header");
            loader_cleanup();
            exit(1);
        }
    // 3. Allocate memory of the size "p_memsz" using mmap function 
   //     and then copy the segment content
        if (phdr->p_type == PT_LOAD && 
            phdr->p_vaddr < ehdr->e_entry && 
            ehdr->e_entry < phdr->p_memsz + phdr->p_vaddr){
            // Map a region of memory to load the program segment
            virt_mem = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            chk_off(phdr->p_offset, "Error seeking to program segment");
            if (virt_mem == MAP_FAILED) {
                ERROR("Error mapping memory for program segment");
                loader_cleanup();
                exit(1);
            }
            if (read(fd, virt_mem, phdr->p_filesz) != phdr->p_filesz){
                ERROR("Error reading program data into memory");
                loader_cleanup();
                munmap(virt_mem, phdr->p_memsz);    
                exit(1);
            }
            break;
        }
    }

    // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
    char *entry_point = (char *)virt_mem + (ehdr->e_entry - phdr->p_vaddr);

    // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
    int (_start)() = (int ()())entry_point;

    // 6. Call the "_start" method and print the value returned from the "_start"
    int result = _start();
    printf("User _start return value = %d\n", result);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    // 1. carry out necessary checks on the input ELF file
    verify_ELF(argv[1]);

    // 2. passing it to the loader for carrying out the loading/execution
    load_and_run_elf(argv);

    // 3. invoke the cleanup routine inside the loader  
    loader_cleanup();
    return 0;
}
