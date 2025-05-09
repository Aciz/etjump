// cg_info.c -- display information while data is being loading

#include "cg_local.h"

/*
===================
CG_DrawLoadingIcons
===================
*/
/*static void CG_DrawLoadingIcons( void ) {
    int		n;
    int		x, y;

    // JOSEPH 5-2-00 Per MAXX
    return;

    for( n = 0; n < loadingPlayerIconCount; n++ ) {
        x = 16 + n * 78;
        y = 324;
        CG_DrawPic( x, y, 64, 64, loadingPlayerIcons[n] );
    }

    for( n = 0; n < loadingItemIconCount; n++ ) {
        y = 400;
        if( n >= 13 ) {
            y += 40;
        }
        x = 16 + n % 13 * 48;
        CG_DrawPic( x, y, 32, 32, loadingItemIcons[n] );
    }
}*/

/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString(const char *s) {
  Q_strncpyz(cg.infoScreenText, s, sizeof(cg.infoScreenText));

  if (s && *s) {
    CG_Printf(va("LOADING... %s\n",
                 s)); //----(SA)	added so you can see
                      // from the console what's going on
  }
  // Arnout: no need for this
  // trap_UpdateScreen();
}

/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation(qboolean forcerefresh) {
  static int lastcalled = 0;

  if (lastcalled && (trap_Milliseconds() - lastcalled < 500)) {
    return;
  }
  lastcalled = trap_Milliseconds();

  if (cg.snap) {
    return; // we are in the world, no need to draw information
  }

  CG_DrawConnectScreen(qfalse, forcerefresh);
}

void CG_ShowHelp_On(int *status) {
  int milli = trap_Milliseconds();

  if (*status == SHOW_SHUTDOWN && milli < cg.fadeTime) {
    cg.fadeTime = 2 * milli + STATS_FADE_TIME - cg.fadeTime;
  } else if (*status != SHOW_ON) {
    cg.fadeTime = milli + STATS_FADE_TIME;
  }

  *status = SHOW_ON;
}

void CG_ShowHelp_Off(int *status) {
  if (*status != SHOW_OFF) {
    int milli = trap_Milliseconds();

    if (milli < cg.fadeTime) {
      cg.fadeTime = 2 * milli + STATS_FADE_TIME - cg.fadeTime;
    } else {
      cg.fadeTime = milli + STATS_FADE_TIME;
    }

    *status = SHOW_SHUTDOWN;
  }
}

