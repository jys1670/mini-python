#pragma once

#include <cassert>
#include <iosfwd>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace parse
{

namespace token_type
{

//! Numeric constant
struct Number
{
    int value;
};

//! User-defined names (variables, classes, etc)
struct Id
{
    std::string value;
};

//! ASCII symbol
struct Char
{
    char value;
};

//! String literal
struct String
{
    std::string value;
};

//! %Class operator
struct Class
{
};

//! %Return keyword
struct Return
{
};

//! %If keyword
struct If
{
};

//! %Else keyword
struct Else
{
};

//! %Def keyword
struct Def
{
};

//! %Newline symbol
struct Newline
{
};

//! %Print function
struct Print
{
};

//! Indentation increased by two spaces
struct Indent
{
};

//! Indentation decreased by two spaces
struct Dedent
{
};

//! End of file
struct Eof
{
};

//! %And keyword
struct And
{
};

//! %Or keyword
struct Or
{
};

//! %Not keyword
struct Not
{
};

//! Sequence of chars ==
struct Eq
{
};

//! Sequence of chars !=
struct NotEq
{
};

//! Sequence of chars <=
struct LessOrEq
{
};

//! Sequence of chars >=
struct GreaterOrEq
{
};

//! %None keyword
struct None
{
};

//! %True keyword
struct True
{
};

//! %False keyword
struct False
{
};
} // namespace token_type

using TokenBase =
    std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String, token_type::Class,
                 token_type::Return, token_type::If, token_type::Else, token_type::Def, token_type::Newline,
                 token_type::Print, token_type::Indent, token_type::Dedent, token_type::And, token_type::Or,
                 token_type::Not, token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
                 token_type::None, token_type::True, token_type::False, token_type::Eof>;

struct Token : TokenBase
{
    using TokenBase::TokenBase;

    template <typename T> [[nodiscard]] bool Is() const
    {
        return std::holds_alternative<T>(*this);
    }

    template <typename T> [[nodiscard]] const T &As() const
    {
        return std::get<T>(*this);
    }

    template <typename T> [[nodiscard]] const T *TryAs() const
    {
        return std::get_if<T>(this);
    }
};

namespace token_type
{
//! Predefined language keywords
static const std::unordered_map<std::string, Token> keyword_to_token = {
    {"class", Class{}}, {"return", Return{}}, {"if", If{}},   {"else", Else{}}, {"def", Def{}},   {"print", Print{}},
    {"and", And{}},     {"or", Or{}},         {"not", Not{}}, {"None", None{}}, {"True", True{}}, {"False", False{}}};
} // namespace token_type

static const std::unordered_set<char> comparison_symbols = {'=', '!', '<', '>'};
static const std::unordered_set<char> special_symbols = {'.', ',', ':', '+', '-', '*', '/', '(', ')', '=', '<', '>'};

bool operator==(const Token &lhs, const Token &rhs);
bool operator!=(const Token &lhs, const Token &rhs);

std::ostream &operator<<(std::ostream &os, const Token &rhs);

class LexerError : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

class Lexer
{
  public:
    explicit Lexer(std::istream &input);

    //! Returns current token or token_type::Eof if token stream is over
    [[nodiscard]] const Token &CurrentToken() const;

    //! Returns next token or token_type::Eof if token stream is over
    Token NextToken();

    //! Returns current token as type T if it's really that type; otherwise throws LexerError
    template <typename T> const T &Expect() const
    {
        using namespace std::literals;
        if (token_.Is<T>())
        {
            return token_.As<T>();
        }
        throw LexerError("Incorrect token"s);
    }

    //! Checks whether current token has type T and value equal to what is given as argument,
    //! otherwise throws LexerError
    template <typename T, typename U> void Expect(const U &value) const
    {
        using namespace std::literals;
        Expect<T>();
        if (token_.As<T>().value == value)
        {
            return;
        }
        throw LexerError("Incorrect value"s);
    }

    //! Returns next token as type T if it's really that type; otherwise throws LexerError
    template <typename T> const T &ExpectNext()
    {
        NextToken();
        return Expect<T>();
    }

    //! Checks whether next token has type T and value equal to what is given as argument,
    //! otherwise throws LexerError
    template <typename T, typename U> void ExpectNext(const U &value)
    {
        using namespace std::literals;
        ExpectNext<T>();
        if (token_.As<T>().value == value)
        {
            return;
        }
        throw LexerError("Incorrect value"s);
    }

  private:
    int indent_{0};
    int indent_diff_{0};
    Token token_;
    std::istream &input_;

    Token GetName();
    Token GetCompOperator();
    Token GetIndentDedent();
    Token GetNumber();
    Token GetStrLiteral();

    void SkipUselessSymbols();
};

} // namespace parse
