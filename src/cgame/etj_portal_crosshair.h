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

#include "cg_local.h"
#include "etj_irenderable.h"
#include "etj_cvar_parser.h"

namespace ETJump {
class PortalCrosshair : public IRenderable {
public:
  PortalCrosshair();

  bool beforeRender() override;
  void render() const override;

private:
  CvarValue::Size size{};
  float blueX{};
  float blueY{};
  float redX{};
  float redY{};
  float offset{};
  float angle{};

  vec4_t color{};
  vec4_t colorAlt{};

  qhandle_t blueShader;
  qhandle_t redShader;

  trace_t tr{};
  vec3_t trStart{};
  vec3_t trEnd{};

  bool canShootBlue{};
  bool canShootRed{};

  void startListeners();
  void adjustSize(const vmCvar_t &cvar);
  void adjustOffset(const vmCvar_t &cvar);
  void adjustAngle(const vmCvar_t &cvar);
  void adjustPos();
  void adjustAlpha(const vmCvar_t &cvar);
  void adjustAlphaAlt(const vmCvar_t &cvar);

  static bool canSkipDraw();
};
} // namespace ETJump
