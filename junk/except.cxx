#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <exception>

namespace foo
{

class exception : public std::exception
{
private:
    char *msg_;

public:
    exception() throw() : msg_(0) {}
    exception(const char *m) throw() : msg_(m ? strdup(m) : 0) {}
    ~exception() throw() { if (msg_) free(msg_); }
    const char* what() const throw() { return msg_; }
};

};

int main(int argc, char **argv)
{
    printf("begin\n");
    try
    {
	__asm__ __volatile__("nopl 42");
	throw foo::exception("curveball");
    }
    catch (foo::exception &e)
    {
	printf("caught: %s\n", e.what());
    }
    printf("end\n");
    return 0;
}
