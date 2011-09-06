#ifndef INCLUDED_CONFIG_FILE_H
#define INCLUDED_CONFIG_FILE_H

#include "unity/string.h"
#include "unity/hash_table.h"
#include "unity/space.h"
#include "unity/any.h"

class SpanCache : public SymbolCache
{
public:
    SpanCache(Span span) : _span(span) { }
    Span span() const { return _span; }
private:
    Span _span;
};

Span spanOf(Symbol symbol) { return symbol.cache<SpanCache>()->span(); }

SpanCache* newSpan(Span span) { return new SpanCache(span); }

SpanCache* newSpan(Location start, Location end)
{
    return newSpan(Span(start, end));
}

SpanCache* newSpan(Symbol symbol) { return newSpan(spanOf(symbol)); }

class Type
{
public:
    String name() const { return _name; }
    bool isEnumeration() const { return _implementation->isEnumeration(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual bool isEnumeration() const { return false; }
    };
    Type(String name) : _name(name) { }
    void setImplementation(Implementation* implementation)
    {
        _implementation = implementation;
    }
protected:
    String _name;
    Reference<Implementation> _implementation;
};

class EnumeratedValue
{
public:
    template<class T> EnumeratedValue(String name, const T& value)
        : _name(name), _value(value)
    { }
    String name() const { return _name; }
    template<class T> T value() const { return _value.value<T>(); }
private:
    String _name;
    Any _value;
};

class EnumerationType : public Type
{
public:
    EnumerationType(Type type) : Type(type) { }
    EnumerationType(String name, List<EnumeratedValue> values)
      : Type(name)
    {
        setImplementation(new Implementation(values));
    }
    Array<EnumeratedValue>* values()
    {
        return Reference<Implementation>(_implementation)->values();
    }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(List<EnumeratedValue> values) : _values(values) { }
        bool isEnumeration() const { return true; }
        Array<EnumeratedValue>* values() { return &_values; }
    private:
        Array<EnumeratedValue> _values;
    };
};

class StructureMember
{
public:
    StructureMember(String name, Type type) : _name(name), _type(type) { }
private:
    String _name;
    Type _type;
};

class StructuredType : public Type
{
public:
    StructuredType(String name, List<StructureMember> members)
      : Type(name)
    {
        setImplementation(new Implementation(members));
    }
private:
    class Implementation : public Type::Implementation
    {
    public:
        Implementation(List<StructureMember> members) : _members(members) { }
    private:
        Array<StructureMember> _members;
    };
};

class IntegerType : public Type
{
public:
    IntegerType() : Type("Integer") { setImplementation(implementation()); }
    class Implementation : public Type::Implementation
    {
    };
private:
    static Reference<Implementation> _implementation;
    static Reference<Implementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new Implementation();
        return _implementation;
    }
};

class StringType : public Type
{
public:
    StringType() : Type("String") { setImplementation(implementation()); }
    class Implementation : public Type::Implementation
    {
    };
private:
    static Reference<Implementation> _implementation;
    static Reference<Implementation> implementation()
    {
        if (!_implementation.valid())
            _implementation = new Implementation();
        return _implementation;
    }
};

class EnumeratedValueRecord
{
public:
    EnumeratedValueRecord(EnumeratedValue value, Type type)
      : _value(value), _type(type) { }
private:
    EnumeratedValue _value;
    Type _type;
};


class ConfigFile
{
public:
    void addType(Type type)
    {
        String name = type.name();
        _types.add(name, type);
        if (type.isEnumeration()) {
            Array<EnumeratedValue>* values =
                EnumerationType(type).values();
            for (int i = 0; i < values->count(); ++i) {
                EnumeratedValue value = (*values)[i];
                _enumeratedValues.add(value.name(),
                    EnumeratedValueRecord(value, type));
            }
        }
    }
    void addOption(String name, Type type)
    {
        _options.add(name, TypedValue(type));
    }
    template<class T> void addOption(String name, Type type, T defaultValue)
    {
        _options.add(name, TypedValue(type, Any(defaultValue)));
    }
    