// Demo playback key catcher support
void CG_DemoClick(int key, qboolean down) {
  int milli = trap_Milliseconds();

  if (key & K_CHAR_FLAG) {
    return;
  }

  key &= ~K_CHAR_FLAG;

  // Avoid active console keypress issues
  if (!down && !cgs.fKeyPressed[key]) {
    return;
  }

  cgs.fKeyPressed[key] = down;

  switch (key) {
    case K_TAB:
      if (down) {
        CG_ScoresDown_f();
      } else {
        CG_ScoresUp_f();
      }
      return;

    // Help info
    case K_BACKSPACE:
      if (!down) {
        if (cg.demohelpWindow != SHOW_ON) {
          CG_ShowHelp_On(&cg.demohelpWindow);
        } else {
          CG_ShowHelp_Off(&cg.demohelpWindow);
        }
      }
      return;

    // Screenshot keys
    case K_F11:
      if (!down) {
        trap_SendConsoleCommand(va(
            "screenshot%s\n", ((cg_useScreenshotJPEG.integer) ? "JPEG" : "")));
      }
      return;
    case K_F12:
      if (!down) {
        CG_autoScreenShot_f();
      }
      return;

    // Window controls
    case K_SHIFT:
      if (cgs.demoCam.renderingFreeCam) {
        CG_RunBinding(key, down);
        return;
      }
      // fall through
    case K_CTRL:
    case K_MOUSE4:
      cgs.fResize = down;
      return;
    case K_MOUSE1:
      cgs.fSelect = down;
      return;
    case K_MOUSE2:
      if (!down) {
        CG_mvSwapViews_f(); // Swap the window
                            // with the main
                            // view
      }
      return;
    case K_INS:
    case K_KP_PGUP:
      if (!down) {
        CG_mvShowView_f(); // Make a window for
                           // the client
      }
      return;
    case K_DEL:
    case K_KP_PGDN:
      if (!down) {
        CG_mvHideView_f(); // Delete the window
                           // for the client
      }
      return;
    case K_MOUSE3:
      if (!down) {
        CG_mvToggleView_f(); // Toggle a window
                             // for the client
      }
      return;

    // Third-person controls
    case K_ENTER:
      if (!down) {
        if (cgs.demoCam.renderingFreeCam) {
          cgs.demoCam.renderingFreeCam = qfalse;
        }

        trap_Cvar_Set("cg_thirdperson",
                      ((cg_thirdPerson.integer == 0) ? "1" : "0"));
      }
      return;
    case K_UPARROW:
    case K_DOWNARROW:

      // when not in thirdperson, arrowkeys should be
      // bindable to +freecam_turnleft, ...
      if (cg.renderingThirdPerson && !cgs.demoCam.renderingFreeCam) {
        if (milli > cgs.thirdpersonUpdate) {
          float range = cg_thirdPersonRange.value;

          cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;
          if (key == K_UPARROW) {
            range -=
                ((range >= 4 * DEMO_RANGEDELTA) ? DEMO_RANGEDELTA
                                                : (range - DEMO_RANGEDELTA));
          } else {
            range += ((range >= 120 * DEMO_RANGEDELTA) ? 0 : DEMO_RANGEDELTA);
          }
          trap_Cvar_Set("cg_thirdPersonRange", va("%f", range));
        }
      } else {
        CG_RunBinding(key, down);
      }
      return;
    case K_RIGHTARROW:
    case K_LEFTARROW:
      if (cg.renderingThirdPerson && !cgs.demoCam.renderingFreeCam) {
        if (milli > cgs.thirdpersonUpdate) {
          float angle = cg_thirdPersonAngle.value;
          cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;
          if (key == K_RIGHTARROW) {
            angle -= DEMO_ANGLEDELTA;
            if (angle < 0) {
              angle += 360.0f;
            }
          } else {
            angle += DEMO_ANGLEDELTA;
            if (angle >= 360.0f) {
              angle -= 360.0f;
            }
          }
          trap_Cvar_Set("cg_thirdPersonAngle", va("%f", angle));
        }
      } else {
        CG_RunBinding(key, down);
      }
      return;
    // Timescale controls
    case K_KP_5:
    case K_SPACE:
      if (cgs.demoCam.renderingFreeCam) {
        CG_RunBinding(key, down);
        return;
      }
      // fall through
    case K_ESCAPE:
      if (!down) {
        trap_Cvar_Set("timescale", "1");
        cgs.timescaleUpdate = cg.time + 1000;
      }
      return;
    case K_KP_ENTER:
      if (!down) {
        if (cg.snap->ps.leanf && !cgs.demoCam.renderingFreeCam) {
          cgs.demoCam.startLean = qtrue;
        }

        cgs.demoCam.renderingFreeCam =
            cgs.demoCam.renderingFreeCam ? qfalse : qtrue;

        if (cgs.demoCam.renderingFreeCam) {
          int viewheight;

          if (cg.snap->ps.eFlags & EF_CROUCHING) {
            viewheight = CROUCH_VIEWHEIGHT;
          } else if (cg.snap->ps.eFlags & EF_PRONE ||
                     cg.snap->ps.eFlags & EF_PRONE_MOVING) {
            viewheight = PRONE_VIEWHEIGHT;
          } else {
            viewheight = DEFAULT_VIEWHEIGHT;
          }
          cgs.demoCam.camOrigin[2] += viewheight;
        }
      }
      return;
    case K_KP_DOWNARROW:
      if (!down) {
        float tscale = cg_timescale.value;

        if (tscale <= 1.1f) {
          if (tscale > 0.1f) {
            tscale -= 0.1f;
          }
        } else {
          tscale -= 1.0;
        }
        trap_Cvar_Set("timescale", va("%f", tscale));
        cgs.timescaleUpdate = cg.time + (int)(1000.0f * tscale);
      }
      return;
    case K_MWHEELDOWN:
      if (!cgs.fKeyPressed[K_SHIFT]) {
        if (!down) {
          CG_ZoomOut_f();
        }
        return;
      } // Roll over into timescale changes
        // fall through
    case K_KP_LEFTARROW:
      if (!down && cg_timescale.value > 0.1f) {
        trap_Cvar_Set("timescale", va("%f", cg_timescale.value - 0.1f));
        cgs.timescaleUpdate =
            cg.time + (int)(1000.0f * cg_timescale.value - 0.1f);
      }
      return;
    case K_KP_UPARROW:
      if (!down) {
        trap_Cvar_Set("timescale", va("%f", cg_timescale.value + 1.0f));
        cgs.timescaleUpdate =
            cg.time + (int)(1000.0f * cg_timescale.value + 1.0f);
      }
      return;
    case K_MWHEELUP:
      if (!cgs.fKeyPressed[K_SHIFT]) {
        if (!down) {
          CG_ZoomIn_f();
        }
        return;
      } // Roll over into timescale changes
        // fall through
    case K_KP_RIGHTARROW:
      if (!down) {
        trap_Cvar_Set("timescale", va("%f", cg_timescale.value + 0.1f));
        cgs.timescaleUpdate =
            cg.time + (int)(1000.0f * cg_timescale.value + 0.1f);
      }
      return;

    // AVI recording controls
    case K_F1:
      if (down) {
        cgs.aviDemoRate = demo_avifpsF1.integer;
      } else {
        trap_Cvar_Set("cl_avidemo", demo_avifpsF1.string);
      }
      return;
    case K_F2:
      if (down) {
        cgs.aviDemoRate = demo_avifpsF2.integer;
      } else {
        trap_Cvar_Set("cl_avidemo", demo_avifpsF2.string);
      }
      return;
    case K_F3:
      if (down) {
        cgs.aviDemoRate = demo_avifpsF3.integer;
      } else {
        trap_Cvar_Set("cl_avidemo", demo_avifpsF3.string);
      }
      return;
    case K_F4:
      if (down) {
        cgs.aviDemoRate = demo_avifpsF4.integer;
      } else {
        trap_Cvar_Set("cl_avidemo", demo_avifpsF4.string);
      }
      return;
    case K_F5:
      if (down) {
        cgs.aviDemoRate = demo_avifpsF5.integer;
      } else {
        trap_Cvar_Set("cl_avidemo", demo_avifpsF5.string);
      }
      return;
    default:
      CG_RunBinding(key, down);
  }
}

