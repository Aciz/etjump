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

#include <vector>
#include <array>

#include "etj_progression_tracker_parser.h"

namespace ETJump {
class ProgressionTrackers {
public:
  static const char *ETJUMP_PROGRESSION_TRACKER_VALUE_NOT_SET;
  static const int ProgressionTrackerValueNotSet = -1;
  struct ProgressionTrackerKeys {
    char *equal;
    char *notEqualAny;
    char *notEqualAll;
    char *greaterThan;
    char *lessThan;
    char *set;
    char *setIf;
    char *increment;
    char *incrementIf;
    char *bitIsSet;
    char *bitNotSet;
    char *bitSet;
    char *bitReset;
  };

  static const int MaxProgressionTrackers = 50;
  static ETJump::ProgressionTrackers::ProgressionTrackerKeys ParseTrackerKeys();
  static void printTrackerChanges(gentity_t *activator, int *oldValues);

  struct ProgressionTracker {
    ProgressionTracker() {
      std::fill_n(equal, MaxProgressionTrackers, -1);
      std::fill_n(notEqualAny, MaxProgressionTrackers, -1);
      std::fill_n(notEqualAll, MaxProgressionTrackers, -1);
      std::fill_n(greaterThan, MaxProgressionTrackers, -1);
      std::fill_n(lessThan, MaxProgressionTrackers, -1);
      std::fill_n(set, MaxProgressionTrackers, -1);
      std::fill_n(setIf, MaxProgressionTrackers, -1);
      std::fill_n(bitIsSet, MaxProgressionTrackers, -1);
      std::fill_n(bitNotSet, MaxProgressionTrackers, -1);
      std::fill_n(bitSet, MaxProgressionTrackers, -1);
      std::fill_n(bitReset, MaxProgressionTrackers, -1);

      // increments default to 0
      std::fill_n(increment, MaxProgressionTrackers, 0);
      std::fill_n(incrementIf, MaxProgressionTrackers, 0);
    }
    int equal[MaxProgressionTrackers]{};
    int notEqualAny[MaxProgressionTrackers]{};
    int notEqualAll[MaxProgressionTrackers]{};
    int greaterThan[MaxProgressionTrackers]{};
    int lessThan[MaxProgressionTrackers]{};
    int set[MaxProgressionTrackers]{};
    int setIf[MaxProgressionTrackers]{};
    int increment[MaxProgressionTrackers]{};
    int incrementIf[MaxProgressionTrackers]{};
    int bitIsSet[MaxProgressionTrackers]{};
    int bitNotSet[MaxProgressionTrackers]{};
    int bitSet[MaxProgressionTrackers]{};
    int bitReset[MaxProgressionTrackers]{};
  };

  ProgressionTrackers();
  ~ProgressionTrackers();
  void printParserErrors(const std::vector<std::string> &errors,
                         const std::string &text);
  std::vector<ProgressionTrackerParser::IndexValuePair>
  parseKey(const std::string &key);
  void
  updateTracker(std::vector<ProgressionTrackerParser::IndexValuePair> pairs,
                int tracker[MaxProgressionTrackers]);
  int registerTracker(ProgressionTrackerKeys keys);
  void useTargetTracker(gentity_t *ent, gentity_t *other, gentity_t *activator);
  void useTriggerTracker(gentity_t *ent, gentity_t *activator);

private:
  void useTracker(gentity_t *ent, gentity_t *activator,
                  const ProgressionTracker &tracker);

  std::vector<ProgressionTracker> _progressionTrackers;
};
} // namespace ETJump
