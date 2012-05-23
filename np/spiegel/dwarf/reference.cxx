#include "reference.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;

const reference_t reference_t::null = { 0, 0 };

string reference_t::as_string() const
{
    char buf[64];
    snprintf(buf, sizeof(buf), "(ref){0x%x,0x%x}", cu, offset);
    return buf;
}

// close the namespaces
}; }; };
