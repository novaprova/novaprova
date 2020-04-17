#ifndef __NP_SPIEGEL_N3996_HXX__
#define __NP_SPIEGEL_N3996_HXX__ 1

#include <string>
#include <vector>
#include <atomic>
#include "np/spiegel/dwarf/reference.hxx"
#include "np/spiegel/dwarf/compile_unit.hxx"

namespace np { namespace spiegel { namespace n3996 {

class Metaobject;
class CompileUnit;
class LinkObject;
class Scope;
class Inheritance;
class Parameter;
// class TemplateParameter;
class Constant;


typedef std::string String;

template <class T /*public Metaobject*/> class Range : public std::vector<T*> {};

class _Refcounted
{
public:
    void ref() { _refcount.fetch_add(1); }
    void unref() { if (_refcount.fetch_sub(1) == 1) delete(this); }

protected:
    _Refcounted() : _refcount(0) {}
    virtual ~_Refcounted() = 0;

private:
    std::atomic_int _refcount;
};

template <class T /*public _Refcounted*/> class _SmartPointer
{
public:
    _SmartPointer() : _p(0) {}
    _SmartPointer(T *p) : _p(0) { up(p); }
    ~_SmartPointer() { down(); }
    // copy ctor
    _SmartPointer(const _SmartPointer<T> &o) : _p(0) { up(o._p); }
    // move ctor
    _SmartPointer(_SmartPointer<T> &&o) { _p = o._p; o._p = 0; }

    operator T*() const { return _p; }
    T *get() const { return _p; }

    T *operator=(T *p) { down(); up(p); return _p; }

private:
    void down() { if (_p) _p->unref(); }
    void up(T *p) { if (p) p->ref(); _p = p; }

    T *_p;
};

template <class T /*public _Refcounted*/> class _SmartVector
{
public:
    ~_SmartVector()
    {
        for (typename std::vector<T*>::iterator i = _v.begin() ; i != _v.end() ; ++i)
            (*i)->unref();
    }

    void push_back(T *t) { t->ref(); _v.push_back(t); }
    operator const Range<T>() const { return (const Range<T>&)_v; }

private:
    std::vector<T *> _v;
};

class Metaobject : public virtual _Refcounted
{
public:
    enum class Category : unsigned char
    {
        NONE = 0,
        LINK_OBJECT,        /* not defined in N3996 */
        COMPILE_UNIT,       /* not defined in N3996 */
        NAMESPACE,
        GLOBAL_SCOPE,
        TYPE,
        TYPEDEF,
        CLASS,
        FUNCTION,
        CONSTRUCTOR,
        OPERATOR,
        OVERLOADED_FUNCTION,
        ENUM,
        ENUM_CLASS,
        INHERITANCE,
        CONSTANT,
        VARIABLE,
        PARAMETER
    };

    virtual Metaobject::Category category() const { return Metaobject::Category::NONE; }
    virtual bool has_name() const { return false; }
    virtual bool has_scope() const { return false; }
    virtual bool is_scope() const { return false; }
    virtual bool is_class_member() const { return false; }
    virtual bool has_template() const { return false; }
    virtual bool is_template() const { return false; }

protected:
    Metaobject() {}
    virtual void reference(np::spiegel::dwarf::reference_t &ref) { _ref = ref; }

    np::spiegel::dwarf::reference_t _ref;

private:
    virtual void do_not_instantiate_me() const = 0;
};

// TODO: should this derive from Metaobject at all??
class Specifier : public virtual Metaobject
{
public:

    enum class Category : unsigned char
    {
        NONE = 0,
        EXTERN,
        STATIC,
        MUTABLE,
        REGISTER,
        THREAD_LOCAL,
        CONST,
        VIRTUAL,
        PRIVATE,
        PROTECTED,
        PUBLIC,
        CLASS,
        STRUCT,
        UNION,
        ENUM,
        ENUM_CLASS,
        CONSTEXPR
    };

    Category specifier_category() { return _specifier_category; }
    String keyword() const { return String(_keywords[(int)_specifier_category]); }

protected:
    Specifier(Category specifier_category) : _specifier_category(specifier_category) {}

private:
    static const char * const _keywords[];
    Category _specifier_category;
    friend class SpecifierBuilder;
};

struct Location
{
    CompileUnit *compile_unit;
    unsigned int line_number;