//
// Color/font info used for all overlays (below)
//
#define COLOR_BG {0.0f, 0.0f, 0.0f, 0.6f}
#define COLOR_BORDER {0.5f, 0.5f, 0.5f, 0.5f}
#define COLOR_BG_TITLE {0.16f, 0.2f, 0.17f, 0.8f}
#define COLOR_BG_VIEW {0.16f, 0.2f, 0.17f, 0.8f}
#define COLOR_BORDER_TITLE {0.1f, 0.1f, 0.1f, 0.2f}
#define COLOR_BORDER_VIEW {0.2f, 0.2f, 0.2f, 0.4f}
#define COLOR_HDR {0.6f, 0.6f, 0.6f, 1.0f}
#define COLOR_HDR2 {0.6f, 0.6f, 0.4f, 1.0f}
#define COLOR_TEXT {0.625f, 0.625f, 0.6f, 1.0f}

inline constexpr fontInfo_t *FONT_HEADER = &cgs.media.limboFont1;
inline constexpr fontInfo_t *FONT_SUBHEADER = &cgs.media.limboFont1_lo;
inline constexpr fontInfo_t *FONT_TEXT = &cgs.media.limboFont2;

vec4_t color_bg = COLOR_BG_VIEW;
vec4_t color_border = COLOR_BORDER_VIEW;
vec4_t color_hdr = COLOR_HDR2;
vec4_t color_name = COLOR_TEXT;

inline constexpr float VD_X = 4;
inline constexpr float VD_Y = 78;
inline constexpr float VD_SCALE_X_HDR = 0.25f;
inline constexpr float VD_SCALE_Y_HDR = 0.30f;
inline constexpr float VD_SCALE_X_NAME = 0.30f;
inline constexpr float VD_SCALE_Y_NAME = 0.30f;

