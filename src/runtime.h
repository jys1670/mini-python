#pragma once

#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace runtime
{

//! Instructions execution context
class Context
{
  public:
    //! return output stream for printing
    virtual std::ostream &GetOutputStream() = 0;

  protected:
    ~Context() = default;
};

//! Base class of all objects, just like in normal Python
class Object
{
  public:
    virtual ~Object() = default;
    //! Each object must have string representation, which can be printed
    virtual void Print(std::ostream &os, Context &context) = 0;
};

//! Special wrapper, contains methods that make it easier to construct Objects and access their values.
class ObjectHolder
{
  public:
    //! Create empty value
    ObjectHolder() = default;

    ObjectHolder(const ObjectHolder &other);
    ObjectHolder &operator=(const ObjectHolder &other);
    ObjectHolder(ObjectHolder &&other) noexcept;
    ObjectHolder &operator=(ObjectHolder &&other) noexcept;

    //! Returns ObjectHolder that owns object of type T
    //! T - type derived from Object
    //! Object is copied or moved to heap
    template <typename T> [[nodiscard]] static ObjectHolder Own(T &&object)
    {
        return ObjectHolder(std::make_shared<T>(std::forward<T>(object)));
    }

    //! Returns non-owning ObjectHolder (shared_ptr with empty deleter), similar to weak_ptr
    //! This is mainly used for "self" parameter in class methods calls
    [[nodiscard]] static ObjectHolder Share(Object &object);
    //! Returns empty ObjectHolder, "None"
    [[nodiscard]] static ObjectHolder None();

    //! Returns reference to Object inside of ObjectHolder
    Object &operator*() const;

    //! Returns pointer to Object inside of ObjectHolder
    Object *operator->() const;

    //! Return raw Object pointer
    [[nodiscard]] Object *Get() const;

    //! Returns raw pointer to object of type T or nullptr
    template <typename T> [[nodiscard]] T *TryAs() const
    {
        return dynamic_cast<T *>(this->Get());
    }

    //! Returns true if ObjectHolder is not empty
    explicit operator bool() const;

  private:
    explicit ObjectHolder(std::shared_ptr<Object> data);
    void AssertIsValid() const;

    std::shared_ptr<Object> data_;
};

//! Stores value of type T
template <typename T> class ValueObject : public Object
{
  public:
    ValueObject(T v) : value_(v)
    {
    }

    void Print(std::ostream &os, [[maybe_unused]] Context &context) override
    {
        os << value_;
    }

    [[nodiscard]] const T &GetValue() const
    {
        return value_;
    }

  private:
    T value_;
};

//! Symbols table, connects object name and its value
using Closure = std::unordered_map<std::string, ObjectHolder>;

//! Checks whether object contains value convertible to boolean "True"
//! Non-zero numbers, non-empty strings are "True", everything else is "False"
bool IsTrue(const ObjectHolder &object);

//! Interface that links runtime module with semantic analyzer
class Executable
{
  public:
    virtual ~Executable() = default;
    //! Does something with objects inside of closure, outputs to context.
    //! Returns result or None
    virtual ObjectHolder Execute(Closure &closure, Context &context) = 0;
};

//! String literal
using String = ValueObject<std::string>;
//! Number
using Number = ValueObject<int>;

//! Boolean
class Bool : public ValueObject<bool>
{
  public:
    using ValueObject<bool>::ValueObject;

    void Print(std::ostream &os, Context &context) override;
};

//! Describes class method
struct Method
{
    //! Method name
    std::string name;
    //! Argument names
    std::vector<std::string> formal_params;
    //! Function body
    std::unique_ptr<Executable> body;
};

//! Implements class
class Class : public Object
{
  public:
    //! Creates class with a given name and set of methods, inherited from parent (can be nullptr)
    explicit Class(std::string name, std::vector<Method> &&methods, const Class *parent);

    //! Returns pointer to method or nullptr
    [[nodiscard]] const Method *GetMethod(const std::string &name) const;

    //! Returns class name
    [[nodiscard]] const std::string &GetName() const;

    //! prints "Class <name>"
    void Print(std::ostream &os, Context &context) override;

  private:
    std::string name_;
    std::vector<Method> methods_;
    std::unordered_map<std::string, size_t> name_to_method_;
    const Class *parent_;
};

//! Class instance
class ClassInstance : public Object
{
  public:
    explicit ClassInstance(const Class &cls);

    //! If objects has __str__ methods, outputs string representation, otherwise prints memory address
    void Print(std::ostream &os, Context &context) override;

    /*!
     * @brief Calls method with give "actual_args" arguments and output context "ctx".
     * In case method does not exist in current or parent class, throws runtime_error
     */
    ObjectHolder Call(const std::string &method, const std::vector<ObjectHolder> &args, Context &ctx);

    //! Checks if there is method that takes "argc" amount of arguments
    [[nodiscard]] bool HasMethod(const std::string &method, size_t argc) const;

    //! Returns closure containing object fields
    [[nodiscard]] Closure &Fields();
    //! Returns constant closure containing object fields
    [[nodiscard]] const Closure &Fields() const;

  private:
    const Class &class_;
    Closure fields_;
};

/*!
 * Returns true if lhs and rhs are equal numbers, strings and booleans.
 * If both are None, returns true as well.
 * For other types checks whether object has __eq__ method and uses it.
 * Lastly, if case is something not previously mentioned, throws runtime_error
 */
bool Equal(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);

//! Same principles as in Equal operator
bool Less(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);
bool NotEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);
bool Greater(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);
bool LessOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);
bool GreaterOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);

// Used in unit tests
struct DummyContext : Context
{
    std::ostream &GetOutputStream() override
    {
        return output;
    }

    std::ostringstream output;
};

class SimpleContext : public runtime::Context
{
  public:
    explicit SimpleContext(std::ostream &output) : output_(output)
    {
    }

    std::ostream &GetOutputStream() override
    {
        return output_;
    }

  private:
    std::ostream &output_;
};

} // namespace runtime
