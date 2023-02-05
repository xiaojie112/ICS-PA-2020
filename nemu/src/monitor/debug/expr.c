#include <isa.h>
#include <memory/vaddr.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

//TK_DEREF解引用*
//TK_NEG 取负
enum {
    TK_NOTYPE = 256, TK_EQ, TK_DEC, TK_HEX, TK_AND, TK_NEQ, TK_REG, TK_DEREF, TK_NEG
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  // {" +", TK_NOTYPE},    // spaces
  // {"\\+", '+'},         // plus
  // {"==", TK_EQ},        // equal

  // {" +", TK_NOTYPE},    // spaces
  // {"\\+", '+'},         // plus
  // {"\\-", '-'},  // minus
  // {"\\*", '*'},  // multiply
  // {"/", '/'},    // divide
  // {"\\(", '('},
  // {"\\)", ')'},
  // {"==", TK_EQ},        // equal
  // {"!=",TK_NEQ},        //not equal
  // {"\\$(\\$0|ra|[sgt]p|t[0-6]|a[0-7]|s([0-9]|1[0-1]))", TK_REG},//registers
  // {"0[xX][0-9a-fA-F]+",TK_HEX},    //hex numbers
  // {"[0-9]+", TK_NUM},   //numbers
  // {"\\|\\|",TK_OR},
  // {"&&",TK_AND}

  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},       // not equal
  {"&&", TK_AND},       // and
  {"\\+", '+'},         // plus
  {"-", '-'},           // minus
  {"\\*", '*'},         // multiply
  {"/", '/'},           // devide
  {"\\(", '('},         // lbracket
  {"\\)", ')'},         // rbracket
  {"0x[0-9]+", TK_HEX}, // hexadecimal
  {"[0-9]+", TK_DEC},   // decimal
  {"\\$[A-Za-z0-9]+", TK_REG},      // register
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {

    //int regcomp(regex_t *preg, const char *regex, int cflags);
    //这个函数把指定的正则表达式pattern编译成一种特定的数据格式preg，这样可以使匹配更有效。函数regexec 会使用这个数据在目标文本串中进行模式匹配。执行成功返回０。

    //regex_t 是一个结构体数据类型，用来存放编译后的正则表达式，它的成员re_nsub 用来存储正则表达式中的子正则表达式的个数，子正则表达式就是用圆括号包起来的部分表达式。
    //pattern 是指向我们写好的正则表达式的指针。
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

static Token tokens[64]  __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

//create tokens array whenever making an calculation
static bool make_token(char *e) {
  int position = 0;//position变量指示当前处理到的位置
  int i;
  regmatch_t pmatch;

  nr_token = 0;//指示已经被识别的token数

  while (e[position] != '\0') {
    /* Try all rules one by one. */
      //NR_REGEX: the number of rules
    for (i = 0; i < NR_REGEX; i ++) {

      //int regexec (regex_t *compiled, char *string, size_t nmatch, regmatch_t matchptr [], int eflags)
      //当我们编译好正则表达式后，就可以用regexec 匹配我们的目标文本串了，
      //如果在编译正则表达式的时候没有指定cflags的参数REG_NEWLINE，则默认情况下是忽略换行符的，也就是把整个文本串当作一个字符串处理。执行成功返回０。
      //regmatch_t 是一个结构体数据类型，在regex.h中定义：
      /*
      
      typedef struct
      {
        regoff_t rm_so;
        regoff_t rm_eo;
      } regmatch_t;

      成员rm_so 存放匹配文本串在目标串中的开始位置，rm_eo 存放结束位置。
      通常我们以数组的形式定义一组这样的结构。因为往往我们的正则表达式中还包含子正则表达式。数组0单元存放主正则表达式位置，后边的单元依次存放子正则表达式位置。
      
      */
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        //%.*s表示从substr_start输出字符串,字符的长度位substr_len
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",//
            i, rules[i].regex, position, substr_len, substr_len, substr_start);


        // TODO:Return error when the number of units or tokens exceeded the length limit
        assert(substr_len < 64);
        assert(nr_token < 64);

        // TODO:Insert unit into tokens
        tokens[nr_token].type = rules[i].token_type;
        memcpy(tokens[nr_token].str, e + position, sizeof(char) * substr_len);
        tokens[nr_token].str[substr_len] = '\0';

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          // default: TODO();
          case TK_NOTYPE:
            nr_token--;
            break;
          case '-':
            if (nr_token == 0 || (tokens[nr_token-1].type != ')' && tokens[nr_token-1].type != TK_DEC 
              && tokens[nr_token-1].type != TK_HEX && tokens[nr_token-1].type != TK_REG)) 
              tokens[nr_token].type = TK_NEG;
            break;
        }
        //TODO
        nr_token++;
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



word_t evalExp(int start, int end, bool *valid) ;
word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return -1;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();

  // return 0;

  //修改token的type, *可能是解引用的情况
  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && (i == 0 || 
        (tokens[i-1].type != TK_DEC 
        && tokens[i-1].type != TK_HEX 
        && tokens[i-1].type != TK_REG
        && tokens[i-1].type != ')') )) {
      tokens[i].type = TK_DEREF;
    }
  }

  *success = true;
  return evalExp(0, nr_token - 1, success);;
}

