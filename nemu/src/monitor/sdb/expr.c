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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <memory/paddr.h>

static void init_tokens();
static void init_regex();

#define SET_FAILED(format_buf, ...) \
  do { \
    *success = false;  \
    printf(format_buf, ## __VA_ARGS__); \
  }while(0)

enum {
  TK_NOTYPE = 256,
  TK_NUM,
  TK_HEXNUM,
  TK_REG,
  TK_NEGATIVE,
  TK_BRACKET_L,
  TK_BRACKET_R,
  TK_DEREFER,
  TK_PLUS,
  TK_MINUS,
  TK_TIMES,
  TK_DIVIDED,
  TK_EQ,
  TK_NOTEQ,
  TK_AND,
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},     // plus
  {"\\-", TK_MINUS},
  {"\\*", TK_TIMES},
  {"\\/", TK_DIVIDED},
  {"0x[0-9A-Fa-f]+", TK_HEXNUM},
  {"[0-9]+", TK_NUM},
  {"\\$[a-z0-9]+", TK_REG},
  {"\\(", TK_BRACKET_L},
  {"\\)", TK_BRACKET_R},
  {"==", TK_EQ},
  {"!=", TK_NOTEQ},
  {"&&", TK_AND},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

void init_expr() {
  init_regex();
  init_tokens();
}

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
static void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

bool is_test_gen_expr_mode = false;
static size_t token_len = 32;
static Token *tokens __attribute__((used));
static int nr_token __attribute__((used))  = 0;

static void init_tokens(){
  if (is_test_gen_expr_mode) {
    token_len = 65535;
  }
  tokens = malloc(sizeof(Token) * token_len);
}

static bool is_token_bl_or_uop(int token_type){
  switch (token_type) {
    case TK_BRACKET_L:
    case TK_NEGATIVE:
    case TK_DEREFER:
      return true;
      break;
    default:
      return false;
      break;
  }
}

static bool is_token_op(int token_type){
  switch (token_type) {
    case TK_PLUS:
    case TK_MINUS:
    case TK_TIMES:
    case TK_DIVIDED:
    case TK_EQ:
    case TK_NOTEQ:
    case TK_AND:
      return true;
      break;
    default:
      return false;
      break;
  }
}

static int add_token(int token_type, char* substr_start, int substr_len, int *nr_token){
  assert(*nr_token < token_len);
  Token new_token = {token_type, };
  if( substr_len > 31){
    printf("The str in struct Token can't contain more than 32 characters.\n");
    return 1;
  }
  memcpy(new_token.str, substr_start, substr_len);
  new_token.str[substr_len + 1] = '\0';
  tokens[*nr_token] = new_token;
  (*nr_token)++;
  return 0;
}

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        /*
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        */
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        bool failed;
        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_MINUS:
            if(nr_token == 0){
              failed = add_token(TK_NEGATIVE, substr_start, substr_len, &nr_token);
            } else {
              const Token *pre_token = &tokens[nr_token - 1];
              int pre_token_type = pre_token -> type;
              if (is_token_op(pre_token_type) || is_token_bl_or_uop(pre_token_type)) {
                failed = add_token(TK_NEGATIVE, substr_start, substr_len, &nr_token);
              } else {
                failed = add_token(rules[i].token_type, substr_start, substr_len, &nr_token);
              }
            }
            if ( failed ) {
              return false;
            }
            // printf("tokens index=%d str=%s regard as %s\n", nr_token - 1, tokens[nr_token - 1].str, ((tokens[nr_token - 1].type == TK_MINUS) ? "TK_MINUS" : "TK_NEGATIVE"));
            break;
          case TK_TIMES:
            if(nr_token == 0){
              failed = add_token(TK_DEREFER, substr_start, substr_len, &nr_token);
            } else {
              const Token *pre_token = &tokens[nr_token - 1];
              int pre_token_type = pre_token -> type;
              if (is_token_op(pre_token_type) || is_token_bl_or_uop(pre_token_type)) {
                failed = add_token(TK_DEREFER, substr_start, substr_len, &nr_token);
              } else {
                failed = add_token(rules[i].token_type, substr_start, substr_len, &nr_token);
              }
            }
            if ( failed ) {
              return false;
            }
            // printf("tokens index=%d str=%s regard as %s\n", nr_token - 1, tokens[nr_token - 1].str, ((tokens[nr_token - 1].type == TK_MINUS) ? "TK_MINUS" : "TK_NEGATIVE"));
            break;
          default:
            failed = add_token(rules[i].token_type, substr_start, substr_len, &nr_token);
            if ( failed ) {
              return false;
            }
            // printf("tokens index=%d str=%s\n", nr_token - 1, tokens[nr_token - 1].str);
          break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int p, int q) {
  Token *token_p = &tokens[p];
  Token *token_q = &tokens[q];
  if((token_p->type != TK_BRACKET_L) || (token_q->type != TK_BRACKET_R)) {
    return false;
  }

  p += 1;
  q -= 1;
  int sum_bracket = 0;
  for (int index = p; index <= q; index++){
    if(sum_bracket < 0){
      return false;
    }
    Token *token_index = &tokens[index];
    int type_index = token_index -> type;
    if(type_index == TK_BRACKET_L) {
      sum_bracket++;
    } else if (type_index == TK_BRACKET_R) {
      sum_bracket--;
    } else {
    }
  }
  if (sum_bracket == 0) {
    return true;
  } else {
    return false;
  }
}

