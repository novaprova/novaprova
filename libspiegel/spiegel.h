#ifndef __SPIEGEL_H__
#define __SPIEGEL_H__ 1

#include "vector"

namespace Spiegel
{

union Value
{
    void *vpointer;
    unsigned long long vuint;
    long long vsint;
    double vdouble;
    float vfloat;
};

class CompileUnit
{
public:
    const char *getName() const;
};

class Type
{
public:
    // all the types including primitives like int
    static Type *forName(const char *name);
    vector<Type*> getClasses() const;
    Type *getComponentType() const;	// component of an array
    Constructor *getConstructor(vector<Type*> types) const;
    vector<Constructor*> getConstructors() const;
    vector<Type*> getDeclaredClasses() const;
    Destructor *getDestructor() const;

    CompileUnit *getDeclaringCompileUnit() const;

    // fields declared by this class
    Field *getDeclaredField(const char *name) const;
    vector<Field*> getDeclaredFields() const;
    // fields declared by this class or any of its ancestors
    Field *getField(const char *name) const;
    vector<Field*> getFields() const;

    // ditto for methods

    int getModifiers() const;

    Type *getSuperclass() const;

    boolean isArray() const;
    boolean isPrimitive() const;

    // Object newInstance();
    char *toString() const;
};

class Member
{
public:
    const char *getName() const;
    Type *getDeclaringClass() const;
    int getModifiers() const;
};

class Field : public Member
{
public:
    Type *getType() const;

    Value get(void *) const;
    boolean getBoolean(void *) const;
    char getByte(void *) const;
    char getChar(void *) const;
    double getDouble(void *) const;
    float getFloat(void *) const;
    int getInt(void *) const;
    long getLong(void *) const;
    short getShort(void *) const;

    void set(void *, Value) const;
    void setBoolean(void *, boolean) const;
    void setByte(void *, char) const;
    void setChar(void *, char) const;
    void setDouble(void *, double) const;
    void setFloat(void *, float) const;
    void setInt(void *, int) const;
    void setLong(void *, long) const;
    void setShort(void *, short) const;

    char *toString() const;
};

class Constructor : public Member
{
public:
    vector<Type*> getExceptionTypes() const;
    vector<Type*> getParameterTypes() const;
    void *newInstance(vector<Value> initargs);
    char *toString() const;
};

class Destructor : public Member
{
    // ???
};

class Method : public Member
{
public:
    Type *getReturnType() const;
    vector<Type*> getParameterTypes() const;
    vector<Type*> getExceptionTypes() const;
    vector<Type*> getParameterTypes() const;
    Value invoke(void *obj, vector<Value> args);
    char *toString() const;
};

};

#endif __SPIEGEL_H__

