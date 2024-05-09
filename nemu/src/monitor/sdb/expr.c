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
#include <assert.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum
{
  TK_NOTYPE = 256,
  TK_EQ,
  TK_PLUS,
  TK_MINUS,
  TK_MULTIPLY,
  TK_DIVIDE,
  TK_ZUO,
  TK_YOU,
  TK_LEQ,
  TK_NEQ,
  TK_OR,
  TK_AND,
  TK_REGISTER,
  TK_HEX,
  TK_NUM,
  TK_NEGATIVE,//负号
  TK_DEREF,
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE}, // spaces
    {"\\+", TK_PLUS},      // plus
    {"\\-", TK_MINUS},      // sub
    {"\\*", TK_MULTIPLY},      // mul
    {"\\/", TK_DIVIDE},      // div

    {"\\(", TK_ZUO},                     // 左括号
    {"\\)", TK_YOU},                     // 右括号
    {"\\<\\=", TK_LEQ},                  // 小于等于
    {"\\=\\=", TK_EQ},                   // 等于
    {"\\!\\=", TK_NEQ},                  // 不等于
    {"\\|\\|", TK_OR},                   // 逻辑或
    {"\\&\\&", TK_AND},                  // 逻辑与
    {"\\!", '!'},                        // 逻辑非
    {"\\$[a-zA-Z]*[0-9]*", TK_REGISTER}, // 寄存器
    {"0[xX][0-9a-fA-F]+", TK_HEX},       // 十六进制数
    {"[0-9]+", TK_NUM},                  // 数字

};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