qboolean CG_ViewingDraw() {
  if (cg.mvTotalClients < 1) {
    return (qfalse);

  } else {
    int w, wTag;
    int tSpacing = 15; // Should derive from CG_Text_Height_Ext
    int pID = cg.mvCurrentMainview->mvInfo & MV_PID;
    const char *viewInfo = "Viewing:";

    wTag = CG_Text_Width_Ext(viewInfo, VD_SCALE_X_HDR, 0, FONT_HEADER);
    w = wTag + 3 +
        CG_Text_Width_Ext(cgs.clientinfo[pID].name, VD_SCALE_X_NAME, 0,
                          FONT_TEXT);

    CG_DrawRect(VD_X - 2, VD_Y, w + 7, tSpacing + 4, 1, color_border);
    CG_FillRect(VD_X - 2, VD_Y, w + 7, tSpacing + 4, color_bg);

    CG_Text_Paint_Ext(VD_X, VD_Y + tSpacing, // x, y
                      VD_SCALE_X_HDR,
                      VD_SCALE_Y_HDR, // scale_x, scale_y
                      color_hdr, viewInfo, 0.0f, 0, ITEM_TEXTSTYLE_SHADOWED,
                      FONT_HEADER);

    CG_Text_Paint_Ext(VD_X + wTag + 5, VD_Y + tSpacing, // x, y
                      VD_SCALE_X_NAME, VD_SCALE_Y_NAME, // scale_x, scale_y
                      color_name, cgs.clientinfo[pID].name, 0.0f, 0,
                      ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);

    return (qtrue);
  }
}

#define GS_X (166 + SCREEN_OFFSET_X)
inline constexpr float GS_Y = 10.0f;
inline constexpr float GS_W = 308.0f;

void CG_GameStatsDraw() {
  if (cgs.gamestats.show == SHOW_OFF) {
    return;

  } else {
    int i, x = GS_X + 4, y = GS_Y, h;
    gameStats_t *gs = &cgs.gamestats;

    vec4_t bgColor = COLOR_BG;         // window
    vec4_t borderColor = COLOR_BORDER; // window

    vec4_t bgColorTitle = COLOR_BG_TITLE;         // titlebar
    vec4_t borderColorTitle = COLOR_BORDER_TITLE; // titlebar

    // Main header
    int hStyle = ITEM_TEXTSTYLE_SHADOWED;
    float hScale = 0.16f;
    float hScaleY = 0.21f;
    fontInfo_t *hFont = FONT_HEADER;

    // Sub header
    int hStyle2 = 0;
    float hScale2 = 0.16f;
    float hScaleY2 = 0.20f;
    fontInfo_t *hFont2 = FONT_SUBHEADER;

    vec4_t hdrColor = COLOR_HDR; // text
                                 //		vec4_t hdrColor2
                                 //= COLOR_HDR2;	// text

    // Text settings
    int tStyle = ITEM_TEXTSTYLE_SHADOWED;
    int tSpacing = 9; // Should derive from CG_Text_Height_Ext
    float tScale = 0.19f;
    fontInfo_t *tFont = FONT_TEXT;
    vec4_t tColor = COLOR_TEXT; // text

    float diff = cgs.gamestats.fadeTime - cg.time;

    // FIXME: Should compute this beforehand
    h = 2 + tSpacing + 2 +     // Header
        2 + 2 + tSpacing + 2 + // Stats columns
        1 +                    // Stats + extra
        tSpacing * ((gs->cWeapons > 0) ? gs->cWeapons : 1) +
        tSpacing * ((gs->fHasStats) ? 3 : 0) +
        (4 + 2 * tSpacing +                // Rank/XP
         1 + tSpacing + 4 + 2 * tSpacing + // Skill columns
         1 +                               // Skillz
         tSpacing * ((gs->cSkills > 0) ? gs->cSkills : 1)) +
        2;

    // Fade-in effects
    if (diff > 0.0f) {
      float scale = (diff / STATS_FADE_TIME);

      if (cgs.gamestats.show == SHOW_ON) {
        scale = 1.0f - scale;
      }

      bgColor[3] *= scale;
      bgColorTitle[3] *= scale;
      borderColor[3] *= scale;
      borderColorTitle[3] *= scale;
      hdrColor[3] *= scale;
      tColor[3] *= scale;

      y -= h * (1.0f - scale);

    } else if (cgs.gamestats.show == SHOW_SHUTDOWN) {
      cgs.gamestats.show = SHOW_OFF;
      return;
    }

    CG_DrawRect(GS_X, y, GS_W, h, 1, borderColor);
    CG_FillRect(GS_X, y, GS_W, h, bgColor);

    // Header
    CG_FillRect(GS_X, y, GS_W, tSpacing + 4, bgColorTitle);
    CG_DrawRect(GS_X, y, GS_W, tSpacing + 4, 1, borderColorTitle);

    y += 1;
    y += tSpacing;
    CG_Text_Paint_Ext(x, y, hScale, hScaleY, hdrColor, "PLAYER STATS", 0.0f, 0,
                      hStyle, hFont);
    y += 3;

    y += 2;

    // Weapon stats
    y += 2;
    CG_FillRect(GS_X, y, GS_W, tSpacing + 3, bgColorTitle);
    CG_DrawRect(GS_X, y, GS_W, tSpacing + 3, 1, borderColorTitle);

    y += 1 + tSpacing;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Weapon", 0.0f, 0,
                      hStyle2, hFont2);
    x += 66;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Accuracy", 0.0f, 0,
                      hStyle2, hFont2);
    x += 53;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Hits / Shots", 0.0f,
                      0, hStyle2, hFont2);
    x += 62;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Kills", 0.0f, 0,
                      hStyle2, hFont2);
    x += 29;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Deaths", 0.0f, 0,
                      hStyle2, hFont2);
    x += 40;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Headshots", 0.0f, 0,
                      hStyle2, hFont2);

    x = GS_X + 4;
    y += 2;

    y += 1;
    if (gs->cWeapons == 0) {
      y += tSpacing;
      CG_Text_Paint_Ext(x, y, tScale, tScale, tColor,
                        "No weapon info available.", 0.0f, 0, tStyle, tFont);
    } else {
      for (i = 0; i < gs->cWeapons; i++) {
        y += tSpacing;
        CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, gs->strWS[i], 0.0f, 0,
                          tStyle, tFont);
      }

      if (gs->fHasStats) {
        y += tSpacing;
        for (i = 0; i < 2; i++) {
          y += tSpacing;
          CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, gs->strExtra[i], 0.0f,
                            0, tStyle, tFont);
        }
      }
    }
  }
}