    void load(File file)
    {
        String contents = file.contents();
        CharacterSource source(contents, file.path());
        Space::parse(&source);
        do {
            CharacterSource s = source;
            if (s.get() == -1)
                break;
            parseAssignment(&source);
        } while (true);
        for (HashTable<String, Symbol>::Iterator i = _options.begin();
            i != _options.end(); ++i) {
            if (!i.value()[2].valid())
                throw Exception(file.messagePath() + colonSpace + i.key() +
                    String(" not defined and no default is available."));
        }
    }
    template<class T> T getValue(String name)
    {
        return _options[name].value().value<T>();
    }
private:
    class TypedValue
    {
    public:
        TypedValue(Type type, Any defaultValue = Any(), Span span = Span())
          : _type(type), _value(defaultValue), _span(span) { }
        Type type() const { return _type; }
        Any value() const { return _value; }
        void setValue(Any value) { _value = value; }
    private:
        Type _type;
        Any _value;
        Span _span;
    };
    class Identifier
    {
    public:
        Identifier() { }
        Identifier(String name, Span span) : _name(name), _span(span) { }
        String name() const { return _name; }
        Span span() const { return _span; }
        bool valid() const { return !_name.empty(); }
    private:
        String _name;
        Span _span;
    };

    Identifier parseIdentifier(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location startLocation = s.location();
        int startOffset = s.offset();
        Span startSpan;
        Span endSpan;
        int c = s.get(&startSpan);
        if (c < 'a' || c > 'z')
            return Identifier();
        CharacterSource s2;
        do {
            s2 = s;
            c = s.get(&endSpan);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_')
                continue;
            break;
        } while (true);
        int endOffset = s2.offset();
        Location endLocation = s2.location();
        Space::parse(&s2);
        String name = s2.subString(startOffset, endOffset);
        *source = s2;
        return Identifier(name, startSpan + endSpan);
    }
    TypedValue valueFromIdentifier(Identifier identifier)
    {
        String name = identifier.name();
        if (!_options.hasKey(name))
            identifier.span().throwError(String("Unknown identifier ") + name);
        TypedValue option = _options[name];
        return TypedValue(option.type(), option.value(), identifier.span()); 
    }
    // TODO: refactor with parseIdentifier
    Identifier parseTypeIdentifier(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location startLocation = s.location();
        int startOffset = s.offset();
        Span startSpan;
        Span endSpan;
        int c = s.get(&startSpan);
        if (c < 'A' || c > 'Z')
            return Identifier();
        CharacterSource s2;
        do {
            s2 = s;
            c = s.get(&endSpan);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_')
                continue;
            break;
        } while (true);
        int endOffset = s2.offset();
        Location endLocation = s2.location();
        Space::parse(&s2);
        String name = s2.subString(startOffset, endOffset);
        *source = s2;
        return Identifier(name, startSpan + endSpan);
    }
    void throwError(CharacterSource* source)
    {
        static String expected("Expected expression");
        source->location().throwError(expected);
    }
    
    Symbol combine(Symbol left, Symbol right)
    {
        if (left.valid())
            return Symbol(atomValue, left[1].string() + right[1].string(),
                Symbol(atomString), newSpan(spanOf(left) + spanOf(right)));
        return right;
    }
    
    int parseHexadecimalCharacter(CharacterSource* source, Span* span)
    {
        CharacterSource s = *source;
        int c = s.get(span);
        if (c >= '0' && c <= '9') {
            *source = s;
            return c - '0';
        }
        if (c >= 'A' && c <= 'F') {
            *source = s;
            return c + 10 - 'A';
        }
        if (c >= 'a' && c <= 'f') {
            *source = s;
            return c + 10 - 'a';
        }
        return -1;
    }
    
