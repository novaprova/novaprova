#include <stdio.h>
#include <limits.h>
#include <mach-o/dyld.h>

int main(int argc, char **argv)
{
    int r;
    uint32_t len;
    char buf[PATH_MAX];

    len = sizeof(buf);
    r = _NSGetExecutablePath(buf, &len);
    if (r == 0)
    {
	printf("Executable path is \"%s\"\n", buf);
    }
    else if (r == -1)
    {
	printf("Buffer too small, need %u\n", len);
    }
    else
    {
	printf("WTF??  not expecting return value %d\n", r);
    }
    return 0;
}
