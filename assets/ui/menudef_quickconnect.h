// quick connect menu macros
#define QUICK_CONNECT_W 196
#define QUICK_CONNECT_H 246
#define QUICK_CONNECT_X WINDOW_WIDTH_WIDE - QUICK_CONNECT_W - (WINDOW_X * 2)

// inner menu rectangle
#define QUICK_CONNECT_RECT_X (QUICK_CONNECT_X + 6)
#define QUICK_CONNECT_RECT_Y 18
#define QUICK_CONNECT_RECT_W (QUICK_CONNECT_W - 12)
#define QUICK_CONNECT_RECT_H (QUICK_CONNECT_H - 14 - 6 - 6 - 12)

// subwindow title bar offsets
#define QUICKCONNECT_TITLE_H 12
#define QUICKCONNECT_TITLE_MARGIN ( QUICKCONNECT_TITLE_H + 6 )
#define QUICK_CONNECT_MARGIN_Y 2

// server labels
#define QUICK_CONNECT_LABEL_H 10
#define QUICK_CONNECT_LABEL_LINE_H 11
#define QUICK_CONNECT_LABEL_Y(pos) ( QUICK_CONNECT_RECT_Y + QUICK_CONNECT_MARGIN_Y + ((QUICK_CONNECT_LABEL_LINE_H * 2) * (pos - 1)) + ((QUICK_CONNECT_BTN_H + QUICK_CONNECT_MARGIN_Y + 6) * (pos - 1)) )

#define QUICK_CONNECT_LABEL_POS(pos) QUICK_CONNECT_RECT_X + 4, QUICK_CONNECT_LABEL_Y(pos), QUICK_CONNECT_RECT_W, QUICK_CONNECT_LABEL_H

// buttons
#define QUICK_CONNECT_BTN_MARGIN_X 4
#define QUICK_CONNECT_BTN_Y(pos) ( QUICK_CONNECT_RECT_Y + (QUICK_CONNECT_LABEL_LINE_H * (pos * 2)) + ((QUICKCONNECT_TITLE_MARGIN + QUICK_CONNECT_MARGIN_Y) * (pos - 1)) + QUICK_CONNECT_MARGIN_Y )
#define QUICK_CONNECT_BTN_W ( (QUICK_CONNECT_RECT_W - (QUICK_CONNECT_BTN_MARGIN_X * 2) - (QUICK_CONNECT_BTN_MARGIN_X * 2)) / 3 )
#define QUICK_CONNECT_BTN_H 12

#define QUICK_CONNECT_BTN_CONNECT_POS(pos) QUICK_CONNECT_RECT_X + QUICK_CONNECT_BTN_MARGIN_X, QUICK_CONNECT_BTN_Y(pos), QUICK_CONNECT_BTN_W, QUICK_CONNECT_BTN_H
#define QUICK_CONNECT_BTN_EDIT_POS(pos) QUICK_CONNECT_RECT_X + (QUICK_CONNECT_RECT_W / 2) - (QUICK_CONNECT_BTN_W / 2), QUICK_CONNECT_BTN_Y(pos), QUICK_CONNECT_BTN_W, QUICK_CONNECT_BTN_H
#define QUICK_CONNECT_BTN_DELETE_POS(pos) QUICK_CONNECT_RECT_X + QUICK_CONNECT_RECT_W - QUICK_CONNECT_BTN_MARGIN_X - QUICK_CONNECT_BTN_W, QUICK_CONNECT_BTN_Y(pos), QUICK_CONNECT_BTN_W, QUICK_CONNECT_BTN_H