static bool check_unary_op(int p, int q, int token_type) {
  Token *token_p = &tokens[p];
  if((token_p->type != token_type)) {
    return false;
  }

  int sum_bracket = 0;
  for (int index = p; index <= q; index++){
    if(sum_bracket < 0){
      return false;
    }
    Token *token_index = &tokens[index];
    int type_index = token_index -> type;
    if(type_index == TK_BRACKET_L) {
      sum_bracket++;
    } else if (type_index == TK_BRACKET_R) {
      sum_bracket--;
    } else {
      if (sum_bracket == 0) {
        if (is_token_op(type_index)) {
          return false;
        }
      }
    }
  }

  return true;
}

static uint32_t eval(int p, int q, bool *success) {
  // printf("p=%d q=%d\n", p, q);
  if (!*success) {
    return 0;
  }
  if (p > q) {
    SET_FAILED("p(%d) > q(%d) is wrong in eval function.\n", p, q);
    return 0;
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    Token *token_p = &tokens[p];
    int type_p = token_p -> type;
    char *str_p = token_p -> str;
    uint32_t num_p = 0;
    if(type_p == TK_NUM){
      sscanf(str_p, "%u", &num_p);
      return num_p;
    } else if(type_p == TK_HEXNUM){
      sscanf(str_p, "%x", &num_p);
      return num_p;
    } else if(type_p == TK_REG){
      num_p = isa_reg_str2val(str_p, success);
      return num_p;
    } else {
      SET_FAILED("Single token should be a number.(%s)\n", str_p);
      return 0;
    }
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1, success);
  } else if (check_unary_op(p, q, TK_NEGATIVE) == true) {
    uint32_t val1 = eval(p + 1, q, success);
    return 0 - val1;
  } else if (check_unary_op(p, q, TK_DEREFER) == true) {
    uint32_t val1 = eval(p + 1, q, success);
    return paddr_read(val1, 4);
  } else {
    int op_priority = 0;
    int op = -1;
    Token *token_op = NULL;
    int sum_bracket = 0;
    for (int index = p; index <= q; index++){
      Token *token_index = &tokens[index];
      int type_index = token_index -> type;
      if(type_index == TK_BRACKET_L) {
        sum_bracket++;
      } else if (type_index == TK_BRACKET_R) {
        sum_bracket--;
      } else {
      }

      if(sum_bracket < 0){
        SET_FAILED("The left and right parentheses in the expression do not match.\n");
        return 0;
      }

      if(sum_bracket == 0){
        int current_priority = 0;
        switch (type_index) {
          case TK_AND:
            current_priority = 1;
            break;
          case TK_EQ:
          case TK_NOTEQ:
            current_priority = 2;
            break;
          case TK_PLUS:
          case TK_MINUS:
            current_priority = 3;
            break;
          case TK_TIMES:
          case TK_DIVIDED:
            current_priority = 4;
            break;
        }
        if(current_priority && ((op < 0) || (current_priority <= op_priority))){
          op_priority = current_priority;
          op = index;
          token_op = token_index;
        }
      }
    }

    if( op < 0 ){
        SET_FAILED("Can't find main operator in the expression.\n");
        return 0;
    }
    assert(token_op != NULL);

    uint32_t val1 = eval(p, op - 1, success);
    uint32_t val2 = eval(op + 1, q, success);
    if (!*success) {
      return 0;
    }
    // printf("val1=%u val2=%u\n", val1, val2);
    int type_op = token_op -> type;
    switch (type_op) {
      case TK_PLUS: return val1 + val2;
      case TK_MINUS: return val1 - val2;
      case TK_TIMES: return val1 * val2;
      case TK_EQ:  return val1 == val2;
      case TK_NOTEQ: return val1 != val2;
      case TK_AND:  return val1 && val2;
      case TK_DIVIDED:
        if( val2 == 0 ){
          SET_FAILED("Can't divided by zero in the expression.\n");
          return 0;
        }
        return val1 / val2;
      default: assert(0);
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  return eval(0, nr_token - 1, success);
}

void sdb_test_gen_expr_mode() {
  is_test_gen_expr_mode = true;
}

void check_expr() {
  unsigned except = 0;
  unsigned index = 0;
  for (char *str; (str = readline("")) != NULL; free(str)) {
    printf("index=%u\n", index);
    char *str_end = str + strlen(str);

    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    sscanf(cmd, "%u", &except);
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }


    bool success = true;
    uint32_t result = expr(args, &success);
    if(!success || (result != except)){
      printf("str_expr=[%s]\n", args);
      assert(success);
      if(result != except){
        printf("result=%d except=%d\n", result, except);
        assert(result == except);
      }
    }
    index++;
  }
  exit(0);
}