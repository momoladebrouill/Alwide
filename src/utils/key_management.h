#ifndef KEY_MANAGEMENT_H
#define KEY_MANAGEMENT_H

#include <ncurses.h>

/*
 * Unified Bit-Packed Key System
 * Bits 0-23: Unicode Codepoint or Ncurses Keycode
 * Bit 24: Shift Modifier
 * Bit 25: Alt Modifier
 * Bit 26: Ctrl Modifier
 * Bit 27: Super (Windows/Cmd) Modifier
 * Bit 28: Special Key Flag (1 = Command/Functional, 0 = Character to be inserted in the file buffer)
 * Bit 29: Release Flag (1 = Release, 0 = Press/Repeat)
 */
#define K_MOD_NONE     0
#define K_MOD_SHIFT    (1 << 24)
#define K_MOD_ALT      (1 << 25)
#define K_MOD_CTRL     (1 << 26)
#define K_MOD_SUPER    (1 << 27)
#define K_FLAG_SPECIAL (1 << 28)
#define K_FLAG_RELEASE (1 << 29)

/* Macros for defining and checking keys */
#define K(mods, key)     ((mods) | (key) | K_FLAG_SPECIAL)
#define R(mods, key)     (K(mods, key) | K_FLAG_RELEASE)
#define IS_SPECIAL(hash) ((hash) & K_FLAG_SPECIAL)
#define IS_RELEASE(hash) ((hash) & K_FLAG_RELEASE)

#define TIME_BETWEEN_EVENT 200 /*ms*/

// --- Unified Key Binds ---

#define H_KEY_RESIZE    K(0, KEY_RESIZE)
#define H_KEY_MOUSE     K(0, KEY_MOUSE)
#define H_KEY_ESCAPE    K(0, 27)
#define KEY_TAB         '\t'
#define H_KEY_TAB       K(0, KEY_TAB)
#define H_KEY_SHIFT_TAB K(K_MOD_SHIFT, KEY_BTAB)
#define H_KEY_ENTER     K(0, KEY_ENTER)
#define H_KEY_BACKSPACE K(0, KEY_BACKSPACE)
#define H_KEY_DELETE    H_KEY_BACKSPACE /* Alias for consistency */

#define H_KEY_SUPPR       K(0, KEY_DC)
#define H_KEY_CTRL_SUPPR  K(K_MOD_CTRL, KEY_DC)
#define H_KEY_CTRL_DELETE K(K_MOD_CTRL, KEY_BACKSPACE)

#define H_KEY_RIGHT K(0, KEY_RIGHT)
#define H_KEY_LEFT  K(0, KEY_LEFT)
#define H_KEY_UP    K(0, KEY_UP)
#define H_KEY_DOWN  K(0, KEY_DOWN)

#define H_KEY_MAJ_RIGHT K(K_MOD_SHIFT, KEY_RIGHT)
#define H_KEY_MAJ_LEFT  K(K_MOD_SHIFT, KEY_LEFT)
#define H_KEY_MAJ_UP    K(K_MOD_SHIFT, KEY_UP)
#define H_KEY_MAJ_DOWN  K(K_MOD_SHIFT, KEY_DOWN)

#define H_KEY_CTRL_RIGHT     K(K_MOD_CTRL, KEY_RIGHT)
#define H_KEY_CTRL_LEFT      K(K_MOD_CTRL, KEY_LEFT)
#define H_KEY_CTRL_DOWN      K(K_MOD_CTRL, KEY_DOWN)
#define H_KEY_CTRL_UP        K(K_MOD_CTRL, KEY_UP)
#define H_KEY_CTRL_SEMICOLON K(K_MOD_CTRL, ';')

#define H_KEY_CTRL_MAJ_RIGHT K(K_MOD_CTRL | K_MOD_SHIFT, KEY_RIGHT)
#define H_KEY_CTRL_MAJ_LEFT  K(K_MOD_CTRL | K_MOD_SHIFT, KEY_LEFT)
#define H_KEY_CTRL_MAJ_DOWN  K(K_MOD_CTRL | K_MOD_SHIFT, KEY_DOWN)
#define H_KEY_CTRL_MAJ_UP    K(K_MOD_CTRL | K_MOD_SHIFT, KEY_UP)

#define H_KEY_BEGIN   K(0, KEY_HOME)
#define H_KEY_END     K(0, KEY_END)
#define H_KEY_MAJ_END K(K_MOD_SHIFT, KEY_END)
#define H_KEY_F12     KEY_F(12)

#define ONLY_REPAINT_INPUT K(0, 0x0FFFFFFF) /* Unique value outside standard range */


// --- Mouse Binds ---

#define BEGIN_MOUSE_LISTEN 588
#define MOUSE_IN_OUT       589

void detectComplexMouseEvents(MEVENT* event);
void printEventList(MEVENT* event);

#endif // KEY_MANAGEMENT_H