bool division_zero = false;//除数为0
/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
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

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e)
{
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

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        Token tmp_token;
        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_PLUS:
            tmp_token.type = TK_PLUS;
            tokens[nr_token++] = tmp_token;
            break;
          case TK_MINUS:
            tmp_token.type = TK_MINUS;
            tokens[nr_token++] = tmp_token;
            break;
          case TK_MULTIPLY:
            tmp_token.type = TK_MULTIPLY;
            tokens[nr_token++] = tmp_token;
            break;
          case TK_DIVIDE:
            tmp_token.type = TK_DIVIDE;
            tokens[nr_token++] = tmp_token;
            break;
          case '!':
            tmp_token.type = '!';
            tokens[nr_token++] = tmp_token;
            break;
          case TK_YOU:
            tmp_token.type = TK_YOU;
            tokens[nr_token++] = tmp_token;
            break;
          case TK_ZUO:
            tmp_token.type = TK_ZUO;
            tokens[nr_token++] = tmp_token;
            break;

            // Special
          case TK_NUM: // num
            tokens[nr_token].type = TK_NUM;
            strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
            nr_token++;
            break;
          case TK_REGISTER: 
            tokens[nr_token].type = TK_REGISTER;
            strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
            nr_token++;
            break;
          case TK_HEX: // HEX
            tokens[nr_token].type = TK_HEX;
            strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
            nr_token++;
            break;
          case TK_EQ:
            tokens[nr_token].type = TK_EQ;
            strcpy(tokens[nr_token].str, "==");
            nr_token++;
            break;
          case TK_NEQ:
            tokens[nr_token].type = TK_NEQ;
            strcpy(tokens[nr_token].str, "!=");
            nr_token++;
          case TK_OR:
            tokens[nr_token].type = TK_OR;
            strcpy(tokens[nr_token].str, "||");
            nr_token++;
            break;
          case TK_AND:
            tokens[nr_token].type = TK_AND;
            strcpy(tokens[nr_token].str, "&&");
            nr_token++;
            break;
          case TK_LEQ:
            tokens[nr_token].type = TK_LEQ;
            strcpy(tokens[nr_token].str, "<=");
            nr_token++;
            break;
          default:
            printf("i = %d and No rules is com.\n", i);
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

// bool check_parentheses(word_t p, word_t q)
// {
//   bool flag = false;
//   if (tokens[p].type == '(' && tokens[q].type == ')')
//   {
//     for (int i = p + 1; i < q;)
//     {

//       if (tokens[i].type == ')')
//       {

//         break;
//       }

//       else if (tokens[i].type == '(')
//       {
//         while (tokens[i + 1].type != ')')
//         {
//           i++;
//           if (i == q - 1)
//           {

//             break;
//           }
//         }
//         i += 2;
//       }

//       else
//         i++;
//     }
//     flag = true;
//   }
//   return flag;
// }


bool check_parentheses(int p, int q)
{
  if(tokens[p].type != TK_ZUO || tokens[q].type != TK_YOU)
    return false;
  while(p<q){
    if(tokens[p].type==TK_ZUO){
      if(tokens[q].type==TK_YOU){
        p++, q--;
        continue;
      }
      else
        q--;
    }
    else if(tokens[p].type==TK_YOU){
      return false;
    }
    else
      p++;
  }
  return true;
}
int get_priority(int type)
{
  switch (type)
  {
  case TK_OR :
  case TK_AND:
    return 1;

  case TK_LEQ:
  case TK_EQ:
  case TK_NEQ:
    return 2;
  
  case TK_PLUS:
  case TK_MINUS:
    return 3;
  
  case TK_MULTIPLY:
  case TK_DIVIDE:
    return 4;
  
  case TK_NEGATIVE:
    return 5;

  default:
    return -1;
}
}

u_int32_t find_major(int p, int q)
{
  int op = -1;
  int lowest_priority = 300; // 初始设定为最大优先级
  bool flag = false;

  for (int i = p; i <= q; i++)
  {
    // 检查是否在括号内，不处理括号内的操作符
    if (tokens[i].type == TK_ZUO)
    {
      while (tokens[i].type != TK_YOU)
        i++;
    }

    // 检查是否是运算符且优先级低于当前最低优先级
    else if (!flag && (tokens[i].type == TK_OR 
                      ||tokens[i].type == TK_AND 
                      ||tokens[i].type == TK_NEQ 
                      ||tokens[i].type == TK_EQ 
                      ||tokens[i].type == TK_LEQ 
                      ||tokens[i].type == TK_PLUS 
                      ||tokens[i].type == TK_MINUS 
                      ||tokens[i].type == TK_MULTIPLY
                      ||tokens[i].type == TK_DIVIDE
                      ||tokens[i].type == TK_NEGATIVE))
    {
      flag = true;
      op = i;
      lowest_priority = get_priority(tokens[i].type); // 获取当前运算符的优先级
    }
    else if (flag && (tokens[i].type == TK_PLUS ||
                      tokens[i].type == TK_MINUS ||
                      tokens[i].type == TK_MULTIPLY ||
                      tokens[i].type == TK_DIVIDE))
    {
      // 如果当前运算符优先级与当前最低优先级相同，根据右结合性选择更靠右的运算符
      int current_priority = get_priority(tokens[i].type);
      if (current_priority <= lowest_priority)
      {
        op = i;
        lowest_priority = current_priority;
      }
    }
  }

  return op;
}
u_int32_t eval(int p, int q)
{
  if (p > q)
  {
    /* Bad expression */
    assert(0);
    return -1;
  }
  else if (p == q)
  {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    return atoi(tokens[p].str);
  }
  else if (check_parentheses(p, q) == true)
  {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else
  {
    u_int32_t op = find_major(p, q); // the position of 主运算符 in the token expression;
    int op_type = tokens[op].type;
    u_int32_t val2 = eval(op + 1, q);
    if (tokens[op].type == TK_NEGATIVE)
    {
      val2 = -val2;
      return val2;
    }
    u_int32_t val1 = eval(p, op - 1);
    
    
    switch (op_type)
    {
    case TK_PLUS:
      return val1 + val2;
    case TK_MINUS: 
      return val1-val2;
    case TK_MULTIPLY: 
      return val1*val2;
    case TK_DIVIDE:
      if(val2==0) {
        division_zero = true;
        assert(0);
        return 0;
      }
      return val1/val2;
    case TK_EQ:
      return val1==val2;
    case TK_NEQ:
      return val1!=val2;
    case TK_OR:
      return val1||val2;
    case TK_AND:
      return val1&&val2;
    default:
      printf("No rules is com.\n");
      assert(0);
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //对一些类型进行处理
  for (int i = 0; i < nr_token;i++){
    if(tokens[i].type==TK_HEX){
      int value = strtol(tokens[i].str, NULL, 16);
      sprintf(tokens[i].str, "%d", value);
    }
  }

  for (int i = 0; i < nr_token;i++){
    if(tokens[i].type==TK_REGISTER){
      bool flag = true;
      //printf("%s\n", tokens[i].str);
      if (strcmp(tokens[i].str, "$0") != 0)
      {
        int len = strlen(tokens[i].str);
        for (int j = 0;j<len;++j){
          tokens[i].str[j] = tokens[i].str[j+1] ;
        }
      }
      int value = isa_reg_str2val(tokens[i].str, &flag);
      if(flag){
        sprintf(tokens[i].str, "%d", value);
        //tokens[i].type = TK_NUM;
        //printf("%s %d\n", tokens[i].str,tokens[i].type);
      }
      else{
        printf("Transfrom error. \n");
        assert(0);
      }
    }
  }

  for (int i = 0; i < nr_token; i++)
  {
    if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type ==TK_PLUS
                                            ||tokens[i-1].type == TK_MINUS
                                            ||tokens[i-1].type == TK_MULTIPLY
                                            ||tokens[i-1].type == TK_DIVIDE
                                            ||tokens[i-1].type == TK_EQ
                                            ||tokens[i-1].type == TK_NEQ
                                            ||tokens[i-1].type == TK_LEQ
                                            ||tokens[i-1].type == TK_AND
                                            ||tokens[i-1].type == TK_OR
                                            ||tokens[i-1].type == '!') ))
    {
      int temp;
      tokens[i].type = TK_DEREF;
      temp = atoi(tokens[i + 1].str);
      int *val = &temp;
      sprintf(tokens[i].str, "%d", *val);
    }
  }

  for (int i = 0; i < nr_token; i++)
  {
    if ((tokens[i].type == TK_MINUS && i > 0 && tokens[i - 1].type != TK_NUM && tokens[i + 1].type == TK_NUM) 
        ||(tokens[i].type == TK_MINUS && i == 0))
    {
      tokens[i].type = TK_NEGATIVE;
      //printf("-----------\n");
    }
  }

  
//----------------------------------------------------------

  *success = true;
  int res;
  res=eval(0,nr_token-1);
  // if (!division_zero)
  //   printf("uint32_t res = %d\n", res);
  // else
  if (division_zero)
    printf("Your input have an error: can't division zeor\n");
  memset(tokens, 0, sizeof(tokens));
  return res;
}
