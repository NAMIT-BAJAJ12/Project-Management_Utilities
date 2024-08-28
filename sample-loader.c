#include "loader.h"
#include <stdint.h>
#include <errno.h>
// ELF data types
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;

#define ELF_MAGIC 0x464c457f // Magic number for ELF files
#define ELF_NIDENT 16

// ELF Header
typedef struct {
    uint8_t     e_ident[ELF_NIDENT];
    Elf32_Half  e_type;
    Elf32_Half  e_machine;
    Elf32_Word  e_version;
    Elf32_Addr  e_entry;
    Elf32_Off   e_phoff;
    Elf32_Off   e_shoff;
    Elf32_Word  e_flags;
    Elf32_Half  e_ehsize;
    Elf32_Half  e_phentsize;
    Elf32_Half  e_phnum;
    Elf32_Half  e_shentsize;
    Elf32_Half  e_shnum;
    Elf32_Half  e_shstrndx;
} Elf32_Ehdr;

// Program Header
typedef struct {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

// Section Header
typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off  sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

// Error macro
#define ERROR(msg) fprintf(stderr, msg)

// Function prototypes
static bool elf_check_file(Elf32_Ehdr *hdr);
static bool elf_check_supported(Elf32_Ehdr *hdr);
static inline Elf32_Shdr *elf_sheader(Elf32_Ehdr *hdr);
static inline Elf32_Shdr *elf_section(Elf32_Ehdr *hdr, int idx);

static Elf32_Ehdr *ehdr;
static Elf32_Phdr *phdr;
static int fd;
static void *mapped_file;

/*
 * Checking if the ELF file is valid
 */
static bool elf_check_file(Elf32_Ehdr *hdr) {
    if (!hdr) return false;
    if (hdr->e_ident[0] != 0x7f || hdr->e_ident[1] != 'E' || hdr->e_ident[2] != 'L' || hdr->e_ident[3] != 'F') {
        ERROR("ELF Header magic number incorrect.\n");
        return false;
    }
    return true;
}

/*
 * Checking if the ELF file is supported
 */
static bool elf_check_supported(Elf32_Ehdr *hdr) {
    if (!elf_check_file(hdr)) {
        ERROR("Invalid ELF File.\n");
        return false;
    }
    if (hdr->e_ident[4] != 1) { // ELF32
        ERROR("Unsupported ELF File Class.\n");
        return false;
    }
    if (hdr->e_ident[5] != 1) { // Little-endian
        ERROR("Unsupported ELF File byte order.\n");
        return false;
    }
    if (hdr->e_machine != 3) { // Intel 80386
        ERROR("Unsupported ELF File target.\n");
        return false;
    }
    if (hdr->e_ident[6] != 1) { // Current version
        ERROR("Unsupported ELF File version.\n");
        return false;
    }
    if (hdr->e_type != 2 && hdr->e_type != 3) { // Executable or relocatable
        ERROR("Unsupported ELF File type.\n");
        return false;
    }
    return true;
}
