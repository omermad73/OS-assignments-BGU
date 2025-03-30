#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{   
    
    printf("the process is using %d bytes\n", memsize());
   char* buff = (char*)malloc(sizeof(char) * 20000);
    printf("the process is using %d bytes\n", memsize());
    free(buff);
    printf("the process is using after the realse is %d bytes\n",memsize());
    exit(0);
}