#define TS_X (SCREEN_WIDTH - 20)      // spacing from right
inline constexpr float TS_Y = -60.0f; // spacing from bottom
inline constexpr float TS_W = 308.0f;

void CG_TopShotsDraw() {
  if (cgs.topshots.show == SHOW_OFF) {
    return;

  } else {
    int i, x = TS_X - TS_W, y = 480, h;
    topshotStats_t *ts = &cgs.topshots;

    vec4_t bgColor = COLOR_BG;         // window
    vec4_t borderColor = COLOR_BORDER; // window

    vec4_t bgColorTitle = COLOR_BG_TITLE;         // titlebar
    vec4_t borderColorTitle = COLOR_BORDER_TITLE; // titlebar

    // Main header
    int hStyle = ITEM_TEXTSTYLE_SHADOWED;
    float hScale = 0.16f;
    float hScaleY = 0.21f;
    fontInfo_t *hFont = FONT_HEADER;

    // Sub header
    int hStyle2 = 0;
    float hScale2 = 0.16f;
    float hScaleY2 = 0.20f;
    fontInfo_t *hFont2 = FONT_SUBHEADER;

    vec4_t hdrColor = COLOR_HDR;   // text
    vec4_t hdrColor2 = COLOR_HDR2; // text

    // Text settings
    int tStyle = ITEM_TEXTSTYLE_SHADOWED;
    int tSpacing = 9; // Should derive from CG_Text_Height_Ext
    float tScale = 0.19f;
    fontInfo_t *tFont = FONT_TEXT;
    vec4_t tColor = COLOR_TEXT; // text

    float diff = cgs.topshots.fadeTime - cg.time;

    // FIXME: Should compute this beforehand
    h = 2 + tSpacing + 2 +     // Header
        2 + 2 + tSpacing + 2 + // Stats columns
        1 +                    // Stats + extra
        tSpacing * ((ts->cWeapons > 0) ? ts->cWeapons : 1) + 1;

    // Fade-in effects
    if (diff > 0.0f) {
      float scale = (diff / STATS_FADE_TIME);

      if (cgs.topshots.show == SHOW_ON) {
        scale = 1.0f - scale;
      }

      bgColor[3] *= scale;
      bgColorTitle[3] *= scale;
      borderColor[3] *= scale;
      borderColorTitle[3] *= scale;
      hdrColor[3] *= scale;
      hdrColor2[3] *= scale;
      tColor[3] *= scale;

      y += (TS_Y - h) * scale;

    } else if (cgs.topshots.show == SHOW_SHUTDOWN) {
      cgs.topshots.show = SHOW_OFF;
      return;
    } else {
      y += TS_Y - h;
    }

    CG_DrawRect(x, y, TS_W, h, 1, borderColor);
    CG_FillRect(x, y, TS_W, h, bgColor);

    // Header
    CG_FillRect(x, y, TS_W, tSpacing + 4, bgColorTitle);
    CG_DrawRect(x, y, TS_W, tSpacing + 4, 1, borderColorTitle);

    y += 1;
    y += tSpacing;
    CG_Text_Paint_Ext(x + 4, y, hScale, hScaleY, hdrColor,
                      "\"TOPSHOT\" ACCURACIES", 0.0f, 0, hStyle, hFont);
    y += 4;

    // Weapon stats
    y += 2;
    CG_FillRect(x, y, TS_W, tSpacing + 3, bgColorTitle);
    CG_DrawRect(x, y, TS_W, tSpacing + 3, 1, borderColorTitle);

    x += 4;
    y += 1 + tSpacing;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Weapon", 0.0f, 0,
                      hStyle2, hFont2);
    x += 60;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Accuracy", 0.0f, 0,
                      hStyle2, hFont2);
    x += 53;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Hits / Shots", 0.0f,
                      0, hStyle2, hFont2);
    x += 62;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Kills", 0.0f, 0,
                      hStyle2, hFont2);
    x += 32;
    CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, "Player", 0.0f, 0,
                      hStyle2, hFont2);

    x = TS_X - TS_W + 4;
    y += 1;

    if (ts->cWeapons == 0) {
      y += tSpacing;
      CG_Text_Paint_Ext(x, y, tScale, tScale, tColor,
                        "No qualifying weapon info available.", 0.0f, 0, tStyle,
                        tFont);
    } else {
      for (i = 0; i < ts->cWeapons; i++) {
        y += tSpacing;
        CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, ts->strWS[i], 0.0f, 0,
                          tStyle, tFont);
      }
    }
  }
}

