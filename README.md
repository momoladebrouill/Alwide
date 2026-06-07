# Alwide — A LightWeight IDE

> **"Sublime Text" in the terminal.** Alwide is a fast, powerful, and user-friendly TUI IDE. It aims to provide the same user experience as a graphical IDE, but right in your terminal. Need an easy editor over a simple SSH connection? Looking for something lighter than VS Code or the JetBrains suite? Or is Vim sometimes too rough for quick actions?


[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C](https://img.shields.io/badge/language-C-orange.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Tree-Sitter](https://img.shields.io/badge/highlighting-Tree--Sitter-green.svg)](https://tree-sitter.github.io/tree-sitter/)
[![LSP](https://img.shields.io/badge/intelligence-LSP-yellow.svg)](https://microsoft.github.io/language-server-protocol/)

<p align="center">
  <img width="700" alt="Alwide Screenshot" src="https://github.com/user-attachments/assets/f73b961f-0fc2-4c0a-81b6-3e391a96031b" />
</p>

https://github.com/user-attachments/assets/c6f40db1-bc5e-4c90-88a5-c7e5a5c72059



---

## The Modern Terminal Experience

Alwide is designed for users who want more than `nano` but find `vim` or `emacs` too complex or rusty. It’s the perfect companion for everything from editing quick configuration files and scripts (Bash, Python, etc.) to working on larger projects.

- **Zero Learning Curve:** Full mouse support means you can click, drag-select, and scroll just like in a desktop app. It’s the friendliest way to work in a terminal.
- **Sublime-Inspired:** We aim to bring the speed and "vibe" of Sublime Text to the terminal, extended with powerful modern features like LSP.
- **Fast & Lightweight:** Written in pure C. It starts in milliseconds, with a single binary size of around 10MB.
- **Advanced Features:** Built-in **Tree-sitter** for high-quality syntax highlighting and **LSP** support for VS Code-like intelligence (completions, hover docs, and goto definition) directly in your terminal.
- **Persistent State:** Alwide provides a fully persistent experience. Quit and reopen files as if nothing happened—your tabs, cursor positions, workspace setup, and even undo/redo history are fully preserved. Copy in Alwide, paste into your terminal.
- **Clean Codebase:** Want to understand how it works or add a feature? Clone, read, write, and compile. It is highly readable and perfect for education or curiosity.


### Supported Languages

Many languages are supported out of the box. If your preferred language is missing, you can add support in just a few minutes by cloning the repo and updating the configuration (ask you best llm friend)!

**C/C++, Python, Java, Go, Rust, JavaScript/TypeScript, Dart, Lua, Bash, HTML, CSS, JSON, Markdown, VHDL, Assembly, and more.**

---

## Getting Started

Currently, you need to compile Alwide from source to use it.

### Submodules

This project is based on many "lib" as submodule, you have to execute at the root folder to checkout submodules :

Clone using : 
```bash
git clone --recursive https://github.com/arnauda-gh/Alwide.git
cd Alwide
```

Or afterward use :
```bash
git submodule update --init --recursive
```

### Dependencies :

#### Ubuntu/Debian :

 - `apt install make gcc libncursesw5-dev`

#### Clang version 18 >=

Install the 18 only if you are currently under the requirement if you are above skip this step.

 - `wget https://apt.llvm.org/llvm.sh`
 - `chmod +x llvm.sh`
 - `sudo ./llvm.sh 18` (check result you may be asked to add some dependencies).

#### Install tree-sitter api.h

May be useless now. Skip for first try.

 - Be in the root folder
 - `make -C lib/tree-sitter/ install`

#### Install tree-sitter cli

Some tree-sitter parsers need to use `tree-sitter generate` to convert grammars to `parser.c` (for now only latex need it).

use :
- `npm install -g tree-sitter-cli`

#### Install rust/rustup/cargo packages

 - `rustc --version`

It's really important to be up to date ! You will be in most cases not up to date.

Ubuntu :
 - `apt install rustup`
 - `rustup update stable`

Others distro may not have rustup package. Check for your personnal distro.
You might find this useful : https://rustup.rs/

#### Compile :

In the root folder :
  - `make`


#### To install and generate the config use :
 - do not use sudo, the generation of the config need to be in user mode.
 - you will be prompted for sudo in the make install it-self.
 - check by yourself the use of sudo (used to cp al /bin/al).
 - `make install`

---

## Essential Shortcuts

| Shortcut | Action |
| :--- | :--- |
| `Ctrl + S` | **Save** & Auto-format |
| `Ctrl + Q` | **Quit** Alwide |
| `Ctrl + E` | **File Explorer** toggle |
| `Ctrl + O` | **Open File** toggle |
| `Ctrl + Space` | **Completion Popup** toggle |
| `Ctrl + W` | **Close** Current Tab |
| `Ctrl + Shift + /` | **Toggle Comment** |
| `Ctrl + R` | **Format** Code (LSP) |
| `Ctrl + Z/Y` | **Undo / Redo** |
| `Shift + Arrows` | **Select Text** |

---

## Configuration

Your preferences live in `~/.config/al/`:
- `languages-features.json`: Custom LSP commands and per-language tweaks.
- `theme`: Color for alwide.
---

## Contributing & Reporting Issues

Alwide needs testing across different terminal emulators, mouse drivers, and distributions. Any feedback, bug reports, or contributions are highly appreciated!

### How to Report Issues

If you encounter crashes, bugs, or unexpected behavior, please report them! To help us debug, compile Alwide with logging enabled.

The [Makefile](file:///home/arno/dev/Alwide/Makefile) has two build configurations:
1. **Release Build (Default):** Optimized for speed, with asserts and logging disabled.
2. **Debug & Logging Build:** Compiles with debug symbols (`-g`), AddressSanitizer (`-fsanitize=address`) to catch memory bugs, and redirects standard error to log files.

#### Steps to Generate Logs

1. Open the [Makefile](file:///home/arno/dev/Alwide/Makefile) and switch the compiler flags:
    * Comment out the release flags:
      ```makefile
      #CFLAGS=-DNDEBUG -O3
      ```
    * Uncomment the debug and logging flags:
      ```makefile
      CFLAGS=-g -D_SHOW_ERROR -fsanitize=address
      ```
2. Recompile:
   ```bash
   make clean
   make
   ```
3. Run the compiled editor (`./al` or `al`) and reproduce the issue.
4. Locate the log files in the directory where you ran `al`:
    * **Application Logs:** `.logs.txt` (captures editor warnings, errors, and crashes)
    * **LSP Logs:** `.lsp_logs.txt` (captures Language Server Protocol communication)

Please attach these logs when opening an issue.

---

Check out our [**Technical Documentation**](doc/architecture.md) to dive into the internals.

---

## License

Distributed under the **MIT License**. See `LICENSE` for more information.