    const Location *if_valid() const { return line_number ? this : 0; }
};

class Named : public virtual Metaobject
{
public:
    bool has_name() const { return true; }
    String base_name() const { return _base_name; }
    const Location *defined_location() const { return _defined_location.if_valid(); }

protected:
    Named() {}

private:
    void base_name(String base_name) { _base_name = base_name; }
    void defined_location(const Location &l) { _defined_location = l; }
    String _base_name;
    Location _defined_location;
    friend class NamespaceBuilder;
};

class Scoped : public virtual Metaobject
{
public:
    bool has_scope() const { return true; }
    // can be Namespace, GlobalScope, Class
    Scope *scope() const { return _scope; }

protected:
    Scoped() {}

private:
    void scope(Scope *scope) { _scope = scope; }
    // this is an up-pointer and so doesn't take a reference
    Scope *_scope;
    friend class NamespaceBuilder;
};

class NamedScoped : public virtual Named, public virtual Scoped
{
public:
    String full_name() const { return _full_name; }

protected:
    NamedScoped() {}

private:
    void full_name(String s) { _full_name = s; }
    String _full_name;
    friend class NamespaceBuilder;
};

class Scope : public virtual NamedScoped
{
public:
    bool is_scope() const { return true; }
    const Range<Scoped> members() const { return _members; }

protected:
    Scope() {}

private:
    void add_member(Scoped *member) { _members.push_back(member); }
    _SmartVector<Scoped> _members;
    friend class NamespaceBuilder;
    friend class GlobalScopeBuilder;
    friend class TypeBuilder;
    friend class TypedefBuilder;
    friend class ClassBuilder;
    friend class FunctionBuilder;
    friend class TemplateBuilder;
};

class Namespace : public virtual Scope
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::NAMESPACE; }

protected:
    Namespace() {}

private:
    friend class NamespaceBuilder;
};

class GlobalScope : public virtual Namespace
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::GLOBAL_SCOPE; }

protected:
    GlobalScope() {}

private:
    friend class GlobalScopeBuilder;
    _SmartVector<CompileUnit> compile_units_;
    _SmartVector<LinkObject> link_objects_;
};

class CompileUnit : public virtual Metaobject
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::COMPILE_UNIT; }
    String filename() const { return _lower->get_filename(); }
    String compilation_directory() const { return _lower->get_compilation_directory(); }
    const LinkObject *link_object() const { return _link_object; }
    const GlobalScope *global_scope() const { return _global_scope; }
    uint32_t language() const { return _lower->get_language(); }
    bool is_C_like_language() const
    {
#if 0
        switch (language())
        {
        case DW_LANG_C:
        case DW_LANG_C89:
        case DW_LANG_C99:
        case DW_LANG_C_plus_plus:
            return true;
        }
#endif
        return false;
    }

protected:
    void link_object(LinkObject *lo) { _link_object = lo; }
    void global_scope(GlobalScope *gs) { _global_scope = gs; }
    void reference(np::spiegel::dwarf::reference_t &ref) override
    {
        dwarf::compile_unit_offset_tuple_t res = ref.resolve();
        _lower = res._cu;
    }

private:
    CompileUnit() {}
    np::spiegel::dwarf::compile_unit_t *_lower;
    LinkObject *_link_object;
    GlobalScope *_global_scope;
    friend class CompileUnitBuilder;
};

class LinkObject : public virtual Metaobject
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::LINK_OBJECT; }
    String filename() const { return _filename; }
    const Range<CompileUnit> compile_units() const { return _compile_units; }

protected:
    void filename(String f) { _filename = f; }
    void add_compile_unit(CompileUnit *cu) { _compile_units.push_back(cu); }

private:
    LinkObject() {}
    // np::spiegel::dwarf::linkobj_t *_dwarf_lo;
    String _filename;
    Range<CompileUnit> _compile_units;
    friend class LinkObjectBuilder;
    friend class CompileUnitBuilder;
};

class Type : public virtual NamedScoped
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::TYPE; }
    const Type *original_type() const { return _original_type; }

protected:
    Type() {}

private:
    void original_type(Type *t) { _original_type = t; }
    _SmartPointer<Type> _original_type;
    friend class TypeBuilder;
    friend class TypedefBuilder;
    friend class ClassBuilder;
};

class Typedef : public virtual Type
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::TYPEDEF; }
    const Type *typedef_type() const { return _typedef_type; }

