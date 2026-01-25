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

#include "etj_client_shader_state_handler.h"
#include "etj_client_commands_handler.h"

#include "../game/etj_container_utilities.h"
#include "../game/etj_string_utilities.h"

namespace ETJump {
ClientShaderStateHandler::ClientShaderStateHandler(
    const std::shared_ptr<ClientCommandsHandler> &serverCommandsHandler)
    : serverCommandsHandler(serverCommandsHandler) {
  registerCommands();
}

ClientShaderStateHandler::~ClientShaderStateHandler() {
  serverCommandsHandler->unsubscribe("clientShaderState");
}

void ClientShaderStateHandler::registerCommands() {
  // 'clientShaderState <clientNum> <index> <state>'
  serverCommandsHandler->subscribe(
      "clientShaderState", [this](const auto &args) {
        if (args.size() < 3) {
          return;
        }

        // it's possible that the shader state contains whitespace due to
        // 'timeOffset' formatting, so join everything after the state index
        updateShaderState(Q_atoi(args[0]), Q_atoi(args[1]),
                          StringUtil::join(Container::skipFirstN(args, 2), ""));
      });
}

void ClientShaderStateHandler::runFrame() {
  if (shaderStates.find(cg.snap->ps.clientNum) == shaderStates.cend()) {
    return;
  }

  for (auto &shaderState : shaderStates.at(cg.snap->ps.clientNum)) {
    // this doesn't necessarily mean that we've exhausted all state strings,
    // we might just not have gotten a state in this index for a client yet
    if (shaderState.state.empty()) {
      continue;
    }

    if (!shaderState.valid) {
      CG_ShaderStateChanged(shaderState.state);
      shaderState.valid = true;
    }
  }
}

void ClientShaderStateHandler::updateShaderState(const int32_t clientNum,
                                                 const int32_t index,
                                                 const std::string &state) {
  shaderStates[clientNum][index].valid = false;
  shaderStates[clientNum][index].state = state;
}

void ClientShaderStateHandler::invalidateShaderState(const int32_t clientNum) {
  // shouldn't happen
  if (shaderStates.find(clientNum) == shaderStates.cend()) {
    return;
  }

  for (auto &shaderState : shaderStates[clientNum]) {
    shaderState.valid = false;
  }
}
} // namespace ETJump
