#include "string_table.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;

int
string_table_t::to_index(const char *name) const
{
    if (prefix_len_)
    {
	if (strncmp(name, prefix_, prefix_len_))
	    return -1;
	name += prefix_len_;
    }

    for (int i = 0 ; names_[i] ; i++)
    {
	if (!strcmp(names_[i], name))
	    return i;
    }
    return -1;
}

const char *
string_table_t::to_name(int i) const
{
    const char *name;
    if (i < 0 || i >= (int)names_count_ || names_[i][0] == '\0')
	name = 0;
    else
	name = names_[i];

    static char buf[256];
    if (prefix_len_)
    {
	if (name)
	    snprintf(buf, sizeof(buf), "%s%s", prefix_, name);
	else
	    snprintf(buf, sizeof(buf), "%s0x%x", prefix_, (unsigned)i);
	name = buf;
    }
    else if (!name)
    {
	snprintf(buf, sizeof(buf), "0x%x", (unsigned)i);
	name = buf;
    }
    return name;
}

// close namespaces
}; }; };
