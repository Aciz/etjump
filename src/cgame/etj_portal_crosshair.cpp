/*
 * MIT License
 *
 * Copyright (c) 2025 ETJump team <zero@etjump.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "etj_portal_crosshair.h"

#include "etj_cvar_update_handler.h"
#include "etj_utilities.h"

#include "../game/etj_portalgun_shared.h"

namespace ETJump {
// base angle for default orientation,
// as the engines texture coordinate system expects
inline constexpr float BASE_ANGLE = 1.0f - (135.0f / 360.0f);

PortalCrosshair::PortalCrosshair()
    : blueShader(cgs.media.portalCrosshairBlueShader),
      redShader(cgs.media.portalCrosshairRedShader) {
  startListeners();

  Vector4Copy(colorWhite, color);
  Vector4Copy(colorWhite, colorAlt);

  adjustSize(etj_portalCrosshairSize);
  adjustOffset(etj_portalCrosshairOffset);
  adjustAngle(etj_portalCrosshairAngle);
  adjustAlpha(etj_portalCrosshairAlpha);
  adjustAlphaAlt(etj_portalCrosshairAlphaAlt);
}

void PortalCrosshair::startListeners() {
  cvarUpdateHandler->subscribe(
      &etj_portalCrosshairSize,
      [this](const vmCvar_t *cvar) { adjustSize(*cvar); });

  cvarUpdateHandler->subscribe(
      &etj_portalCrosshairOffset,
      [this](const vmCvar_t *cvar) { adjustOffset(*cvar); });

  cvarUpdateHandler->subscribe(
      &etj_portalCrosshairAngle,
      [this](const vmCvar_t *cvar) { adjustAngle(*cvar); });

  cvarUpdateHandler->subscribe(
      &etj_portalCrosshairAlpha,
      [this](const vmCvar_t *cvar) { adjustAlpha(*cvar); });

  cvarUpdateHandler->subscribe(
      &etj_portalCrosshairAlphaAlt,
      [this](const vmCvar_t *cvar) { adjustAlphaAlt(*cvar); });
}

void PortalCrosshair::adjustSize(const vmCvar_t &cvar) {
  size = CvarValueParser::parse<CvarValue::Size>(cvar, 0.0f, 256.0f);
  adjustPos();
}

void PortalCrosshair::adjustOffset(const vmCvar_t &cvar) {
  offset = std::clamp(cvar.value, 0.0f, 256.0f);
  adjustPos();
}

void PortalCrosshair::adjustAngle(const vmCvar_t &cvar) {
  angle = std::fmod(BASE_ANGLE + AngleNormalize360(cvar.value) / 360.0f, 1.0f);
  adjustPos();
}

void PortalCrosshair::adjustPos() {
  const float radius = (size.x * 0.5f) + offset;
  const float angleRad = etj_portalCrosshairAngle.value * (M_PI / 180.0f);

  // if we're drawing a rotated pic, the engine forces 1:1 aspect ratio,
  // using the x size as the y size
  // take this into account when calculating the coordinates,
  // if user tries to draw non-square crosshair with rotation
  const float sizeY = angle == BASE_ANGLE ? size.y : size.x;

  // offset by pi since we're on the left side of the screen
  const float blueCenterX =
      SCREEN_CENTER_X + (radius * std::cos(angleRad + M_PI));
  const float blueCenterY =
      SCREEN_CENTER_Y + (radius * std::sin(angleRad + M_PI));
  blueX = blueCenterX - (size.x * 0.5f);
  blueY = blueCenterY - (sizeY * 0.5f);

  const float redCenterX = SCREEN_CENTER_X + (radius * std::cos(angleRad));
  const float redCenterY = SCREEN_CENTER_Y + (radius * std::sin(angleRad));
  redX = redCenterX - (size.x * 0.5f);
  redY = redCenterY - (sizeY * 0.5f);
}

void PortalCrosshair::adjustAlpha(const vmCvar_t &cvar) {
  color[3] = std::clamp(cvar.value, 0.0f, 1.0f);
}

void PortalCrosshair::adjustAlphaAlt(const vmCvar_t &cvar) {
  colorAlt[3] = std::clamp(cvar.value, 0.0f, 1.0f);
}

bool PortalCrosshair::beforeRender() {
  if (canSkipDraw()) {
    return false;
  }

  std::memset(&tr, 0, sizeof(tr));

  VectorCopy(cg.refdef.vieworg, trStart);
  VectorMA(trStart, MAX_MAP_SIZE, cg.refdef.viewaxis[0], trEnd);

  CG_Trace(&tr, trStart, nullptr, nullptr, trEnd, cg.snap->ps.clientNum,
           MASK_PORTAL);

  canShootBlue = true;
  canShootRed = true;

  // if we're aiming at a surface we cannot shoot a portal onto,
  // drop the alpha to indicate that we're aiming at an invalid surface
  if (tr.fraction != 1.0f) {
    if (tr.surfaceFlags & SURF_NOIMPACT ||
        (cgs.shared & BG_LEVEL_PORTAL_SURFACES &&
         tr.surfaceFlags & SURF_PORTALSURFACE) ||
        (!(cgs.shared & BG_LEVEL_PORTAL_SURFACES) &&
         !(tr.surfaceFlags & SURF_PORTALSURFACE))) {
      canShootBlue = false;
      canShootRed = false;
    } else {
      // check if we're aiming at a portal
      // TODO:
      // - handle func_portaltarget
      // - handle portalteam
      // - handle overlap calcs, etj_portalgun.cpp -> portalsOverlap
      //   must be moved to etj_entitityutils_shared.cpp or similar and
      //   must be callable via both cgame and qagame
      for (const auto &portal : clientPortals) {
        if (PointInBounds(tr.endpos, portal.mins, portal.maxs)) {
          if (portal.s->eType == ET_PORTAL_BLUE) {
            canShootRed = false;
          } else {
            canShootBlue = false;
          }

          break;
        }
      }
    }
  }

  return true;
}

void PortalCrosshair::render() const {
  if (angle == BASE_ANGLE) {
    drawPic(blueX, blueY, size.x, size.y, blueShader,
            canShootBlue ? color : colorAlt);
    drawPic(redX, redY, size.x, size.y, redShader,
            canShootRed ? color : colorAlt);
  } else {
    // rotated pics are forced 1:1 aspect ratio, so ignore 'size.y'
    drawRotatedPic(blueX, blueY, size.x, size.x, blueShader, angle,
                   canShootBlue ? color : colorAlt);
    drawRotatedPic(redX, redY, size.x, size.x, redShader, angle,
                   canShootRed ? color : colorAlt);
  }
}

bool PortalCrosshair::canSkipDraw() {
  if (!etj_drawPortalCrosshair.integer) {
    return true;
  }

  if (!isPlaying(cg.snap->ps.clientNum)) {
    return true;
  }

  if (weapnumForClient() != WP_PORTAL_GUN &&
      etj_drawPortalCrosshair.integer != 2) {
    return true;
  }

  if (showingScores()) {
    return true;
  }

  if (cg.snap->ps.leanf != 0) {
    return true;
  }

  if (cg.renderingThirdPerson) {
    return true;
  }

  if (!cg.editingSpeakers) {
    if (cg.zoomedBinoc && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
      return true;
    }

    if (BG_IsScopedWeapon(weapnumForClient())) {
      return true;
    }
  }

  if (cg.predictedPlayerState.weapon == WP_MORTAR_SET &&
      cg.predictedPlayerState.weaponstate != WEAPON_RAISING) {
    return true;
  }

  // pretty sure these hints are bogus but keeping it here just in case
  if (cg.snap->ps.serverCursorHint >= HINT_EXIT &&
      cg.snap->ps.serverCursorHint <= HINT_NOEXIT) {
    return true;
  }

  return false;
}
} // namespace ETJump
