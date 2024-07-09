#include <common.h>
#include <elf.h>

typedef MUXDEF(CONFIG_ISA64, Elf64_Ehdr, Elf32_Ehdr) Elf_Ehdr;
typedef MUXDEF(CONFIG_ISA64, Elf64_Half, Elf32_Half) Elf_Half;
typedef MUXDEF(CONFIG_ISA64, Elf64_Off,  Elf32_Off ) Elf_Off;
typedef MUXDEF(CONFIG_ISA64, Elf64_Word, Elf32_Word) Elf_Word;
typedef MUXDEF(CONFIG_ISA64, Elf64_Addr, Elf32_Addr) Elf_Addr;

typedef MUXDEF(CONFIG_ISA64, Elf64_Shdr, Elf32_Shdr) Elf_Shdr;
typedef MUXDEF(CONFIG_ISA64, Elf64_Sym,  Elf32_Sym)  Elf_Sym;
#define ELF_ST_TYPE MUXDEF(CONFIG_ISA64, ELF64_ST_TYPE,  ELF32_ST_TYPE)

struct ELF {
  char *elf_file;
  FILE *elf_fp;
  Elf_Ehdr *elf_ehdr_ptr;
  Elf_Shdr *elf_shdrs_ptr;
  char *shstrtab_ptr;
  char *strtab_ptr;
  Elf_Sym *symtab_ptr;
  Elf_Word symtab_size;
  struct ELF* next;
};

typedef struct ELF ELF;

static char *load_elf_data(ELF *elf, long offset, size_t sizes);
static void load_elf_section_header_table(ELF *elf);

static ELF *elf_list = NULL;

void add_elf(char *elf_file) {
  if (elf_file != NULL) {

    ELF *new_elf = malloc(sizeof(ELF));
    memset(new_elf, 0, sizeof(ELF));
    new_elf -> elf_file = elf_file;

    if ( !elf_list ) {
      elf_list = new_elf;
    } else {
      ELF *cur = elf_list;
      while ( cur->next != NULL ){
        cur = cur->next;
      }
      cur->next = new_elf;
    }
  } else {
    Log("No Elf is given.");
  }
}

static char *load_elf_data(ELF *elf, long offset, size_t sizes) {
  char *ptr = malloc(sizes);
  fseek(elf -> elf_fp, offset, SEEK_SET);
  int ret = fread(ptr, sizes, 1, elf -> elf_fp);
  assert(ret == 1);
  return ptr;
}

void load_elf() {
  ELF *cur = elf_list;
  while ( cur != NULL ){
    char *elf_file = cur->elf_file;
    FILE *fp = fopen(elf_file, "r");
    Assert(fp, "Can not open '%s'", elf_file);
    cur->elf_fp = fp;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    Log("The Elf is %s, size = %ld", elf_file, size);

    long ehdr_size = sizeof (Elf_Ehdr);
    cur->elf_ehdr_ptr = (Elf_Ehdr *) load_elf_data(cur, 0, ehdr_size);

    load_elf_section_header_table(cur);

    fclose(fp);
    cur = cur->next;
  }
}

static void load_elf_section_header_table(ELF *cur_elf) {
  // Log("Section header table file offset: "FMT_PADDR"", cur_elf->elf_ehdr_ptr->e_shoff);
  // Log("Section header table entry size: %d", cur_elf->elf_ehdr_ptr->e_shentsize);
  // Log("Section header table entry count: %d", cur_elf->elf_ehdr_ptr->e_shnum);
  // Log("Section header string table index: %d", cur_elf->elf_ehdr_ptr->e_shstrndx);

  Elf_Ehdr *elf_ehdr_ptr = cur_elf->elf_ehdr_ptr;
  
  long shdr_size = sizeof (Elf_Shdr);
  assert(shdr_size == (elf_ehdr_ptr->e_shentsize));
  long shdrs_size = elf_ehdr_ptr->e_shentsize * elf_ehdr_ptr->e_shnum;  

  assert(elf_ehdr_ptr->e_shoff);
  cur_elf->elf_shdrs_ptr = (Elf_Shdr *) load_elf_data(cur_elf, elf_ehdr_ptr->e_shoff, shdrs_size);

  assert(elf_ehdr_ptr->e_shstrndx != SHN_UNDEF);  
  Elf_Shdr *shdr_shstrtab = cur_elf->elf_shdrs_ptr + elf_ehdr_ptr->e_shstrndx;
  assert(shdr_shstrtab->sh_type == SHT_STRTAB);
  cur_elf->shstrtab_ptr = load_elf_data(cur_elf, shdr_shstrtab->sh_offset, shdr_shstrtab->sh_size);
  
  for(size_t i = 0; i < elf_ehdr_ptr->e_shnum; i++){
    Elf_Shdr *shdr = cur_elf->elf_shdrs_ptr + i;
    char *shdr_name = cur_elf->shstrtab_ptr + shdr->sh_name;
    Elf_Off sh_offset = shdr->sh_offset;
    Elf_Word sh_size = shdr->sh_size;
    // Log("Section header name=[%s] offset=["FMT_PADDR"] size=[%d]", shdr_name, sh_offset, sh_size);
    if(strcmp(shdr_name, ".strtab") == 0){
      cur_elf->strtab_ptr = load_elf_data(cur_elf, sh_offset, sh_size);
    } else if(strcmp(shdr_name, ".symtab") == 0) {
      cur_elf->symtab_ptr = (Elf_Sym *) load_elf_data(cur_elf, sh_offset, sh_size);
      cur_elf->symtab_size = sh_size;
    } else {
    }
  }
}

char *load_func_from_elf(paddr_t address, bool is_exact) {
  ELF *cur = elf_list;
  while ( cur != NULL ) {
    // Elf_Ehdr *elf_ehdr_ptr = cur->elf_ehdr_ptr;
    // Elf_Shdr *elf_shdrs_ptr = cur->elf_shdrs_ptr;
    // char *shstrtab_ptr = cur->shstrtab_ptr;
    char *strtab_ptr = cur->strtab_ptr;
    Elf_Sym *symtab_ptr = cur->symtab_ptr;
    Elf_Word symtab_size = cur->symtab_size;
    Elf_Word sym_size = sizeof (Elf_Sym);
    size_t len = symtab_size / sym_size;
    for(size_t i = 0; i < len; i++){
      Elf_Sym *sym = symtab_ptr + i;
      unsigned char st_info = sym->st_info;
      if (ELF_ST_TYPE(st_info) == STT_FUNC) {
        char *sym_name = strtab_ptr + sym->st_name;
        Elf_Addr st_value = sym->st_value;    
        Elf_Word st_size = sym->st_size;
        if (is_exact) {
          if (st_value == address) {
            return sym_name;
          }
        } else {
          if ((address >= st_value) && (address < st_value + st_size)) {
            return sym_name;
          }
        }
      }
    }

    cur = cur->next;
  }
  return NULL;
}
