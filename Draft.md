# Rewriting

- ps1 copy Source and Release/IncludeOnly
- ps1 call msbuild to create .i for all .h and .cpp
- Generate mapping before and after preprocessing, only keep contents of the current file
- Generate .html fot each file, including clickable include and row number
- Lexical analyse each file and store a .tokens.json file
- Update .html with colorizing
- Parse each file and store a .ast.json file
- Resolve symbols and store a .symbols.json file
- Update .html with context awared colorizing and clickable links
  - When a symbol resolves to multiple targets, clicking it shows a dropdown