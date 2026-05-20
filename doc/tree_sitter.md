# Tree-Sitter Parser Integration and Highlighting Engine

Alwide implements robust, real-time syntax highlighting by embedding the **Tree-Sitter** parsing library. Rather than relying on simple regular expressions or lexers, it parses the document buffer into a concrete Abstract Syntax Tree (AST), producing highly accurate semantic highlighting.

---

## 1. C Host and Rust Submodule Bridge

The core Tree-Sitter runtime is written in C (`lib/tree-sitter/lib/src/lib.c`), but Alwide obtains language-specific grammars from upstream Rust parser crates (such as `tree-sitter-c` or `tree-sitter-python`).

### Linking Rust Grammars (`.rlib` to Clang)
To compile these libraries without manual code conversions, Alwide’s compilation uses a clever bridge inside [Makefile](file:///home/arno/dev/Alwide/Makefile):

1. **Rust Library Targets**: Grammars are defined as git submodules inside `lib/`.
2. **Cargo Compilation**: The Makefile rule compiles these submodules into static Rust libraries (`.rlib` format) under release profiles:
   ```make
   lib/tree-sitter-c/target/release/libtree_sitter_c.rlib:
       cd lib/tree-sitter-c && cargo build --release
   ```
3. **Clang Linking**: Clang links the C object files and the Rust `.rlib` static archives together, generating the final `al` binary. Since Rust symbols follow C calling conventions, the host program declares and invokes these functions directly:
   ```c
   const TSLanguage* tree_sitter_c(void);
   const TSLanguage* tree_sitter_python(void);
   ```

---

## 2. Parser Lifecycle and `ParserList`

The editor registers and manages loaded language grammars inside a global registry.

```
  ParserList
     │
     ├──► ParserContainer (c) ──► TSParser ──► TSQuery (Queries & Predicates)
     │                                               └──► RegexMap (Compiled patterns)
     ├──► ParserContainer (py) 
     └──► ParserContainer (java)
```

### The `ParserContainer` Structure
Defined in [src/advanced/tree-sitter/tree_manager.h](file:///home/arno/dev/Alwide/src/advanced/tree-sitter/tree_manager.h):

```c
typedef struct {
  char lang_id[LANG_ID_LENGTH];   // Language identifier (e.g. "python")
  const TSLanguage* lang;         // Pointer to TSLanguage struct
  TSParser* parser;               // Active Tree-Sitter parser instance
  TSQuery* queries;               // S-expression queries compiled into TSQuery
  RegexMap regex_map;             // POSIX regex maps used for query predicates
  HighlightThemeList theme_list;  // Theme colors matched to tokens
} ParserContainer;

typedef struct {
  ParserContainer* list;          // Active parser instances array
  int size;                       // Array length
} ParserList;
```

### Language Detection and Parser Loading
1. **Detection**: On file loading, `ft_detectLanguage()` matches extension patterns or shebang headers to fetch the file's language configuration.
2. **Grammar Lookup**: `getTSLanguageFromString()` maps language IDs to static grammars (e.g. "c" maps to `tree_sitter_c()`).
3. **Parser Initialization**: `loadNewParser()` instantiates the parser using `ts_parser_new()` and binds it to the loaded language.
4. **Queries Compilation**: The program reads S-expression highlight files (e.g. `highlights.scm`) and compiles them via `ts_query_new()`.

---

## 3. Highlighting Engine and Predicates

Syntax coloring uses a two-phase process: **Query AST Matching** and **NCurses Paint Actions**.

### S-Expression AST Queries
Tree-sitter uses Lisp-style S-expressions to query matches in the AST. For example:
```query
(comment) @comment
(string) @string
(function_declarator declarator: (identifier) @function)
```
When Tree-Sitter runs these queries, matched nodes are labeled with tags (`comment`, `string`, `function`).

### Dynamic Predicates and `RegexMap`
Highlighting queries often contain conditional match predicates, such as matching specific patterns. To support these conditions:
* **POSIX Compilation**: Alwide parses `#match?` query conditions and compiles the target pattern into a POSIX `regex_t` object using `regcomp()`.
* **The Regex Map**: These compiled expressions are indexed inside the parser's `RegexMap` (`regex_map`). During parsing, Tree-Sitter evaluates the pattern predicates against the AST nodes, matching only if the regex query succeeds.

### Paint Token Mapping
Matched nodes are saved in the `WindowHighlightDescriptor`. During visual repaints, Alwide scans these records and maps tags to NCurses attributes:
* `@keyword` maps to purple bold text.
* `@comment` maps to grey italic text.
* `@function` maps to blue bold text.

---

## 4. Incremental Parsing and AST Sync

Re-parsing an entire document on every keystroke causes latency in large files. Alwide guarantees fast, real-time typing by updating the AST **incrementally**.

### Zero-Copy Stream Reader
Tree-sitter requires a text reader to pull text dynamically. Instead of creating a flat character buffer copy of the unrolled linked list, Alwide implements a zero-copy stream callback:

```c
const char* internalReaderForTree(void* payload, uint32_t byte_index, TSPoint position, uint32_t* bytes_read);
```

When called, `internalReaderForTree` uses cached line byte counts to jump directly to the target `LineNode` block and returns a direct pointer to the `Char_U8` data array. Tree-Sitter reads the buffer in small chunks without extra allocations.

### Incremental Updates via `onStateChangeTS`
When the document is edited, Alwide intercepts changes through `onStateChangeTS(Action action, TS_Data* data)`:

1. **Calculate Changes**: Converts the history transaction `Action` into a `TSInputEdit` structure:
   ```c
   TSInputEdit edit;
   edit.start_byte = descriptor.start_byte;
   edit.old_end_byte = descriptor.old_end_byte;
   edit.new_end_byte = descriptor.new_end_byte;
   edit.start_point = (TSPoint){descriptor.start_point.row, descriptor.start_point.column};
   edit.old_end_point = (TSPoint){descriptor.old_end_point.row, descriptor.old_end_point.column};
   edit.new_end_point = (TSPoint){descriptor.new_end_point.row, descriptor.new_end_point.column};
   ```
2. **Apply Edit to Tree**: Triggers `ts_tree_edit(data->tree, &edit)`. This marks the changed branches of the AST as "dirty".
3. **Incremental Re-parse**: Invokes `ts_parser_parse(parser, data->tree, input_reader)`. The parser only re-lexes and re-parses the dirty nodes, leaving the rest of the AST tree intact. This operation completes in microseconds even on multi-megabyte files.

---

## 🗺️ Codebase Documentation Map

* **[Back to Master README](file:///home/arno/dev/Alwide/README.md)**
* **[System Architecture](file:///home/arno/dev/Alwide/doc/architecture.md)** | **[Buffer Data Structures](file:///home/arno/dev/Alwide/doc/data_structures.md)** | **[Tree-Sitter Highlighting](file:///home/arno/dev/Alwide/doc/tree_sitter.md)** | **[Asynchronous LSP Client](file:///home/arno/dev/Alwide/doc/lsp_client.md)** | **[UI Windows & Features](file:///home/arno/dev/Alwide/doc/features.md)**

