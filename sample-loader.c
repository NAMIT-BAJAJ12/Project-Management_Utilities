#include "loader.h"
#include "stdint.h"
#define ERROR(msg) do { perror(msg); exit(1); } while(0)
static Elf32_Ehdr *e_hdr;
static Elf32_Phdr *p_hdr;
int f;
static void *virt_mem; 

void free(){
    if (f > 2){
        close(f);
    }
    if (e_hdr != NULL){
        free(e_hdr);
    }
    if (p_hdr != NULL){
        free(p_hdr);
    }
    if (virt_mem != NULL){
        munmap(virt_mem, p_hdr->p_memsz);  // This will unmap only if virt_mem was successfully mapped
    }
}
void chk_off(off_t offset, const char *error_msg){
    if (lseek(f, offset, SEEK_SET) == (off_t) -1) {
        ERROR(error_msg);
    }
}
void verify_ELF(const char* filename) {
    f = open(filename, O_RDONLY);  // Open file for reading
    if (f < 0) {
        ERROR("Error opening ELF file");
    }
    Elf32_Ehdr t_hdr;
    chk_off(0, "Error seeking to the beginning of the file");
    if (read(f, &t_hdr, sizeof(t_hdr)) != sizeof(t_hdr)) {
        close(f);
        ERROR("Error reading ELF header");
    }
    // Valid ELF magic number and other fields
    if (t_hdr.e_ident[0] != 0x7f || t_hdr.e_ident[1] != 'E' || t_hdr.e_ident[2] != 'L' || t_hdr.e_ident[3] != 'F' ||
        t_hdr.e_ident[EI_CLASS] != ELFCLASS32 || t_hdr.e_ident[EI_DATA] != ELFDATA2LSB ||
        t_hdr.e_machine != EM_386 || t_hdr.e_ident[EI_VERSION] != EV_CURRENT ||
        (t_hdr.e_type != ET_EXEC && t_hdr.e_type != ET_REL)) {
        close(f);
        ERROR("Invalid or unsupported ELF file");
    }
    printf("ELF file is valid and supported\n");
    close(f);  // Closing the file descriptor after validation
}