private:
    void typedef_type(Type *typedef_type) { _typedef_type = typedef_type; }
    _SmartPointer<Type> _typedef_type;
    friend class TypedefBuilder;
};

class Class : public virtual Scope, public virtual Type
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::CLASS; }
    const Specifier *elaborated_type() const { return _elaborated_type; }
    const Range<Inheritance> base_classes() const { return _base_classes; }

private:
    Class() {}
    void elaborated_type(Specifier *t) { _elaborated_type = t; }
    void add_inheritance(Inheritance *i) { _base_classes.push_back(i); }
    _SmartPointer<Specifier> _elaborated_type;
    _SmartVector<Inheritance> _base_classes;
    friend class ClassBuilder;
    friend class TemplateBuilder;
};

class Function : public virtual Scope
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::FUNCTION; }
    const Specifier *linkage() const { return _linkage; }
    const Range<Parameter> parameters() const { return _parameters; }
    const Range<Type> exceptions() const { return _exceptions; }
    const Specifier *constness() const { return _constness; }
    bool is_constexpr() const { return !!(_flags & Flags::IS_CONSTEXPR); }
    bool is_noexcept() const { return !!(_flags & Flags::IS_NOEXCEPT); }
    bool is_pure() const { return !!(_flags & Flags::IS_PURE); }
    // TODO
    // XXX call(XXX);

protected:
    Function() {}

private:
    enum Flags
    {
        NONE=0,
        IS_CONSTEXPR=(1<<0),
        IS_NOEXCEPT=(1<<1),
        IS_PURE=(1<<2),
        // TODO: GNU extension function attributes such as noexit
        // if any of those are visible in the DWARF
    };

    void add_parameter(Parameter *p) { _parameters.push_back(p); }
    void add_exception(Type *t) { _exceptions.push_back(t); }
    void constness(Specifier *s) { _constness = s; }
    void flags(unsigned int f) { _flags = f; }

    _SmartPointer<Specifier> _linkage;
    _SmartVector<Parameter> _parameters;
    _SmartVector<Type> _exceptions;
    _SmartPointer<Specifier> _constness;
    unsigned int _flags;
    friend class FunctionBuilder;
    friend class TemplateBuilder;
};

// Represents a meta object which is *possibly* a member of a class,
// i.e. under at least circumstances it's a member of a class.
class ClassMember : public virtual NamedScoped
{
public:
    // TODO: a lot of classes derive from ClassMember which can be
    // either a class member or in fact not.  For example a function
    // can be a class member or declared in the global scope.  In
    // this case the Scope we point to will be either a Class or
    // some kind of Namespace.  Given that, does this behavior
    // of is_class_member() make any sense?  Should it check for
    // an enclosing Scope that is a Class?
    bool is_class_member() const { return true; }
    const Specifier *access_type() const { return _access_type; }

protected:
    ClassMember() {}
    void access_type(Specifier *s) { _access_type = s; }

private:
    _SmartPointer<Specifier> _access_type;
    friend class TemplateBuilder;
};

class Initializer : public virtual Function
{
};

class Constructor : public virtual ClassMember, public virtual Function
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::CONSTRUCTOR; }

private:
    Constructor() {}

};


// doc says: Operator is a Function and possibly a ClassMember reflecting an operator.
// TODO WTF does "possibly" mean?
class Operator : public virtual ClassMember, public virtual Function
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::OPERATOR; }

private:
};

// doc says: OverloadedFunction is a NamedScoped and possibly a ClassMember reflecting an over- loaded function
// TODO WTF does "possibly" mean?
// I resolved this by deriving from ClassMember and re-iterpreting
// ClassMember as meaning "possibly a class member"
class OverloadedFunction : public ClassMember
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::OVERLOADED_FUNCTION; }
    const Range<Function> overloads() const { return _overloads; }

protected:
    OverloadedFunction() {}

private:
    void add_overload(Function *f) { _overloads.push_back(f); }
    _SmartVector<Function> _overloads;
};

#if 0
// doc says: Template is a NamedScoped and possibly a Class, ClassMember or Function reflecting a class or (member) function template
// TODO: WTF does "possibly" mean?
// TODO: WTF is a "concept"
class Template :
    public virtual ClassMember,
    public virtual Class,
    public virtual Function,
    public virtual NamedScoped
{
public:
    bool is_template() const { return true; }
    Range<TemplateParameter> template_parameters() const { return _template_parameters; }

private:
    Template();
    void add_template_parameter(TemplateParameter *p) { _template_parameters.push_back(p); }
    _SmartVector<TemplateParameter> _template_parameters;
    friend class TemplateBuilder;
};

