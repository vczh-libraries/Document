# C++ and Document Compiler

The original project is in the [release-1.0](https://github.com/vczh-libraries/Document/tree/release-1.0) branch,
which is a manually written C++ frontend, resolving symbols, indexing code in HTML, and generating documents in XML.

The new project will stop doing precise resolving, it will focus on document only.
The parser will be implemented using [VlppParser2](https://github.com/vczh-libraries/VlppParser2).
It allows ambiguity without the need of resolving symbols.