#define DH_X (SCREEN_WIDTH - 20) // spacing from right
constexpr float DH_Y = -60.0f;   // spacing from bottom
constexpr float DH_W = 148.0f;

void CG_DemoHelpDraw() {
  if (cg.demohelpWindow == SHOW_OFF) {
    return;

  } else {
    const char *help[] = {
        "^nTAB       ^mscores",
        "^nF1-F5     ^mavidemo record",
        "^nF11-F12   ^mscreenshot",
        NULL,
        "^nKP_DOWN   ^mslow down (--)",
        "^nKP_LEFT   ^mslow down (-)",
        "^nKP_UP     ^mspeed up (++)",
        "^nKP_RIGHT  ^mspeed up (+)",
        "^nSPACE     ^mnormal speed",
        NULL,
        "^nENTER     ^mExternal view",
        "^nLFT/RGHT  ^mChange angle",
        "^nUP/DOWN   ^mMove in/out",
        NULL,
        va("^nKP_ENTER  ^mFreecam    ^m%s",
           cgs.demoCam.renderingFreeCam ? "ON" : "OFF"),
    };

    const char *mvhelp[] = {NULL,
                            "^nMOUSE1    ^mSelect/move view",
                            "^nMOUSE2    ^mSwap w/main view",
                            "^nMOUSE3    ^mToggle on/off",
                            "^nSHIFT     ^mHold to resize",
                            "^nKP_PGUP   ^mEnable a view",
                            "^nKP_PGDN   ^mClose a view"};

    int i, x, y = 480, w, h;

    vec4_t bgColor = COLOR_BG;         // window
    vec4_t borderColor = COLOR_BORDER; // window

    vec4_t bgColorTitle = COLOR_BG_TITLE;         // titlebar
    vec4_t borderColorTitle = COLOR_BORDER_TITLE; // titlebar

    // Main header
    int hStyle = ITEM_TEXTSTYLE_SHADOWED;
    float hScale = 0.16f;
    float hScaleY = 0.21f;
    fontInfo_t *hFont = FONT_HEADER;
    vec4_t hdrColor2 = COLOR_HDR2; // text

    // Text settings
    int tStyle = ITEM_TEXTSTYLE_SHADOWED;
    int tSpacing = 9; // Should derive from CG_Text_Height_Ext
    float tScale = 0.19f;
    fontInfo_t *tFont = FONT_TEXT;
    vec4_t tColor = COLOR_TEXT; // text

    float diff = cg.fadeTime - trap_Milliseconds();

    // FIXME: Should compute this beforehand
    w = DH_W + ((cg.mvTotalClients > 1) ? 12 : 0);
    x = DH_X - w;
    h = 2 + tSpacing + 2 + // Header
        2 + 1 +
        tSpacing * (2 + (sizeof(help) +
                         ((cg.mvTotalClients > 1) ? sizeof(mvhelp) : 0)) /
                            sizeof(char *)) +
        2;

    // Fade-in effects
    if (diff > 0.0f) {
      float scale = (diff / STATS_FADE_TIME);

      if (cg.demohelpWindow == SHOW_ON) {
        scale = 1.0f - scale;
      }

      bgColor[3] *= scale;
      bgColorTitle[3] *= scale;
      borderColor[3] *= scale;
      borderColorTitle[3] *= scale;
      hdrColor2[3] *= scale;
      tColor[3] *= scale;

      y += (DH_Y - h) * scale;

    } else if (cg.demohelpWindow == SHOW_SHUTDOWN) {
      cg.demohelpWindow = SHOW_OFF;
      return;
    } else {
      y += DH_Y - h;
    }

    CG_DrawRect(x, y, w, h, 1, borderColor);
    CG_FillRect(x, y, w, h, bgColor);

    // Header
    CG_FillRect(x, y, w, tSpacing + 4, bgColorTitle);
    CG_DrawRect(x, y, w, tSpacing + 4, 1, borderColorTitle);

    x += 4;
    y += 1;
    y += tSpacing;
    CG_Text_Paint_Ext(x, y, hScale, hScaleY, hdrColor2, "DEMO CONTROLS", 0.0f,
                      0, hStyle, hFont);
    y += 3;

    // Control info
    for (i = 0; i < static_cast<int>(sizeof(help) / sizeof(char *)); i++) {
      y += tSpacing;
      if (help[i] != NULL) {
        CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, help[i], 0.0f, 0,
                          tStyle, tFont);
      }
    }

    if (cg.mvTotalClients > 1) {
      for (i = 0; i < static_cast<int>(sizeof(mvhelp) / sizeof(char *)); i++) {
        y += tSpacing;
        if (mvhelp[i] != NULL) {
          CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, mvhelp[i], 0.0f, 0,
                            tStyle, tFont);
        }
      }
    }

    y += tSpacing * 2;
    CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, "^nBACKSPACE ^mhelp on/off",
                      0.0f, 0, tStyle, tFont);
  }
}

