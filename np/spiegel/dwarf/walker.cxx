/*
 * Copyright 2011-2012 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "walker.hxx"
#include "enumerations.hxx"
#include "state.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;
using namespace np::util;

#define DEBUG_WALK 0

uint32_t walker_t::next_id_ = 1;

reader_t
walker_t::get_section_contents(uint32_t sec) const
{
    return compile_unit_->get_section(sec)->get_contents();
}

void
walker_t::seek(reference_t ref)
{
    state_t::compile_unit_offset_tuple_t res = state_t::instance()->resolve_reference(ref);
    compile_unit_ = res._cu;
    assert(compile_unit_);
    reader_ = compile_unit_->get_contents();
    reader_.seek(res._off);
    level_ = 0;
}

int
walker_t::read_entry()
{
    /* Refs are offsets relative to the start of the
     * compile unit header, but the reader starts
     * at the first byte after that header. */
    size_t offset = reader_.get_offset();
    uint32_t acode;

    if (!reader_.read_uleb128(acode))
    {
	return RE_EOF;    // end of section
    }

    if (!acode)
    {
	if (!level_)
	    return RE_EOF;   // end of subtree in scope
	level_--;
#if DEBUG_WALK
	fprintf(stderr, "\n# XXX [%u] %s:%d level=%u return 0\n",
	       id_, __FUNCTION__, __LINE__, level_);
#endif
	return RE_EOL;
    }

    const abbrev_t *a = compile_unit_->get_abbrev(acode);
    if (!a)
    {
	// TODO: bad DWARF info - throw an exception
	fatal("XXX wtf - no abbrev for code 0x%x\n", acode);
    }

    int r = RE_OK;

    if (filter_tag_ && a->tag != filter_tag_)
    {
	entry_.partial_setup(offset, level_, a);
	r = skip_attributes();
	if (r == RE_OK)
	    r = RE_FILTERED;
    }
    else
    {
	entry_.setup(offset, level_, a);
	r = read_attributes();
    }

    if (a->children)
	level_++;

#if DEBUG_WALK
    fprintf(stderr, "\n# XXX [%u]%s:%d level=%u entry={tag=%s level=%u offset=0x%x} return 1\n",
	   id_, __FUNCTION__, __LINE__,
	   level_,
	   tagnames.to_name(entry_.get_tag()),
	   entry_.get_level(),
	   entry_.get_offset());
#endif

    return r;
}

