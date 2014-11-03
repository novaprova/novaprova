#include <typeinfo>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" void __cxa_throw(void *thrown_exception,
			    struct std::type_info *tinfo,
			    void (*dtor)(void *))
{
    fprintf(stderr, "Throwing %s from %p\n",
	   tinfo->name(),
	   __builtin_return_address(0));
    exit(1);
}