// TemplateParameter is a either Typedef or Constant which reflects a type or non-type template parameter or parameter pack
// TODO: WTF does this mean
// TODO: this is not going to work... will have to get creative
class TemplateParameter : public Typedef, public Constant
{
public:
    bool is_template() const { return true; }
    size_t position() const { return _position; }
    bool is_pack() const { return _is_pack; };

private:
    size_t _position;
    bool _is_pack;
};


// Instantiation is a either Class, ClassMember or Function that reflects an instantiation of a class or (member) function template.
// TODO: WTF does this mean
class Instantiation : ClassMember, Class, Function
{
    bool has_template() const { return true; }
    Template template() const { return _template; }

private:
    Template _template;
};
#endif

class Enum : public Type
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::ENUM; }
    const Type *base_type() const { return _base_type; }
    // TODO: missing from the doc
    const Range<Constant> members() const { return _members; }

private:
    void base_type(Type *t) { _base_type = t; }
    void add_member(Constant *c) { _members.push_back(c); }
    _SmartVector<Constant> _members;
    _SmartPointer<Type> _base_type;
};


class EnumClass : public Type, public Scope
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::ENUM_CLASS; }
    const Type *base_type() const { return _base_type; }
    // TODO: missing from the doc
    const Range<Constant> members() const { return _members; }

private:
    void base_type(Type *t) { _base_type = t; }
    void add_member(Constant *c) { _members.push_back(c); }
    _SmartVector<Constant> _members;
    _SmartPointer<Type> _base_type;
};


class Inheritance : public Metaobject
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::INHERITANCE; }
    const Specifier *access_type() const { return _access_type; }
    const Specifier *inheritance_type() const { return _inheritance_type; }
    const Class *base_class() const { return _base_class; }
    const Class *derived_class() const { return _derived_class; }

private:
    void access_type(Specifier *s) { _access_type = s; }
    void inheritance_type(Specifier *s) { _inheritance_type = s; }
    void base_class(Class *c) { _base_class = c; }
    void derived_class(Class *c) { _derived_class = c; }
    _SmartPointer<Specifier> _access_type;
    _SmartPointer<Specifier> _inheritance_type;
    // Inheritance owns a reference on the base class but not
    // the derived class, because the derived class owns a
    // reference on us.
    _SmartPointer<Class> _base_class;
    Class *_derived_class;
    friend class ClassBuilder;
};

// TODO doc says: Variable is a NamedScoped and possibly a ClassMember which reflects a variable defined in a namespace, class, function, etc.
// of course deriving from both of those makes gcc unhappy
class Variable : public ClassMember
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::VARIABLE; }
    const Specifier *storage_class() const { return _storage_class; }
    const Type *type() const { return _type; }

protected:
    Variable() {}
    void storage_class(Specifier *s) { _storage_class = s; }
    void type(Type *t) { _type = t; }

private:
    _SmartPointer<Specifier> _storage_class;
    _SmartPointer<Type> _type;
    friend class VariableBuilder;
};

class Parameter : public Variable
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::PARAMETER; }
    size_t position() const { return _position; }
    bool is_pack() const { return _is_pack; }

protected:
    Parameter() {}
    void position(size_t p) { _position = p; }
    void pack(bool b) { _is_pack = b; }

private:
    size_t _position;
    bool _is_pack;
    friend class ParameterBuilder;
};

// TODO: doc says Constant is a Named and possibly a NamedScoped or ClassMember which reflects
// a named compile-time constant values, like non-type template parameters and enumeration val- ues.
// of course deriving from both of those makes gcc unhappy
class Constant : public ClassMember
{
public:
    Metaobject::Category category() const override { return Metaobject::Category::CONSTANT; }

protected:
    Constant() {}

private:
    friend class ConstantBuilder;
};

// TODO: where is the support for unions

} } }; // namespaces

namespace std
{

np::spiegel::n3996::_SmartPointer<np::spiegel::n3996::GlobalScope> reflected_global_scope();
np::spiegel::n3996::_SmartPointer<np::spiegel::n3996::GlobalScope> reflected_global_scope_for_object_file(const char *filename);

};  // std

#endif /* __NP_SPIEGEL_N3996_HXX__ */
