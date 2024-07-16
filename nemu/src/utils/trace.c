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
		printf("The ELF file does not exist\n");
		return;
	}

	FILE *fp = fopen(elf_file, "rb");
	if (fp == NULL)
	{
		printf("Error: Failed to open the ELF file\n");
		exit(1);
	}

	Elf32_Ehdr ehdr;
	if (fread(&ehdr, sizeof(Elf32_Ehdr), 1, fp) < 1)
	{
		printf("Error: Failed to read the ELF header\n");
		exit(1);
	}

	fseek(fp, ehdr.e_shoff, SEEK_SET);

	Elf32_Shdr *shdrs = malloc(ehdr.e_shnum * sizeof(Elf32_Shdr));
	if (fread(shdrs, sizeof(Elf32_Shdr), ehdr.e_shnum, fp) < ehdr.e_shnum)
	{
		printf("Error: Failed to read section headers\n");
		exit(1);
	}

	char *shstrtab = NULL;
	for (int i = 0; i < ehdr.e_shnum; i++)
	{
		if (shdrs[i].sh_type == SHT_STRTAB && i == ehdr.e_shstrndx)
		{
			shstrtab = malloc(shdrs[i].sh_size);
			fseek(fp, shdrs[i].sh_offset, SEEK_SET);
			if (fread(shstrtab, shdrs[i].sh_size, 1, fp) < 1)
			{
				printf("Error: Failed to read section string table\n");
				exit(1);
			}
		}
	}

	for (int i = 0; i < ehdr.e_shnum; i++)
	{
		if (shdrs[i].sh_type == SHT_SYMTAB)
		{
			int symnum = shdrs[i].sh_size / shdrs[i].sh_entsize;
			Elf32_Sym *symtab = malloc(shdrs[i].sh_size);
			fseek(fp, shdrs[i].sh_offset, SEEK_SET);
			if (fread(symtab, shdrs[i].sh_size, 1, fp) < 1)
			{
				printf("Error: Failed to read the symbol table\n");
				exit(1);
			}

			char *strtab = NULL;
			for (int j = 0; j < ehdr.e_shnum; j++)
			{
				if (shdrs[j].sh_type == SHT_STRTAB && strcmp(&shstrtab[shdrs[j].sh_name], ".strtab") == 0)
				{
					strtab = malloc(shdrs[j].sh_size);
					fseek(fp, shdrs[j].sh_offset, SEEK_SET);
					if (fread(strtab, shdrs[j].sh_size, 1, fp) < 1)
					{
						printf("Error: Failed to read the string table\n");
						exit(1);
					}
				}
			}

			symbol = malloc(symnum * sizeof(Symbol));
			func_num = 0;
			for (int j = 0; j < symnum; j++)
			{
				if (ELF32_ST_TYPE(symtab[j].st_info) == STT_FUNC)
				{
					sprintf(symbol[func_num].name, "%s", &strtab[symtab[j].st_name]);
					symbol[func_num].addr = symtab[j].st_value;
					symbol[func_num].size = symtab[j].st_size;
					func_num++;
				}
			}

			free(symtab);
			free(strtab);
		}
	}

	free(shstrtab);
	free(shdrs);
	fclose(fp);


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


