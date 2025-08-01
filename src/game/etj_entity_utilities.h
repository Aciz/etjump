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

#pragma once

#include "g_local.h"

namespace ETJump {
class EntityUtilities {
  static void drawRailBox(const gentity_t *ent,
                          const std::vector<float> &color);

public:
  static bool isPlayer(gentity_t *ent);
  static void checkForRailBox(gentity_t *ent);
  static bool playerIsSolid(int self, int other);

  // 'threshold' indicates the number of entities that must be free
  static bool entitiesFree(int threshold);

  // sets 'value' to corresponding cursorhint from 'hint'
  // if 'hint' isn't found in hintStrings, no modification is performed
  static void setCursorhintFromString(int &value, const std::string &hint);

  // sets 'origin' to the entity's origin, or the center of it's absmins/maxs
  static void getOriginOrBmodelCenter(const gentity_t *ent, vec3_t origin);

  // returns true if any portals were removed
  static bool clearPortals(gentity_t *ent);
};
} // namespace ETJump
