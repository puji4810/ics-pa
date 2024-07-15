#include <common.h>
#include <elf.h>
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
		return;

	FILE *fp;
	fp = fopen(elf_file, "rb");

	if (fp == NULL)
	{
		printf("failed to open the elf file!\n");
		exit(0);
	}

	Elf32_Ehdr edhr;
	// 读取elf头
	if (fread(&edhr, sizeof(Elf32_Ehdr), 1, fp) <= 0)
	{
		printf("fail to read the elf_head!\n");
		exit(0);
	}

	if (edhr.e_ident[0] != 0x7f || edhr.e_ident[1] != 'E' ||
		edhr.e_ident[2] != 'L' || edhr.e_ident[3] != 'F')
	{
		printf("The opened file isn't a elf file!\n");
		exit(0);
	}

	fseek(fp, edhr.e_shoff, SEEK_SET);

	Elf32_Shdr shdr;
	char *string_table = NULL;
	// 寻找字符串表
	for (int i = 0; i < edhr.e_shnum; i++)
	{
		if (fread(&shdr, sizeof(Elf32_Shdr), 1, fp) <= 0)
		{
			printf("fail to read the shdr\n");
			exit(0);
		}

		if (shdr.sh_type == SHT_STRTAB)
		{
			// 获取字符串表
			string_table = malloc(shdr.sh_size);
			fseek(fp, shdr.sh_offset, SEEK_SET);
			if (fread(string_table, shdr.sh_size, 1, fp) <= 0)
			{
				printf("fail to read the strtab\n");
				exit(0);
			}
		}
	}

	// 寻找符号表
	fseek(fp, edhr.e_shoff, SEEK_SET);

	for (int i = 0; i < edhr.e_shnum; i++)
	{
		if (fread(&shdr, sizeof(Elf32_Shdr), 1, fp) <= 0)
		{
			printf("fail to read the shdr\n");
			exit(0);
		}

		if (shdr.sh_type == SHT_SYMTAB)
		{
			fseek(fp, shdr.sh_offset, SEEK_SET);

			Elf32_Sym sym;

			size_t sym_count = shdr.sh_size / shdr.sh_entsize;
			symbol = malloc(sizeof(Symbol) * sym_count);

			for (size_t j = 0; j < sym_count; j++)
			{
				if (fread(&sym, sizeof(Elf32_Sym), 1, fp) <= 0)
				{
					printf("fail to read the symtab\n");
					exit(0);
				}

				if (ELF32_ST_TYPE(sym.st_info) == STT_FUNC)
				{
					const char *name = string_table + sym.st_name;
					//strncpy(symbol[func_num].name, name, sizeof(symbol[func_num].name) - 1);
					memcpy(symbol[func_num].name, name, strlen(name));
					symbol[func_num].addr = sym.st_value;
					symbol[func_num].size = sym.st_size;
					func_num++;
				}
			}
		}
	}
	fclose(fp);
	free(string_table);
}

int call_depth = 0;
int rec_depth = 0;
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


