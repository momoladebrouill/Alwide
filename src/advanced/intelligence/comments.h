#ifndef COMMENTS_H
#define COMMENTS_H

#include "../../data-management/file_management.h"

/**
 * Toggles line comments for the current selection or current line.
 */
void ft_toggleComments(FileContainer* fc, History** history_frame, PayloadStateChange* payload_state_change);

#endif // COMMENTS_H
