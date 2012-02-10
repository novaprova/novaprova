#include "string_table.hxx"

namespace spiegel { namespace dwarf {
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
    if (i < 0 || i >= (int)names_count_ || names_[i][0] == '\0')
	return "unknown";
    if (prefix_len_)
    {
	// TODO: use an estring
	static char buf[256];
	strncpy(buf, prefix_, sizeof(buf));
	strncat(buf, names_[i], sizeof(buf)-prefix_len_);
	return buf;
    }
    else
    {
	return names_[i];
    }
}

// close namespace
} }
