CC=clang-18
CFLAGS=-g -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -O3  # -fsanitize=address # -lncurses # -Wall -Wextra -Werror -gdwarf-4
#CFLAGS=-g -fsanitize=address # -lncurses # -Wall -Wextra -Werror -gdwarf-4

BUILD_DIR=build
executable= al # lsp_test #test_line.o test_file.o  test_line test_file  # utils/debug.o

# C sources files
SRC_MODULES= \
	src/data-management/utf_8_extractor.c \
	src/advanced/shared.c \
	src/data-management/file_structure.c \
	src/data-management/file_management.c \
	src/utils/tools.c \
	src/io_management/io_manager.c \
	src/utils/key_management.c \
	src/utils/clipboard_manager.c \
	src/io_management/viewport_history.c \
	src/data-management/state_control.c \
	src/terminal/term_handler.c \
	src/io_management/io_explorer.c \
	src/advanced/lsp/lsp_client.c \
	src/advanced/tree-sitter/tree_manager.c \
	src/advanced/tree-sitter/tree_query.c \
	src/advanced/tree-sitter/tree_sitter_highlighter.c \
	src/advanced/theme.c \
	src/terminal/highlight.c \
	src/terminal/click_handler.c \
	src/terminal/windows/few.c \
	src/terminal/windows/edw.c \
	src/terminal/windows/ofw.c \
	src/terminal/windows/pow.c \
	src/config/config.c \
	src/io_management/workspace_settings.c \
	src/advanced/lsp/lsp_handler.c \
	src/advanced/lsp/lsp_notification_dispatcher.c \
	src/advanced/lsp/lsp_response_dispatcher.c \
	src/advanced/lsp/lsp_highlighter.c \
	src/advanced/lsp/lsp_features/lsp_completion.c \
	src/advanced/lsp/lsp_features/lsp_goto.c \
	src/advanced/lsp/lsp_features/lsp_hover.c \
	src/advanced/lsp/lsp_dispatcher.c \
	src/advanced/lsp/lsp_emitter.c

# C Library sources
LIB_C_MODULES= \
	lib/cJSON/cJSON.c \
	lib/tree-sitter/lib/src/lib.c

# Rust Libraries (keep as .rlib)
RUST_MODULES= \
	lib/tree-sitter-c/target/release/libtree_sitter_c.rlib \
	lib/tree-sitter-python/target/release/libtree_sitter_python.rlib \
	lib/tree-sitter-java/target/release/libtree_sitter_java.rlib \
	lib/tree-sitter-cpp/target/release/libtree_sitter_cpp.rlib \
	lib/tree-sitter-c-sharp/target/release/libtree_sitter_c_sharp.rlib \
	lib/tree-sitter-make/target/release/libtree_sitter_make.rlib \
	lib/tree-sitter-css/target/release/libtree_sitter_css.rlib \
	lib/tree-sitter-dart/target/release/libtree_sitter_dart.rlib \
	lib/tree-sitter-go/target/release/libtree_sitter_go.rlib \
	lib/tree-sitter-javascript/target/release/libtree_sitter_javascript.rlib \
	lib/tree-sitter-json/target/release/libtree_sitter_json.rlib \
	lib/tree-sitter-bash/target/release/libtree_sitter_bash.rlib \
	lib/tree-sitter-markdown/target/release/libtree_sitter_md.rlib \
	lib/tree-sitter-query/target/release/libtree_sitter_query.rlib \
	lib/tree-sitter-vhdl/target/release/libtree_sitter_vhdl.rlib \
	lib/tree-sitter-lua/target/release/libtree_sitter_lua.rlib \
	lib/tree-sitter-asm/target/release/libtree_sitter_asm.rlib \
	lib/tree-sitter-html/target/release/libtree_sitter_html.rlib

# Map sources to objects in BUILD_DIR
OBJECTS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC_MODULES) $(LIB_C_MODULES))

SHARED_LIBS = -lncursesw -ltinfo

all: $(OBJECTS) $(RUST_MODULES) $(executable)

lib/tree-sitter-markdown/tree-sitter-markdown/libtree-sitter-markdown.a:
	cd lib/tree-sitter-markdown/tree-sitter-markdown/ && tree-sitter generate && $(MAKE)

lib/tree-sitter-markdown/tree-sitter-markdown-inline/libtree-sitter-markdown-inline.a:
	cd lib/tree-sitter-markdown/tree-sitter-markdown-inline/ && tree-sitter generate && $(MAKE)

%.rlib:
	cd  $(shell echo $@ | cut -d/ -f1-2) && cargo build --release

# Compilation rule for objects
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test_line: src/test_line.c $(OBJECTS) $(RUST_MODULES)
	$(CC) $(CFLAGS) $^ -o $@ $(SHARED_LIBS)

test_file: src/test_file.c $(OBJECTS) $(RUST_MODULES)
	$(CC) $(CFLAGS) $^ -o $@ $(SHARED_LIBS)

al: src/main.c $(OBJECTS) $(RUST_MODULES)
	$(CC) $(CFLAGS) $^ -o $@ $(SHARED_LIBS) #utils/debug.o

lsp_test: src/lsp_test.c $(OBJECTS) $(RUST_MODULES)
	$(CC) $(CFLAGS) $^ -o $@ $(SHARED_LIBS)

clean:
	rm -rf $(BUILD_DIR) $(executable) lsp_test test_line test_file *.o

clean_all: clean
	find . -type d -name "target" -exec rm -rf {} +


# !! DO NOT EXECUTE AS SUDO !!. To generate config you have to be as user. sudo will be asked to cp to
# /bin/al
install:
	make && mkdir -p ~/.config/al && cp -r ./assets/* ~/.config/al && ./generate_config.sh && sudo cp al /bin/al

# find . -type d -name "target" -exec rm -rf {} + # Remove target folders
