#ifndef __spiegel_spiegel_hxx__
#define __spiegel_spiegel_hxx__ 1

#include <stdint.h>
#include <vector>
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
//     static compile_unit_t *for_name(const char *name);

    std::vector<function_t *> get_functions();

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

#if 0
class type_t
{
public:
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
#endif
    char *to_string() const;
};
#endif

class member_t
{
public:
    const char *get_name() const { return name_; }
//     type_t *get_declaring_class() const;
//     int get_modifiers() const;

protected:
    member_t() {}
    ~member_t() {}

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
//     type_t *get_return_type() const;
//     std::vector<type_t*> get_parameter_types() const;
//     std::vector<type_t*> get_exception_types() const;

#if SPIEGEL_DYNAMIC
    value_t invoke(void *obj, std::vector<value_t> args);
#endif
//     char *to_string() const;

private:
    function_t() {}
    ~function_t() {}

    bool populate(const spiegel::dwarf::entry_t *);

    friend class compile_unit_t;
};

}; // namespace spiegel

#endif // __spiegel_spiegel_hxx__

