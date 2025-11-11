#include "shared.h"

#include "../advanced/lsp/lsp_emitter.h"

void globalOnStageChange(Action action, Cursor* cursor, void* payload_p) {
  PayloadStateChange* payload = payload_p;
  onStateChangeTS(action, payload->ts_data);
  onStateChangeLSP(action, payload->lsp_data, cursor);
}

PayloadStateChange getPayloadStateChange(TS_Data* highlight_datas, LSP_Data* lsp_data) {
  PayloadStateChange payload;
  payload.ts_data = highlight_datas;
  payload.lsp_data = lsp_data;
  return payload;
}
