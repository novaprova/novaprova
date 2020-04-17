#include "np/spiegel/n3996.hxx"
#include "np/util/log.hxx"

namespace np {
namespace spiegel {
namespace n3996 {

#if 0
class CompileUnitBuilder
{
public:
    CompileUnitBuilder();

    CompileUnit *build(const np::spiegel::dwarf::walker_t &ref)
    {
        dprintf("Reading compile unit header");
        // move to DW_TAG_compile_unit
        const np::spiegel::dwarf::entry_t *e = w.move_next();
        if (!e)
            return 0;
        CompileUnit *cu = new CompileUnit();
        cu->reference(ref);
        return cu;
    };

    void connect(CompileUnit *cu, LinkObject *lo)
    {
        cu->link_object(lo);
        lo->add_compile_unit(cu);
    }

    void connect(CompileUnit *cu, GlobalScope *gs)
    {
        cu->global_scope(gs);
        gs->add_compile_unit(cu);
    }
}
#endif

}; }; };    // close namespaces


using namespace np::spiegel::n3996;

const char * const Specifier::_keywords[] =
{
    0,
    "extern",
    "static",
    "mutable",
    "register",
    "thread_local",
    "const",
    "virtual",
    "private",
    "protected",
    "public",
    "class",
    "struct",
    "union",
    "enum",
    "enum class",
    "constexpr"
};

_SmartPointer<GlobalScope> reflected_global_scope_for_object_file(const char *filename __attribute__((unused)))
{
#if 0
    np::spiegel::dwarf::state_t *state = new np::spiegel::dwarf::state_t();
    if (filename)
        state->add_executable(filename);
    else
        state->add_self();

    GlobalScopeBuilder gs_builder;
    GlobalScope = gs_builder.build();

    auto link_objects = state->get_link_objects();
    vector<np::spiegel::dwarf::state_t::linkobj_t*> linkobjs;
    for (auto itr = link_objects.begin() ; itr != link_objects.end() ; ++itr)
    {
        LinkObjectBuilder lo_builder;
        linkobjs.push_back(lo_builder.build(*itr));
    }

    auto compile_units = state->get_compile_units();
    for (auto i = compile_units.begin() ; i != compile_units.end() ; ++i)
    {
        CompileUnitBuilder cu_builder;
        CompileUnit *cu = cu_builder.build(walker TODO)
        cu_builder.connect(cu, gs);
    }

#endif
    return _SmartPointer<GlobalScope>((GlobalScope *)0);
}

_SmartPointer<GlobalScope> reflected_global_scope()
{
    return reflected_global_scope_for_object_file(0);
}
