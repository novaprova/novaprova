#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int main(int argc, char **argv)
{
    void *addr = dlsym(RTLD_DEFAULT, argv[1]);
    printf("Symbol %s address %p\n", argv[1], addr);
    return 0;
}