char *CG_getBindKeyName(const char *cmd, char *buf, int len) {
  int j;

  for (j = 0; j < 256; j++) {
    trap_Key_GetBindingBuf(j, buf, len);
    if (*buf == 0) {
      continue;
    }

    if (!Q_stricmp(buf, cmd)) {
      trap_Key_KeynumToStringBuf(j, buf, MAX_STRING_TOKENS);
      Q_strupr(buf);
      return (buf);
    }
  }

  Q_strncpyz(buf, va("(%s)", cmd), len);
  return (buf);
}

typedef struct {
  const char *cmd;
  const char *info;
} helpType_t;

inline constexpr float SH_X = 2.0f;   // spacing from left
inline constexpr float SH_Y = 155.0f; // spacing from top

// FIXME: remove
void CG_SpecHelpDraw() {
  if (cg.spechelpWindow == SHOW_OFF) {
    return;

  } else {
    const helpType_t help[] = {
        {"+zoom", "hold for pointer"},    {"+attack", "window move/resize"},
        {"+sprint", "hold to resize"},    {"weapnext", "window on/off"},
        {"weapprev", "swap w/main view"}, {NULL, NULL},
        {"weapalt", "swingcam toggle"},   {"spechelp", "help on/off"}};

    int i, x, y = 480, w, h;
    int len, maxlen = 0;
    char format[MAX_STRING_TOKENS], buf[MAX_STRING_TOKENS];
    char *lines[16];

    vec4_t bgColor = COLOR_BG;         // window
    vec4_t borderColor = COLOR_BORDER; // window

    vec4_t bgColorTitle = COLOR_BG_TITLE;         // titlebar
    vec4_t borderColorTitle = COLOR_BORDER_TITLE; // titlebar

    // Main header
    int hStyle = ITEM_TEXTSTYLE_SHADOWED;
    float hScale = 0.16f;
    float hScaleY = 0.21f;
    fontInfo_t *hFont = FONT_HEADER;
    vec4_t hdrColor2 = COLOR_HDR2; // text

    // Text settings
    int tStyle = ITEM_TEXTSTYLE_SHADOWED;
    int tSpacing = 9; // Should derive from CG_Text_Height_Ext
    float tScale = 0.19f;
    fontInfo_t *tFont = FONT_TEXT;
    vec4_t tColor = COLOR_TEXT; // text

    float diff = cg.fadeTime - trap_Milliseconds();

    // FIXME: Should compute all this stuff beforehand
    // Compute required width
    for (i = 0; i < static_cast<int>(sizeof(help) / sizeof(helpType_t)); i++) {
      if (help[i].cmd != NULL) {
        len = strlen(CG_getBindKeyName(help[i].cmd, buf, sizeof(buf)));
        if (len > maxlen) {
          maxlen = len;
        }
      }
    }

    Q_strncpyz(format, va("^2%%%ds ^N%%s", maxlen), sizeof(format));
    for (i = 0, maxlen = 0;
         i < static_cast<int>(sizeof(help) / sizeof(helpType_t)); i++) {
      if (help[i].cmd != NULL) {
        lines[i] = va(format, CG_getBindKeyName(help[i].cmd, buf, sizeof(buf)),
                      help[i].info);
        len = CG_Text_Width_Ext(lines[i], tScale, 0, FONT_TEXT);
        if (len > maxlen) {
          maxlen = len;
        }
      } else {
        lines[i] = NULL;
      }
    }

    w = maxlen + 8;
    x = SH_X;
    y = SH_Y;
    h = 2 + tSpacing + 2 + // Header
        2 + 1 + tSpacing * (sizeof(help) / sizeof(helpType_t)) + 2;

    // Fade-in effects
    if (diff > 0.0f) {
      float scale = (diff / STATS_FADE_TIME);

      if (cg.spechelpWindow == SHOW_ON) {
        scale = 1.0f - scale;
      }

      bgColor[3] *= scale;
      bgColorTitle[3] *= scale;
      borderColor[3] *= scale;
      borderColorTitle[3] *= scale;
      hdrColor2[3] *= scale;
      tColor[3] *= scale;

      x -= w * (1.0f - scale);

    } else if (cg.spechelpWindow == SHOW_SHUTDOWN) {
      cg.spechelpWindow = SHOW_OFF;
      return;
    }

    CG_DrawRect(x, y, w, h, 1, borderColor);
    CG_FillRect(x, y, w, h, bgColor);

    // Header
    CG_FillRect(x, y, w, tSpacing + 4, bgColorTitle);
    CG_DrawRect(x, y, w, tSpacing + 4, 1, borderColorTitle);

    x += 4;
    y += 1;
    y += tSpacing;
    CG_Text_Paint_Ext(x, y, hScale, hScaleY, hdrColor2, "SPECTATOR CONTROLS",
                      0.0f, 0, hStyle, hFont);
    y += 3;

    // Control info
    for (i = 0; i < static_cast<int>(sizeof(help) / sizeof(helpType_t)); i++) {
      y += tSpacing;
      if (lines[i] != NULL) {
        CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, lines[i], 0.0f, 0,
                          tStyle, tFont);
      }
    }
  }
}

void CG_DrawOverlays(void) {
  CG_GameStatsDraw();
  CG_TopShotsDraw();
#ifdef MV_SUPPORT
  CG_SpecHelpDraw();
#endif
  if (cg.demoPlayback && etj_predefineddemokeys.integer) {
    CG_DemoHelpDraw();
  }
}
