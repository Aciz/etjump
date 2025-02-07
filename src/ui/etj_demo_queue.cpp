/*
 * MIT License
 *
 * Copyright (c) 2024 ETJump team <zero@etjump.com>
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

#include "ui_local.h"
#include "etj_demo_queue.h"
#include "../game/etj_filesystem.h"

namespace ETJump {
void DemoQueue::parse() {
  const std::string path = "demos/" + std::string(etj_demoQueueDir.string);

  char buf[MAX_CVAR_VALUE_STRING]{};
  trap_Cvar_VariableStringBuffer("protocol", buf, sizeof(buf));
  const std::string ext = "dm_" + std::string(buf);

  queue = FileSystem::getFileList(path, ext, true);

  if (queue.empty()) {
    stop();
    return;
  }

  const auto size = static_cast<int>(queue.size());

  // we're starting a fresh queue, set the progress cvar
  if (etj_currentDemoName.string[0] == '\0' || doRestart) {
    trap_Cvar_Set("etj_demoQueueProgress", va("1 %i", size));
  }

  trap_Cvar_Update(&etj_demoQueueProgress);
  const auto progress = StringUtil::split(etj_demoQueueProgress.string, " ");

  // something went horribly wrong, bail
  if (progress.size() < 2) {
    stop();
    return;
  }

  const char *current = queue[Q_atoi(progress[0].c_str()) - 1].c_str();
  trap_Cvar_Set("etj_demoQueueCurrent", current);

  // we're at the end, clear next demo
  if (progress[0] == progress[1]) {
    trap_Cvar_Set("etj_demoQueueNext", "");
  } else {
    const char *next = queue[Q_atoi(progress[0].c_str())].c_str();
    trap_Cvar_Set("etj_demoQueueNext", next);
  }

  trap_Cvar_Update(&etj_demoQueueCurrent);
  trap_Cvar_Update(&etj_demoQueueNext);

  // update current progress
  for (int i = 0; i < static_cast<int>(queue.size()); i++) {
    if (queue[i] == etj_demoQueueNext.string) {
      trap_Cvar_Set("etj_demoQueueProgress", va("%i %i", i + 1, size));
    }
  }
}

void DemoQueue::start() {
  if (etj_demoQueueDir.string[0] == '\0') {
    uiInfo.uiDC.Print(
        "Demo queue directory not set. Set ^3'etj_demoQueueDir' ^7to a "
        "directory inside 'demos' directory to play demos from.\n");
    return;
  }

  if (currentDemo.empty() || doRestart) {
    parse();
    doRestart = false;
  }

  if (queue.empty()) {
    uiInfo.uiDC.Print("No demos found. Make sure the demos are located in "
                      "^3'demos/%s' ^7directory.\n",
                      etj_demoQueueDir.string);
    return;
  }

  trap_Cmd_ExecuteText(EXEC_APPEND, va("demo \"%s/%s\"\n", etj_demoQueueDir.string,
                                       etj_demoQueueCurrent.string));
}

void DemoQueue::stop() {
  trap_Cvar_Set("etj_demoQueueProgress", "");
  trap_Cvar_Set("etj_demoQueueCurrent", "");
  trap_Cvar_Set("etj_demoQueueNext", "");
}

void DemoQueue::next() {
  if (etj_demoQueueNext.string[0] == '\0') {
    stop();
    return;
  }

  // always re-run parse in case a vid/ui_restart is performed during a playback
  // TODO: store the cvar values in private variables to get around cvar update bs
  parse();
  trap_Cmd_ExecuteText(EXEC_APPEND, va("demo \"%s/%s\"\n", etj_demoQueueDir.string, etj_demoQueueCurrent.string));
}


void DemoQueue::restart() {
  doRestart = true;
  start();
}
} // namespace ETJump
