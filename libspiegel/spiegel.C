#include "common.h"
#include <bfd.h>

static void
dump_dwarf(const char *filename)
{
    bfd_init();

    bfd *b = bfd_openr(filename, NULL);
    if (!b)
    {
	bfd_perror(filename);
	return;
    }
    if (!bfd_check_format(b, bfd_object))
    {
	fprintf(stderr, "spiegel: %s: not an object\n", filename);
	bfd_close(b);
	return;
    }

    asection *sec;
    for (sec = b->sections ; sec ; sec = sec->next)
    {
	printf("Section: %s\n", sec->name);
    }

    bfd_close(b);
}

int
main(int argc, char **argv)
{
    if (argc != 2)
	fatal("Usage: spiegel EXE\n");
    dump_dwarf(argv[1]);
    return 0;
}

