## Minimalistic Python

![Alt Text](https://github.com/jys1670/mini-python/blob/master/docs/example/example.gif?raw=true)

This is educational mini-Python interpreter build under the guidance of yandex practicum.
The project consists of several independent parts:

1) *Lexical analyzer* — reads symbols stream and groups them into meaningful sequences called lexemes.
Each lexeme is then used to generate so-called token.

2) *Syntax analyzer* — reads token stream and generates abstract syntax tree (AST). 
Each inner node represents some kind of operation, where child nodes are its arguments.

3) *Runtime module* — traverses AST, completes node actions according to their semantics, updates symbols table, etc.


[Documentation](https://jys1670.github.io/mini-python/html/index.html) might make things a bit more clear

### Building:

Requirements:
* GCC or alternative (supporting C++17 or later)
* Cmake (tested with 3.24)
* [optional] Doxygen (tested with 1.9.5)

Preparations:
```sh
git clone https://github.com/jys1670/mini-python
cd mini-python
mkdir build && cd build
```

Building and running main executable:
```sh
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release --target mini-python
./mini-python # reads from stdin
```

Updating documentation:
```sh
cmake --build . --config Release --target doxygen
```

