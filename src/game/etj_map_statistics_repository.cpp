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

#include "etj_map_statistics_repository.h"

namespace ETJump {
MapStatisticsV2::MapInformation
MapStatisticsRepository::getMapInfoFromStandardQuery(STANDARD_QUERY_PARAMS) {
  MapStatisticsV2::MapInformation mi{};

  mi.mapname = mapname;

  mi.pStats.secondsPlayed = secondsPlayed;
  mi.pStats.timesPlayed = timesPlayed;
  mi.pStats.firstPlayed = firstPlayed;
  mi.pStats.lastPlayed = lastPlayed;
  mi.pStats.avgSecondsPlayed = avgPlaytime;
  mi.pStats.avgPlayers = avgPlayers;

  mi.vStats.callvoted = callvoted;
  mi.vStats.rtvVoted = rtvVoted;
  mi.vStats.votesPassed = votesPassed;

  return mi;
}

void MapStatisticsRepository::insertMap(
    const MapStatisticsV2::MapInformation &mi) const {
  database->sql << "begin;";

  try {
    database->sql << R"(
      insert into play_stats (
        map,
        seconds_played,
        times_played,
        first_played,
        last_played,
        avg_playtime,
        avg_players
      ) values (
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?
      );
    )" << mi.mapname
                  << mi.pStats.secondsPlayed << mi.pStats.timesPlayed
                  << mi.pStats.firstPlayed << mi.pStats.lastPlayed
                  << mi.pStats.avgSecondsPlayed << mi.pStats.avgPlayers;

    database->sql << R"(
      insert into vote_stats (
        map,
        callvoted,
        rtv_voted,
        votes_passed
      ) values (
        ?,
        ?,
        ?,
        ?
      );
    )" << mi.mapname
                  << mi.vStats.callvoted << mi.vStats.rtvVoted
                  << mi.vStats.votesPassed;

    database->sql << "commit;";
  } catch (...) {
    database->sql << "rollback;";
    throw;
  }
}

void MapStatisticsRepository::updateMap(
    const MapStatisticsV2::MapInformation &mi) const {
  database->sql << "begin;";

  try {
    database->sql << R"(
      update
        play_stats
      set
        seconds_played=?,
        times_played=?,
        first_played=?,
        last_played=?,
        avg_playtime=?,
        avg_players=?
      where
        map=?;
      )" << mi.pStats.secondsPlayed
                  << mi.pStats.timesPlayed << mi.pStats.firstPlayed
                  << mi.pStats.lastPlayed << mi.pStats.avgSecondsPlayed
                  << mi.pStats.avgPlayers << mi.mapname;

    database->sql << R"(
      update
        vote_stats
      set
        callvoted=?,
        rtv_voted=?,
        votes_passed=?
      where
        map=?;
  )" << mi.vStats.callvoted
                  << mi.vStats.rtvVoted << mi.vStats.votesPassed << mi.mapname;

    database->sql << "commit";
  } catch (...) {
    database->sql << "rollback;";
    throw;
  }
}

std::vector<MapStatisticsV2::MapInformation>
MapStatisticsRepository::getMaps() const {
  std::vector<MapStatisticsV2::MapInformation> maps;

  database->sql << R"(
    select
      ps.map,
      ps.seconds_played,
      ps.times_played,
      ps.first_played,
      ps.last_played,
      ps.avg_playtime,
      ps.avg_players,
      vs.callvoted,
      vs.rtv_voted,
      vs.votes_passed
    from play_stats ps
    left join vote_stats vs on ps.map = vs.map
  )" >>
      [&](STANDARD_QUERY_PARAMS) {
        maps.emplace_back(getMapInfoFromStandardQuery(
            mapname, secondsPlayed, timesPlayed, firstPlayed, lastPlayed,
            avgPlaytime, avgPlayers, callvoted, rtvVoted, votesPassed));
      };

  return maps;
}

void MapStatisticsRepository::tryToMigrateMapstatistics() const {
  int count = 0;

  oldDatabase->sql << R"(
    select
      count(*)
    from sqlite_schema
    where
      tbl_name='map_statistics')" >>
      count;

  if (!count) {
    return;
  }

  std::vector<MapStatisticsV2::MapInformation> mapInfo;

  oldDatabase->sql << R"(
    select
      name,
      seconds_played,
      callvoted,
      votes_passed,
      times_played,
      last_played
    from map_statistics;
  )" >>
      [&mapInfo](const std::string &mapname, const uint32_t secondsPlayed,
                 const uint32_t callvoted, const uint32_t votesPassed,
                 const uint32_t timesPlayed, const uint64_t lastPlayed) {
        MapStatisticsV2::MapInformation mi{};

        mi.mapname = mapname;
        mi.pStats.secondsPlayed = secondsPlayed;
        mi.vStats.callvoted = callvoted;
        mi.vStats.votesPassed = votesPassed;
        mi.pStats.timesPlayed = timesPlayed;
        mi.pStats.lastPlayed = lastPlayed;

        // handle migration - set 'first played' to  'last played' date if
        // the map has already been played at least once,
        // and calculate average session time
        // FIXME: store seconds played at the time of migration separately
        //  so we can get a more realistic avg player count going forward
        if (mi.pStats.timesPlayed > 0) {
          mi.pStats.firstPlayed = mi.pStats.lastPlayed;
          mi.pStats.avgSecondsPlayed =
              mi.pStats.secondsPlayed / mi.pStats.timesPlayed;
        }

        mapInfo.emplace_back(mi);
      };

  for (const auto &mi : mapInfo) {
    insertMap(mi);
  }
}

void MapStatisticsRepository::initialize() const {
  database->addMigration("initial", {R"(
    create table play_stats (
      map text primary key not null unique,
      seconds_played integer not null,
      times_played integer not null,
      first_played integer not null,
      last_played integer not null,
      avg_playtime integer not null,
      avg_players real not null
    );
  )",
                                     R"(
    create table vote_stats (
      map text primary key not null unique,
      callvoted integer not null,
      rtv_voted integer not null,
      votes_passed integer not null
    );
  )",
                                     R"(
    create index idx_play_stats_map_seconds_played
      on play_stats(map, seconds_played);
  )"});

  database->applyMigrations();

  int count = 0;
  database->sql << "select count(*) from play_stats" >> count;

  if (!count) {
    tryToMigrateMapstatistics();
  }
}
} // namespace ETJump