    Symbol parseDoubleQuotedString(CharacterSource* source)
    {
        static String endOfFile("End of file in string");
        static String endOfLine("End of line in string");
        static String printableCharacter("printable character");
        static String escapedCharacter("escaped character");
        static String hexadecimalDigit("hexadecimal digit");
        static String toString("toString");
        Span span;
        Span startSpan;
        if (!source->parse('"', &startSpan))
            return Symbol();
        Span stringStartSpan = startSpan;
        Span stringEndSpan = startSpan;
        int startOffset = source->offset();
        int endOffset;
        String insert(empty);
        int n;
        int nn;
        String string(empty);
        Symbol expression;
        Symbol part;
        do {
            CharacterSource s = *source;
            endOffset = s.offset();
            int c = s.get(&span);
            if (c < 0x20 && c != 10) {
                if (c == -1)
                    source->location().throwError(endOfFile);
                source->throwUnexpected(printableCharacter,
                    String::hexadecimal(c, 2));
            }
            *source = s;
            switch (c) {
                case '"':
                    string += s.subString(startOffset, endOffset);
                    Space::parse(source);
                    return combine(expression, Symbol(atomValue, string,
                        Symbol(atomString), newSpan(stringStartSpan + span)));
                case '\\':
                    string += s.subString(startOffset, endOffset);
                    c = s.get(&stringEndSpan);
                    if (c < 0x20) {
                        if (c == 10)
                            source->location().throwError(endOfLine);
                        if (c == -1)
                            source->location().throwError(endOfFile);
                        source->throwUnexpected(escapedCharacter,
                            String::hexadecimal(c, 2));
                    }
                    *source = s;
                    switch (c) {
                        case 'n':
                            insert = newLine;
                            break;
                        case 't':
                            insert = tab;
                            break;
                        case '$':
                            insert = dollar;
                            break;
                        case '"':
                            insert = doubleQuote;
                            break;
                        case '\'':
                            insert = singleQuote;
                            break;
                        case '`':
                            insert = backQuote;
                            break;
                        case 'U':
                            source->assert('+', &stringEndSpan);
                            n = 0;
                            for (int i = 0; i < 4; ++i) {
                                nn = parseHexadecimalCharacter(source,
                                    &stringEndSpan);
                                if (nn == -1) {
                                    s = *source;
                                    source->throwUnexpected(hexadecimalDigit,
                                        String::codePoint(s.get()));
                                }
                                n = (n << 4) | nn;
                            }
                            nn = parseHexadecimalCharacter(source,
                                &stringEndSpan);
                            if (nn != -1) {
                                n = (n << 4) | nn;
                                nn = parseHexadecimalCharacter(source,
                                    &stringEndSpan);
                                if (nn != -1)
                                    n = (n << 4) | nn;
                            }
                            insert = String::codePoint(n);
                            break;
                        default:
                            source->throwUnexpected(escapedCharacter,
                                String::codePoint(c));
                    }
                    string += insert;
                    startOffset = source->offset();
                    break;
                case '$':
                    part = parseIdentifier(source);
                    if (!part.valid()) {
                        if (Space::parseCharacter(source, '(', &span)) {
                            part = parseExpression(source);
                            source->assert(')', &span);
                        }
                        else
                            source->location().throwError(String(
                                "Expected identifier or parenthesized "
                                    "expression"));
                    }
                    if (part[2].atom() == atomInteger)
                        part = Symbol(atomValue,
                            String::decimal(part[1].integer()),
                            Symbol(atomString), newSpan(part));
                    else
                        if (part[2].atom() != atomString)
                            source->location().throwError(
                                String("Don't know how to convert type ") +
                                typeToString(part[2].symbol()) +
                                String(" to a string"));
                    string += s.subString(startOffset, endOffset);
                    startOffset = source->offset();
                    expression = combine(expression,
                        Symbol(atomValue, string, Symbol(atomString),
                        newSpan(stringStartSpan + stringEndSpan)));
                    string = empty;
                    expression = combine(expression, part);
                    break;
                default:
                    stringEndSpan = span;
            }
        } while (true);
    }
    
    Symbol parseInteger(CharacterSource* source)
    {
        CharacterSource s = *source;
        int n = 0;
        Span span;
        int c = s.get(&span);
        if (c < '0' || c > '9')
            return Symbol();
        do {
            n = n*10 + c - '0';
            *source = s;
            Span span2;
            c = s.get(&span2);
            if (c < '0' || c > '9') {
                Space::parse(source);
                return Symbol(atomValue, n, Symbol(atomInteger),
                    newSpan(span));
            }
            span += span2;
        } while (true);
    }
    
