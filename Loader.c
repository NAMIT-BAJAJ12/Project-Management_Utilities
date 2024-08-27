#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>



// Global variables
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
void *mapped_mem;s

/*
 * Release memory and other cleanups
 */
void loader_cleanup() {
    if (mapped_mem) {
        free(mapped_mem);  // Free the allocated memory
    }
    if (fd >= 0) {
        close(fd);
    }
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char *exe) {
    fd = open(exe, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    // Get the size of the file
    off_t filesize = lseek(fd, 0, SEEK_END);
    if (filesize == (off_t)-1) {
        perror("lseek");
        loader_cleanup();
        exit(1);
    }

    // Reset the file offset to the beginning of the file
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        loader_cleanup();
        exit(1);
    }

    // Allocate memory to hold the entire file
    mapped_mem = malloc(filesize);
    if (!mapped_mem) {
        perror("malloc");
        loader_cleanup();
        exit(1);
    }

    // Read the file into memory
    ssize_t bytes_read = read(fd, mapped_mem, filesize);
    if (bytes_read < 0) {
        perror("read");
        loader_cleanup();
        exit(1);
    } else if (bytes_read != filesize) {
        fprintf(stderr, "Partial read: expected %zu, got %zd\n", (size_t)filesize, bytes_read);
        loader_cleanup();
        exit(1);
    }

    // Verify that this is an ELF file by checking the magic number
    ehdr = (Elf32_Ehdr *)mapped_mem;
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Not a valid ELF file\n");
        loader_cleanup();
        exit(1);
    }

    // Process the program headers to load segments into memory
    phdr = (Elf32_Phdr *)(mapped_mem + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
    if (phdr[i].p_type == PT_LOAD) {
        // Step 1: Allocate memory for the segment
        void *segment = mmap(NULL, phdr[i].p_memsz,
                             PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_PRIVATE | MAP_ANONYMOUS,
                             -1, 0);

        // Check if mmap succeeded
        if (segment == MAP_FAILED) {
            perror("mmap");
            loader_cleanup();
            exit(1);
        }

        // Step 2: Copy the segment data from the ELF file into the allocated memory
        memcpy(segment, mapped_mem + phdr[i].p_offset, phdr[i].p_filesz);

        // Optionally, clear the rest of the segment if it includes uninitialized data
        if (phdr[i].p_memsz > phdr[i].p_filesz) {
            memset((char *)segment + phdr[i].p_filesz, 0,
                   phdr[i].p_memsz - phdr[i].p_filesz);
        }
    }
}

    }

    // Navigate to the entrypoint address
    void (*entry_point)() = (void (*)())ehdr->e_entry;

    // Call the entry point function and capture its return value
    int result = entry_point();
    printf("User _start return value = %d\n", result);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable> \n", argv[0]);
        exit(1);
    }

    // Load and run the ELF executable
    load_and_run_elf(argv[1]);

    // Cleanup resources
    loader_cleanup();

    return 0;
}
