# Document
C++ and Document Compiler

## Goal
This is a command line tool for generating indexed code in HTML and documents from comments with Markdown for C++

## "The compiler"
All **the compiler** here means the compiler created by this project.

## Requirements
- The compiler accepts a preprocessed cpp file from cl.exe.
  - The cpp file for preprocessed should compile using the same options which is also used to create the preprocessed file
- Comments containing documents should begin with `///`
- Comments should contain Markdown in XML tags
  - Details will be filled later

## Limitations
- **All limitations could be changed in the future**
- The compiler doesn't parse all C++17 features, but it parses more enough for most of the C++ programs.
- All features that cl.exe doesn't support, are also not supported here.
- The compiler doesn't do `const` or `constexpr` calculation, the compiler treats all values identical in types.
  - This usually means the compiler assumes `int[1]` and `int[2]` are the same type, which will cause the compiler fail to compile the input file in some very limited cases.
  - This also applies to `T<1>` and `T<2>`.
- The compiler doesn't check bodies of template declarations when creating their instances.
  - For example, the compiler resolves the body of a template function right after it is parsed. All recognizable names will be identified at this moment. The body will not be processed again later when this function is instanciating in somewhere else.
  - The reason for this is, **GacUI** and its dependeices offers a lot of template declarations. They are supposed to be used by users. So there is no reason not to process template function bodies if they are not used inside the library. There is also no reason to offer more detailed indexing just for all places using these template functions inside the library.
- The compiler will report errors only if they stopped further processing.
  - For example, when the compiler sees `int Foo::Bar(int)`, it will tries to find `int Bar(int)` in class `Foo`. If the compiler failed to do so, it stops here.
  - If you have `int Foo::Bar(int(&)[1])` and `int Foo::Bar(int(&)[2])`, although this code is valid, but due to the limitation mentioned above, the compiler stops here.
  - This also applies when the compiler finds two function definitions match the same forward declaration in a similar way.
  - For most of the cases, like `struct Foo { static inline virtual int Bar() = 0; };`, the compiler doesn't care.

## If you want to use it but you have trouble processing your own source code ...
- I will announce when I think this project is ready to release. Now it is not completed so you are expected to have trouble.
- This project is not open sourced. I may change the license once this project is ready to release.
- If you believe the compiler should be able to handle a valid case but it doesn't, please open an issue with a minimum reproduction, including your cpp file, with all options for cl.exe to compile and create the preprocessed file.
