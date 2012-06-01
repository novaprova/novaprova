#include "np/spiegel/spiegel.hxx"
#include "np/spiegel/dwarf/state.hxx"

using namespace std;
using namespace np::util;

int vegan(int x)
{
    string trace = np::spiegel::describe_stacktrace();
    printf("Stacktrace: \n%s\n", trace.c_str());
    return x/2;
}

namespace umami
{
    namespace pickled
    {
	int irony(int x)
	{
	    return vegan(x-3);
	}
    };
};

class leggings
{
public:
    static int dreamcatcher(int x)
    {
	return umami::pickled::irony(x+3);
    }
};

int
main(int argc, char **argv)
{
    np::util::argv0 = argv[0];
    if (argc != 1)
	fatal("Usage: tstack\n");

    np::spiegel::dwarf::state_t state;
    if (!state.add_self())
	return 1;

    leggings::dreamcatcher(42);

    return 0;
}

