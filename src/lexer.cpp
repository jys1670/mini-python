#include "lexer.h"

#include <algorithm>
#include <limits>

using namespace std;

namespace parse
{

constexpr auto max_size = std::numeric_limits<std::streamsize>::max();

bool operator==(const Token &lhs, const Token &rhs)
{
    using namespace token_type;

    if (lhs.index() != rhs.index())
    {
        return false;
    }
    if (lhs.Is<Char>())
    {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>())
    {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>())
    {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>())
    {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token &lhs, const Token &rhs)
{
    return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &os, const Token &rhs)
{
    using namespace token_type;

#define VALUED_OUTPUT(type)                                                                                            \
    if (auto p = rhs.TryAs<type>())                                                                                    \
        return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type)                                                                                          \
    if (rhs.Is<type>())                                                                                                \
        return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream &input) : input_(input)
{
    SkipUselessSymbols();
    token_ = token_type::Newline{};
    token_ = NextToken();
}

//! Returns current token
const Token &Lexer::CurrentToken() const
{
    return token_;
}

//! Generates token after reading from input stream, this is core function of lexer
Token Lexer::NextToken()
{

    if (token_ == token_type::Eof{})
    {
        return token_type::Eof{};
    }

    else if (token_ == token_type::Newline{})
    {
        SkipUselessSymbols();
    }

    if (indent_diff_)
    {
        return token_ = GetIndentDedent();
    }

    // Here comes block with the same indentation, spaces should be ignored
    for (; input_.peek() == ' '; input_.ignore())
    {
    }

    int ch = input_.get();
    if (ch == EOF)
    {
        if (token_ == token_type::Indent{} || token_ == token_type::Dedent{} || token_ == token_type::Newline{})
            token_ = token_type::Eof{};
        else
            token_ = token_type::Newline{};
        return token_;
    }

    else if (ch == '\n')
    {
        return token_ = token_type::Newline{};
    }

    else if (isdigit(ch))
    {
        input_.putback(static_cast<char>(ch));
        return token_ = GetNumber();
    }

    else if (ch == '\'' || ch == '\"')
    {
        input_.putback(static_cast<char>(ch));
        return token_ = GetStrLiteral();
    }

    else if (ch == '#')
    {
        input_.ignore(max_size, '\n');
        SkipUselessSymbols();
        switch (input_.peek())
        {
        case EOF:
            return token_ = token_type::Eof{};
        case '#':
            return token_ = NextToken();
        default:
            return token_ = token_type::Newline{};
        }
    }

    else if (ch == '=' && input_.peek() != '=')
    {
        return token_ = token_type::Char{'='};
    }

    else if (comparison_symbols.count(static_cast<char>(ch)) && input_.peek() == '=')
    {
        input_.putback(static_cast<char>(ch));
        return token_ = GetCompOperator();
    }

    else if (isalpha(ch) || ch == '_')
    {
        input_.putback(static_cast<char>(ch));
        return token_ = GetName();
    }

    else if (special_symbols.count(static_cast<char>(ch)))
    {
        return token_ = token_type::Char{static_cast<char>(ch)};
    }

    else
    {
        throw LexerError("Error occurred in token generation process");
    }
}

//! Extracts indentation changes
Token Lexer::GetIndentDedent()
{
    if (indent_diff_ > 0)
    {
        --indent_diff_;
        return token_type::Indent{};
    }
    ++indent_diff_;
    return token_type::Dedent{};
}

//! Extracts numeric constant
Token Lexer::GetNumber()
{
    int ch = input_.get();
    string number{};
    for (; isdigit(ch); number += ch, ch = input_.get())
    {
    }
    input_.putback(ch);
    return token_type::Number{stoi(number)};
}

//! Extracts string literal
Token Lexer::GetStrLiteral()
{
    const int open_ch = input_.get();
    string text{};

    while (true)
    {
        int ch = input_.get();

        if (ch != open_ch && ch != '\\')
        {
            text += ch;
        }

        else if (ch == '\\')
        {
            switch (ch = input_.get())
            {
            case 'n':
                text += '\n';
                break;
            case 't':
                text += '\t';
                break;
            default:
                text += ch;
            }
        }

        else
        {
            break;
        }
    }
    return token_type::String{text};
}

//! Extracts user-defined name or one of predefined keywords
Token Lexer::GetName()
{
    int ch = input_.get();
    string word{};
    for (; ch == '_' || isdigit(ch) || isalpha(ch); word += ch, ch = input_.get())
    {
    }
    input_.putback(ch);

    if (token_type::keyword_to_token.count(word))
    {
        return token_type::keyword_to_token.at(word);
    }
    return token_type::Id{word};
}

//! Extracts comparison operators consisting from two symbols
Token Lexer::GetCompOperator()
{
    string op(2, ' ');
    input_.get(&op[0], 3);

    if (op == "<=")
        return token_type::LessOrEq{};

    else if (op == ">=")
        return token_type::GreaterOrEq{};

    else if (op == "==")
        return token_type::Eq{};

    else if (op == "!=")
        return token_type::NotEq{};

    throw LexerError("Expected two chars long comparison operator"s);
}

//! Skips comments, spaces, newlines etc
void Lexer::SkipUselessSymbols()
{
    int spaces{0};
    for (; input_.peek() == ' '; input_.ignore(), ++spaces)
    {
    }

    switch (input_.peek())
    {
    case '#': {
        input_.ignore(max_size, '\n');
        SkipUselessSymbols();
        return;
    }
    case '\n': {
        input_.ignore();
        SkipUselessSymbols();
        return;
    }
    }

    indent_diff_ = spaces / 2 - indent_;
    indent_ = spaces / 2;
}

} // namespace parse
