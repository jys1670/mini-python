#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>

using namespace std;

namespace runtime
{

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data) : data_(std::move(data))
{
}

void ObjectHolder::AssertIsValid() const
{
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object &object)
{
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto * /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None()
{
    return {};
}

ObjectHolder::ObjectHolder(const ObjectHolder &other) = default;
ObjectHolder &ObjectHolder::operator=(const ObjectHolder &other) = default;
ObjectHolder::ObjectHolder(ObjectHolder &&other) noexcept : data_(std::move(other.data_))
{
}
ObjectHolder &ObjectHolder::operator=(ObjectHolder &&other) noexcept
{
    data_ = std::move(other.data_);
    return *this;
}

Object &ObjectHolder::operator*() const
{
    AssertIsValid();
    return *Get();
}

Object *ObjectHolder::operator->() const
{
    AssertIsValid();
    return Get();
}

Object *ObjectHolder::Get() const
{
    return data_.get();
}

ObjectHolder::operator bool() const
{
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder &object)
{
    if (!object)
    {
        return false;
    }
    if (const auto *ptr = object.TryAs<Number>())
    {
        return ptr->GetValue();
    }
    if (const auto *ptr = object.TryAs<Bool>())
    {
        return ptr->GetValue();
    }
    if (const auto *ptr = object.TryAs<String>())
    {
        return !(ptr->GetValue()).empty();
    }
    return false;
}

void ClassInstance::Print(std::ostream &os, Context &context)
{
    if (class_.GetMethod("__str__"))
    {
        os << Call("__str__", {}, context).TryAs<String>()->GetValue();
        return;
    }
    os << this;
}

bool ClassInstance::HasMethod(const std::string &method, size_t argc) const
{
    if (const auto *method_ptr = class_.GetMethod(method))
    {
        if (method_ptr->formal_params.size() == argc)
            return true;
    }
    return false;
}

Closure &ClassInstance::Fields()
{
    return fields_;
}

const Closure &ClassInstance::Fields() const
{
    return fields_;
}

ClassInstance::ClassInstance(const Class &cls) : class_{cls}
{
}

ObjectHolder ClassInstance::Call(const std::string &method, const std::vector<ObjectHolder> &args, Context &ctx)
{
    if (!HasMethod(method, args.size()))
    {
        throw std::runtime_error("Method does not exist"s);
    }
    const auto *method_ptr = class_.GetMethod(method);
    Closure closure = {{"self", ObjectHolder::Share(*this)}};
    for (size_t i{0}; i < args.size(); ++i)
    {
        closure[method_ptr->formal_params[i]] = args[i];
    }
    ObjectHolder res = method_ptr->body->Execute(closure, ctx);
    if (closure.at("self").Get() != this)
    {
        return closure.at("self");
    }
    return res;
}

Class::Class(std::string name, std::vector<Method> &&methods, const Class *parent)
    : name_{std::move(name)}, methods_{std::move(methods)}, parent_{parent}
{
    for (size_t i = 0; i < methods_.size(); ++i)
    {
        name_to_method_[methods_[i].name] = i;
    }
}

const Method *Class::GetMethod(const std::string &name) const
{
    if (name_to_method_.count(name))
    {
        return &methods_[name_to_method_.at(name)];
    }
    if (parent_)
    {
        return parent_->GetMethod(name);
    }
    return nullptr;
}

[[nodiscard]] const std::string &Class::GetName() const
{
    return name_;
}

void Class::Print(ostream &os, [[maybe_unused]] Context &context)
{
    os << "Class "sv << GetName();
}

void Bool::Print(std::ostream &os, [[maybe_unused]] Context &context)
{
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    // None
    if (!lhs && !rhs)
    {
        return true;
    }
    // Bool
    if (auto *left = lhs.TryAs<Bool>())
    {
        if (auto *right = rhs.TryAs<Bool>())
        {
            return left->GetValue() == right->GetValue();
        }
        throw std::runtime_error("Equality operator is not applicable"s);
    }
    // Number
    if (auto *left = lhs.TryAs<Number>())
    {
        if (auto *right = rhs.TryAs<Number>())
        {
            return left->GetValue() == right->GetValue();
        }
        throw std::runtime_error("Equality operator is not applicable"s);
    }
    // String
    if (auto *left = lhs.TryAs<String>())
    {
        if (auto *right = rhs.TryAs<String>())
        {
            return left->GetValue() == right->GetValue();
        }
        throw std::runtime_error("Equality operator is not applicable"s);
    }
    // User-defined types
    if (auto *left = lhs.TryAs<ClassInstance>())
    {
        if (left->HasMethod("__eq__", 1))
        {
            return IsTrue(left->Call("__eq__", {rhs}, context));
        }
    }
    throw std::runtime_error("Equality operator is not applicable"s);
}

bool Less(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    if (auto *left = lhs.TryAs<Bool>())
    {
        if (auto *right = rhs.TryAs<Bool>())
        {
            return left->GetValue() < right->GetValue();
        }
        throw std::runtime_error("Less operator is not applicable"s);
    }
    if (auto *left = lhs.TryAs<Number>())
    {
        if (auto *right = rhs.TryAs<Number>())
        {
            return left->GetValue() < right->GetValue();
        }
        throw std::runtime_error("Less operator is not applicable"s);
    }
    if (auto *left = lhs.TryAs<String>())
    {
        if (auto *right = rhs.TryAs<String>())
        {
            return left->GetValue() < right->GetValue();
        }
        throw std::runtime_error("Less operator is not applicable"s);
    }
    if (auto *left = lhs.TryAs<ClassInstance>())
    {
        if (left->HasMethod("__lt__", 1))
        {
            return IsTrue(left->Call("__lt__", {rhs}, context));
        }
    }
    throw std::runtime_error("Less operator is not applicable"s);
}

bool NotEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    // Exception might not be thrown due to short circuit, so long way is implemented
    bool eq = !Equal(lhs, rhs, context), ls = !Less(lhs, rhs, context);
    return eq && ls;
}

bool LessOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    return !Greater(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    return !Less(lhs, rhs, context);
}

} // namespace runtime
