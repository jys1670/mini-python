#pragma once

#include "runtime.h"

#include <functional>
#include <optional>

namespace ast
{

using Statement = runtime::Executable;

/*!
 * Statement that returns value of type T. This is used to create constants.
 */
template <typename T> class ValueStatement : public Statement
{
  public:
    explicit ValueStatement(T v) : value_(std::move(v))
    {
    }

    runtime::ObjectHolder Execute([[maybe_unused]] runtime::Closure &closure,
                                  [[maybe_unused]] runtime::Context &context) override
    {
        return runtime::ObjectHolder::Share(value_);
    }

  private:
    T value_;
};

using NumericConst = ValueStatement<runtime::Number>;
using StringConst = ValueStatement<runtime::String>;
using BoolConst = ValueStatement<runtime::Bool>;

/*!
 * Computes variable or object methods call chain.
 * Example: x = circle.center.x where circle.center.x - call chain
 */
class VariableValue : public Statement
{
  public:
    explicit VariableValue(const std::string &name);
    explicit VariableValue(std::vector<std::string> dotted_ids);

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    std::vector<std::string> ids_;
};

//! Assigns the value of the "rv" statement to the variable "var"
class Assignment : public Statement
{
  public:
    Assignment(std::string var, std::unique_ptr<Statement> &&rv);

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    std::string var_;
    std::unique_ptr<Statement> rv_;
};

//! Assigns value of the "rv" to "object.field_name" field
class FieldAssignment : public Statement
{
  public:
    FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> &&rv);

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    VariableValue object_;
    std::string field_name_;
    std::unique_ptr<Statement> rv_;
};

//! None value
class None : public Statement
{
  public:
    runtime::ObjectHolder Execute([[maybe_unused]] runtime::Closure &closure,
                                  [[maybe_unused]] runtime::Context &context) override
    {
        return {};
    }
};

//! print command
class Print : public Statement
{
  public:
    //! Initializes print command to output value of the "argument" statement
    explicit Print(std::unique_ptr<Statement> &&argument);
    //! Initializes print command to output values of vector of statements
    explicit Print(std::vector<std::unique_ptr<Statement>> &&args);

    //! Initializes print command to output value of variable with given name
    static std::unique_ptr<Print> Variable(const std::string &name);

    //! Print outputs to stream given by "context"
    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    std::vector<std::unique_ptr<Statement>> args_;
};

//! Calls method "object.method" with a given arguments "args"
class MethodCall : public Statement
{
  public:
    MethodCall(std::unique_ptr<Statement> &&object, std::string method, std::vector<std::unique_ptr<Statement>> &&args);

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    std::unique_ptr<Statement> object_;
    std::string method_;
    std::vector<std::unique_ptr<Statement>> args_;
};

/*!
 * Creates a new instance of the "class_" class, passing its constructor an "args" vector
 * of parameters. If the class does not have the "__init__" method with the specified number
 * of arguments, then an instance of the class is created without calling the constructor (the object fields
 * will not be initialized):
 */
class NewInstance : public Statement
{
  public:
    explicit NewInstance(const runtime::Class &class_);
    NewInstance(const runtime::Class &class_, std::vector<std::unique_ptr<Statement>> &&args);
    //! Returns object containing value of type ClassInstance
    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    const runtime::Class &class_;
    std::vector<std::unique_ptr<Statement>> args_;
};

//! Unary operations base class
class UnaryOperation : public Statement
{
  public:
    explicit UnaryOperation(std::unique_ptr<Statement> &&argument) : argument_(std::move(argument))
    {
    }

  protected:
    std::unique_ptr<Statement> argument_;
};

//! str operation, returns string representation of whatever object
class Stringify : public UnaryOperation
{
  public:
    using UnaryOperation::UnaryOperation;
    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;
};

//! Binary operation base class
class BinaryOperation : public Statement
{
  public:
    BinaryOperation(std::unique_ptr<Statement> &&lhs, std::unique_ptr<Statement> &&rhs)
        : left_(std::move(lhs)), right_(std::move(rhs))
    {
    }

  protected:
    std::unique_ptr<Statement> left_;
    std::unique_ptr<Statement> right_;
};

//! Returns result of addition
class Add : public BinaryOperation
{
  public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;
};

//! Returns result of subtraction
class Sub : public BinaryOperation
{
  public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;
};

//! Returns result of multiplication
class Mult : public BinaryOperation
{
  public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;
};

//! Returns result of division
class Div : public BinaryOperation
{
  public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;
};

//! Returns result of logical OR
class Or : public BinaryOperation
{
  public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;
};

//! Returns result of logical AND
class And : public BinaryOperation
{
  public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;
};

//! Returns result of logical NOT
class Not : public UnaryOperation
{
  public:
    using UnaryOperation::UnaryOperation;
    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;
};

//! Compound statement, combines other statements (i.e. method body insides, if-else blocks, etc)
class Compound : public Statement
{
  public:
    template <typename... Args> explicit Compound(Args &&...args)
    {
        (..., AddStatement(std::forward<Args>(args)));
    }

    //! Adds statement to the end of compound
    void AddStatement(std::unique_ptr<Statement> &&stmt)
    {
        statements_.emplace_back(std::move(stmt));
    }

    //! Sequentially executes all compound statements and returns None
    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

//! Method body. Usually contains compound statement
class MethodBody : public Statement
{
  public:
    explicit MethodBody(std::unique_ptr<Statement> &&body);

    //! Computes statement passed as body_, returns None unless there is return statement as body
    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    std::unique_ptr<Statement> body_;
};

//! Executes return with a given statement
class Return : public Statement
{
  public:
    explicit Return(std::unique_ptr<Statement> &&statement) : statement_(std::move(statement))
    {
    }

    //! Stops current method execution and returns value of whatever given "statement_"
    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    std::unique_ptr<Statement> statement_;
};

//! Class definition
class ClassDefinition : public Statement
{
  public:
    //! It is guaranteed that ObjectHolder contains runtime::Class object
    explicit ClassDefinition(runtime::ObjectHolder cls);

    //! Creates new object in closure with a name equal to class name and value which was passed to constructor
    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    runtime::ObjectHolder cls_;
};

//! if \<condition\> \<if_body\> else \<else_body\>
class IfElse : public Statement
{
  public:
    //! else_body can be nullptr
    IfElse(std::unique_ptr<Statement> &&condition, std::unique_ptr<Statement> &&if_body,
           std::unique_ptr<Statement> &&else_body);

    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    std::unique_ptr<Statement> condition_;
    std::unique_ptr<Statement> if_body_;
    std::unique_ptr<Statement> else_body_;
};

//! Comparison operation
class Comparison : public BinaryOperation
{
  public:
    using Comparator =
        std::function<bool(const runtime::ObjectHolder &, const runtime::ObjectHolder &, runtime::Context &)>;

    Comparison(Comparator cmp, std::unique_ptr<Statement> &&lhs, std::unique_ptr<Statement> &&rhs);

    //! Computes lhs/rhs and returns comparator execution result, converted into runtime::Bool
    runtime::ObjectHolder Execute(runtime::Closure &closure, runtime::Context &context) override;

  private:
    Comparator cmp_;
};

} // namespace ast
