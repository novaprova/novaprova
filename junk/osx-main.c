#include <stdio.h>
#include <unistd.h>

static void dump_string_array(const char *name, char **array)
{
    int i;
    char **p;

    printf("%s=%p [\n", name, array);
    for (i = 0 ; *array ; i++, array++)
	printf("    [%d] \"%s\"\n", i, *array);
    printf("]\n");
}

/*
 * This is the full calling convention for main() on OSX.  For more
 * details see this article
 *
 * https://www.mikeash.com/pyblog/friday-qa-2012-11-09-dyld-dynamic-linking-on-os-x.html
 *
 * and the dyld source code
 *
 * http://opensource.apple.com/source/dyld/dyld-210.2.3/
 *
 * All these arguments come more or less completely unmolested from the
 * kernel via dyld.  The argc, argv[], and envp[] arguments are traditional
 * UNIX. The apple[] argument appears to be an equivalent of Linux' and
 * Solaris' aux vector, i.e. a private communication mechanism between
 * the kernel and the dynamic linker and/or C runtime; the [0] element
 * appears to be the same as argv[0] followed by 3 mysterious "" elements.
 */
int main(int argc, char **argv, char **envp, char **apple)
{
    printf("argc=%d\n", argc);
    dump_string_array("argv", argv);
    dump_string_array("envp", envp);
    dump_string_array("apple", apple);

    return 0;
}
