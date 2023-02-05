#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};//表达式缓冲区
static char code_buf[65536 + 128] = {}; // a little larger than `buf`  代码缓冲区
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int tail;


int choose(int x) {
  return rand() % x;
}

static inline void gen_num() {
  buf[tail++] = choose(10) + '0';
  if (buf[tail - 1] == '0') return;//前导0
  switch (choose(5)) {//生成一个1-5位的随机数
    case 0: buf[tail++] = choose(10) + '0';
    case 1: buf[tail++] = choose(10) + '0';
    case 2: buf[tail++] = choose(10) + '0';
    case 3: buf[tail++] = choose(10) + '0';
    case 4: buf[tail++] = choose(10) + '0'; break;
  }
}

static inline void gen(char ch) {
  buf[tail++] = ch;
}

static inline void gen_rand_op() {
  char op = '+';
  switch (choose(4)) {
    case 0: op = '+'; break;
    case 1: op = '-'; break;
    case 2: op = '*'; break;
    case 3: op = '/'; break;
  }
  buf[tail++] = op;
}


static inline void gen_rand_expr() {
  if (tail >= 60000) return;
  switch (choose(3)) {
    case 0: gen_num(); break;//根据BNF，一个数可以是表达式
    case 1: gen('('); gen_rand_expr(); gen(')'); break;//在表达式两边加个括号也是表达式
    default: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); break;
  } 
  buf[tail] = '\0';
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 10;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    tail = 0;//!!!!
    gen_rand_expr();
    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    // int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    // if (ret != 0) continue;
    int ret = system("gcc /tmp/.code.c -Wall -Werror -o /tmp/.expr 2>/dev/null");
    if(ret != 0){
       i--;
       continue;
    }

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
