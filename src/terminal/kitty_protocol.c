#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include "../utils/key_management.h"
#include "kitty_protocol.h"

void kitty_enable(void) {
  /* Use Push (>) flag 3 (1|2) to set disambiguate and event types */
  printf("\033[>3u");
  fflush(stdout);
}

void kitty_disable(void) {
  /* Pop (<) keyboard mode from stack */
  printf("\033[<u");
  fflush(stdout);
}

bool kitty_parse_sequence(int first_char, KittyKeyEvent* event, int* out_unread) {
  *out_unread = ERR;
  if (first_char != 27) {
    return false;
  }

  /* Patience: Wait up to 50ms for the very first char after Escape */
  timeout(50);
  int next = getch();
  timeout(20);

  if (next == ERR) {
    event->key_code  = 27;
    event->modifiers = 1;
    event->type      = KITTY_EVENT_PRESS;
    return true;
  }

  if (next != '[' && next != 'O') {
    *out_unread      = next;
    event->key_code  = 27;
    event->modifiers = 1;
    event->type      = KITTY_EVENT_PRESS;
    return true;
  }

  /* We are inside a sequence (CSI or SS3). Consume until terminator. */
  char buf[64];
  int  len        = 0;
  char terminator = '\0';

  /* For the body of the sequence, wait up to 20ms per byte */
  while (len < (int)sizeof(buf) - 1) {
    timeout(20);
    int c = getch();
    if (c == ERR) {
      break; /* Sequence ended prematurely */
    }

    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '~') {
      terminator = (char)c;
      break;
    }
    buf[len++] = (char)c;
  }
  timeout(20);
  buf[len] = '\0';

  if (terminator == '\0') {
    /* Premature end: swallow what we have to avoid garbage */
    event->key_code  = -2;
    event->modifiers = 1;
    event->type      = KITTY_EVENT_PRESS;
    return true;
  }

  /* 1. Parse parameters */
  int p1 = 1, p2 = 1, p3 = 1;
  int parsed = sscanf(buf, "%d;%d:%d", &p1, &p2, &p3);
  if (parsed == 2) {
    p3 = 1;
  }
  else if (parsed == 1) {
    p2 = 1;
    p3 = 1;
  }
  else if (parsed == 0) {
    p1 = 1;
    p2 = 1;
    p3 = 1;
  }

  /* 2. Map terminators */
  event->modifiers = p2;
  event->type      = (KittyEventType)p3;

  if (terminator == 'u') {
    event->key_code = p1;
    return true;
  }

  /* Map ANSI terminators */
  switch (terminator) {
    case 'A':
      event->key_code = 57352;
      break; /* Up */
    case 'B':
      event->key_code = 57353;
      break; /* Down */
    case 'C':
      event->key_code = 57351;
      break; /* Right */
    case 'D':
      event->key_code = 57350;
      break; /* Left */
    case 'H':
      event->key_code = 57356;
      break; /* Home */
    case 'F':
      event->key_code = 57357;
      break; /* End */
    case 'Z':
      event->key_code  = 57346;
      event->modifiers = 2;
      break; /* Backtab */
    case '~':
      switch (p1) {
        case 2:
          event->key_code = 57348;
          break; /* Insert */
        case 3:
          event->key_code = 57349;
          break; /* Delete */
        case 5:
          event->key_code = 57354;
          break; /* PageUp */
        case 6:
          event->key_code = 57355;
          break; /* PageDown */
        case 7:
          event->key_code = 57356;
          break; /* Home */
        case 8:
          event->key_code = 57357;
          break; /* End */
        default:
          event->key_code = -2;
          break;
      }
      break;
    default:
      event->key_code = -2;
      break;
  }

  return true;
}

bool kitty_translate_event(const KittyKeyEvent* event, int* out_unified) {
  int key  = event->key_code;
  int mods = event->modifiers;

  int  val   = (mods > 0) ? mods - 1 : 0;
  bool shift = (val & 1) != 0;
  bool alt   = (val & 2) != 0;
  bool ctrl  = (val & 4) != 0;
  bool super = (val & 8) != 0;

  int unified_mods = 0;
  if (shift) {
    unified_mods |= K_MOD_SHIFT;
  }
  if (alt) {
    unified_mods |= K_MOD_ALT;
  }
  if (ctrl) {
    unified_mods |= K_MOD_CTRL;
  }
  if (super) {
    unified_mods |= K_MOD_SUPER;
  }

  /* Filter releases: pass only modifier key state changes or Ctrl releases */
  bool is_release = (event->type == KITTY_EVENT_RELEASE);
  if (is_release) {
    bool is_mod_key = (key >= 57441 && key <= 57448);
    if (!is_mod_key && !ctrl) {
      return false;
    }
  }

  bool is_functional  = false;
  int  translated_key = key;

  if (key >= 57344 && key <= 57387) {
    is_functional = true;
    switch (key) {
      case 57344:
        translated_key = 27;
        break; /* Escape */
      case 57345:
        translated_key = '\n';
        break; /* Enter */
      case 57346:
        translated_key = KEY_TAB;
        break;
      case 57347:
        translated_key = KEY_BACKSPACE;
        break;
      case 57348:
        translated_key = KEY_IC;
        break;
      case 57349:
        translated_key = KEY_DC;
        break;
      case 57350:
        translated_key = KEY_LEFT;
        break;
      case 57351:
        translated_key = KEY_RIGHT;
        break;
      case 57352:
        translated_key = KEY_UP;
        break;
      case 57353:
        translated_key = KEY_DOWN;
        break;
      case 57354:
        translated_key = KEY_PPAGE;
        break;
      case 57355:
        translated_key = KEY_NPAGE;
        break;
      case 57356:
        translated_key = KEY_HOME;
        break;
      case 57357:
        translated_key = KEY_END;
        break;
      default:
        if (key >= 57364 && key <= 57375) {
          translated_key = KEY_F(key - 57364 + 1);
        }
        break;
    }
  }
  else if (key == 27 || key == '\n' || key == '\r' || key == '\t' || key == 127 || key == 8) {
    is_functional = true;
    if (key == '\r') {
      translated_key = '\n';
    }
    else if (key == 127 || key == 8) {
      translated_key = KEY_BACKSPACE;
    }
  }

  if (translated_key == KEY_TAB && shift) {
    translated_key = KEY_BTAB;
  }

  if (is_functional) {
    *out_unified = K_SPECIAL(unified_mods, translated_key);
    if (is_release) {
      *out_unified |= K_FLAG_RELEASE;
    }
    return true;
  }

  if (ctrl || alt || super) {
    if (translated_key >= 'A' && translated_key <= 'Z') {
      translated_key += ('a' - 'A');
    }
    *out_unified = K_SPECIAL(unified_mods, translated_key);
    if (is_release) {
      *out_unified |= K_FLAG_RELEASE;
    }
  }
  else {
    if (shift && translated_key >= 'a' && translated_key <= 'z') {
      translated_key -= ('a' - 'A');
    }
    *out_unified = translated_key;
  }

  return true;
}
