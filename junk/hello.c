#include <unistd.h>

int main(int argc, char **argv)
{
    static const char msg[] = "Hello world\n";
    int r = write(1, msg, sizeof(msg)-1);
    return 0;
}