    Symbol parseExpressionElement(CharacterSource* source)
    {
        Location location = source->location();
        Symbol e = parseDoubleQuotedString(source);
        if (e.valid())
            return e;
        e = parseInteger(source);
        if (e.valid())
            return e;
        e = parseIdentifier(source);
        if (e.valid()) {
            String s = e[1].string();
            static String trueKeyword("true");
            if (s == trueKeyword)
                return Symbol(atomValue, Symbol(atomTrue), Symbol(atomBoolean),
                    newSpan(e));
            static String falseKeyword("false");
            if (s == falseKeyword)
                return Symbol(atomValue, Symbol(atomFalse),
                    Symbol(atomBoolean), newSpan(e));
            if (_enumeratedValues.hasKey(s)) {
                Symbol valueRecord = _enumeratedValues[s];
                return Symbol(atomValue, valueRecord[1].symbol(),
                    valueRecord[2].symbol(), newSpan(e));
            }
            return valueFromIdentifier(e);
        }
        e = parseTypeIdentifier(source);
        if (e.valid()) {
            String s = e[1].string();
            if (!_types.hasKey(s))
                spanOf(e).throwError(String("Unknown type ") + s);
            Symbol type = _types[s];
            if (type.atom() != atomStructure)
                spanOf(e).throwError(
                    String("Only structure types can be constructed"));
            SymbolArray elements = type[2].array();
            SymbolList values;
            Span span;
            Space::assertCharacter(source, '(', &span);
            for (int i = 0; i < elements.count(); ++i) {
                Symbol component = elements[i];
                if (i > 0)
                    Space::assertCharacter(source, ',', &span);
                Symbol value = parseExpression(source);
                Symbol expectedType = component[1].symbol();
                Symbol observedType = value[2].symbol();
                if (observedType != expectedType)
                    spanOf(value).throwError(String("Type mismatch: ") + s + 
                        dot + component[2].string() + String(" has type ") + 
                        typeToString(expectedType) + 
                        String(" but value has type ") + 
                        typeToString(observedType));
                values.add(value);
            }
            Space::assertCharacter(source, ')', &span);
            return Symbol(atomValue, SymbolArray(values), type,
                newSpan(spanOf(e) + span));
        }
        Span span;
        if (Space::parseCharacter(source, '{', &span)) {
            SymbolList values;
            Span span2;
            SymbolList types;
            do {
                Symbol e = parseExpression(source);
                values.add(e);
                types.add(e[2].symbol());
            } while (Space::parseCharacter(source, ',', &span2));
            Space::assertCharacter(source, '}', &span2);
            return Symbol(atomValue, SymbolArray(values),
                Symbol(atomTuple, SymbolArray(types)), newSpan(span + span2));
        }
        if (Space::parseCharacter(source, '(', &span)) {
            e = parseExpression(source);
            Space::assertCharacter(source, ')', &span);
            return e;
        }
        return Symbol();
    }
    
    Symbol parseUnaryExpression(CharacterSource* source)
    {
        Span span;
        if (Space::parseCharacter(source, '-', &span)) {
            Symbol e = parseUnaryExpression(source);
            if (e.atom() != atomInteger)
                throw Exception(String("Only numbers can be negated"));
            return Symbol(atomInteger, -e[1].integer());
        }
        return parseExpressionElement(source);
    }
    
