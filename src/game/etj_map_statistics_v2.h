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
#include <string>

#include "etj_log.h"
#include "etj_synchronization_context.h"
#include "q_shared.h"

namespace ETJump {
class MapStatisticsRepository;

class MapStatisticsV2 {
public:
  struct VoteStatistics {
    VoteStatistics() : callvoted(0), rtvVoted(0), votesPassed(0) {}

    uint32_t callvoted;
    uint32_t rtvVoted; // cumulative vote count from rtv
    uint32_t votesPassed;
  };

  struct PlayStatistics {
    PlayStatistics()
        : secondsPlayed(0), timesPlayed(0), firstPlayed(0), lastPlayed(0),
          avgSecondsPlayed(0.0f), avgPlayers(0) {}

    uint32_t secondsPlayed;
    uint32_t timesPlayed;
    uint64_t firstPlayed;
    uint64_t lastPlayed;

    uint32_t avgSecondsPlayed;
    float avgPlayers;
  };

  struct MapInformation {
    MapInformation() : isOnServer(false), changed(false) {}

    std::string mapname{};

    VoteStatistics vStats{};
    PlayStatistics pStats{};

    bool isOnServer;
    bool changed;
  };

  enum class VoteUpdateType {
    Callvote = 0,
    VotePassed = 1,
    RtvVote = 2,
  };

  void initialize();
  void runFrame();
  void shutdown();

  void setBlockedMaps();

  // returns sorted list of all playable maps on the server
  const std::vector<std::string> &getCurrentMaps() const;

  // does size checks, it's safe to pass any value as 'count' to this function
  // newest map is the first map in the vector
  std::vector<std::string> getNewestMaps(size_t count) const;

  void listMapsByPlaytime(gentity_t *ent, size_t count, bool ascending);
  void printMapInformation(gentity_t *ent, const std::string &mapname);

  bool isBlockedMap(const std::string &mapname) const;
  bool isValidMap(const std::string &mapname) const;

  std::string getRandomMap();

  // voteCount is used for RTV
  void updateVoteStats(VoteUpdateType type, const std::string &mapname,
                       int voteCount = 0);

  explicit MapStatisticsV2(std::string currentMap,
                           std::unique_ptr<MapStatisticsRepository> repository,
                           std::unique_ptr<Log> logger,
                           std::unique_ptr<SynchronizationContext> sc);
  ~MapStatisticsV2() = default;

private:
  std::string currentMap;
  std::string database;

  // every map ever registered on server
  std::vector<MapInformation> registeredMaps{};
  // all valid maps on the server (excludes blockedMaps), sorted alphabetically
  std::vector<std::string> mapsOnServer{};
  std::vector<std::string> blockedMaps{};

  MapInformation currentMapInfo{};

  int lastUpdate{};
  std::array<uint32_t, MAX_CLIENTS> playTimeForPlayercount{}; // in seconds

  bool initialized = false;

  std::unique_ptr<MapStatisticsRepository> repository;
  std::unique_ptr<Log> logger;
  std::unique_ptr<SynchronizationContext> sc;

  void addNewMaps();
  bool setCurrentMap();
  void saveChanges();

  static std::string playTimeToString(uint32_t seconds);
};
} // namespace ETJump
