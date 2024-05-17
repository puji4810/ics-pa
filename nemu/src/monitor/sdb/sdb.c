/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/
#include <memory/paddr.h>
#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include "./watchpoint.h"
static int is_batch_mode = false;

void init_regex();
//void init_wp_pool();
void remove_wp(int no);
void wp_set(char *e);
void wp_check();
void free_wp(WP *wp);
void sdb_watchpoint_display();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state=NEMU_QUIT; 
  return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

static struct
{
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    {"si", "Step the program", cmd_si},
    {"info", "Generic command for showing things about the program being debugged.", cmd_info},
    {"x", "scan the memory", cmd_x},
    {"p", "Find the value of the expression EXPR",cmd_p},
    {"w", "Watchpoint", cmd_w},
    {"d", "Delete the watchpoint", cmd_d},
    /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args){
  char *arg = strtok(NULL, " ");
  if(arg == NULL){
    cpu_exec(1);
  }
  else{
    int n = atoi(arg);
    cpu_exec(n);
  }
  return 0;
}



void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}

static int cmd_info(char *args)
{
  char *arg = strtok(NULL, " ");
  if(arg == NULL){
    printf("info r\n");
    printf("info w\n");
    return 0;
  }
  else if(*arg=='r'){
    isa_reg_display();
  }
  else if (*arg=='w')
    sdb_watchpoint_display();
  return 0;
}

static int cmd_x(char *args){
  if(args==NULL){
    printf("No args\n");
    return 0;
  }
  char *n = strtok(args, " ");
  char *baseaddr = strtok(NULL, " ");
  int len = 0;
  paddr_t addr = 0;
  sscanf(n, "%d", &len);
  sscanf(baseaddr, "%x", &addr);
  for (int i = 0; i < len; i++)
  {
    printf("%x\n", paddr_read(addr, 4)); 
    addr = addr + 4;
  }
  return 0;
}

static int cmd_p(char *args){
  if (args == NULL)
  {
    printf("No args\n");
    return 0;
  }
  //  printf("args = %s\n", args);
  bool flag = false;
  printf("uint32_t res = %d\n",expr(args, &flag)) ;
  return 0;
}

static int cmd_d(char *args)
{
  if (args == NULL)
    printf("No args.\n");
  else
  {
    remove_wp(atoi(args));
  }
  return 0;
}

static int cmd_w(char *args)
{
  if(args==NULL)
  {
    printf("No args\n");
    return 0;
  }
  else
  {
    //assert(0);
    wp_set(args);
    return 0;
  }
}

void wp_set(char *args)
{
  WP *p = new_wp();
  p->expr = strdup(args);
  printf("set watchpoint %d : %s\n", p->NO, p->expr);
}

void remove_wp(int no)
{
  if (no < 0 || no > NR_WP - 1)
  {
    printf("no such watchpoint\n");
    assert(0);
  }
  else
  {
    WP *p = &wp_pool[no];
    free_wp(p);
    printf("remove watchpoint %d : %s\n", p->NO, p->expr);
    free(p->expr);
    p->expr = NULL;
  }
}

void sdb_watchpoint_display()
{
  if (head == NULL)
  {
    printf("no watchpoint\n");
    return;
  }
  WP *p = head;
  while (p != NULL)
  {
    printf("watchpoint %d : %s \n", p->NO, p->expr);
    p = p->next;
  }
}
