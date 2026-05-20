#ifndef AUTO_PAIRS_H
#define AUTO_PAIRS_H

#include "../../data-management/file_management.h"
#include "../../data-management/file_structure.h"

/**
 * Handles auto-pairing of characters (e.g., inserting '}' after '{').
 * Returns true if the input was handled (either via auto-pairing or overtyping).
 */
bool ft_handleAutoPairs(FileContainer* fc, Char_U8 input, History** history_p, PayloadStateChange payload_state_change);

#endif // AUTO_PAIRS_H