int
walker_t::read_attributes()
{
    const abbrev_t *a = entry_.get_abbrev();
    vector<abbrev_t::attr_spec_t>::const_iterator i;
    for (i = a->attr_specs.begin() ; i != a->attr_specs.end() ; ++i)
    {
	switch (i->form)
	{
	case DW_FORM_data1:
	    {
		uint8_t v;
		if (!reader_.read_u8(v))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_uint32(v));
		break;
	    }
	case DW_FORM_data2:
	    {
		uint16_t v;
		if (!reader_.read_u16(v))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_uint32(v));
		break;
	    }
	case DW_FORM_data4:
	    {
		uint32_t v;
		if (!reader_.read_u32(v))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_uint32(v));
		break;
	    }
	case DW_FORM_data8:
	case DW_FORM_ref_sig8:
	    {
		uint64_t v;
		if (!reader_.read_u64(v))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_uint64(v));
		break;
	    }
	case DW_FORM_udata:
	    {
		uint32_t v;
		if (!reader_.read_uleb128(v))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_uint32(v));
		break;
	    }
	case DW_FORM_sdata:
	    {
		int32_t v;
		if (!reader_.read_sleb128(v))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_sint32(v));
		break;
	    }
	case DW_FORM_addr:
	    {
		np::spiegel::addr_t v;
		if (!reader_.read_addr(v))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_addr(v));
		break;
	    }
	case DW_FORM_flag:
	    {
		uint8_t v;
		if (!reader_.read_u8(v))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_uint32(v));
		break;
	    }
	case DW_FORM_ref1:
	    {
		uint8_t off;
		if (!reader_.read_u8(off))
		    return RE_EOF;
		entry_.add_attribute(i->name,
			value_t::make_ref(compile_unit_->make_reference(off)));
		break;
	    }
	case DW_FORM_ref2:
	    {
		uint16_t off;
		if (!reader_.read_u16(off))
		    return RE_EOF;
		entry_.add_attribute(i->name,
			value_t::make_ref(compile_unit_->make_reference(off)));
		break;
	    }
	case DW_FORM_ref4:
	    {
		uint32_t off;
		if (!reader_.read_u32(off))
		    return RE_EOF;
		entry_.add_attribute(i->name,
			value_t::make_ref(compile_unit_->make_reference(off)));
		break;
	    }
	case DW_FORM_ref8:
	    {
		uint64_t off;
		if (!reader_.read_u64(off))
		    return RE_EOF;
		// TODO: detect truncation
		entry_.add_attribute(i->name,
			value_t::make_ref(compile_unit_->make_reference(off)));
		break;
	    }
	case DW_FORM_ref_addr:
	    {
                // Note that DW_FORM_ref_addr is poorly named in the
                // standard, it's actually encoded as a file offset
                // which will be 4B or 8B depending on the file format
		np::spiegel::offset_t off = 0;
		if (!reader_.read_offset(off))
		    return RE_EOF;
		entry_.add_attribute(i->name,
                        value_t::make_ref(reference_t::make_addr(compile_unit_->get_link_object_index(), off)));
		break;
	    }
	case DW_FORM_string:
	    {
		const char *v;
		if (!reader_.read_string(v))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_string(v));
		break;
	    }
	case DW_FORM_strp:
	    {
		np::spiegel::offset_t off;
		if (compile_unit_->get_version() == 2)
		{
		    uint32_t o32;
		    if (!reader_.read_u32(o32))
			return RE_EOF;
		    off = o32;
		}
		else
		{
		    if (!reader_.read_offset(off))
			return RE_EOF;
		}
		const char *v = compile_unit_->get_section(DW_sec_str)->offset_as_string(off);
		if (!v)
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_string(v));
		break;
	    }
	case DW_FORM_block1:
	    {
		uint8_t len;
		const unsigned char *v;
		if (!reader_.read_u8(len) ||
		    !reader_.read_bytes(v, len))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_bytes(v, len));
		break;
	    }
	case DW_FORM_block2:
	    {
		uint16_t len;
		const unsigned char *v;
		if (!reader_.read_u16(len) ||
		    !reader_.read_bytes(v, len))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_bytes(v, len));
		break;
	    }
	case DW_FORM_block4:
	    {
		uint32_t len;
		const unsigned char *v;
		if (!reader_.read_u32(len) ||
		    !reader_.read_bytes(v, len))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_bytes(v, len));
		break;
	    }
	case DW_FORM_block:
	case DW_FORM_exprloc:
	    {
		uint32_t len;
		const unsigned char *v;
		if (!reader_.read_uleb128(len) ||
		    !reader_.read_bytes(v, len))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_bytes(v, len));
		break;
	    }
	case DW_FORM_sec_offset:
	    {
		np::spiegel::offset_t v;
		if (!reader_.read_offset(v))
		    return RE_EOF;
		entry_.add_attribute(i->name, value_t::make_offset(v));
		break;
	    }
	case DW_FORM_flag_present:
	    /* This form has no representation in the attribute
	     * stream, it's always true.  Presumably this is
	     * useful in combination with a careful choice of
	     * abbrevs. */
	    entry_.add_attribute(i->name, value_t::make_uint32(true));
	    break;
	default:
	    // TODO: bad DWARF info - throw an exception
	    fatal("Can't handle %s at %s:%d\n",
		  formvals.to_name(i->form), __FILE__, __LINE__);
	}
    }
    return RE_OK;
}