    Symbol parseMultiplicativeExpression(CharacterSource* source)
    {
        Symbol e = parseUnaryExpression(source);
        if (!e.valid())
            return Symbol();
        do {
            Span span;
            if (Space::parseCharacter(source, '*', &span)) {
                Symbol e2 = parseUnaryExpression(source);
                if (!e2.valid())
                    throwError(source);
                bool okay = false;
                if (e[2].atom() == atomInteger) {
                    if (e2[2].atom() == atomInteger) {
                        e = Symbol(atomValue, e[1].integer() * e2[1].integer(),
                            Symbol(atomInteger),
                            newSpan(spanOf(e) + spanOf(e2)));
                        okay = true;
                    }
                    else
                        if (e2[2].atom() == atomString) {
                            e = Symbol(atomValue,
                                e2[1].string() * e[1].integer(),
                                Symbol(atomString),
                                newSpan(spanOf(e) + spanOf(e2)));
                            okay = true;
                        }
                }
                else
                    if (e[2].atom() == atomString) {
                        if (e2[2].atom() == atomInteger) {
                            e = Symbol(atomValue,
                                e[1].string() * e2[1].integer(),
                                Symbol(atomString),
                                newSpan(spanOf(e) + spanOf(e2)));
                            okay = true;
                        }
                    }
                if (!okay) 
                    span.throwError(
                        String("Don't know how to multiply type ") +
                        typeToString(e[2].symbol()) +
                        String(" and type ") +
                        typeToString(e2[2].symbol()) +
                        String::codePoint('.'));
                continue;
            }
            if (Space::parseCharacter(source, '/', &span)) {
                Symbol e2 = parseUnaryExpression(source);
                if (!e2.valid())
                    throwError(source);
                if (e[2].atom() == atomInteger && e2[2].atom() == atomInteger)
                    e = Symbol(atomValue, e[1].integer() / e2[1].integer(),
                        Symbol(atomInteger), newSpan(spanOf(e) + spanOf(e2)));
                else
                    span.throwError(
                        String("Don't know how to divide type ") +
                        typeToString(e[2].symbol()) +
                        String(" by type ") +
                        typeToString(e2[2].symbol()) +
                        String::codePoint('.'));
                continue;
            }
            return e;
        } while (true);
    }
    
    Symbol parseExpression(CharacterSource* source)
    {
        Symbol e = parseMultiplicativeExpression(source);
        if (!e.valid())
            throwError(source);
        do {
            Span span;
            if (Space::parseCharacter(source, '+', &span)) {
                Symbol e2 = parseMultiplicativeExpression(source);
                if (!e2.valid())
                    throwError(source);
                if (e[2].symbol().atom() == atomInteger &&
                    e2[2].symbol().atom() == atomInteger)
                    e = Symbol(atomValue, e[1].integer() + e2[1].integer(),
                        Symbol(atomInteger), newSpan(spanOf(e) + spanOf(e2)));
                else
                    if (e[2].atom() == atomString &&
                        e2[2].atom() == atomString)
                        e = Symbol(atomValue, e[1].string() + e2[1].string(),
                            Symbol(atomString),
                            newSpan(spanOf(e) + spanOf(e2)));
                    else
                        span.throwError(String("Don't know how to add type ") +
                            typeToString(e2[2].symbol()) +
                            String(" to type ") + typeToString(e[2].symbol()) +
                            String::codePoint('.'));
                continue;
            }
            if (Space::parseCharacter(source, '-', &span)) {
                Symbol e2 = parseMultiplicativeExpression(source);
                if (!e2.valid())
                    throwError(source);
                if (e[2].atom() == atomInteger && e2[2].atom() == atomInteger)
                    e = Symbol(atomValue,
                        e[1].integer() - e2[1].integer(), Symbol(atomInteger),
                        newSpan(spanOf(e) + spanOf(e2)));
                else
                    span.throwError(
                        String("Don't know how to subtract type ") +
                        typeToString(e2[2].symbol()) + String(" from type ") +
                        typeToString(e[2].symbol()) + String::codePoint('.'));
                continue;
            }
            return e;
        } while (true);
    }
    String typeToString(Symbol type)
    {
        return atomToString(type.atom());
    }
    void parseAssignment(CharacterSource* source)
    {
        Identifier identifier = parseIdentifier(source);
        Span span;
        String name = identifier.name();
        if (!_options.hasKey(name))
            identifier.span().throwError(String("Unknown identifier ") + name);
        Space::assertCharacter(source, '=', &span);
        TypedValue e = parseExpression(source);
        Type expectedType = _options[name].type();
        Type observedType = e.type();
        if (observedType != expectedType)
            e.span().throwError(String("Expected an expression of type ") +
                expectedType.toString() + String(" but found one of type ") +
                observedType.toString());
        Space::assertCharacter(source, ';', &span);
        _options[name].setValue(e.value());
    }

    HashTable<String, TypedValue> _options;
    HashTable<String, EnumeratedValueRecord> _enumeratedValues;
    HashTable<String, Type> _types;
};

#endif // INCLUDED_CONFIG_FILE_H
