#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <stdio.h>


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

int main(){
    char* str = rl_gets();
    char *str_end = str + strlen(str);
    int len = strlen(str);
    printf("%d\n", len);
}