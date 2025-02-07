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

#include <utility>
#include <random>

#include "etj_map_statistics_v2.h"
#include "etj_map_statistics_repository.h"
#include "etj_filesystem.h"
#include "etj_utilities.h"
#include "etj_printer.h"
#include "g_local.h"

namespace ETJump {
MapStatisticsV2::MapStatisticsV2(
    std::string currentMap, std::unique_ptr<MapStatisticsRepository> repository,
    std::unique_ptr<Log> logger, std::unique_ptr<SynchronizationContext> sc)
    : currentMap(std::move(currentMap)), repository(std::move(repository)),
      logger(std::move(logger)), sc(std::move(sc)) {}

class PrintResult final : public SynchronizationContext::ResultBase {
public:
  explicit PrintResult(std::string message) : message(std::move(message)) {}
  std::string message;
};

void MapStatisticsV2::initialize() {
  try {
    repository->initialize();
  } catch (const std::exception &e) {
    logger->error("Unable to initialize map statistics database '%s': %s",
                  g_mapDatabase2.string, e.what());
    return;
  }

  sc->startWorkerThreads(1);
  registeredMaps = repository->getMaps();
  addNewMaps();
  setBlockedMaps();
  initialized = setCurrentMap();
}

void MapStatisticsV2::shutdown() {
  saveChanges();
  sc->stopWorkerThreads();
}

void MapStatisticsV2::runFrame() {
  if (!initialized) {
    return;
  }

  sc->processCompletedTasks();

  if (!Utilities::anyonePlaying()) {
    return;
  }

  // we're tracking seconds here so update every 1s
  if (lastUpdate + 1000 > level.time) {
    return;
  }

  lastUpdate = level.time;
  playTimeForPlayercount[level.numPlayingClients]++;
}

bool MapStatisticsV2::setCurrentMap() {
  for (const auto &mi : registeredMaps) {
    if (!mi.isOnServer) {
      continue;
    }

    if (mi.mapname == currentMap) {
      currentMapInfo = mi;
      currentMapInfo.changed = true;

      if (!currentMapInfo.pStats.timesPlayed) {
        time_t t;
        time(&t);
        currentMapInfo.pStats.firstPlayed = static_cast<uint64_t>(t);
      }

      return true;
    }
  }

  // map not found
  // TODO: G_Error here?
  logger->error("Failed to set current map to '%' - map was not found in the "
                "maps vector (map count: %i)",
                currentMap, static_cast<int>(registeredMaps.size()));
  return false;
}

void MapStatisticsV2::setBlockedMaps() {
  blockedMaps.clear();
  const std::string blockedMapsStr =
      StringUtil::toLowerCase(g_blockedMaps.string);
  blockedMaps = StringUtil::split(blockedMapsStr, " ");

  auto it = mapsOnServer.begin();

  while (it != mapsOnServer.end()) {
    if (isBlockedMap(*it)) {
      it = mapsOnServer.erase(it);
    } else {
      ++it;
    }
  }
}

bool MapStatisticsV2::isBlockedMap(const std::string &mapname) const {
  return std::any_of(blockedMaps.cbegin(), blockedMaps.cend(),
                     [&](const std::string &blockedMap) {
                       return StringUtil::iEqual(blockedMap, mapname);
                     });
}

bool MapStatisticsV2::isValidMap(const std::string &mapname) const {
  return level.rawmapname != StringUtil::toLowerCase(mapname) &&
         !isBlockedMap(mapname);
}

const std::vector<std::string> &MapStatisticsV2::getCurrentMaps() const {
  return mapsOnServer;
}

std::vector<std::string>
MapStatisticsV2::getNewestMaps(const size_t count) const {
  std::vector<std::string> newestMaps{};

  for (size_t i = registeredMaps.size(); i > 0; i--) {
    if (!registeredMaps[i].isOnServer) {
      continue;
    }

    if (isValidMap(registeredMaps[i].mapname)) {
      newestMaps.emplace_back(registeredMaps[i].mapname);

      if (newestMaps.size() == count) {
        break;
      }
    }
  }

  return newestMaps;
}

std::string MapStatisticsV2::playTimeToString(uint32_t seconds) {
  uint32_t minutes = seconds / 60;
  seconds -= minutes * 60;
  uint32_t hours = minutes / 60;
  minutes -= hours * 60;
  uint32_t days = hours / 24;
  hours -= days * 24;
  uint32_t weeks = days / 7;
  days -= weeks * 7;

  std::string str;

  if (weeks) {
    str += getWeeksString(weeks) + " ";

    if (days) {
      str += getDaysString(days) + " ";
    }

    if (hours) {
      str += getHoursString(hours);
    }
  } else if (days) {
    str += getDaysString(days) + " ";

    if (hours) {
      str += getHoursString(hours) + " ";
    }

    if (minutes) {
      str += getMinutesString(minutes);
    }
  } else {
    if (hours) {
      str += getHoursString(hours) + " ";
    }

    if (minutes) {
      str += getMinutesString(minutes) + " ";
    }

    if (seconds) {
      str += getSecondsString(seconds);
    }
  }

  if (str.empty()) {
    str = "never";
  }

  return str;
}

void MapStatisticsV2::listMapsByPlaytime(gentity_t *ent, const size_t count,
                                         const bool ascending) {
  sc->postTask(
      [this, count, ascending] {
        auto maps = repository->getMaps();
        const size_t numMaps = maps.size() > count ? count : maps.size();

        std::sort(maps.begin(), maps.end(),
                  [&](const MapInformation &lhs, const MapInformation &rhs) {
                    return ascending ? lhs.pStats.secondsPlayed <
                                           rhs.pStats.secondsPlayed
                                     : lhs.pStats.secondsPlayed >
                                           rhs.pStats.secondsPlayed;
                  });

        std::string message =
            stringFormat("Listing ^3%i ^7%s played maps on the server:\n\n",
                         numMaps, ascending ? "least" : "most");
        message += stringFormat("^z%-22s %-30s %-18s %s\n", "Map", "Playtime",
                                "Last played", "Times played");

        for (int i = 0; i < numMaps; i++) {
          message += i % 2 == 0 ? "^7" : "^z";
          // FIXME: 32-bit time
          message +=
              stringFormat("%-22s %-30s %-18s %i\n", maps[i].mapname,
                           playTimeToString(maps[i].pStats.secondsPlayed),
                           Utilities::timestampToString(static_cast<uint32_t>(
                               maps[i].pStats.lastPlayed)),
                           maps[i].pStats.timesPlayed);
        }

        return std::make_unique<PrintResult>(message);
      },
      [this, ent, ascending](const auto r) {
        const auto result = dynamic_cast<PrintResult *>(r.get());

        if (!result) {
          logger->error("listMapsByPlaytime: Unable to fetch maps for client "
                        "'%i': PrintResult is NULL",
                        ClientNum(ent));
          throw std::runtime_error("Fetching map statistics failed.\n");
        }

        Printer::chat(
            ent, stringFormat("^3%s: ^7check console for more information.",
                              ascending ? "leastplayed" : "mostplayed"));
        Printer::console(ent, result->message);
      },
      [ent, ascending](const auto e) {
        Printer::chat(
            ent,
            stringFormat(
                "^3%s: ^7operation failed. Check console for more information.",
                ascending ? "leastplayed" : "mostplayed"));
        Printer::console(ent, e.what());
      });
}

void MapStatisticsV2::printMapInformation(gentity_t *ent,
                                          const std::string &mapname) {
  sc->postTask(
      [this, mapname] {
        MapInformation mi;

        // TODO: match by partial name

        // FIXME: I don't like that we copy things here
        if (mapname == level.rawmapname) {
          mi = currentMapInfo;
        } else {
          auto maps = repository->getMaps();

          for (const auto &map : maps) {
            if (map.mapname == mapname) {
              mi = map;
              break;
            }
          }

          if (mi.mapname.empty()) {
            throw std::runtime_error(stringFormat(
                "Unable to find map information for ^3'%s'\n", mapname));
          }
        }

        std::string message =
            stringFormat("Map information for ^3%s:\n\n", mi.mapname);

        // FIXME: 32-bit time
        message += stringFormat("^7%-22s ^z%s\n", "Playtime: ",
                                playTimeToString(mi.pStats.secondsPlayed));
        message += stringFormat("^7%-22s ^z%u\n",
                                "Times played: ", mi.pStats.timesPlayed);
        message +=
            stringFormat("^7%-22s ^z%s\n", "First played: ",
                         Utilities::timestampToString(
                             static_cast<uint32_t>(mi.pStats.firstPlayed)));
        message +=
            stringFormat("^7%-22s ^z%s\n", "Last played: ",
                         Utilities::timestampToString(
                             static_cast<uint32_t>(mi.pStats.lastPlayed)));
        message += stringFormat("^7%-22s ^z%s\n", "Avg. playtime: ",
                                playTimeToString(mi.pStats.avgSecondsPlayed));
        message += stringFormat("^7%-22s ^z%.2f\n",
                                "Avg. players: ", mi.pStats.avgPlayers);
        message +=
            stringFormat("^7%-22s ^z%u\n", "Voted for: ", mi.vStats.callvoted);
        message +=
            stringFormat("^7%-22s ^z%u\n", "RTV votes: ", mi.vStats.rtvVoted);
        message += stringFormat("^7%-22s ^z%u\n",
                                "Votes passed: ", mi.vStats.votesPassed);

        return std::make_unique<PrintResult>(message);
      },
      [this, ent](const auto r) {
        const auto result = dynamic_cast<PrintResult *>(r.get());

        if (!result) {
          logger->error("printMapInformation: Unable to fetch data for client "
                        "'%i': PrintResult is NULL",
                        ClientNum(ent));
          throw std::runtime_error("Fetching data failed.\n");
        }

        Printer::chat(ent, "^3mapinfo: ^7check console for more information.");
        Printer::console(ent, result->message);
      },
      [ent](const auto e) {
        Printer::chat(ent, "^3mapinfo: ^7operation failed. Check console for "
                           "more information.");
        Printer::console(ent, e.what());
      });
}

std::string MapStatisticsV2::getRandomMap() {
  std::random_device rd;
  std::mt19937 re(rd());
  std::uniform_int_distribution<size_t> ui(0, mapsOnServer.size() - 1);
  constexpr int MAX_TRIES = 15;

  std::string result;

  for (int i = 0; i < MAX_TRIES; i++) {
    const size_t testIndex = ui(re);

    if (isValidMap(mapsOnServer[testIndex])) {
      result = mapsOnServer[testIndex];
      break;
    }
  }

  // fallback, get first valid map
  if (result.empty()) {
    for (const auto &map : mapsOnServer) {
      if (isValidMap(map)) {
        result = map;
        break;
      }
    }
  }

  // extra fallback, no valid maps on the server (all blocked)
  if (result.empty()) {
    return "oasis";
  }

  return result;
}

void MapStatisticsV2::updateVoteStats(const VoteUpdateType type,
                                      const std::string &mapname,
                                      const int voteCount) {
  bool found = false;

  for (auto &map : registeredMaps) {
    if (!map.isOnServer) {
      continue;
    }

    if (map.mapname == mapname) {
      switch (type) {
        case VoteUpdateType::Callvote:
          map.vStats.callvoted++;
          break;
        case VoteUpdateType::VotePassed:
          map.vStats.votesPassed++;
          break;
        case VoteUpdateType::RtvVote:
          map.vStats.rtvVoted += voteCount;
          break;
        default:
          break;
      }

      map.changed = true;
      found = true;
      break;
    }
  }

  // not found
  // TODO: G_Error?
  if (!found) {
    logger->error("Failed to update vote stats for map '%' - map was not found "
                  "in the maps vector (map count: %i)",
                  mapname, static_cast<int>(registeredMaps.size()));
  }
}

void MapStatisticsV2::addNewMaps() {
  int totalMaps = 0;
  int newMaps = 0;

  mapsOnServer = FileSystem::getFileList("maps", ".bsp", true, true);

  for (auto &map : mapsOnServer) {
    map = StringUtil::toLowerCase(map);
    totalMaps++;
    bool found = false;

    for (auto &mi : registeredMaps) {
      if (mi.mapname == map) {
        mi.isOnServer = true;
        found = true;
        break;
      }
    }

    if (found) {
      continue;
    }

    MapInformation mapInformation{};
    mapInformation.mapname = map;
    mapInformation.isOnServer = true;

    try {
      repository->insertMap(mapInformation);
    } catch (const std::exception &e) {
      logger->error("Failed to insert new map to database: %s", e.what());
      continue;
    }

    registeredMaps.emplace_back(mapInformation);
    logger->info("Registered new map '%s'", map);
    newMaps++;
  }

  if (newMaps > 0) {
    logger->info("Registered %i new maps.", newMaps);
  }

  logger->info("%i maps on the server.", totalMaps);
}

void MapStatisticsV2::saveChanges() {
  if (!initialized) {
    return;
  }

  if (currentMapInfo.mapname.empty()) {
    logger->info(
        "Map was changed before a new map was loaded, no statistics saved.");
    return;
  }

  uint32_t totalSessionPlayTime = 0;
  uint32_t weightedPlayTime = 0;

  for (size_t i = 0; i < playTimeForPlayercount.size(); i++) {
    totalSessionPlayTime += playTimeForPlayercount[i];
    weightedPlayTime += i * playTimeForPlayercount[i];
  }

  currentMapInfo.pStats.secondsPlayed += totalSessionPlayTime;

  if (totalSessionPlayTime) {
    currentMapInfo.pStats.avgPlayers =
        static_cast<float>(weightedPlayTime) /
        static_cast<float>(currentMapInfo.pStats.secondsPlayed);
  }

  time_t t;
  time(&t);
  currentMapInfo.pStats.lastPlayed = static_cast<uint64_t>(t);
  currentMapInfo.pStats.timesPlayed++;
  currentMapInfo.pStats.secondsPlayed += totalSessionPlayTime;
  currentMapInfo.pStats.avgSecondsPlayed =
      currentMapInfo.pStats.secondsPlayed / currentMapInfo.pStats.timesPlayed;

  for (const auto &mi : registeredMaps) {
    if (!mi.changed || !mi.isOnServer) {
      continue;
    }

    try {
      repository->updateMap(mi);
    } catch (const std::exception &e) {
      logger->error("Failed to update map statistics for map '%s': %s",
                    mi.mapname, e.what());
    }
  }
}
} // namespace ETJump
