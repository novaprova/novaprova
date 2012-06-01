#include "np/spiegel/spiegel.hxx"
#include "np/spiegel/dwarf/state.hxx"

using namespace std;
using namespace np::util;

void
some_function(void)
{
    printf("This function exists\n");
    printf("Solely to have its address\n");
    printf("Used to lookup DWARF info.\n");
}

static void addr2line(unsigned long addr)
{
    np::spiegel::location_t loc;

    if (!np::spiegel::describe_address(addr, loc))
    {
	printf("address 0x%lx filename - line - function - offset -\n", addr);
	return;
    }

    printf("address 0x%lx filename %s line %u function %s offset 0x%x\n",
	  addr,
	  loc.compile_unit_->get_absolute_path().c_str(),
	  loc.line_,
	  loc.function_ ? loc.function_->get_full_name().c_str() : "-",
	  loc.offset_);
}

int
main(int argc, char **argv __attribute__((unused)))
{
    const char *filename = 0;
    unsigned long addr = (unsigned long)&some_function + 2;
    bool stdin_flag = false;
    if (argc > 1)
    {
	if (!strcmp(argv[1], "-"))
	    stdin_flag = true;
	else
	    addr = strtoul(argv[1], 0, 0);
    }
    if (argc > 2)
    {
	filename = argv[2];
    }
    if (argc > 3)
    {
	fatal("Usage: taddr2line [addr [executable]]\n");
    }

    np::spiegel::dwarf::state_t state;
    if (filename)
    {
	if (!state.add_executable(filename))
	    return 1;
    }
    else
    {
	if (!state.add_self())
	    return 1;
    }

    if (stdin_flag)
    {
	char buf[128];
	while (fgets(buf, sizeof(buf), stdin))
	{
	    char *p = strchr(buf, '\n');
	    if (p)
		*p = '\0';
	    addr = strtoul(buf, 0, 0);
	    addr2line(addr);
	}
    }
    else
    {
	addr2line(addr);
    }
    return 0;
}

