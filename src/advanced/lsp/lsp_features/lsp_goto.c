#include "lsp_goto.h"
#include "../../../terminal/term_handler.h"
#include "../../../terminal/windows/edw.h"
#include "../../../terminal/windows/pow.h"
#include "../lsp_features/lsp_completion.h"
#include <stdlib.h>
#include <string.h>

void jumpToLocation(DispatcherPayload* data, LSP_Location location) {
  openNewFile(location.file_name.file_name, data->files_state.files, data->files_state.size,
              data->files_state.current_file_index, &data->view_port.gui->ofw_context.refresh_ofw,
              data->files_state.refresh_local_vars);

  FileContainer* current = *data->files_state.files + *data->files_state.current_file_index;
  // LSP rows are 0-based, WishWim rows are 1-based.
  // LSP columns are 0-based, WishWim columns are 0-based.
  current->cursor = tryToReachAbsPosition(current->cursor, location.range.pos1.row + 1, location.range.pos1.column);

  moveScreenToMatchCursor(data->view_port.gui, current->cursor, &current->screen_x, &current->screen_y);
}

bool isPositionInRange(LSP_Position wishwim_pos, LSP_Range lsp_range) {
  // wishwim_pos.row is 1-based, lsp_range rows are 0-based.
  int lsp_pos_row = wishwim_pos.row - 1;

  if (lsp_pos_row < lsp_range.pos1.row || lsp_pos_row > lsp_range.pos2.row) {
    return false;
  }
  // LSP columns are 0-based, WishWim columns are 0-based.
  if (lsp_pos_row == lsp_range.pos1.row && wishwim_pos.column < lsp_range.pos1.column) {
    return false;
  }
  if (lsp_pos_row == lsp_range.pos2.row && wishwim_pos.column > lsp_range.pos2.column) {
    return false;
  }
  return true;
}

void receiveGotoData(cJSON* packet, LSP_Server* lsp, FileContainer* file, DispatcherPayload* data,
                     LSP_LocationArray* location_array, char* method, void* payload) {
  GotoContext* goto_payload = (GotoContext*)payload;
  LSP_destroyLocationArray(location_array);
  LSP_getLocationArrayFromJSON(LSP_getPacketResult(packet), location_array);

  // Filter out the current location (same word clicked)
  if (goto_payload != NULL) {
    int filtered_count = 0;
    for (int i = 0; i < location_array->size; i++) {
      LSP_Location loc = location_array->items[i];
      // Check if the location is in the same file and the original position is within its range
      if (strcmp(loc.file_name.file_name, file->io_file.path_abs) == 0 &&
          isPositionInRange(goto_payload->pos, loc.range)) {
        // Skip this location
        continue;
      }
      location_array->items[filtered_count++] = loc;
    }
    location_array->size = filtered_count;
  }

  // fall back
  if (location_array->size == 0) {
    if (goto_payload != NULL) {
      if (goto_payload->goto_type == LSP_GOTO_DEFINITION) {
        LSP_requestGoto(lsp, file->io_file.path_abs, goto_payload->pos.row, goto_payload->pos.column, LSP_FIND_REFERENCE);
      }
      else if (goto_payload->goto_type == LSP_FIND_REFERENCE) {
        LSP_requestGoto(lsp, file->io_file.path_abs, goto_payload->pos.row, goto_payload->pos.column, LSP_GOTO_DECLARATION);
      }
      else if (goto_payload->goto_type == LSP_GOTO_DECLARATION) {
        LSP_requestGoto(lsp, file->io_file.path_abs, goto_payload->pos.row, goto_payload->pos.column, LSP_GOTO_IMPLEMENTATION);
      }
      else {
        fprintf(stderr, "No location found for %s\n", method);
        gui_closePopup(data->view_port.gui);
      }
      free(payload);
    }
    return;
  }

  if (goto_payload != NULL) {
    file->lsp_datas.computed->goto_type = goto_payload->goto_type;
    free(payload);
  }

  if (location_array->size == 1) {
    jumpToLocation(data, location_array->items[0]);
  }
  else {
    gui_resumeGotoChoice(&data->view_port, data->cursor);
  }
}
