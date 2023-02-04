#include <isa.h>
#include "expr.h"
#include "watchpoint.h"
#include "memory/vaddr.h"
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
int is_batch_mode();


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


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_info(char *args);


static int cmd_si(char *args){
  //execute the program with N steps
  uint64_t steps = 1;
  char * cmp = " ";
  if(args == NULL || strcmp(args, cmp) == 0){
    cpu_exec(steps);
  }else{
    steps = atol(args);
    cpu_exec(steps);
  }
  return 0;
}

static int cmd_x(char *args){
    //获取内存起始地址和扫描长度。
    if(args == NULL){
        printf("too few parameter! \n");
        return 1;
    }
     
    char *arg = strtok(args," ");
    if(arg == NULL){
        printf("too few parameter! \n");
        return 1;
    }
    int  n = atoi(arg);
    char *EXPR = strtok(NULL," ");
    if(EXPR == NULL){                                                                                                                                          
        printf("too few parameter! \n");
        return 1;
    }
    if(strtok(NULL," ")!=NULL){
        printf("too many parameter! \n");
        return 1;
    }
    bool success = true;
    //vaddr_t addr = expr(EXPR , &success);
    if (success!=true){
        printf("ERRO!!\n");
        return 1;
    }

   // 因为开头有一个0x, 我们需要去掉它, 不然解析会出错
  // BUG here: atoi默认是十进制, 但是这里应该是十六进制
  // paddr_t addr = atoi(s_num2+2);

    vaddr_t addr =  strtol(EXPR,NULL,16 );
   // printf("%#lX\n",ad);
    //进行内存扫描,每次四个字节;
    for(int i = 0 ; i < n ; i++){
        uint32_t data = vaddr_read(addr + i * 4,4);
        printf("0x%08x  " , addr + i * 4 );
        for(int j =0 ; j < 4 ; j++){
            printf("0x%02x " , data & 0xff);
            data = data >> 8 ;
        }
        printf("\n");
    }
     
    return 0;
}    



static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Let the program pause after executing N instructions in a single step,The default is 1 when N is not given", cmd_si},
  { "info", "Print program status", cmd_info},
  { "x", "Scanning memory", cmd_x},
  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))



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

// static int cmd_info(char *args){
//     char *arg = strtok(args," ");
//     printf("%s\n",arg);
//     //cpu info
//     if (strcmp(arg,"r")==0){
//         printf("eax is %x\n",cpu.eax);
//         printf("ecx is %x\n",cpu.ecx);
//         printf("edx is %x\n",cpu.edx);
//         printf("ebx is %x\n",cpu.ebx);
//         printf("esp is %x\n",cpu.esp);
//         printf("ebp is %x\n",cpu.ebp); 
//         printf("esi is %x\n",cpu.esi);
//         printf("edi is %x\n",cpu.edi);
//         printf("---------------------------\n");
//     }
//     else if(strcmp(arg,"w")==0){
//         // print_wp();    //此部分是后期用来打印监测点状态使用，前期可以先注释掉。
//     }
     
//     return 0;
// }  


static int cmd_info(char *args) {
  char *subcmd = strtok(NULL, " ");

  /* Show help information when subcommand is illegal */
  if (subcmd == NULL || strlen(subcmd) != 1) {
    for (int i = 0; i < NR_CMD; i ++) {
      if (strcmp("info", cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    return 0;
  }

  /* Deal with subcommand */
  switch(subcmd[0]) {
    case 'r':
      isa_reg_display();
      break;
    case 'w':
      break;
  }
  return 0;
}

void ui_mainloop() {
  if (is_batch_mode()) {
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

#ifdef HAS_IOE
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
