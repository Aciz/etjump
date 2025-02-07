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

#pragma once

#include <memory>

#include "etj_database_v2.h"
#include "etj_map_statistics_v2.h"

#define STANDARD_QUERY_PARAMS                                                  \
  const std::string &mapname, const uint32_t secondsPlayed,                    \
      const uint32_t timesPlayed, const uint64_t firstPlayed,                  \
      const uint64_t lastPlayed, const uint32_t avgPlaytime,                   \
      const float avgPlayers, const uint32_t callvoted,                        \
      const uint32_t rtvVoted, const uint32_t votesPassed

namespace ETJump {
class MapStatisticsRepository {
  std::unique_ptr<DatabaseV2> database;
  std::unique_ptr<DatabaseV2> oldDatabase;

  void tryToMigrateMapstatistics() const;
  static MapStatisticsV2::MapInformation getMapInfoFromStandardQuery(
      const std::string &mapname, uint32_t secondsPlayed, uint32_t timesPlayed,
      uint64_t firstPlayed, uint64_t lastPlayed, uint32_t avgPlaytime,
      float avgPlayers, uint32_t callvoted, uint32_t rtvVoted,
      uint32_t votesPassed);

public:
  void initialize() const;
  void insertMap(const MapStatisticsV2::MapInformation &mi) const;
  void updateMap(const MapStatisticsV2::MapInformation &mi) const;
  std::vector<MapStatisticsV2::MapInformation> getMaps() const;

  explicit MapStatisticsRepository(std::unique_ptr<DatabaseV2> db,
                                   std::unique_ptr<DatabaseV2> oldDb)
      : database(std::move(db)), oldDatabase(std::move(oldDb)) {}
  ~MapStatisticsRepository() = default;
};
} // namespace ETJump
