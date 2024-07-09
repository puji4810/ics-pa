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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

#include <memory/paddr.h>
#include <cpu/difftest.h>

static int is_batch_mode = false;
extern bool is_test_gen_expr_mode;

extern void init_expr();
void init_wp_pool();
bool new_wp(char *expression);
void free_wp(int no);
void printf_wps();
void check_expr();

word_t expr(char *e, bool *success);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
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

static int cmd_si(char *args) {
  uint64_t n = 1;
  if (args)
  {
    int match = sscanf(args, "%ld", &n);
    if(!match){
      printf("Unknown args '%s'.Usage: si [N]\n", args);
      return 0;
    }
  }
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
  char usage_msg[] = "Usage: info SUBCMD(such as 'info r' or 'info w')";
  char info_args[10] = "";
  if (!args) {
    printf("%s\n", usage_msg);
    return 0;
  }
  sscanf(args, "%s", info_args);
  if (strcmp(info_args, "r") == 0) {
    isa_reg_display();
  }else if(strcmp(info_args, "w") == 0){
    printf_wps();
  }else{
    printf("Unknown args '%s'.%s\n", args, usage_msg);
  }
  return 0;
}

static int cmd_x(char *args) {
  char usage_msg[] = "Usage: x N EXPR(such as 'x 10 $esp')";
  int n = 0;
  paddr_t addr = 0;
  if (!args) {
    printf("%s\n", usage_msg);
    return 0;
  }
  int match = sscanf(args, "%d %" MUXDEF(CONFIG_ISA64, PRIx64, PRIx32), &n, &addr);
  if(match != 2){
    printf("Unknown args '%s'.%s\n", args, usage_msg);
    return 0;
  }
  printf_memory_by_paddr_len(addr, n);
  return 0;
}

static int cmd_p(char *args) {
  char usage_msg[] = "Usage: p EXPR(such as 'p $eax + 1')";
  if (!args) {
    printf("%s\n", usage_msg);
    return 0;
  }
  // printf("%s\n", args);
  bool success = true;
  uint32_t result = expr(args, &success);
  if(success){
    printf("%d (0x%x)\n", result, result);
  }
  return 0;
}

static int cmd_w(char *args) {
  IFNDEF(CONFIG_WATCHPOINT, printf("Please enable watchpoint in menuconfig\n");return 0);
  char usage_msg[] = "Usage: w EXPR(such as 'w *0x2000')";
  if (!args) {
    printf("%s\n", usage_msg);
    return 0;
  }
  new_wp(args);
  return 0;
}

static int cmd_d(char *args) {
  char usage_msg[] = "Usage: d [N](such as 'd 2')";
  if (args)
  {
    int wp_index = 0;
    int match = sscanf(args, "%d", &wp_index);
    if(!match){
      printf("Unknown args '%s'.%s\n", args, usage_msg);
      return 0;
    }
    free_wp(wp_index);
  } else {
    init_wp_pool();
  }
  return 0;
}

#ifdef CONFIG_DIFFTEST
static int cmd_detach(char *args) {
  difftest_detach();
  return 0;
}

static int cmd_attach(char *args) {
  difftest_attach();
  return 0;
}
#endif

static int cmd_save(char *args) {
  char usage_msg[] = "Usage: save [path]";
  #define MAX_LEN 2048
  char path_arg[MAX_LEN] = "";
  if (!args) {
    printf("%s\n", usage_msg);
    return 0;
  }
  sscanf(args, "%s", path_arg);

  FILE *fp = fopen(path_arg, "wb");
  assert(fp);
  size_t cpu_size = sizeof(CPU_state);
  int write_size = fwrite(&cpu, 1, cpu_size, fp);
  assert(write_size == cpu_size);
  size_t csr_size = sizeof(CPU_csr);
  write_size = fwrite(&csr, 1, csr_size, fp);
  assert(write_size == csr_size);

  write_size = fwrite(guest_to_host(RESET_VECTOR), 1, CONFIG_MSIZE, fp);
  assert(write_size == CONFIG_MSIZE);
  
  fclose(fp);
  return 0;
}

static int cmd_load(char *args) {
  char usage_msg[] = "Usage: load [path]";
  #define MAX_LEN 2048
  char path_arg[MAX_LEN] = "";
  if (!args) {
    printf("%s\n", usage_msg);
    return 0;
  }
  sscanf(args, "%s", path_arg);

  FILE *fp = fopen(path_arg, "rb");
  assert(fp);
  size_t cpu_size = sizeof(CPU_state);
  int read_size = fread(&cpu, 1, cpu_size, fp);
  assert(read_size == cpu_size);
  size_t csr_size = sizeof(CPU_csr);
  read_size = fread(&csr, 1, csr_size, fp);
  assert(read_size == csr_size);

  read_size = fread(guest_to_host(RESET_VECTOR), 1, CONFIG_MSIZE, fp);
  assert(read_size == CONFIG_MSIZE);
  
  fclose(fp);

#ifdef CONFIG_DIFFTEST
  cmd_attach(NULL);
#endif
  return 0;
}

static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "si", "Step one instruction exactly", cmd_si },
  { "info", "Generic command for showing things about the program being debugged", cmd_info },
  { "x", "Examine memory", cmd_x },
  { "p", "Print value of expression EXP", cmd_p },
  { "w", "Set a watchpoint for EXPRESSION", cmd_w },
  { "d", "Delete all or some breakpoints", cmd_d },
#ifdef CONFIG_DIFFTEST
  { "detach", "Disable Difftest", cmd_detach },
  { "attach", "Enable Difftest", cmd_attach },
#endif
  { "save", "Save Status", cmd_save },
  { "load", "Load Status", cmd_load },
  { "q", "Exit NEMU", cmd_q },

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

void sdb_set_batch_mode(bool val) {
  is_batch_mode = val;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  if (is_test_gen_expr_mode) {
    check_expr();
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
  init_expr();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
