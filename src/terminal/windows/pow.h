#ifndef WISHWIM_POW_H
#define WISHWIM_POW_H
#include "gui_entities.h"

#include "../../io_management/viewport_history.h"

bool gui_resumeHoverInformation(Cursor* cursor, ViewPort* view_port, LSP_Hover *hover);

bool gui_resumeGotoChoice(ViewPort* view_port, Cursor* cursor);

bool gui_resumeCompletionTextAnchor(ViewPort* view_port, Cursor* cursor);

void gui_showGenericPopupWithTextAnchor(ViewPort* view_port, Cursor* cursor, int height, int width, PopupOwner popup_owner);

void gui_showGenericPopup(GUIContext* gui_context, int y, int x, int prefered_height, int prefered_width, PopupOwner popup_owner);

void gui_setLastTextAnchor(GUIContext* gui_context, CursorDescriptor descriptor);

void gui_showDiagnostic(GUIContext* gui_context, int y, int x, LSP_Diagnostic* diagnostic);

void gui_printPopup(EDW_GUIContext* context, Cursor* cursor, LSP_ComputedData* lsp_data);

struct DispatcherPayload;
typedef struct DispatcherPayload DispatcherPayload;

bool gui_handlePopupInput(GUIContext* context, Cursor* cursor, int c_hash, int c_raw, LSP_ComputedData* lsp_data,
                          History** history_p, PayloadStateChange payload_state_change, DispatcherPayload* payload,
                          MEVENT* m_event);

#endif // WISHWIM_POW_H
