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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>  
#include <sys/types.h>  

#define MAXLEN 65536u

#define  UPDATE_BUF(format_buf, format_new_buf, val) \
        do { \
            int write_num = snprintf(buf + buf_index, MAXLEN - buf_index, format_buf, val); \
            buf[buf_index + write_num] = '\0';\
            buf_index += write_num;\
            int new_write_num = snprintf(new_buf + new_buf_index, MAXLEN - new_buf_index, format_new_buf, val); \
            new_buf[new_buf_index + new_write_num] = '\0';\
            new_buf_index += new_write_num;\
        }while(0)  

// this should be enough
static char buf[MAXLEN] = {};
static char new_buf[MAXLEN] = {};
static char code_buf[MAXLEN + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static size_t buf_index = 0;
static size_t new_buf_index = 0;

static unsigned int choose(unsigned int n){
  int num = rand();
  return num % n;
}

static void gen_num(){
  uint32_t num = choose(10000);
  UPDATE_BUF("%uu", "%u", num);
}

static void gen(char c){
    UPDATE_BUF("%c", "%c", c);
}

static void gen_rand_op(){
  switch (choose(7)) {
    case 0:
      UPDATE_BUF("%c", "%c", '+');
      break;
    case 1:
      UPDATE_BUF("%c", "%c", '-');
      break;
    case 2:
      UPDATE_BUF("%c", "%c",  '*');
      break;
    case 3:
      UPDATE_BUF("%c", "%c", '/');
      break;
    case 4:
      UPDATE_BUF("%s", "%s", "==");
      break;
    case 5:
      UPDATE_BUF("%s", "%s", "!=");
      break;
    case 6:
      UPDATE_BUF("%s", "%s", "&&");
      break;
    default:
      assert(0);
      break;
  }
}


static void gen_rand_expr() {
  if(buf_index >= MAXLEN){
    return;
  }
  switch (choose(5)) {
    case 0: gen(' ');gen_rand_expr();break;
    case 1: gen_num(); break;
    case 2: gen('('); gen_rand_expr(); gen(')'); break;
    case 3: gen('-');gen_rand_expr();break;
    default: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    memset(buf, 0, MAXLEN);
    buf_index = 0;

    memset(new_buf, 0, MAXLEN);
    new_buf_index = 0;

    gen_rand_expr();

    // printf("%s\n", buf);
    if(buf_index >= MAXLEN){
      continue;
    }
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);
    int ret = system("gcc /tmp/.code.c -o /tmp/.expr > /tmp/.log 2>&1");
    if (ret != 0) continue;

    FILE *fp_log = fopen("/tmp/.log", "r");
    char log_msg[MAXLEN] = {};
    memset(log_msg, 0, MAXLEN);
    assert(fp_log != NULL);
    int zeroflag;
    zeroflag = 0;
    while (fgets(log_msg, sizeof(log_msg), fp_log)) {
        if (strstr(log_msg, "warning: division by zero") != NULL) {
            // printf("File contains 'warning: division by zero'\n");
            zeroflag = 1;
            break;
        }
    }
    fclose(fp_log);
    if(zeroflag){
      continue;
    }

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    int exitstatus = pclose(fp);
    if(!exitstatus){
      printf("%u %s\n", result, new_buf);
    } else {
      printf("exitstatus=%d WIFEXITED(%d) WEXITSTATUS(%d)\n", exitstatus, WIFEXITED(exitstatus), WEXITSTATUS(exitstatus));
      break;
    }
  }
  return 0;
}
