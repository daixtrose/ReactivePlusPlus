# ReactivePlusPlus
[![Unit tests](https://github.com/victimsnino/ReactivePlusPlus/actions/workflows/Tests.yml/badge.svg?branch=main)](https://github.com/victimsnino/ReactivePlusPlus/actions/workflows/Tests.yml) 
[![codecov](https://codecov.io/gh/victimsnino/ReactivePlusPlus/branch/main/graph/badge.svg?token=INEHPRF18E)](https://codecov.io/gh/victimsnino/ReactivePlusPlus)

ReactivePlusPlus is [ReactiveX](https://reactivex.io/) library for C++ language inspired by "official implementation" ([RxCpp](https://github.com/ReactiveX/RxCpp)) 

## What about existing Reactive Extension libraries for C++?

Reactive programming is excelent programming paradigm and approach for creation of multi-threading and real-time programs which reacts on some events. Unfortunately, there is only one stable and fully-implemented library at the moment of creation of ReactivePlusPlus - [RxCpp](https://github.com/ReactiveX/RxCpp). 

[RxCpp](https://github.com/ReactiveX/RxCpp) is great and awesome library and perfect implementation of ReactiveX approach. However RxCpp has some disadvantages:
- It is a bit **"old" library written in C++11** with some parts written in the **pre-C++11 style** (mess of old-style classes and wrappers)
- **Issue** with **template parameters**:  `rxcpp::observable` contains **full chain of operators** as second template parameter... where each operator has a bunch of another template parameters itself. It forces **IDEs** works **slower** while parsing resulting type of observable. Also it forces to generate **heavier binaries and debug symbols and slower build time**.

Another implementation of RX for c++: [another-rxcpp](https://github.com/CODIANZ/another-rxcpp). It partly solves issues of RxCpp via **eliminating of template parameter**  with help of **type-erasing** and making each callback as `std::function`. As a result issue with templates resvoled, but this approach has disadvantages related to runtime: resulting size of observers/observables becomes greater due to heavy `std::function` object, usage of heap for storing everything causes perfomance issues, implementation is just pretty simple and provides a lot of copies of passed objects.

## Why ReactivePlusPlus?

ReactivePlusPlus tries to solve all mentioned issues:
- ReactivePlusPlus written in **Modern C++ (C++20)** with concepts which makes code-base a lot of understandable and clean.
- ReactivePlusPlus keeps balance between perfomance and type-erasing mechanism: user can easily understand and choose where creation of object is expensive (via type-erasure mechanism) and how to avoid it if needed. Read about this in [Perfomance vs Flexibility, Specific vs Dynamic]()
- ReactivePlusPlus is fast: every part of code written with perfomance in mind. Starting from tests over amoun of copies/move and finishing to Continous Benchmarking


## Documentation

Doxygen documentation generated per each commit can be found [here](https://victimsnino.github.io/ReactivePlusPlus/docs/html/index.html)

## Perfomance
Perfomance is really **important**! It is **doubly important** when we speak about **realtime applications and libraries**! **ReactivePlusPlus** targets as a realtime library to process and handle a tremendous volumes of data. 

This repository uses continous benchmarking: every commit and pull request measured and diff per each benchmark provided. Graphs over benchmark results can be found [here](https://victimsnino.github.io/ReactivePlusPlus/benchmark)