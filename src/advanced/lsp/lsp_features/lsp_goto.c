#include "lsp_goto.h"
#include "../../../terminal/term_handler.h"
#include "../lsp_features/lsp_completion.h"

void jumpToLocation(DispatcherPayload* data, Location location) {
  openNewFile(location.file_name.file_name, data->files_state.files, data->files_state.size,
              data->files_state.current_file_index, &data->view_port.gui->ofw_context.refresh_ofw,
              data->files_state.refresh_local_vars);

  FileContainer* current = *data->files_state.files + *data->files_state.current_file_index;
  // LSP rows are 0-based, WishWim rows are 1-based.
  // LSP columns are 0-based, WishWim columns are 0-based.
  current->cursor = tryToReachAbsPosition(current->cursor, location.range.pos1.row + 1, location.range.pos1.column);

  moveScreenToMatchCursor(data->view_port.gui, current->cursor, &current->screen_x, &current->screen_y);
}

void receiveGotoData(cJSON* packet, FileContainer* file, DispatcherPayload* data, LocationArray* location_array,
                         char* method) {
  LSP_destroyLocationArray(location_array);
  LSP_getLocationArrayFromJSON(LSP_getPacketResult(packet), location_array);

  if (location_array->size == 0) {
    fprintf(stderr, "No location found for %s\n", method);
    return;
  }

  if (location_array->size == 1) {
    jumpToLocation(data, location_array->items[0]);
  }
  else {
    // TODO implement picker for multiple locations
    // For now jump to first one.
    jumpToLocation(data, location_array->items[0]);
  }
}

