#include "loader.h"
#include <stdint.h>
#include <errno.h>

#define ELF_MAGIC 0x464c457f // Magic number for ELF files
#define ELF_NIDENT 16
#define ERROR(msg) fprintf(stderr, msg)

static Elf32_Ehdr *ehdr;
static Elf32_Phdr *phdr;
static int fd;
static int entrypoint;
void *virt_mem = NULL;
static void *mapped_file;

#define ERROR(msg) do { printf(msg); exit(1); } while(0)

// Function to check if the ELF file is valid and supported
void validate_elf_file(const char* exe) {
    int fd = open(exe, O_RDONLY);  // Open file for reading
    if (fd < 0) {
        ERROR("Error opening ELF file\n");
    }

    Elf32_Ehdr hdr;
    check_offset(lseek(fd, 0, SEEK_SET));  // Ensure we are at the beginning
    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        close(fd);
        ERROR("Error reading ELF header\n");
    }

    // Validate ELF magic number and other fields
    if (hdr.e_ident[0] != 0x7f || hdr.e_ident[1] != 'E' || hdr.e_ident[2] != 'L' || hdr.e_ident[3] != 'F' ||
        hdr.e_ident[EI_CLASS] != ELFCLASS32 || hdr.e_ident[EI_DATA] != ELFDATA2LSB ||
        hdr.e_machine != EM_386 || hdr.e_ident[EI_VERSION] != EV_CURRENT ||
        (hdr.e_type != ET_EXEC && hdr.e_type != ET_REL)) {
        close(fd);
        ERROR("Invalid or unsupported ELF file\n");
    }

    printf("ELF file is valid and supported\n");
    close(fd);  // Close the file descriptor after validation
}


// Function to check if offset seeking was successful
void check_offset(off_t new_position) {
    if (new_position == -1) {
        ERROR("Failed to seek offset\n");
    }
}

/*
 * Release memory and perform other cleanups
 */
void free_space(){
    free(ehdr);
    free(phdr);
}

// Unmapping virtual mem and closing the file descriptor
void unmapping_virt_mem(){
    if (virtual_mem != NULL) {
        munmap(virtual_mem, phdr[i].p_memsz);
    }
    close(fd);
}

