#include <stdio.h>
#include <stdlib.h>

// both structs from linux foundation
typedef struct
{
    unsigned char ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} elf_header_t;

typedef struct
{
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset; // this gives the offset in the file. Use this
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
} elf_sh_t;

// typedef struct {

// } elf_section_offsets_t;

enum
{
    SHT_NULL,
    SHT_PROGBITS,
    SHT_SYMTAB,
    SHT_STRTAB,
    SHT_RELA,
    SHT_HASH,
    SHT_DYNAMIC,
    SHT_NOTE,
    SHT_NOBITS,
    SHT_REL,
    SHT_SHLIB,
    SHT_DYNSYM,
    SHT_INIT_ARRAY,
    SHT_FINI_ARRAY,
    SHT_PREINIT_ARRAY,
    SHT_GROUP,
    SHT_SYMTAB_SHNDX,
};

#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MERGE 0x10
#define SHF_STRINGS 0x20
#define SHF_INFO_LINK 0x40
#define SHF_LINK_ORDER 0x80
#define SHF_OS_NONCONFORMING 0x100
#define SHF_GROUP 0x200
#define SHF_TLS 0x400
#define SHF_MASKOS 0x0ff00000
#define SHF_MASKPROC 0xf0000000

#define CHECK_TYPE(e, _type)   (e->type == _type)
#define CHECK_FLAGS(e, _flags) ((e->flags ^ (_flags)) == 0) 
/* 
XOR places 1 in any bit where that flag is wrong.
If all right ->  0x0000000
if some wrong -> 0x0001000
If number = 0, then it was correct
*/
#define IS_BSS(e)     ((e->size != 0) && (CHECK_TYPE(e, SHT_NOBITS)   && (CHECK_FLAGS(e, SHF_ALLOC + SHF_WRITE))))
#define IS_DATA(e)    ((e->size != 0) && (CHECK_TYPE(e, SHT_PROGBITS) && (CHECK_FLAGS(e, SHF_ALLOC + SHF_WRITE))))
#define IS_RODATA(e)  ((e->size != 0) && (CHECK_TYPE(e, SHT_PROGBITS) && (CHECK_FLAGS(e, SHF_ALLOC))))
#define IS_TEXT(e)    ((e->size != 0) && (CHECK_TYPE(e, SHT_PROGBITS) && (CHECK_FLAGS(e, SHF_ALLOC + SHF_EXECINSTR))))
//#define IS_RELTEXT(e) ((e->size != 0) && (CHECK_TYPE(e, SHT_REL)      && (CHECK_FLAGS(e, SHF_ALLOC))))

void elf_exec(char *file, uint32_t size)
{
    elf_header_t *header = (elf_header_t *)file;
    uint32_t SH_table = header->shoff;
    uint16_t SH_numEntries = header->shnum;    // number of SH entries
    uint16_t SH_entrySize = header->shentsize; // should be 40 (int size * 10 ints in SH struct)

    elf_sh_t *rodata_entry = 0;
    elf_sh_t *data_entry = 0;
    elf_sh_t *bss_entry = 0;
    elf_sh_t *text_entry = 0;


    // find data section in UNLINKED
    for (int i = 0; i < SH_numEntries; i++) // for each entry
    {
        // addr = file pointer + table offset + (index * size of struct)
    
        elf_sh_t *SH_entry = (elf_sh_t *)(file + SH_table + (i * SH_entrySize));

        if (IS_RODATA(SH_entry)) // rules rodata section
        {
            if(rodata_entry == 0)
                rodata_entry = SH_entry;
        }
        //printf("%p %i %i %i\n", SH_entry, SH_table, SH_numEntries, SH_entrySize); fflush(stdout); 

        else if (IS_DATA(SH_entry))
        {
            if(data_entry == 0)
                data_entry = SH_entry;
        }
        else if (IS_BSS(SH_entry))
        {
            if(bss_entry == 0)
                bss_entry = SH_entry;
        }
        else if (IS_TEXT(SH_entry))
        {
            if(text_entry == 0)
                text_entry = SH_entry;
        }
    }
    printf("RODATA %i:%i\n", rodata_entry->offset, rodata_entry->size);
    printf("DATA   %i:%i\n",   data_entry->offset, data_entry->size);
    printf("TEXT   %i:%i\n",   text_entry->offset, text_entry->size);

    uint32_t first_data_offset; 
    uint32_t total_data_size; // total size of both data sectors
    
    if(rodata_entry->offset < data_entry->offset)
    {
        first_data_offset = rodata_entry->offset;  
        total_data_size = (data_entry->size + data_entry->offset) - rodata_entry->offset;
    } else {
        first_data_offset = data_entry->offset;
        total_data_size = (rodata_entry->size + rodata_entry->offset) - data_entry->offset;
    }

    printf("===FIRST===\n -addr: %i\n -size: %i\n", first_data_offset, total_data_size);
    printf("===ENTRY=== %p (needs to be added to text section)\n", (void*) header->entry);
}

int main()
{
    // data section offsets are actually starting at 0. Ignore dissasembler
    FILE *fp = fopen("/Users/squijano/Documents/osReal/apps/printHello", "r");
    char buff[1000];
    fread(buff, sizeof(buff), 1, fp);
    elf_exec(buff, 1000);
    return 0;
}