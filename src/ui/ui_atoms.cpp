// Copyright (C) 1999-2000 Id Software, Inc.
//
/**********************************************************************
    UI_ATOMS.C

    User interface building blocks and support functions.
**********************************************************************/
#include "etj_demo_queue.h"
#include "ui_local.h"

uiStatic_t uis;
qboolean m_entersound; // after a frame, so caching won't disrupt the sound

// these are here so the functions in q_shared.c can link

inline constexpr int MAXPRINTMSG = 4096;

void QDECL Com_DPrintf(const char *fmt, ...) {
  va_list argptr;
  char msg[MAXPRINTMSG];
  int developer;

  developer = trap_Cvar_VariableValue("developer");
  if (!developer) {
    return;
  }

  va_start(argptr, fmt);
  Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
  va_end(argptr);

  Com_Printf("%s", msg);
}
// jpw

[[noreturn]] void QDECL Com_Error(int level, const char *error, ...) {
  va_list argptr;
  char text[1024];

  va_start(argptr, error);
  Q_vsnprintf(text, sizeof(text), error, argptr);
  va_end(argptr);

  trap_Error(va("%s", text));
}

void QDECL Com_Printf(const char *msg, ...) {
  va_list argptr;
  char text[1024];

  va_start(argptr, msg);
  Q_vsnprintf(text, sizeof(text), msg, argptr);
  va_end(argptr);

  trap_Print(va("%s", text));
}

// prints only in localhost
void QDECL Com_LocalPrintf(const char *msg, ...) {
  if (trap_Cvar_VariableValue("sv_running") == 0) {
    return;
  }

  va_list argptr;
  char text[1024];

  va_start(argptr, msg);
  Q_vsnprintf(text, sizeof(text), msg, argptr);
  va_end(argptr);

  trap_Print(va("%s", text));
}

/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar(float min, float max, float value) {
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}

/*
// TTimo: unused
static void NeedCDAction( qboolean result ) {
    if ( !result ) {
        trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
    }
}

static void NeedCDKeyAction( qboolean result ) {
    if ( !result ) {
        trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
    }
}
*/

char *UI_Argv(int arg) {
  static char buffer[MAX_STRING_CHARS];

  trap_Argv(arg, buffer, sizeof(buffer));

  return buffer;
}

char *UI_Cvar_VariableString(const char *var_name) {
  static char buffer[2][MAX_STRING_CHARS];
  static int toggle;

  toggle ^= 1; // flip-flop to allow two returns without clash

  trap_Cvar_VariableStringBuffer(var_name, buffer[toggle], sizeof(buffer[0]));

  return buffer[toggle];
}

void UI_LoadBestScores(const char *map, int game) {}

/*
===============
UI_ClearScores
===============
*/
void UI_ClearScores() {}

static void UI_Cache_f() { Display_CacheAll(); }

/*
=======================
UI_CalcPostGameStats
=======================
*/
static void UI_CalcPostGameStats() {}

