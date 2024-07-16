#include <common.h>
#include <elf.h>
#include <sys/mman.h>
#define MAX_IRINGBUF 16

typedef struct
{
	word_t pc;
	uint32_t inst;
} ItraceNode;

ItraceNode iringbuf[MAX_IRINGBUF];
int p_cur = 0;
bool full = false;

void trace_inst(word_t pc, uint32_t inst)
{
	iringbuf[p_cur].pc = pc;
	iringbuf[p_cur].inst = inst;
	p_cur = (p_cur + 1) % MAX_IRINGBUF;
	full = full || p_cur == 0;
}

void display_inst()
{
	if (!full && !p_cur)
		return;

	int end = p_cur;
	int i = full ? p_cur : 0;

	void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
	char buf[128]; // 128 should be enough!
	char *p;
	//Statement("Most recently executed instructions");
	do
	{
		p = buf;
		p += sprintf(buf, "%s" FMT_WORD ": %08x ", (i + 1) % MAX_IRINGBUF == end ? " --> " : "     ", iringbuf[i].pc, iringbuf[i].inst);
		disassemble(p, buf + sizeof(buf) - p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);

		if ((i + 1) % MAX_IRINGBUF == end)
			printf(ANSI_FG_RED);
		puts(buf);
	} while ((i = (i + 1) % MAX_IRINGBUF) != end);
	puts(ANSI_NONE);
}

void mtrace_pread(paddr_t addr, int len)
{
	printf("pread at " FMT_PADDR " len=%d\n", addr, len);
}

void mtrace_pwrite(paddr_t addr, int len, word_t data)
{
	printf("pwrite at " FMT_PADDR " len=%d, data=" FMT_WORD "\n", addr, len, data);
}

typedef struct
{
	char name[64];
	paddr_t addr; // the function head address
	Elf32_Xword size;
} Symbol;

Symbol *symbol = NULL; // dynamic allocate memory  or direct allocate memory (Symbol symbol[NUM])
int func_num = 0;
void parse_elf(char *elf_file)
{
	if (elf_file == NULL)
	{
		printf("The ELF file is not exist\n");
		return;
	}
	FILE *fp = fopen(elf_file, "rb");
	if (fp == NULL)
	{
		printf("error: failed to open the elf file\n");
		exit(0);
	}

	Elf32_Ehdr *ehdr=(Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
	if(fread(ehdr, sizeof(Elf32_Ehdr),1,  fp) < 1){
		printf("error: failed to read the elf header\n");
		exit(0);
	}
	fseek(fp, ehdr->e_shoff , SEEK_SET);

	Elf32_Shdr *shdr =NULL;
	char *shstrtab = NULL;
	for (int i = 0; i < ehdr->e_shnum;i++){
		if(fread(shdr, sizeof(Elf32_Shdr), 1, fp) < 1){
			printf("error: failed to read the section header\n");
			exit(0);
		}
		if(shdr->sh_type == SHT_STRTAB){
			shstrtab = (char *)malloc(shdr->sh_size);
			fseek(fp,shdr->sh_offset,SEEK_SET);
			if(fread(shstrtab, shdr->sh_size, 1, fp) < 1){
				printf("error: failed to read the section header\n");
				exit(0);
			}
		}
	}

	fseek(fp, ehdr->e_shoff, SEEK_SET);
	for (int i = 0;i<ehdr->e_shnum;i++){
		if(fread(shdr, sizeof(Elf32_Shdr), 1, fp) < 1){
			printf("error: failed to read the section header\n");
			exit(0);
		}
		if(shdr->sh_type == SHT_SYMTAB){
			int symnum = shdr->sh_size / shdr->sh_entsize;
			symbol = (Symbol *)malloc(symnum * sizeof(Symbol));
			Elf32_Sym *symtab ;
			fseek(fp, shdr->sh_offset, SEEK_SET);
			for (int j = 0;j<symnum;j++){
				symtab = (Elf32_Sym *)malloc(sizeof(Elf32_Sym));
				if(fread(symtab, sizeof(Elf32_Sym), 1, fp) < 1){
					printf("error: failed to read the symbol table\n");
					exit(0);
				}
				if(ELF32_ST_TYPE(symtab->st_info) == STT_FUNC){
					strcpy(symbol[func_num].name, shstrtab + symtab->st_name);
					symbol[func_num].addr = symtab->st_value;
					symbol[func_num].size = symtab->st_size;
					func_num++;
				}
			}
		}
	}
}

int rec_depth = 1;
void ftrace_call(word_t pc, paddr_t func_addr)
{
	int i = 0;
	for (; i < func_num; i++)
	{
		if (func_addr >= symbol[i].addr && func_addr < (symbol[i].addr + symbol[i].size))
		{
			break;
		}
	}
	printf("0x%08x:", pc);

	for (int k = 0; k < rec_depth; k++)
		printf("  ");

	rec_depth++;

	printf("call  [%s@0x%08x]\n", symbol[i].name, func_addr);


}

void ftrace_ret(word_t pc){
	int i = 0;
	for (; i < func_num; i++)
	{
		if (pc >= symbol[i].addr && pc < (symbol[i].addr + symbol[i].size))
		{
			break;
		}
	}
	printf("0x%08x:", pc);

	rec_depth--;

	for (int k = 0; k < rec_depth; k++)
		printf("  ");

	printf("ret  [%s]\n", symbol[i].name);

}


