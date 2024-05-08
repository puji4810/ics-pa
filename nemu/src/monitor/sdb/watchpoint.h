#ifndef WATCHPOINT
#define WATCHPOINT

#include "sdb.h"
#define NR_WP 32

typedef struct watchpoint
{
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
	char *expr;
	word_t pre_val;
} WP;

extern WP wp_pool[NR_WP];
// static WP head_pool[NR_WP] = {};
extern WP *head, *free_;

void init_wp_pool();
void free_wp(WP *wp);
WP *new_wp();

#endif