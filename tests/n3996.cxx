#include <stdio.h>
#include "np/spiegel/n3996.hxx"
#include "np/spiegel/dwarf/state.hxx"
#include "np/spiegel/dwarf/compile_unit.hxx"
#include "np/util/log.hxx"

void fuckme(void)
{
    dprintf("fuck fuck fuck\n");
}

static void doit()
{
    dprintf("About to create np::spiegel::dwarf::state_t\n");
    np::spiegel::dwarf::state_t *state = new np::spiegel::dwarf::state_t();
    dprintf("About to add_self()\n");
    state->add_self();

    dprintf("Iterating over compile_units\n");
    auto compile_units = state->get_compile_units();
    for (auto i = compile_units.begin() ; i != compile_units.end() ; ++i)
        iprintf("XXX compile_unit %s %u\n", (*i)->get_executable(), (*i)->get_index());
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    np::log::basic_config(np::log::DEBUG, NULL);
    iprintf("Hello world\n");
    doit();
    return 0;
}
