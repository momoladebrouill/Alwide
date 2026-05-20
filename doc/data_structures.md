# Buffer Representation and State Control Data Structures

To handle document content efficiently without resorting to complex Ropes or simplistic continuous character arrays, Alwide implements a custom **Unrolled Linked List** architecture, accompanied by a precise multi-layer coordinate converter and a timeline-based undo/redo history manager.

---

## 1. Unrolled Linked List Buffer

Alwide represents an open text file as a doubly-linked chain of large blocks (`FileNode`), each block containing a dynamically resizable array of line nodes (`LineNode`).

```
  FileContainer
        │
        ▼
  ┌──────────┐      next      ┌──────────┐
  │ FileNode │ ─────────────► │ FileNode │  ...
  │ (Block1) │ ◄─────────────  │ (Block2) │
  └────┬─────┘      prev      └────┬─────┘
       │                           │
       ▼ (Array of Lines)          ▼ (Array of Lines)
  ┌──────────┐                ┌──────────┐
  │ LineNode │                │ LineNode │
  ├──────────┤                ├──────────┤
  │ LineNode │                │ LineNode │
  └──────────┘                └──────────┘
```

### Data Layout: `LineNode` and `FileNode`
Defined in [src/data-management/file_structure.h](file:///home/arno/dev/Alwide/src/data-management/file_structure.h):

```c
typedef struct LineNode_ {
  bool fixed;
  struct LineNode_* next;
  struct LineNode_* prev;
  Char_U8* ch;                    // Dynamic array of UTF-8 characters
  int current_max_element_number; // Maximum capacity of ch array
  int element_number;             // Actual character count in this line
  int byte_count;                 // Byte count of this line
} LineNode;

typedef struct FileNode_ {
  struct FileNode_* next;
  struct FileNode_* prev;
  LineNode* lines;                // Array of LineNodes (up to MAX_ELEMENT_NODE)
  int current_max_element_number; // Maximum capacity of lines array
  int element_number;             // Actual line count in this FileNode
  int lines_byte_count[MAX_ELEMENT_NODE]; // Cached byte counts for fast search
  int byte_count;                 // Cumulative bytes in this entire FileNode block
} FileNode;
```

### Structural Merits and Caching
* **Locality & Indexing**: Standard doubly-linked list lines degrade search performances on long documents. By grouping up to `MAX_ELEMENT_NODE = 200` lines into consecutive arrays in a `FileNode` block, Alwide speeds up index lookups.
* **Line Byte Count Caching**: The `lines_byte_count` array enables $O(1)$ scans over line sizes when mapping flat document byte indexes to row/column offsets.
* **Integrity Auditing**: The editor validates buffers using `checkFileIntegrity()` and `checkByteCountIntegrity()`, traversing chains to check if cached byte accumulators match character byte weights.

---

## 2. Coordinate Systems & Cursor Tracking

Alwide processes text across three distinct coordinate schemes:

| Coordinate System | Row Range | Column Range | Primary Consumer |
| :--- | :--- | :--- | :--- |
| **Rich Pointer Cursor** | 1-based (Absolute) | 0-based (Relative to Node) | Core Buffer Traversal (`src/core/editor_input.c`) |
| **Flat Descriptor** | 1-based | 0-based (Absolute) | Undo-Redo and History Manager |
| **LSP Standards** | 0-based | 0-based (Character count) | Language Server Protocol messages |

### The `Cursor` Structure
To avoid scanning the linked list on every cursor movement, Alwide uses a "rich cursor" carrying direct structural pointers:

```c
typedef struct {
  int relative_column; // Character column index within the active LineNode
  int absolute_column; // Flat character column index relative to start of line
  LineNode* line;      // Pointer to active LineNode struct
} LineIdentifier;

typedef struct {
  int relative_row;    // Line index within the parent FileNode array
  int absolute_row;    // Flat line index from the document start
  FileNode* file;      // Pointer to active FileNode block struct
} FileIdentifier;

typedef struct {
  FileIdentifier file_id;
  LineIdentifier line_id;
} Cursor;
```

### Pointer vs. Flat Coordinates Conversion
Functions in `file_structure.c` bridge structural pointers with absolute coordinate indices:
* `cursor_to_desc(Cursor c)`: Extracts the absolute coordinates, returning a simple structure:
  ```c
  typedef struct { int row; int column; } CursorDescriptor;
  ```
* `desc_to_cursor(Cursor base, CursorDescriptor desc)`: Restores a rich pointer `Cursor` from integer coordinates. It starts at a `base` cursor (usually the document root node) and traverses line and file nodes until the targeted absolute position is reached.
* `byteCursorToCursor(Cursor base, int row, int byte_col)`: Conversions for systems that require byte boundaries rather than character indexes (e.g., Tree-Sitter highlight positions).

---

## 3. Undo/Redo & State History

Document modifications are tracked inside a robust transaction log to support undo and redo chains.

### The History Model
History is tracked via a doubly-linked chain of `History` nodes containing an `Action` struct:

```c
typedef enum { INSERT = 'i', DELETE = 'd', DELETE_ONE = 'D', ACTION_NONE = 'n' } ACTION_TYPE;

typedef struct {
  ACTION_TYPE action;
  char unique_ch;               // Character acted upon if singular
  char* ch;                     // Text string affected by bulk operations
  time_val time;                // Action timestamp (in milliseconds)
  CursorDescriptor cur;         // Operation start position
  unsigned int byte_start;      // Byte start offset
  CursorDescriptor cur_end;     // Operation end position
  unsigned int byte_end;        // Byte end offset
} Action;

struct History_ {
  Action action;
  struct History_* next;        // Re-do path pointer
  struct History_* prev;        // Un-do path pointer
};
```

### Action Coalescing (Time-based Optimizations)
Keystroke insertions usually generate dozens of individual actions. To prevent bloat, Alwide optimizes the history chain:
1. **Timestamp Comparison**: Checks `TIME_CONSIDER_UNIQUE_UNDO = 300 ms` in `optimizeHistory()`.
2. **Coalescing**: Consecutive `INSERT` or `DELETE` keystrokes written at adjacent offsets within 300ms are combined into single history actions. Tapping `Undo` reverts full words or sentences rather than single characters.

### Change Descriptors and Incremental Updates
When an action is undone or redone, it is translated into a `ChangeDescriptor` via `actionToChangeDescriptor()`:

```c
typedef struct {
  uint32_t start_byte;
  uint32_t old_end_byte;
  uint32_t new_end_byte;
  ChangePoint start_point;
  ChangePoint old_end_point;
  ChangePoint new_end_point;
} ChangeDescriptor;
```

This descriptor matches the incremental edit payload required by **Tree-Sitter** (`TSInputEdit`) and the **LSP client** (`textDocument/didChange`), allowing instant parser updates without re-reading the entire file.

### Caching and Persistence
Alwide caches and persists editing history on disk:
- When a document is saved, `saveCurrentStateControl()` serializes the action stack to `/tmp/al/state_control/`.
- If a document is re-opened, the editor scans this directory using `loadCurrentStateControl()`, reconstructing the active undo/redo history to preserve state across editing sessions.

---

## 🗺️ Codebase Documentation Map

* **[Back to Master README](file:///home/arno/dev/Alwide/README.md)**
* **[System Architecture](file:///home/arno/dev/Alwide/doc/architecture.md)** | **[Buffer Data Structures](file:///home/arno/dev/Alwide/doc/data_structures.md)** | **[Tree-Sitter Highlighting](file:///home/arno/dev/Alwide/doc/tree_sitter.md)** | **[Asynchronous LSP Client](file:///home/arno/dev/Alwide/doc/lsp_client.md)** | **[UI Windows & Features](file:///home/arno/dev/Alwide/doc/features.md)**

