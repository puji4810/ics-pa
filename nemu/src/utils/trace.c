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

#ifdef CONFIG_IRINGBUF
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
#endif

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
extern int func_num;
extern Symbol *symbol;

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


