#ifndef __spiegel_spiegel_hxx__
#define __spiegel_spiegel_hxx__ 1

#include <stdint.h>
#include <vector>
#include <string>
#include "spiegel/dwarf/reference.hxx"

#define SPIEGEL_DYNAMIC 0

namespace spiegel {

namespace dwarf {
class walker_t;
class compile_unit_t;
class state_t;
class entry_t;
};

#if SPIEGEL_DYNAMIC
union value_t
{
    void *vpointer;
    unsigned long long vuint;
    long long vsint;
    double vdouble;
    float vfloat;
};
#endif

class function_t;

class compile_unit_t
{
public:
    static std::vector<compile_unit_t *> get_compile_units();

    const char *get_name() const { return name_; }
    const char *get_compile_dir() const { return comp_dir_; }
    const char *get_executable() const;
//     static compile_unit_t *for_name(const char *name);

    std::vector<function_t *> get_functions();

    void dump_types();

private:
    compile_unit_t(spiegel::dwarf::reference_t ref)
     :  ref_(ref)
    {}
    ~compile_unit_t() {}

    bool populate();

    spiegel::dwarf::reference_t ref_;
    const char *name_;
    const char *comp_dir_;
    uint64_t low_pc_;	    // TODO: should be an addr_t
    uint64_t high_pc_;
    uint32_t language_;
};

class type_t
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
    static int major(unsigned int tc) { return (tc & _TC_MAJOR_MASK); }
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
    std::string to_string() const;

private:
    std::string to_string(std::string inner) const;
    type_t(spiegel::dwarf::reference_t ref)
     :  ref_(ref) {}
    ~type_t() {}

    spiegel::dwarf::reference_t ref_;

    friend class function_t;
    friend class compile_unit_t;
};

class member_t
{
public:
    const char *get_name() const { return name_; }
//     type_t *get_declaring_class() const;
//     int get_modifiers() const;

protected:
    member_t(spiegel::dwarf::walker_t &w);
    ~member_t() {}

    // We pull almost everything from DWARF info on demand
    // using this reference to the DW_TAG_subprogram
    spiegel::dwarf::reference_t ref_;

    const char *name_;
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
    type_t *get_return_type() const;
    std::vector<type_t*> get_parameter_types() const;
    std::vector<const char *> get_parameter_names() const;
    bool has_unspecified_parameters() const;
    void *get_address() const;

//     std::vector<type_t*> get_exception_types() const;

#if SPIEGEL_DYNAMIC
    value_t invoke(void *obj, std::vector<value_t> args);
#endif
    std::string to_string() const;

private:
    function_t(spiegel::dwarf::walker_t &w) : member_t(w) {}
    ~function_t() {}


    friend class compile_unit_t;
};

}; // namespace spiegel

#endif // __spiegel_spiegel_hxx__

