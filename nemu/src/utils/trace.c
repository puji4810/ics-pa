#include <common.h>
#include <elf.h>
#include <device/map.h>
void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
uint32_t inst_fetch(vaddr_t *pc, int inst);
#define MAX_INS_LEN 16

typedef struct
{
	vaddr_t pc;
	uint32_t inst_val;
} Itrace;

Itrace itrace[MAX_INS_LEN];
int itrace_num = 0;
bool is_full = false;

void trace_inst(vaddr_t pc, int inst){
	itrace[itrace_num].pc = pc;
	itrace[itrace_num].inst_val = inst;
	itrace_num = (itrace_num+1 % MAX_INS_LEN);
	is_full = is_full || (itrace_num == 0);
}

void display_inst(){
	int end = itrace_num;
	int i = is_full ? itrace_num : 0;

	char buf[128];
	char *p;
	do
	{
		p = buf;
		p += sprintf(buf, "%s" FMT_WORD ": %08x ", (i + 1) % MAX_INS_LEN == end ? " --> " : "     ", itrace[i].pc, itrace[i].inst_val);
		disassemble(p, buf + sizeof(buf) - p, itrace[i].pc, (uint8_t *)&itrace[i].inst_val, 4);

		if ((i + 1) % MAX_INS_LEN == end)
			printf(ANSI_FG_RED);
		puts(buf);
	} while ((i = (i + 1) % MAX_INS_LEN) != end);
	puts(ANSI_NONE);
}