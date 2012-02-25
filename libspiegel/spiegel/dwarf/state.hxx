#ifndef __libspiegel_dwarf_state_hxx__
#define __libspiegel_dwarf_state_hxx__ 1

#include "spiegel/common.hxx"
#include "spiegel/spiegel.hxx"
#include "section.hxx"
#include "reference.hxx"
#include "enumerations.hxx"

namespace spiegel {
namespace dwarf {

class walker_t;
class compile_unit_t;

class state_t
{
public:
    state_t();
    ~state_t();

    bool add_self();
    bool add_executable(const char *filename);

    void dump_structs();
    void dump_functions();
    void dump_variables();
    void dump_info(bool preorder, bool paths);
    void dump_abbrevs();

    bool describe_address(spiegel::addr_t addr, const char **filenamep,
		          unsigned int *linenop, const char **classp,
			  const char **functionp) const;

    // state_t is a Singleton
    static state_t *instance() { return instance_; }

    const std::vector<compile_unit_t*> &get_compile_units() const { return compile_units_; }
    compile_unit_t *get_compile_unit(reference_t ref) const
    {
	return compile_units_[ref.cu];
    }

private:
    struct linkobj_t
    {
	linkobj_t(const char *n, uint32_t idx)
	 :  filename_(xstrdup(n)),
	    index_(idx)
	{
	    memset(sections_, 0, sizeof(sections_));
	}
	~linkobj_t()
	{
	    unmap_sections();
	    free(filename_);
	}

	char *filename_;
	uint32_t index_;
	section_t sections_[DW_sec_num];
	std::vector<section_t> mappings_;

	bool map_sections();
	void unmap_sections();
    };

    bool read_compile_units(linkobj_t *);
    bool is_within(spiegel::addr_t addr, const walker_t &w) const;

    static state_t *instance_;

    std::vector<linkobj_t*> linkobjs_;
    std::vector<compile_unit_t*> compile_units_;

    friend class walker_t;
    friend class compile_unit_t;
};


// close namespaces
} }

#endif // __libspiegel_dwarf_state_hxx__