int
walker_t::skip_attributes()
{
    const abbrev_t *a = entry_.get_abbrev();
    vector<abbrev_t::attr_spec_t>::const_iterator i;
    for (i = a->attr_specs.begin() ; i != a->attr_specs.end() ; ++i)
    {
	switch (i->form)
	{
	case DW_FORM_data1:
	    if (!reader_.skip_u8())
		return EOF;
	    break;
	case DW_FORM_data2:
	    if (!reader_.skip_u16())
		return EOF;
	    break;
	case DW_FORM_data4:
	    if (!reader_.skip_u32())
		return EOF;
	    break;
	case DW_FORM_data8:
	    if (!reader_.skip_u64())
		return EOF;
	    break;
	case DW_FORM_udata:
	    if (!reader_.skip_uleb128())
		return EOF;
	    break;
	case DW_FORM_sdata:
	    if (!reader_.skip_sleb128())
		return EOF;
	    break;
	case DW_FORM_addr:
            if (!reader_.skip_addr())
                return EOF;
	    break;
	case DW_FORM_flag:
	    if (!reader_.skip_u8())
		return EOF;
	    break;
	case DW_FORM_ref1:
	    if (!reader_.skip_u8())
		return EOF;
	    break;
	case DW_FORM_ref2:
	    if (!reader_.skip_u16())
		return EOF;
	    break;
	case DW_FORM_ref4:
	    if (!reader_.skip_u32())
		return EOF;
	    break;
	case DW_FORM_ref8:
	    if (!reader_.skip_u64())
		return EOF;
	    break;
	case DW_FORM_ref_addr:
	    if (!reader_.skip_offset())
		return EOF;
	    break;
	case DW_FORM_string:
	    if (!reader_.skip_string())
		return EOF;
	    break;
	case DW_FORM_strp:
	    if (compile_unit_->get_version() == 2)
	    {
		if (!reader_.skip_u32())
		    return EOF;
	    }
	    else
	    {
		if (!reader_.skip_offset())
		    return EOF;
	    }
	    break;
	case DW_FORM_block1:
	    {
		uint8_t len;
		if (!reader_.read_u8(len) ||
		    !reader_.skip_bytes(len))
		    return EOF;
		break;
	    }
	case DW_FORM_block2:
	    {
		uint16_t len;
		if (!reader_.read_u16(len) ||
		    !reader_.skip_bytes(len))
		    return EOF;
		break;
	    }
	case DW_FORM_block4:
	    {
		uint32_t len;
		if (!reader_.read_u32(len) ||
		    !reader_.skip_bytes(len))
		    return EOF;
		break;
	    }
	case DW_FORM_block:
	case DW_FORM_exprloc:
	    {
		uint32_t len;
		if (!reader_.read_uleb128(len) ||
		    !reader_.skip_bytes(len))
		    return EOF;
	    }
	    break;
	case DW_FORM_sec_offset:
	    reader_.skip_offset();
	    break;
	case DW_FORM_flag_present:
	    /* Nothing to skip */
	    break;
	case DW_FORM_ref_sig8:
	    reader_.skip_u64();
	    break;
	default:
	    // TODO: bad DWARF info - throw an exception
	    fatal("Can't handle %s at %s:%d\n",
		  formvals.to_name(i->form), __FILE__, __LINE__);
	}
    }
    return RE_OK;
}


vector<reference_t>
walker_t::get_path() const
{
    walker_t w2(compile_unit_);
    vector<reference_t> path;

    for (;;)
    {
	int r = w2.read_entry();
	if (r == RE_EOF)
	    break;
	if (r == RE_EOL)
	{
	    path.pop_back();
	    continue;
	}
	// treat RE_FILTERED like RE_OK
	path.push_back(w2.get_reference());
	if (get_reference() == w2.get_reference())
	    break;
	if (!w2.entry_.has_children())
	    path.pop_back();
    }
    return path;
}

#if DEBUG_WALK
#define BEGIN \
    fprintf(stderr, "\n# [%u] XXX %s:%d level=%u entry.level=%u\n", \
	   id_, __FUNCTION__, __LINE__, level_, entry_.get_level());
#define RETURN(ret) \
    do { \
	const entry_t *_ret = (ret); \
	fprintf(stderr, "\n# XXX [%u]%s:%d r=%d level %u entry.level=%u return %p\n", \
	       id_, __FUNCTION__, __LINE__, r, \
	       level_, entry_.get_level(), _ret); \
	return _ret; \
    } while(0)
#else
#define BEGIN
#define RETURN(ret) \
    return (ret)
#endif

// level_ is the level relative to the very first entry
// at which the walker was initialised or seeked to,
// of the next entry to be read from the file

const entry_t *
walker_t::move_preorder()
{
    BEGIN
    int r;
    do
    {
	if ((r = read_entry()) == RE_EOF)
	    RETURN(0);
    } while (r != RE_OK);
    RETURN(&entry_);
}

const entry_t *
walker_t::move_next()
{
    const value_t *sib = entry_.get_attribute(DW_AT_sibling);
    if (sib && sib->type == value_t::T_REF)
	return move_to(sib->val.ref);

    BEGIN
    unsigned target_level = entry_.get_level();
    int r = 0;
    for (;;)
    {
	if (level_ < target_level)
	{
	    if (target_level)
		entry_.partial_setup(entry_.get_offset(),
				     target_level-1);
	    RETURN(0);
	}
	r = read_entry();
	if (r == RE_EOF)
	    RETURN(0);
	if (r == RE_OK && entry_.get_level() == target_level)
	    RETURN(&entry_);
    }
}

const entry_t *
walker_t::move_down()
{
    BEGIN
    int r = 0;
    if (!entry_.has_children())
	RETURN(0);
    r = read_entry();
    RETURN(r == RE_OK ? &entry_ : 0);
}

const entry_t *
walker_t::move_to(reference_t ref)
{
    BEGIN
    seek(ref);
    int r = read_entry();
    RETURN(r == RE_OK ? &entry_ : 0);
}

const entry_t *
walker_t::move_up()
{
    vector<reference_t> path = get_path();
    if (path.size() > 1)
	return move_to(path[path.size()-2]);
    return 0;
}

#undef BEGIN
#undef RETURN

// close namespaces
}; }; };
