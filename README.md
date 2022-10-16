## Minimalistic Python

![C++ version](https://img.shields.io/badge/C%2B%2B-17-blue)

An interpreter consists from several parts:

1) Lexical analyzer — reads symbols stream and groups them into meaningful sequences called lexemes.
Each lexeme is then used to generate so-called token.

2) Syntax analyzer — reads token stream and generates abstract syntax tree (AST). 
Each inner node represents some kind of operation, where child nodes are its arguments.

3) Semantic analyzer — traverses AST, completes node actions, updates symbols table.

