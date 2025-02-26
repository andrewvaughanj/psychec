![](https://github.com/ltcmelo/psychec/workflows/build/badge.svg)
![](https://github.com/ltcmelo/psychec/workflows/test-suite/badge.svg)

![](https://docs.google.com/drawings/d/e/2PACX-1vT-pCvcuO4U63ERkXWfBzOfVKwMQ_kh-ntzANYyNrnkt8FUV2wRHd5fN6snq33u5hWmnNQR3E3glsnH/pub?w=375&h=150)

# Psyche-C

Psyche is a compiler frontend for the C programming language. Psyche-C is specifically designed for the implementation of static analysis tools. These are the distinct features that make Psyche-C rather unique:

- Clean separation between the syntactic and semantic compiler phases.
- Both algorithmic- and heuristic-based syntax disambiguation strategies.
- Independent of `#include`, with type inference for missing `struct`, `union`, `enum`, and `typedef`.
- API inspired by that of the [Roslyn .NET compiler](https://github.com/dotnet/roslyn).
- A parser's AST resembling that of the [LLVM's Clang frontend](https://clang.llvm.org/).

Applications:

- Enabling, on incomplete source-code, static analysis techniques that require fully-typed programs.
- Compiling partial code (e.g., a snippet retrieved from a bug tracker) for object-code inspection.
- Generating test-input data for a function in isolation (without its dependencies).
- Quick prototyping of an algorithm, without the need of explicit types.

**NOTE**: The master branch is going through a major overhaul; it's expected that syntax analysis (parsing and AST construction) already is functional, though. The original version of Psyche-C is available in [this branch](https://github.com/ltcmelo/psychec/tree/original).

## The *cnippet* Driver Adaptor

While Psyche-C is primarily used as a library for the implementation of static analysis tools, it still is a compiler frontend, and may also be used as an ordinary C parser through the *cnippet* driver adaptor.

```c
// node.c
void f()
{
    T v = 0;
    v->value = 42;
    v->next = v;
}
```

If you compile the above snippet with GCC or Clang, you'd see the diagnostic _"declaration for_`T`_is not available"_. But with *cnippet* the compilation would succeed: a definition for `T` is inferred.

## Documentation and Resources

- The Doxygen-generated [API](https://ltcmelo.github.io/psychec/api-docs/html/index.html).
- An [online interface](http://cuda.dcc.ufmg.br/psyche-c/) that offers a glimpse of Psyche-C's functionality.
- HOW-TO blog posts:
  - [Dumping a C program’s AST with Psyche-C](https://ltcmelo.github.io/psychec/2021/03/03/c-ast-dump-psyche.html) /
    [Visualizando a AST de um programa C com o Psyche-C](https://www.embarcados.com.br/visualizando-a-ast-psyche-c/)
- Articles:
  - [Programming in C with type inference](https://www.codeproject.com/Articles/1238603/Programming-in-C-with-Type-Inference) /
    [Programando em C com inferência de tipos usando PsycheC](https://www.embarcados.com.br/inferencia-de-tipos-em-c-usando-psychec/)

## Building and Testing

Except for type inference, which is written in Haskell, Psyche-C is written in C++17; *cnippet* is written in Python 3.

To build:

    cmake CMakeLists.txt && make -j 4

To run the tests:

    ./test-suite

## Related Publications

- [Type Inference for C: Applications to the Static Analysis of Incomplete Programs](https://dl.acm.org/doi/10.1145/3421472)<br/>
ACM Transactions on Programming Languages and Systems — **TOPLAS**, Volume 42, Issue 3, Article No. 15, Dec. 2020.

- [Inference of static semantics for incomplete C programs](https://dl.acm.org/doi/10.1145/3158117)<br/>
Proceedings of the ACM on Programming Languages, Volume 2, Issue **POPL**, Jan. 2018, Article No. 29.

- [AnghaBench: a Suite with One Million Compilable C Benchmarks for Code-Size Reduction](https://conf.researchr.org/info/cgo-2021/accepted-papers)<br/>
Proceedings of the IEEE/ACM International Symposium on Code Generation and Optimization — **CGO**, 2021.

- [Generation of in-bounds inputs for arrays in memory-unsafe languages](https://dl.acm.org/citation.cfm?id=3314890)<br/>
Proceedings of the IEEE/ACM International Symposium on Code Generation and Optimization — **CGO**, Feb. 2019, p. 136-148.

- [Automatic annotation of tasks in structured code](https://dl.acm.org/citation.cfm?id=3243200)<br/>
Proceedings of the International Conference on Parallel Architectures and Compilation Techniques — **PACT**, Nov. 2018, Article No. 31.
