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

#include "etj_client_commands_handler.h"
#include "etj_overbounce_watcher.h"
#include "etj_cvar_update_handler.h"
#include "etj_utilities.h"
#include "cg_local.h"
#include "etj_overbounce_shared.h"
#include "../game/etj_string_utilities.h"

namespace ETJump {
OverbounceWatcher::OverbounceWatcher(
    ClientCommandsHandler *clientCommandsHandler)
    : _clientCommandsHandler{clientCommandsHandler}, _positions{},
      _current{nullptr}, ps(nullptr) {
  if (!clientCommandsHandler) {
    CG_Error("OverbounceWatcher: clientCommandsHandler is null.\n");
    return;
  }

  shader = cgs.media.voiceChatShader;
  _positions.clear();

  clientCommandsHandler->subscribe(
      "ob_save", [&](const std::vector<std::string> &args) {
        vec3_t c;
        ps = getValidPlayerState();
        for (int i = 0; i < 3; ++i) {
          c[i] = ps->origin[i];
        }
        // shift z-coordinate to feet level
        c[2] += ps->mins[2];

        const auto &name = !args.empty() ? sanitize(args[0], true) : "default";
        save(name, c);
        CG_AddPMItem(
            PM_MESSAGE,
            va("^3OB watcher: ^7saved coordinate as ^3%s ^7(%f, %f, %f)\n",
               name.c_str(), c[0], c[1], c[2]),
            shader);
      });

  clientCommandsHandler->subscribe(
      "ob_load", [&](const std::vector<std::string> &args) {
        const auto &name = !args.empty() ? sanitize(args[0], true) : "default";
        if (!load(name)) {
          CG_AddPMItem(PM_MESSAGE,
                       va("^3OB watcher: ^7coordinate ^3%s ^7was not found.\n",
                          name.c_str()),
                       shader);
          return;
        }

        CG_AddPMItem(
            PM_MESSAGE,
            va("^3OB watcher: ^7loaded coordinate ^3%s ^7(%f, %f, %f)\n",
               name.c_str(), (*_current)[0], (*_current)[1], (*_current)[2]),
            shader);
      });

  clientCommandsHandler->subscribe(
      "ob_reset", [&](const std::vector<std::string> &args) {
        reset();
        CG_AddPMItem(PM_MESSAGE,
                     "^3OB watcher: ^7current coordinates have been reset.\n",
                     shader);
      });

  clientCommandsHandler->subscribe(
      "ob_list", [&](const std::vector<std::string> &args) {
        if (_positions.empty()) {
          CG_Printf("^3OB watcher: ^7no saved positions.\n");
          return;
        }
        list();
      });

  cvarUpdateHandler->subscribe(&etj_obWatcherColor, [&](const vmCvar_t *cvar) {
    parseColorString(etj_obWatcherColor.string, _color);
  });

  cvarUpdateHandler->subscribe(&etj_obWatcherSize,
                               [&](const vmCvar_t *) { setSize(); });

  setSize();
  parseColorString(etj_obWatcherColor.string, _color);
}

OverbounceWatcher::~OverbounceWatcher() {
  _clientCommandsHandler->unsubscribe("ob_save");
  _clientCommandsHandler->unsubscribe("ob_load");
  _clientCommandsHandler->unsubscribe("ob_reset");
  _clientCommandsHandler->unsubscribe("ob_list");
}

void OverbounceWatcher::setSize() {
  size = CvarValueParser::parse<CvarValue::Size>(etj_obWatcherSize, 0, 10);
  size.x *= 0.1f;
  size.y *= 0.1f;
}

bool OverbounceWatcher::beforeRender() {
  if (canSkipDraw()) {
    return false;
  }

  ps = getValidPlayerState();
  pmoveSec = static_cast<float>(cgs.pmove_msec) / 1000.f;
  gravity = ps->gravity;
  zVel = ps->velocity[2];
  startHeight = ps->origin[2] + ps->mins[2];

  // impossible OB - negative z velocity & below saved coordinate
  if (zVel < 0 && startHeight < endHeight) {
    return false;
  }

  overbounce = false;

  VectorSet(snap, 0, 0, gravity * pmoveSec);
  trap_SnapVector(snap);
  zVelSnapped = snap[2];

  endHeight = (*_current)[2];

  x = etj_obWatcherX.value;
  ETJump_AdjustPosition(&x);

  // setup & do trace, so we can determine if surface allows OB
  trace_t trace;
  VectorCopy(*_current, start);
  start[2] = startHeight;
  VectorCopy(start, end);
  end[2] -= Overbounce::MAX_TRACE_DIST;

  CG_Trace(&trace, start, vec3_origin, vec3_origin, end, ps->clientNum,
           CONTENTS_SOLID);

  // CG_Printf("startHeight: %f endHeight: %f\n", startHeight, endHeight);
  if (Overbounce::isOverbounce(zVel, startHeight, endHeight, zVelSnapped,
                               pmoveSec, gravity) &&
      Overbounce::surfaceAllowsOverbounce(&trace)) {
    overbounce = true;
  }

  return overbounce;
}

void OverbounceWatcher::render() const {
  DrawString(x, etj_obWatcherY.value, size.x, size.y, _color, qfalse, "OB", 0,
             ITEM_TEXTSTYLE_SHADOWED);
}

void OverbounceWatcher::save(const std::string &name, const vec3_t coordinate) {
  VectorCopy(coordinate, _positions[name]);
  _current = &_positions[name];
}

void OverbounceWatcher::reset() { _current = nullptr; }

bool OverbounceWatcher::load(const std::string &name) {
  auto pos = _positions.find(name);
  if (pos == _positions.end()) {
    return false;
  }

  _current = &(pos->second);
  return true;
}

void OverbounceWatcher::list() const {
  CG_Printf("^gSaved OB watcher coordinates\n"
            "^g-----------------------------------\n");
  for (const auto &pos : _positions) {
    CG_Printf("^3%-15s ^7(%f %f %f)%s\n", pos.first.c_str(), pos.second[0],
              pos.second[1], pos.second[2],
              _current && VectorCompare(*_current, pos.second) ? " ^9(current)"
                                                               : "");
  }
}

bool OverbounceWatcher::canSkipDraw() const {
  if (!etj_drawObWatcher.integer || !_current) {
    return true;
  }

  if (ps->groundEntityNum != ENTITYNUM_NONE) {
    return true;
  }

  if (ps->pm_type == PM_NOCLIP) {
    return true;
  }

  if (showingScores()) {
    return true;
  }

  return false;
}
} // namespace ETJump
