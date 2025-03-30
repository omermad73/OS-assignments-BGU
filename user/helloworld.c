#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    char* message = "Hello World xv6\n";
    write(1, message, strlen(message));
    exit(0, "");
}
