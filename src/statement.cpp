#include "statement.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <utility>

using namespace std;

namespace ast
{

using runtime::Bool;
using runtime::Class;
using runtime::ClassInstance;
using runtime::Closure;
using runtime::Context;
using runtime::Method;
using runtime::Number;
using runtime::ObjectHolder;
using runtime::String;

namespace
{
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
} // namespace

ObjectHolder Assignment::Execute(Closure &closure, Context &context)
{
    closure[var_] = rv_->Execute(closure, context);
    return closure.at(var_);
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> &&rv) : var_(std::move(var)), rv_(std::move(rv))
{
}

VariableValue::VariableValue(const std::string &name) : ids_({name})
{
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids) : ids_(std::move(dotted_ids))
{
}

ObjectHolder VariableValue::Execute(Closure &closure, [[maybe_unused]] Context &context)
{
    string err{"Unknown variable - "};
    if (!closure.count(ids_[0]))
    {
        err += ids_[0];
        throw runtime_error(err);
    }
    ObjectHolder obj = closure.at(ids_[0]);
    for (size_t i = 1; i < ids_.size(); ++i)
    {
        if (auto *ptr = obj.TryAs<ClassInstance>())
        {
            obj = ptr->Fields()[ids_[i]];
        }
        else
        {
            err += ids_[i];
            throw runtime_error(err);
        }
    }
    return obj;
}

unique_ptr<Print> Print::Variable(const std::string &name)
{
    return make_unique<Print>(std::make_unique<VariableValue>(name));
}

Print::Print(unique_ptr<Statement> &&argument)
{
    args_.emplace_back(std::move(argument));
}

Print::Print(vector<unique_ptr<Statement>> &&args) : args_(std::move(args))
{
}

ObjectHolder Print::Execute(Closure &closure, Context &context)
{
    ostream &out(context.GetOutputStream());
    auto argc = args_.size();
    for (size_t i{0}; i < argc; ++i)
    {
        if (ObjectHolder tmp_obj = args_[i]->Execute(closure, context))
        {
            tmp_obj->Print(out, context);
        }
        else
        {
            out << "None"s;
        }
        if (i + 1 != argc)
            out << ' ';
    }
    out << '\n';
    return ObjectHolder::None();
}

MethodCall::MethodCall(std::unique_ptr<Statement> &&object, std::string method,
                       std::vector<std::unique_ptr<Statement>> &&args)
    : object_(std::move(object)), method_(std::move(method)), args_(std::move(args))
{
}

ObjectHolder MethodCall::Execute(Closure &closure, Context &context)
{
    vector<ObjectHolder> curr_args;
    for (auto &arg : args_)
    {
        curr_args.push_back(arg->Execute(closure, context));
    }
    return object_->Execute(closure, context).TryAs<ClassInstance>()->Call(method_, curr_args, context);
}

ObjectHolder Stringify::Execute(Closure &closure, Context &context)
{
    ObjectHolder obj = argument_->Execute(closure, context);
    ostringstream os;
    if (!obj)
    {
        return ObjectHolder::Own(String("None"s));
    }
    auto *instance = obj.TryAs<ClassInstance>();
    if (instance && instance->HasMethod("__str__", 0))
    {
        instance->Call("__str__", {}, context)->Print(os, context);
    }
    else
    {
        obj->Print(os, context);
    }
    return ObjectHolder::Own(String(os.str()));
}

ObjectHolder Add::Execute(Closure &closure, Context &context)
{
    ObjectHolder left = left_->Execute(closure, context), right = right_->Execute(closure, context);

    if (auto *lp = left.TryAs<String>())
    {
        if (auto *rp = right.TryAs<String>())
        {
            string str{lp->GetValue()};
            str += rp->GetValue();
            return ObjectHolder::Own(String(str));
        }
        throw runtime_error("Incorrect addition");
    }

    if (auto *lp = left.TryAs<Number>())
    {
        if (auto *rp = right.TryAs<Number>())
        {
            return ObjectHolder::Own(Number(rp->GetValue() + lp->GetValue()));
        }
        throw runtime_error("Incorrect addition");
    }

    if (auto *lp = left.TryAs<ClassInstance>())
    {
        if (lp->HasMethod(ADD_METHOD, 1))
        {
            return lp->Call(ADD_METHOD, {right}, context);
        }
    }
    throw runtime_error("Incorrect addition");
}

ObjectHolder Sub::Execute(Closure &closure, Context &context)
{
    auto *left = left_->Execute(closure, context).TryAs<Number>(),
         *right = right_->Execute(closure, context).TryAs<Number>();
    if (left && right)
    {
        return ObjectHolder::Own(Number(left->GetValue() - right->GetValue()));
    }
    throw runtime_error("Incorrect subtraction");
}

ObjectHolder Mult::Execute(Closure &closure, Context &context)
{
    auto *left = left_->Execute(closure, context).TryAs<Number>(),
         *right = right_->Execute(closure, context).TryAs<Number>();
    if (left && right)
    {
        return ObjectHolder::Own(Number(left->GetValue() * right->GetValue()));
    }
    throw runtime_error("Incorrect multiplication");
}

ObjectHolder Div::Execute(Closure &closure, Context &context)
{
    auto *left = left_->Execute(closure, context).TryAs<Number>(),
         *right = right_->Execute(closure, context).TryAs<Number>();
    if (left && right)
    {
        if (right->GetValue())
        {
            return ObjectHolder::Own(Number(left->GetValue() / right->GetValue()));
        }
    }
    throw runtime_error("Incorrect division");
}

ObjectHolder Compound::Execute(Closure &closure, Context &context)
{
    for (const auto &st : statements_)
    {
        st->Execute(closure, context);
        if (closure.count("returned_value"))
        {
            return ObjectHolder::None();
        }
    }
    return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure &closure, Context &context)
{
    closure["returned_value"] = statement_->Execute(closure, context);
    return ObjectHolder::None();
}

ClassDefinition::ClassDefinition(ObjectHolder cls) : cls_(std::move(cls))
{
}

ObjectHolder ClassDefinition::Execute(Closure &closure, [[maybe_unused]] Context &context)
{
    closure[cls_.TryAs<Class>()->GetName()] = cls_;
    return ObjectHolder::None();
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> &&rv)
    : object_(std::move(object)), field_name_(std::move(field_name)), rv_(std::move(rv))
{
}

ObjectHolder FieldAssignment::Execute(Closure &closure, Context &context)
{
    Closure &fields = object_.Execute(closure, context).TryAs<ClassInstance>()->Fields();
    fields[field_name_] = rv_->Execute(closure, context);
    return fields.at(field_name_);
}

IfElse::IfElse(std::unique_ptr<Statement> &&condition, std::unique_ptr<Statement> &&if_body,
               std::unique_ptr<Statement> &&else_body)
    : condition_(std::move(condition)), if_body_(std::move(if_body)), else_body_(std::move(else_body))
{
}

ObjectHolder IfElse::Execute(Closure &closure, Context &context)
{
    if (IsTrue(condition_->Execute(closure, context)))
    {
        if_body_->Execute(closure, context);
    }
    else if (else_body_)
    {
        else_body_->Execute(closure, context);
    }
    return ObjectHolder::None();
}

ObjectHolder Or::Execute(Closure &closure, Context &context)
{
    bool left = IsTrue(left_->Execute(closure, context));
    if (!left)
    {
        return ObjectHolder::Own(Bool(IsTrue(right_->Execute(closure, context))));
    }
    return ObjectHolder::Own(Bool(true));
}

ObjectHolder And::Execute(Closure &closure, Context &context)
{
    bool left = IsTrue(left_->Execute(closure, context));
    if (left)
    {
        return ObjectHolder::Own(Bool(IsTrue(right_->Execute(closure, context))));
    }
    return ObjectHolder::Own(Bool(false));
}

ObjectHolder Not::Execute(Closure &closure, Context &context)
{
    return ObjectHolder::Own(Bool(!IsTrue(argument_->Execute(closure, context))));
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> &&lhs, unique_ptr<Statement> &&rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(std::move(cmp))
{
}

ObjectHolder Comparison::Execute(Closure &closure, Context &context)
{
    return ObjectHolder::Own(Bool(cmp_(left_->Execute(closure, context), right_->Execute(closure, context), context)));
}

NewInstance::NewInstance(const runtime::Class &class_, std::vector<std::unique_ptr<Statement>> &&args)
    : class_(class_), args_(std::move(args))
{
}

NewInstance::NewInstance(const runtime::Class &class_) : class_(class_)
{
}

ObjectHolder NewInstance::Execute(Closure &closure, Context &context)
{
    ObjectHolder obj = ObjectHolder::Own(ClassInstance(class_));
    const Method *method = class_.GetMethod(INIT_METHOD);
    vector<ObjectHolder> args;
    if (!method || method->formal_params.size() != args_.size())
    {
        return obj;
    }
    for (auto &arg : args_)
    {
        args.emplace_back(arg->Execute(closure, context));
    }
    if (auto post_init_obj = obj.TryAs<ClassInstance>()->Call(INIT_METHOD, args, context))
    {
        return post_init_obj;
    }
    return obj;
}

MethodBody::MethodBody(std::unique_ptr<Statement> &&body) : body_(std::move(body))
{
}

ObjectHolder MethodBody::Execute(Closure &closure, Context &context)
{
    body_->Execute(closure, context);
    if (closure.count("returned_value"))
    {
        return closure.at("returned_value");
    }
    return ObjectHolder::None();
}

} // namespace ast
