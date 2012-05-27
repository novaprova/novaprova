#include "mycode.h"

int myatoi(const char *s)
{
    int v = 0;

    for ( ; *s ; s++)
    {
	v *= 10;
	v += (*s - '0');
    }

    return v;
}