//检查左右括号是否匹配
int checkParentheses(int start, int end, bool* valid) {
  int par_level = 0;
  if(start > end){
    *valid = false;
    return false;
  };

  for (int i = start; i <= end; i++) {
    if (tokens[i].type == '(') {
      par_level++;
    } else if (tokens[i].type == ')') {
      par_level--;
      // assert(par_level >= 0);
      if(par_level < 0){
        *valid = false;
        return false;
      }
      if (par_level == 0 && i != end) {
        return false;
      }
    }
  }
  assert(par_level == 0);
  //"(4 + 3) * (2 - 1)"   // false, the leftmost '(' and the rightmost ')' are not matched
  if (tokens[start].type != '(' || tokens[end].type != ')') return false;
  return true;
}

//找出主运算符
int findMainOp(int start, int end) {
  //par_level:括号中的token都不可能成为主运算符
  int par_level = 0, priority = 120, main_operator = -1;
  for (int i = start; i <= end; i++) {
    if (tokens[i].type == '(') {
      par_level++;
    } else if (tokens[i].type == ')') {
      par_level--;
      continue;
    }
    if (par_level) continue;
    
    if ((tokens[i].type == TK_DEREF || tokens[i].type == TK_NEG) && priority > 100) {//TODO
      main_operator = i;
      priority = 100;
    } else if ((tokens[i].type == '*' || tokens[i].type == '/') && priority >= 80) {
      main_operator = i;
      priority = 80;
    } else if ((tokens[i].type == '+' || tokens[i].type == '-') && priority >= 50) {
      main_operator = i;
      priority = 50;
    } else if (tokens[i].type == TK_AND && priority >= 30) {
      main_operator = i;
      priority = 30;
    } else if ((tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ) && priority >= 20) {
      main_operator = i;
      priority = 20;
    }
  }
  assert(main_operator != -1);
  return main_operator;
}

word_t evalExp(int start, int end, bool *valid) {
  if (start > end) {
    return 0;
  } else if (start == end) {
    word_t val = 0;
    if (tokens[start].type == TK_DEC) {
      sscanf(tokens[start].str, "%u", &val);
    } else if (tokens[start].type == TK_HEX) {
      sscanf(tokens[start].str + 2, "%x", &val);
    } else if (tokens[start].type == TK_REG) {
      bool success = false;
      val = isa_reg_str2val(tokens[start].str + 1, &success);
      assert(success);
    }
    return val;
  } else if (checkParentheses(start, end, valid) == true) {
    return evalExp(start + 1, end - 1, valid);
  } else {
    if(!*valid)return false;
    int main_operator = findMainOp(start, end);
    
    word_t val2 = evalExp(main_operator + 1, end, valid);

    //只有一个操作数的情况
    switch (tokens[main_operator].type) {
      case TK_DEREF:
        return vaddr_read(val2, 1);
      case TK_NEG:
        return -val2;
    }

    word_t val1 = evalExp(start, main_operator - 1, valid);
    switch (tokens[main_operator].type) {
      case '+':
        return val1 + val2;
      case '-':
        return val1 - val2;
      case '*':
        return val1 * val2;
      case '/': 
        assert(val2 != 0);
        return val1 / val2;
      case TK_AND:
        return val1 && val2;
      case TK_EQ:
        return val1 == val2;
      case TK_NEQ:
        return val1 != val2;
      default:
        assert(false);
    }
  }
}

