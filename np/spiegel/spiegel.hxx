/*
 * Copyright 2011-2020 Gregory Banks
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
#ifndef __spiegel_spiegel_hxx__
#define __spiegel_spiegel_hxx__ 1

#include "np/util/common.hxx"
#include "np/spiegel/dwarf/reference.hxx"
#include "np/spiegel/dwarf/compile_unit.hxx"
#include "np/spiegel/intercept.hxx"
#include "np/util/filename.hxx"

#define SPIEGEL_DYNAMIC 1

namespace np {
namespace spiegel {

namespace dwarf {
class walker_t;
class compile_unit_t;
class state_t;
class entry_t;
};

class _factory_t;
class state_t;

#if SPIEGEL_DYNAMIC
struct value_t
{
    // Valid values for 'which' are
    // TC_INVALID
    // TC_VOID
    // TC_POINTER
    // TC_UNSIGNED_LONG_LONG
    // TC_SIGNED_LONG_LONG
    // TC_DOUBLE
    unsigned int which;
    union
    {
	void *vpointer;
	unsigned long long vuint;
	long long vsint;
	double vdouble;
    } val;

    static value_t make_invalid();
    static value_t make_void();
    static value_t make_sint(int64_t i);
    static value_t make_sint(int32_t i);
    static value_t make_pointer(void *p);
};
#endif

class function_t;

class _cacheable_t
{
public:
    _cacheable_t(np::spiegel::dwarf::reference_t ref, _factory_t &factory)
     : ref_(ref), factory_(factory) {}
    ~_cacheable_t() {}

    np::spiegel::dwarf::reference_t ref() const { return ref_; }

protected:
    np::spiegel::dwarf::reference_t ref_;
    _factory_t &factory_;
};

class compile_unit_t : public _cacheable_t
{
public:
    np::util::filename_t get_filename() const { return lower()->get_filename(); }
    np::util::filename_t get_compilation_directory() const { return lower()->get_compilation_directory(); }
    np::util::filename_t get_absolute_path() const { return lower()->get_absolute_path(); }
    const char *get_executable() const { return lower()->get_executable(); }
//     static compile_unit_t *for_name(const char *name);

    std::vector<function_t *> get_functions();

    void dump_types();

private:
    compile_unit_t(np::spiegel::dwarf::reference_t ref, _factory_t &factory)
      : _cacheable_t(ref.normalize_to_cu(), factory) {}
    ~compile_unit_t() {}

    // We keep the upper and lower compile unit objects
    // as two separate allocations with pointer in both
    // directions.  It's ugly, but there you go.
    static compile_unit_t *to_upper(np::spiegel::dwarf::compile_unit_t *lcu)
    {
        return static_cast<compile_unit_t*>(lcu->get_upper());
    }
    np::spiegel::dwarf::compile_unit_t *lower() const
    {
        // this only works because we called normalize_to_cu() in the ctor
        return static_cast<np::spiegel::dwarf::compile_unit_t*>(
            const_cast<np::spiegel::dwarf::reference_resolver_t*>(
                ref_.resolver
            )
        );
    }

    friend class member_t;
    friend class np::spiegel::state_t;
};


class type_t : public _cacheable_t
{
public:

    // Type classification constants
    enum
    {
	TC_INVALID =		0,

	TC_MAJOR_VOID =		(1<<8),
	TC_MAJOR_POINTER =	(2<<8),
	TC_MAJOR_ARRAY =	(3<<8),
	TC_MAJOR_INTEGER =	(4<<8),
	TC_MAJOR_FLOAT =	(5<<8),
	TC_MAJOR_COMPOUND =	(6<<8),
	_TC_MAJOR_MASK =	(7<<8),

	_TC_UNSIGNED =		(0<<7),
	_TC_SIGNED =		(1<<7),

	TC_VOID =		TC_MAJOR_VOID|0,
	TC_POINTER =		TC_MAJOR_POINTER|0,
	TC_REFERENCE =		TC_MAJOR_POINTER|1,
	TC_ARRAY =		TC_MAJOR_ARRAY|0,
	TC_UNSIGNED_CHAR =	TC_MAJOR_INTEGER|_TC_UNSIGNED|sizeof(char),
	TC_SIGNED_CHAR =	TC_MAJOR_INTEGER|_TC_SIGNED|sizeof(char),
	TC_UNSIGNED_INT8 =	TC_MAJOR_INTEGER|_TC_UNSIGNED|1,
	TC_SIGNED_INT8 =	TC_MAJOR_INTEGER|_TC_SIGNED|1,
	TC_UNSIGNED_SHORT =	TC_MAJOR_INTEGER|_TC_UNSIGNED|sizeof(short),
	TC_SIGNED_SHORT =	TC_MAJOR_INTEGER|_TC_SIGNED|sizeof(short),
	TC_UNSIGNED_INT16 =	TC_MAJOR_INTEGER|_TC_UNSIGNED|2,
	TC_SIGNED_INT16 =	TC_MAJOR_INTEGER|_TC_SIGNED|2,
	TC_UNSIGNED_INT =	TC_MAJOR_INTEGER|_TC_UNSIGNED|sizeof(int),
	TC_SIGNED_INT =		TC_MAJOR_INTEGER|_TC_SIGNED|sizeof(int),
	TC_UNSIGNED_INT32 =	TC_MAJOR_INTEGER|_TC_UNSIGNED|4,
	TC_SIGNED_INT32 =	TC_MAJOR_INTEGER|_TC_SIGNED|4,
	TC_UNSIGNED_LONG =	TC_MAJOR_INTEGER|_TC_UNSIGNED|sizeof(long),
	TC_SIGNED_LONG =	TC_MAJOR_INTEGER|_TC_SIGNED|sizeof(long),
	TC_UNSIGNED_LONG_LONG =	TC_MAJOR_INTEGER|_TC_UNSIGNED|sizeof(long long),
	TC_SIGNED_LONG_LONG =	TC_MAJOR_INTEGER|_TC_SIGNED|sizeof(long long),
	TC_UNSIGNED_INT64 =	TC_MAJOR_INTEGER|_TC_UNSIGNED|8,
	TC_SIGNED_INT64 =	TC_MAJOR_INTEGER|_TC_SIGNED|8,
	TC_FLOAT =		TC_MAJOR_FLOAT|sizeof(float),
	TC_DOUBLE =		TC_MAJOR_FLOAT|sizeof(double),
	TC_LONG_DOUBLE =	TC_MAJOR_FLOAT|sizeof(long double),
	TC_STRUCT =		TC_MAJOR_COMPOUND|0,
	TC_UNION =		TC_MAJOR_COMPOUND|1,
	TC_CLASS =		TC_MAJOR_COMPOUND|2,
    };

    unsigned int get_classification() const;
    std::string get_classification_as_string() const;
    static int major_type(unsigned int tc) { return (tc & _TC_MAJOR_MASK); }
    static int is_signed(unsigned int tc) { return !!(tc & _TC_SIGNED); }

    unsigned int get_sizeof() const;

#if 0
    // all the types including primitives like int
    std::vector<type_t*> get_classes() const;
    type_t *get_component_type() const;	// component of an array
    constructor_t *get_constructor(std::vector<type_t*> types) const;
    std::vector<constructor_t*> get_constructors() const;
    destructor_t *get_destructor() const;
    std::vector<type_t*> get_declared_classes() const;

    compile_unit_t *get_compile_unit() const;

    // fields declared by this class
    field_t *get_declared_field(const char *name) const;
    std::vector<field_t*> get_declared_fields() const;
    // fields declared by this class or any of its ancestors
    field_t *get_field(const char *name) const;
    std::vector<field_t*> get_fields() const;

    // functions declared by this class
    function_t *get_declared_function(const char *name) const;
    std::vector<function_t*> get_declared_functions() const;
    // functions declared by this class or any of its ancestors
    function_t *get_function(const char *name) const;
    std::vector<function_t*> get_functions() const;

    int get_modifiers() const;

    type_t *get_superclass() const;

    boolean is_array() const;
    boolean is_primitive() const;

#if SPIEGEL_DYNAMIC
    void *new_instance() const;
    value_t get(void *) const;
    void set(void *, value_t) const;
#endif
#endif
    // for class, struct, union, typedef
    std::string get_name() const;

    std::string to_string() const;

private:
    std::string to_string(std::string inner) const;
    type_t(np::spiegel::dwarf::reference_t ref, _factory_t &factory) : _cacheable_t(ref, factory) {}
    ~type_t() {}

    friend class function_t;
    friend class compile_unit_t;
    friend class _factory_t;
};

class member_t : public _cacheable_t
{
public:
    std::string get_name() const { return name_; }
    const compile_unit_t *get_compile_unit() const;
//     type_t *get_declaring_class() const;
//     int get_modifiers() const;

protected:
    member_t(np::spiegel::dwarf::walker_t &w, _factory_t &factory);
    ~member_t() {}

    const char *name_;
    friend class _factory_t;
};

#if 0
class field_t : public member_t
{
public:
    type_t *get_type() const;

#if SPIEGEL_DYNAMIC
    value_t get(void *) const;
    boolean get_boolean(void *) const;
    char get_char(void *) const;
    double get_double(void *) const;
    float get_float(void *) const;
    int get_int(void *) const;
    long get_long(void *) const;
    long long get_long_long(void *) const;
    short get_short(void *) const;

    void set(void *, value_t) const;
    void set_boolean(void *, boolean) const;
    void set_char(void *, char) const;
    void set_double(void *, double) const;
    void set_float(void *, float) const;
    void set_int(void *, int) const;
    void set_long(void *, long) const;
    void set_long_long(void *, long) const;
    void set_short(void *, short) const;
#endif

    char *to_string() const;
};

class constructor_t : public member_t
{
public:
    std::vector<type_t*> get_exception_types() const;
    std::vector<type_t*> get_parameter_types() const;
#if SPIEGEL_DYNAMIC
    void *new_instance(std::vector<value_t> initargs);
#endif
    char *to_string() const;
};

class destructor_t : public member_t
{
public:
#if SPIEGEL_DYNAMIC
    void delete_instance(void *);
#endif
    char *to_string() const;
};
#endif

class function_t : public member_t
{
public:
    std::string get_full_name() const;
    type_t *get_return_type() const;
    std::vector<type_t*> get_parameter_types() const;
    std::vector<const char *> get_parameter_names() const;
    bool has_unspecified_parameters() const;
    addr_t get_address() const;
    addr_t get_live_address() const;
    bool is_declaration() const;

//     std::vector<type_t*> get_exception_types() const;

#if SPIEGEL_DYNAMIC
    value_t invoke(std::vector<value_t> args) const;
#endif
    std::string to_string() const;

private:
    function_t(np::spiegel::dwarf::walker_t &w, _factory_t &factory) : member_t(w, factory) {}
    ~function_t() {}

    bool populate();

    uint64_t low_pc_;	    // TODO: should be an addr_t
    bool is_declaration_;

    friend class compile_unit_t;
    friend class _factory_t;
};

class location_t
{
public:
    compile_unit_t *compile_unit_;
    np::util::filename_t filename_;
    uint32_t line_;
    uint32_t column_;
    type_t *class_;
    function_t *function_;
    unsigned int offset_;
};


class _factory_t
{
public:
    type_t *make_type(np::spiegel::dwarf::reference_t);
    function_t *make_function(np::spiegel::dwarf::walker_t &);
    function_t *make_function(np::spiegel::dwarf::reference_t ref);
//     constructor_t *make_constructor(np::spiegel::dwarf::reference_t);
//     destructor_t *make_destructor(np::spiegel::dwarf::reference_t);
//     field_t *make_field(np::spiegel::dwarf::reference_t);

private:
    _factory_t() {}
    ~_factory_t() {}

    _cacheable_t *find(np::spiegel::dwarf::reference_t ref);
    _cacheable_t *add(_cacheable_t *cc);

    std::map<np::spiegel::dwarf::reference_t, _cacheable_t*> cache_;

    friend class state_t;
};

class state_t
{
public:
    state_t();
    ~state_t();

    bool add_self();
    bool add_executable(const char *filename);

    std::vector<compile_unit_t *> get_compile_units();
    bool describe_address(addr_t, class location_t &);
    std::string describe_stacktrace();

    // for testing only
    void dump_structs();
    void dump_functions();
    void dump_variables();
    void dump_info(bool preorder, bool paths);
    void dump_abbrevs();

private:
    compile_unit_t *cu_from_lower(np::spiegel::dwarf::compile_unit_t *lcu);

     np::spiegel::dwarf::state_t *state_;
     _factory_t factory_;
};


// close the namespaces
}; };

#endif // __spiegel_spiegel_hxx__

