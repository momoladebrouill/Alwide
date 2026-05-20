# User Interface Layout, Keybindings, and Settings

Alwide offers a highly-responsive terminal interface using overlapping NCurses panels. This document details the layout structure, interactive keyboard hotkeys, and global configuration mechanics.

---

## 1. Visual Layout and NCurses Windows

The visual layout of Alwide is split into distinct windows, each controlled by its own context inside `GUIContext` defined in [src/terminal/windows/gui_entities.h](file:///home/arno/dev/Alwide/src/terminal/windows/gui_entities.h):

```
┌──────────────────────────────────────────────────────────────┐
│  OFW: Opened Files Window (Tabs bar)                         │
├───────────────┬──────────────────────────────────────────────┤
│               │  EDW: Editor Window                          │
│  FEW:         │  ┌─────┬──────────────────────────────────┐  │
│  File Explorer│  │ LNW │  FTW: File Text Window           │  │
│  Sidebar      │  │ Gutt│                                  │  │
│  (Tree View)  │  │ er  │   ┌──────────────────────────┐   │  │
│               │  │     │   │ POW: Popup Window        │   │  │
│               │  │     │   │ (Completions/Hover Menu) │   │  │
│               │  │     │   └──────────────────────────┘   │  │
│               │  └─────┴──────────────────────────────────┘  │
└───────────────┴──────────────────────────────────────────────┘
```

### Window Contexts & Partitions
1. **Opened Files Window (OFW)**: 
   * **Purpose**: A horizontal top bar displaying the list of open buffers.
   * **Details**: The active buffer is highlighted. Supports click events to switch tabs. The window height is defined by `OPENED_FILE_WINDOW_HEIGHT` (2 lines).
2. **File Explorer Window (FEW)**:
   * **Purpose**: A collapsible left sidebar tree showing the project directory.
   * **Details**: Default width is `FILE_EXPLORER_WIDTH` (30 columns). Supports double-clicks on folders to expand or collapse them, and double-clicks on files to open them in a new tab.
3. **Editor Window (EDW)**:
   * Consists of three nested window elements:
     * **Line Number Window (LNW)**: Gutter displaying line numbers. Its width adjusts dynamically based on the file’s row count.
     * **File Text Window (FTW)**: Main editor area where text is drawn and styled with syntax colors.
     * **Popup Window (POW)**: Contextual floating overlay. Used for completions, hover tooltips, method signatures, or goto definition lists. Its position is tied to the cursor coordinate anchor (`lastTextAnchor`).

---

## 2. Interactive Keyboard Shortcuts

Keyboard hotkeys are captured, hashed, and processed in `runKeyHandler()` inside [src/core/editor_input.c](file:///home/arno/dev/Alwide/src/core/editor_input.c).

| Keystroke | Unified Hash Token | Operation Performed |
| :--- | :--- | :--- |
| **Arrow keys** | `H_KEY_RIGHT`/`LEFT`/`UP`/`DOWN` | Moves the cursor. Closes the popup window if moving Up/Down. |
| **Shift + Arrow** | `H_KEY_MAJ_RIGHT`/`LEFT`/`UP`/`DOWN` | Begins or expands text selection range. |
| **Ctrl + Left / Right** | `H_KEY_CTRL_RIGHT` / `LEFT` | Jumps the cursor to the next or previous word boundary. |
| **Ctrl + Up / Down** | `H_KEY_CTRL_DOWN` / `UP` | Selects the entire word under the cursor. |
| **Ctrl + Maj + Up / Down** | `H_KEY_CTRL_MAJ_UP` / `DOWN` | Switches active buffers (next or previous opened tab). |
| **Ctrl + A** | `CTRL('a')` | Selects all text in the active buffer. |
| **Ctrl + C** | `CTRL('c')` | Copies the selected text to the system clipboard. |
| **Ctrl + X** | `CTRL('x')` | Cuts the selected text to the system clipboard. |
| **Ctrl + V** | `CTRL('v')` | Pastes text from the system clipboard. |
| **Ctrl + Z** | `CTRL('z')` | Reverts the last text change (Undo). |
| **Ctrl + Y** | `CTRL('y')` | Re-applies the last undone change (Redo). |
| **Ctrl + /** | `0x1F` | Toggles comments on the active line or selection block. |
| **Ctrl + R** | `CTRL('r')` | Triggers automatic code formatting via LSP. |
| **Ctrl + S** | `CTRL('s')` | Saves the document and commits history state log to disk. |
| **Ctrl + W** | `CTRL('w')` | Closes the current buffer tab. |
| **Ctrl + Q** | `CTRL('q')` | Shuts down the editor sessions. |
| **Tab** | `KEY_TAB` | Inserts indents (uses spaces or tabs depending on language settings). |
| **Enter** | `KEY_ENTER` / `\n` | Inserts a new line and inherits indentation from the preceding line. |
| **Backspace** | `H_KEY_DELETE` | Deletes character to the left, or deletes the active text selection. |
| **Delete** | `H_KEY_SUPPR` | Deletes character to the right, or deletes the active text selection. |

---

## 3. Editing Features

Alwide implements several modern quality-of-life editing features:

### Auto-Closing Pairs
Defined in [src/advanced/intelligence/auto_pairs.c](file:///home/arno/dev/Alwide/src/advanced/intelligence/auto_pairs.c):
* When typing an opening bracket or quotation mark (e.g. `(`, `{`, `[`, `"`, `'`), `ft_handleAutoPairs` automatically inserts the matching closing character right after the cursor.
* If the user types the closing character manually when the cursor is positioned directly before it, Alwide skips the cursor past the closing character rather than inserting a duplicate.

### Line and Block Commenting
Defined in [src/advanced/intelligence/comments.c](file:///home/arno/dev/Alwide/src/advanced/intelligence/comments.c):
* Pressing `Ctrl + /` comments out the current line or active block selection.
* If a line is already commented, it removes the comment characters.
* Toggling comments adapts to the active language settings (e.g. inserting `//` for C, `#` for Python, or `/*` / `*/` for block selections).

---

## 4. Configurations and Settings

Alwide supports global and project-level settings to customize layout bounds and core behaviors.

### Global Settings File
User-specific preferences are stored inside `~/.config/al/config.json`. On startup, `loadConfig()` reads this JSON configuration using `cJSON`. Global configurations include:
* **UI Themes**: Set color schemes for backgrounds (`BG_COLOR_DEFAULT`), hover selections (`BG_COLOR_HOVER`), and popup overlays (`BG_COLOR_POPUP`).
* **LSP Executables**: Specify custom paths and arguments for language servers.
* **Tabulation Default**: Set fallback tab width (e.g. 2 or 4) and choose whether to insert spaces or raw tabs.

### Workspace Settings
Project-specific preferences are configured inside `workspace_settings.c`.
* **Workspace Directories**: Reads project directory configuration to open and organize folder paths in the file explorer sidebar pane.
* **Session Restoration**: Remembers window splits, active tabs, and layout adjustments when re-opening project folders.

---

## 🗺️ Codebase Documentation Map

* **[Back to Master README](file:///home/arno/dev/Alwide/README.md)**
* **[System Architecture](file:///home/arno/dev/Alwide/doc/architecture.md)** | **[Buffer Data Structures](file:///home/arno/dev/Alwide/doc/data_structures.md)** | **[Tree-Sitter Highlighting](file:///home/arno/dev/Alwide/doc/tree_sitter.md)** | **[Asynchronous LSP Client](file:///home/arno/dev/Alwide/doc/lsp_client.md)** | **[UI Windows & Features](file:///home/arno/dev/Alwide/doc/features.md)**