/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand(const int realTime) {
  const char *cmd = UI_Argv(0);
  uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
  uiInfo.uiDC.realTime = realTime;

  if (Q_stricmp(cmd, "ui_test") == 0) {
    UI_ShowPostGame(qtrue);
  }

  if (Q_stricmp(cmd, "ui_report") == 0) {
    UI_Report();
    return qtrue;
  }

  if (Q_stricmp(cmd, "ui_load") == 0) {
    UI_Load();
    return qtrue;
  }

  if (Q_stricmp(cmd, "postgame") == 0) {
    UI_CalcPostGameStats();
    return qtrue;
  }

  if (Q_stricmp(cmd, "ui_cache") == 0) {
    UI_Cache_f();
    return qtrue;
  }

  if (Q_stricmp(cmd, "ui_teamOrders") == 0) {
    // UI_TeamOrdersMenu_f();
    return qtrue;
  }

  if (Q_stricmp(cmd, "ui_cdkey") == 0) {
    // UI_CDKeyMenu_f();
    return qtrue;
  }

  if (Q_stricmp(cmd, "iamacheater") == 0) {
    return qtrue;
  }

  if (!Q_stricmp(cmd, "uiParseMaplist")) {
    ETJump::parseMaplist();
    return qtrue;
  }

  if (!Q_stricmp(cmd, "uiNumCustomvotes")) {
    ETJump::parseNumCustomvotes();
    return qtrue;
  }

  if (!Q_stricmp(cmd, "uiParseCustomvote")) {
    ETJump::parseCustomvote();
    return qtrue;
  }

  if (!Q_stricmp(cmd, "uiResetCustomvotes")) {
    ETJump::resetCustomvotes();
    return qtrue;
  }

  if (!Q_stricmp(cmd, "demoQueue")) {
    ETJump::demoQueue->commandHandler();
    return qtrue;
  }

  if (!Q_stricmp(cmd, "uiDemoQueueManualSkip")) {
    ETJump::demoQueue->setManualSkip();
    return qtrue;
  }

  if (!Q_stricmp(cmd, "uiDemoPlaybackEnabled")) {
    uiInfo.demoPlayback = true;
    return qtrue;
  }

  if (!Q_stricmp(cmd, "uiToggleETJumpSettings")) {
    uiClientState_t cstate;
    trap_GetClientState(&cstate);

    // FIXME: this breaks if 'ui_restart' is performed while in demo playback,
    //  but there's no nice way to make this work for now, it is what it is
    if (cstate.connState == CA_ACTIVE && !uiInfo.demoPlayback) {
      ETJump::toggleSettingsMenu();
    }

    return qtrue;
  }

  // catch this commnand here and do nothing, otherwise we get
  // 'Unknown command "uiChatMenuOpen"' when we use in-game menu
  // button to disconnect from a server as '_UI_KeyEvent' sends this,
  // but cgame isn't loaded anymore to handle the command
  if (!Q_stricmp(cmd, "uiChatMenuOpen")) {
    return qtrue;
  }

  return qfalse;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown(void) {}

/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640(float *x, float *y, float *w, float *h) {
  // expect valid pointers
#if 0
	*x  = *x * uiInfo.uiDC.scale + uiInfo.uiDC.bias;
	*y *= uiInfo.uiDC.scale;
	*w *= uiInfo.uiDC.scale;
	*h *= uiInfo.uiDC.scale;
#endif

  *x *= uiInfo.uiDC.xscale;
  *y *= uiInfo.uiDC.yscale;
  *w *= uiInfo.uiDC.xscale;
  *h *= uiInfo.uiDC.yscale;
}

void UI_DrawNamedPic(float x, float y, float width, float height,
                     const char *picname) {
  qhandle_t hShader;

  hShader = trap_R_RegisterShaderNoMip(picname);
  UI_AdjustFrom640(&x, &y, &width, &height);
  trap_R_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

void UI_DrawHandlePic(float x, float y, float w, float h, qhandle_t hShader) {
  float s0;
  float s1;
  float t0;
  float t1;

  if (w < 0) // flip about vertical
  {
    w = -w;
    s0 = 1;
    s1 = 0;
  } else {
    s0 = 0;
    s1 = 1;
  }

  if (h < 0) // flip about horizontal
  {
    h = -h;
    t0 = 1;
    t1 = 0;
  } else {
    t0 = 0;
    t1 = 1;
  }

  UI_AdjustFrom640(&x, &y, &w, &h);
  trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, hShader);
}

/*
================
UI_DrawRotatedPic

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRotatedPic(float x, float y, float width, float height,
                       qhandle_t hShader, float angle) {

  UI_AdjustFrom640(&x, &y, &width, &height);

  trap_R_DrawRotatedPic(x, y, width, height, 0, 0, 1, 1, hShader, angle);
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect(float x, float y, float width, float height,
                 const float *color) {
  trap_R_SetColor(color);

  UI_AdjustFrom640(&x, &y, &width, &height);
  trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0,
                        uiInfo.uiDC.whiteShader);

  trap_R_SetColor(NULL);
}

void UI_DrawSides(float x, float y, float w, float h) {
  UI_AdjustFrom640(&x, &y, &w, &h);
  trap_R_DrawStretchPic(x, y, 1, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
  trap_R_DrawStretchPic(x + w - 1, y, 1, h, 0, 0, 0, 0,
                        uiInfo.uiDC.whiteShader);
}

void UI_DrawTopBottom(float x, float y, float w, float h) {
  UI_AdjustFrom640(&x, &y, &w, &h);
  trap_R_DrawStretchPic(x, y, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
  trap_R_DrawStretchPic(x, y + h - 1, w, 1, 0, 0, 0, 0,
                        uiInfo.uiDC.whiteShader);
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect(float x, float y, float width, float height,
                 const float *color) {
  trap_R_SetColor(color);

  UI_DrawTopBottom(x, y, width, height);
  UI_DrawSides(x, y, width, height);

  trap_R_SetColor(NULL);
}

void UI_SetColor(const float *rgba) { trap_R_SetColor(rgba); }

void UI_UpdateScreen(void) { trap_UpdateScreen(); }

void UI_DrawTextBox(int x, int y, int width, int lines) {
  UI_FillRect(x + BIGCHAR_WIDTH / 2, y + BIGCHAR_HEIGHT / 2,
              (width + 1) * BIGCHAR_WIDTH, (lines + 1) * BIGCHAR_HEIGHT,
              colorBlack);
  UI_DrawRect(x + BIGCHAR_WIDTH / 2, y + BIGCHAR_HEIGHT / 2,
              (width + 1) * BIGCHAR_WIDTH, (lines + 1) * BIGCHAR_HEIGHT,
              colorWhite);
}

qboolean UI_CursorInRect(int x, int y, int width, int height) {
  if (uiInfo.uiDC.cursorx < x || uiInfo.uiDC.cursory < y ||
      uiInfo.uiDC.cursorx > x + width || uiInfo.uiDC.cursory > y + height) {
    return qfalse;
  }

  return qtrue;
}
