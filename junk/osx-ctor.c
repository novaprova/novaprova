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
 * In OSX global constructors are passed the same arguments as main(),
 * because "just in case".  For more details see this article
 *
 * https://www.mikeash.com/pyblog/friday-qa-2012-11-09-dyld-dynamic-linking-on-os-x.html
 *
 * and the dyld source code
 *
 * http://opensource.apple.com/source/dyld/dyld-210.2.3/
 */
void __attribute__((constructor)) my_ctor(int argc, char **argv, char **envp, char **apple)
{
    printf("In my_ctor() now\n");
    printf("argc=%d\n", argc);
    dump_string_array("argv", argv);
    dump_string_array("envp", envp);
    dump_string_array("apple", apple);
}

int main(int argc, char **argv, char **envp, char **apple)
{
    printf("In main() now\n");
    printf("argc=%d\n", argc);
    dump_string_array("argv", argv);
    dump_string_array("envp", envp);
    dump_string_array("apple", apple);

    return 0;
}
