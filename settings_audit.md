# Alwide Settings Menu & Configuration Audit

This document outlines all the settings currently defined in Alwide (either as hardcoded C `#define` constants, workspace settings, or JSON configuration values) that should be exposed in a future **Settings Window**. 

We have organized these settings into 5 main menus (Appearance, Shortcuts, Code Scheme, Workspace, and General) with global titles and sub-settings.

---

## 1. Appearance Settings

These settings control the visual style, themes, colors, and layout elements of the Alwide editor interface.

### Theme & Syntax Highlighting
*For each highlight group, users can customize text color, background color, and text style attributes.*

- **Syntax Highlight Theme**: Path/Name of the active syntax theme file (currently loads `assets/theme`).
- **Highlight Group Color Customization (RGB 0–1000 scale)**:
  - `@function` (Functions)
  - `@method` (Methods)
  - `@comment` (Comments)
  - `@keyword` (Keywords)
  - `@string` (Strings)
  - `@string.strong` (Bold strings/text)
  - `@string.italic` (Italic strings/text)
  - `@type` (Data types)
  - `@property` (Object properties)
  - `@label` (Labels)
  - `@number` (Numeric values)
  - `@variable` (Variables)
  - `@constant` (Constants)
  - `@todo` (TODO items)
- **Text Styles**: Checkboxes for toggling `Italic` ("i"), `Bold` ("b"), or `Underline` ("u") for each group.

### Interface Colors
- **Default Background Color** (`BG_COLOR_DEFAULT`)
- **Hover Background Color** (`BG_COLOR_HOVER`)
- **Popup/Dropdown Background Color** (`BG_COLOR_POPUP`)

---

## 2. Shortcut Settings (Keyboard & Mouse Bindings)

These control the keyboard shortcuts for editor actions, which are currently defined as hardcoded hashes in [key_management.h](file:///home/arno/dev/Alwide/src/utils/key_management.h) and handled in [editor_input.c](file:///home/arno/dev/Alwide/src/core/editor_input.c).

### File & App Actions
- **Quit Editor**: Defaults to `Ctrl + Q`
- **Save File**: Defaults to `Ctrl + S`
- **Close Current File**: Defaults to `Ctrl + W`

### Editing & Clipboard
- **Undo**: Defaults to `Ctrl + Z`
- **Redo**: Defaults to `Ctrl + Y`
- **Copy**: Defaults to `Ctrl + C`
- **Cut**: Defaults to `Ctrl + X`
- **Paste**: Defaults to `Ctrl + V`
- **Select All**: Defaults to `Ctrl + A`
- **Delete Selection / Line**: Defaults to `Ctrl + D`
- **Toggle Comments**: Defaults to `Ctrl + Shift + /` or `Ctrl + _`
- **Auto-Completion**: Defaults to `Ctrl + Space`
- **Format Document (LSP)**: Defaults to `Ctrl + R`
- **Search In File**: Defaults to `Ctrl + F`
- **Close Popup / Escape**: Defaults to `Escape` or `Ctrl + [`

### Navigation & Indentation
- **Go to Start of Line**: Defaults to `Home` / `H_KEY_BEGIN`
- **Go to End of Line**: Defaults to `End` / `H_KEY_END`
- **Indent Line/Selection**: Defaults to `Tab`
- **De-indent Line/Selection**: Defaults to `Shift + Tab`
- **Move Word Left/Right**: Defaults to `Ctrl + Left` / `Ctrl + Right`
- **Move Line Up/Down**: Defaults to `Ctrl + Up` / `Ctrl + Down`

### Panel Toggles
- **Toggle File Explorer Window**: Defaults to `Ctrl + E`
- **Toggle Opened Files Top Bar**: Defaults to `Ctrl + L`

---

## 3. Code Scheme (Language Features)

These settings apply on a per-language basis and are currently loaded dynamically from [languages-features.json](file:///home/arno/dev/Alwide/assets/languages-features.json).

### Language Detection
- **File Extensions**: Array of extensions mapped to this language (e.g., `[".c", ".h"]`).
- **Exact Filenames**: Mapped filenames (e.g., `["Makefile"]`).
- **Shebang Interpreters**: Shebang strings to match (e.g., `["python"]`).

### Editor Features
- **Line Comment Characters**: Symbol used for single-line comments (e.g., `//` or `#`).
- **Block Comment Characters**: Pair used for multi-line block comments (e.g., `["/*", "*/"]`).
- **Tab Size**: Number of spaces for a tab indent (e.g., `2` or `4`).
- **Use Spaces**: Toggle whether to insert spaces instead of a literal tab character (`true`/`false`).
- **Auto-Pairs**: Mapped pairs to auto-close when typing (e.g., `()`, `{}`, `[]`, `""`, `''`).

### Language Server Protocol (LSP)
- **LSP Server Executable**: Executable name or absolute path of the language server (e.g., `clangd`, `pylsp`).
- **LSP Server Arguments**: CommandLine arguments passed to the server upon launch (e.g., `["-v"]`).

---

## 4. Workspace Settings

These settings are workspace-specific, persisted in the directory under `.workspace_settings` mapped to a hash of the current working directory path (see [workspace_settings.h](file:///home/arno/dev/Alwide/src/io-management/workspace_settings.h)).

- **Side File Explorer Width** (`file_explorer_size`): Current width of the side navigation bar (Default: `30`).
- **Show Top Tab Bar** (`showing_opened_file_window`): Show/hide list of currently open tabs (Default: `true`).
- **Show Side File Explorer** (`showing_file_explorer_window`): Show/hide side folder tree (Default: `false`).
- **Restore Open Files**: Keep track of and reopen the files active during the last session.

---

## 5. General / System Settings

These core parameters are currently defined in [constants.h](file:///home/arno/dev/Alwide/src/environnement/constants.h) and can be adjusted globally.

- **Default Config Path**: Path where theme files and languages metadata are looked up (currently set via `.config/al/config`).
- **Scroll Speed** (`SCROLL_SPEED`): Number of lines scrolled per wheel tick (Default: `3`).
- **Max File Load Size** (`MAX_SIZE_FILE_LOGIC`): Maximum size threshold of a file allowed to be loaded in the editor (Default: `4,000,000` bytes / ~4MB).
- **Key Repeat/Throttle Delay** (`TIME_BETWEEN_EVENT`): Debounce timing between input events (Default: `200` ms).
- **Mouse Skip Delay** (`SKIP_MOUSE_EVENT_DELAY`): Rate-limiting interval for mouse event parsing (Default: `0` ms).
- **Top Bar Window Height** (`OPENED_FILE_WINDOW_HEIGHT`): Vertical height occupied by the top tabs pane (Default: `2`).
