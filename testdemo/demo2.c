#include<string.h>
#include<stdio.h>
int main(void)
{
    char input[16]="abc,d";
    char *p = strtok(input, ",");
    char *next = p + strlen(p) + 1;
    printf("%s\n", next);

    
    return 0;
 
}