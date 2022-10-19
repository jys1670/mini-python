#include "lexer.h"
#include "parse.h"
#include "runtime.h"
#include "statement.h"
#include <iostream>

int main()
{
    parse::Lexer lexer(std::cin);
    runtime::SimpleContext context{std::cout};
    runtime::Closure closure;
    auto program = ParseProgram(lexer);
    auto obj_holder = program->Execute(closure, context);
    if (obj_holder)
    {
        std::cout << std::endl;
        obj_holder->Print(std::cout, context);
    }
}