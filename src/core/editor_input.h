#ifndef WISHWIM_EDITOR_INPUT_H
#define WISHWIM_EDITOR_INPUT_H

#include "../advanced/lsp/lsp_dispatcher.h"
#include "editor_context.h"

/**
 * @brief Reads the next key event from the terminal input stream.
 *
 * This function orchestrates the decoding of Kitty Keyboard Protocol sequences,
 * ANSI escape codes, and multibyte UTF-8 characters from Ncurses.
 *
 * @param ctx The editor context.
 * @return A 32-bit unified bit-packed key value, or ERR if no input is available.
 */
int readNextInput(EditorContext* ctx);

/**
 * @brief Main input dispatcher for the editor.
 *
 * Routes the unified key to the appropriate handler in the following order:
 * 1. Internal Logic (Global commands like Quit/Resize)
 * 2. Active Popups
 * 3. Special Key Handler (Navigation, Actions)
 * 4. Character Insertion (Typing)
 *
 * @param ctx The editor context.
 * @param key The 32-bit unified key to dispatch.
 * @return The action to be taken by the main event loop.
 */
EventLoopAction dispatchInput(EditorContext* ctx, int key);

/**
 * @brief Handles editor-level internal logic and global shortcuts.
 *
 * @param ctx The editor context.
 * @param key The unified key.
 * @param out_action Output parameter for the loop action if logic was matched.
 * @return true if the input was handled, false otherwise.
 */
bool runInternalLogic(EditorContext* ctx, int key, EventLoopAction* out_action);

/**
 * @brief Routes input to the currently active popup windows.
 *
 * Checks both toplevel popups (TPW) and editor-internal popups.
 *
 * @param ctx The editor context.
 * @param key The unified key.
 * @return true if a popup consumed the input, false otherwise.
 */
bool handlePopupInput(EditorContext* ctx, int key);

/**
 * @brief Executes functional commands for special keys.
 *
 * Handles navigation (arrows), editing actions (Undo/Redo), and clipboard
 * operations (Copy/Paste) when the K_FLAG_SPECIAL bit is set.
 *
 * @param ctx The editor context.
 * @param key The unified special key.
 * @return The action to be taken by the event loop.
 */
EventLoopAction runSpecialKeyHandler(EditorContext* ctx, int key);

/**
 * @brief Handles the insertion of printable characters into the file buffer.
 *
 * This is the final step in the typing pipeline, responsible for deleting
 * selections, handling auto-pairs, and triggering LSP on-type requests.
 *
 * @param ctx The editor context.
 * @param key The unified key representing a printable character.
 */
void handleCharInsertion(EditorContext* ctx, int key);

#endif
