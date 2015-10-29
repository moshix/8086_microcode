#include "alfe/main.h"

#ifndef INCLUDED_STRING_FUNCTIONS_H
#define INCLUDED_STRING_FUNCTIONS_H

#include "alfe/function.h"

class AddStringString : public Nullary<Function, AddStringString>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            String l = i->value<String>();
            ++i;
            return TypedValue(l + i->value<String>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(StringType(), StringType(), StringType()), this);
        }
    };
};

class MultiplyIntegerString : public Nullary<Function, MultiplyIntegerString>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return TypedValue(l*i->value<String>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(StringType(), IntegerType(), StringType()), this);
        }
    };
};

class MultiplyStringInteger : public Nullary<Function, MultiplyStringInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            String l = i->value<String>();
            ++i;
            return TypedValue(l*i->value<int>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(StringType(), StringType(), IntegerType()), this);
        }
    };
};

#endif // INCLUDED_STRING_FUNCTIONS_H
