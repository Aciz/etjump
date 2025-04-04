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

#include <utility>

#include "etj_timerun_repository.h"

#include "etj_container_utilities.h"
#include "etj_database_v2.h"
#include "q_shared.h"

ETJump::Timerun::Record getRecordFromStandardQueryResult(
    int seasonId, std::string map, std::string runName, int userId, int time,
    std::string checkpointsString, std::string recordDate,
    std::string playerName, std::string metadataString) {
  auto checkpoints = ETJump::Container::map(
      ETJump::Container::filter(
          ETJump::StringUtil::split(checkpointsString, ","),
          [](const std::string &input) {
            return ETJump::trim(input).length() > 0;
          }),
      [](const std::string &checkpoint) {
        try {
          return std::stoi(ETJump::trim(checkpoint));
        } catch (const std::logic_error &) {
          return TIMERUN_CHECKPOINT_NOT_SET;
        }
      });
  ETJump::Time recordDateTime = ETJump::Time::fromString(recordDate);

  std::map<std::string, std::string> metadata;
  for (const auto &kvp :
       ETJump::Container::map(ETJump::StringUtil::split(metadataString, ","),
                              [](const std::string &kvp) {
                                return ETJump::StringUtil::split(kvp, "=");
                              })) {
    if (kvp.size() != 2) {
      continue;
    }

    metadata[kvp[0]] = kvp[1];
  }

  ETJump::Timerun::Record record;
  record.seasonId = seasonId;
  record.map = std::move(map);
  record.run = std::move(runName);
  record.userId = userId;
  record.time = time;
  record.recordDate = recordDateTime;
  record.checkpoints = std::move(checkpoints);
  record.playerName = std::move(playerName);
  record.metadata = metadata;

  return record;
}

void ETJump::TimerunRepository::initialize() { migrate(); }

void ETJump::TimerunRepository::shutdown() { _database = nullptr; }

std::vector<ETJump::Timerun::Record>
ETJump::TimerunRepository::getRecordsForPlayer(
    const std::vector<int> activeSeasons, const std::string &map, int userId) {
  auto parameters = StringUtil::join(
      Container::map(activeSeasons,
                     [](int season) { return std::to_string(season); }),
      ", ");

  auto binder = _database->sql
                << stringFormat(R"(
          select
            %s
          from record
          where season_id in (%s) and
            map=? and
            user_id=?;
        )",
                                _defaultRecordFieldsStr, parameters)
                << map << userId;

  auto records = getRecordsFromQuery(binder);

  return records;
}

ETJump::Timerun::Season
ETJump::TimerunRepository::addSeason(Timerun::AddSeasonParams params) {
  int count = 0;
  _database->sql << R"(
                      select count(name) from season where name=? collate nocase;
                    )"
                 << params.name >>
      count;

  if (count > 0) {
    throw std::runtime_error(stringFormat(
        "Cannot add season `%s` as it already exists.", params.name));
  }

  if (params.endTime.hasValue()) {
    if (params.startTime >= params.endTime.value()) {
      throw std::runtime_error("Start time cannot be after end time");
    }
  }

  std::string insert = R"(
                    insert into season (
                      name,
                      start_time,
                      end_time
                    ) values (
                      ?,
                      ?,
                      ?
                    );
                  )";

  if (params.endTime.hasValue())
    _database->sql << insert << params.name
                   << params.startTime.toDateTimeString()
                   << (*params.endTime).toDateTimeString();
  else
    _database->sql << insert << params.name
                   << params.startTime.toDateTimeString() << nullptr;

  return Timerun::Season{static_cast<int>(_database->sql.last_insert_rowid()),
                         params.name, params.startTime, params.endTime};
}

std::vector<ETJump::Timerun::Record>
ETJump::TimerunRepository::getRecordsForPlayer(
    const std::vector<int> &activeSeasons, const std::string &map,
    const std::string &run, int userId) {
  auto records = std::vector<Timerun::Record>();

  _database->sql << stringFormat(R"(
    select
      season_id,
      map,
      run,
      user_id,
      time,
      checkpoints,
      record_date,
      player_name,
      metadata
    from record
    where season_id in (%s) and
      map=? and
      run=? and
      user_id=?;
    )",
                                 StringUtil::join(activeSeasons, ", "))
                 << map << run << userId >>
      [&records](int seasonId, std::string map, std::string runName, int userId,
                 int time, std::string checkpointsString,
                 std::string recordDate, std::string playerName,
                 std::string metadataString) {
        auto record = getRecordFromStandardQueryResult(
            seasonId, map, runName, userId, time, checkpointsString, recordDate,
            playerName, metadataString);

        records.push_back(record);
      };

  return records;
}

std::vector<ETJump::Timerun::Record>
ETJump::TimerunRepository::getRecordsForRun(const std::string &map,
                                            const std::string &run) const {
  throw std::runtime_error("Not implemented");
}

void ETJump::TimerunRepository::insertRecord(const Timerun::Record &record) {
  _database->sql << R"(
    insert into record (
      season_id,
      map,
      run,
      user_id,
      time,
      checkpoints,
      record_date,
      player_name,
      metadata
    ) values (
      ?,
      ?,
      ?,
      ?,
      ?,
      ?,
      ?,
      ?,
      ?
    );
  )" << record.seasonId
                 << record.map << record.run << record.userId << record.time
                 << StringUtil::join(record.checkpoints, ",")
                 << record.recordDate.toDateTimeString() << record.playerName
                 << serializeMetadata(record.metadata);
}

void ETJump::TimerunRepository::updateRecord(const Timerun::Record &record) {
  _database->sql << R"(
    update
      record
    set
      time=?,
      checkpoints=?,
      record_date=?,
      player_name=?,
      metadata=?
    where
      season_id=? and
      map=? and
      run=? and
      user_id=?;
  )" << record.time
                 << StringUtil::join(record.checkpoints, ",")
                 << record.recordDate.toDateTimeString() << record.playerName
                 << serializeMetadata(record.metadata) << record.seasonId
                 << record.map << record.run << record.userId;
}

ETJump::opt<ETJump::Timerun::Record>
ETJump::TimerunRepository::getTopRecord(int seasonId, const std::string &map,
                                        const std::string &run) {
  opt<Timerun::Record> record;
  _database->sql << R"(
    select
      season_id,
      map,
      run,
      user_id,
      time,
      checkpoints,
      record_date,
      player_name,
      metadata
    from record
    where 
      season_id=? and
      map=? and
      run=?
    order by time asc
    limit 1
    )" << seasonId
                 << map << run >>
      [&record](int seasonId, std::string map, std::string runName, int userId,
                int time, std::string checkpointsString, std::string recordDate,
                std::string playerName, std::string metadataString) {
        record = opt<Timerun::Record>(getRecordFromStandardQueryResult(
            seasonId, map, runName, userId, time, checkpointsString, recordDate,
            playerName, metadataString));
      };

  return record;
}

std::vector<ETJump::Timerun::Record>
ETJump::TimerunRepository::getTopRecords(const std::vector<int> &seasonIds,
                                         const std::string &map,
                                         const std::string &run) const {
  auto seasonIdsPlaceholder = DatabaseV2::createPlaceholderString(seasonIds);

  std::string query = stringFormat(
      R"(
        select *
        from (select season_id,
                     map,
                     run,
                     user_id,
                     time,
                     checkpoints,
                     record_date,
                     player_name,
                     metadata,
                     rank() over (partition by season_id, map, run order by time asc) as rank
              FROM record
              where season_id in (%s)
                and map = ?
                and run = ?) as ranked_records
        where rank = 1;
      )",
      seasonIdsPlaceholder);

  auto binder = _database->sql << query;

  for (const auto &seasonId : seasonIds) {
    binder << seasonId;
  }

  binder << map << run;

  std::vector<Timerun::Record> records;
  binder >> [&records](int seasonId, std::string map, std::string runName,
                       int userId, int time, std::string checkpointsString,
                       std::string recordDate, std::string playerName,
                       std::string metadataString, int rank) {
    records.push_back(getRecordFromStandardQueryResult(
        seasonId, map, runName, userId, time, checkpointsString, recordDate,
        playerName, metadataString));
  };

  return records;
}

void ETJump::TimerunRepository::editSeason(
    const Timerun::EditSeasonParams &params) {
  int seasonId = -1;
  Time startTime;
  ETJump::opt<Time> endTime;

  _database->sql << R"(
    select
      id,
      start_time,
      end_time
    from season
    where name=?
    collate nocase
  )" << params.name >>
      [&](int sid, std::string s, std::unique_ptr<std::string> e) {
        seasonId = sid;
        startTime = Time::fromString(s);
        if (e) {
          endTime = ETJump::opt<ETJump::Time>(Time::fromString(*e));
        }
      };

  if (seasonId < 0) {
    throw std::runtime_error(
        stringFormat("No season matching name `%s`", params.name));
  }

  std::vector<std::string> updatedFields;
  std::vector<std::string> updatedParams;
  bool anythingToUpdate = false;

  Time newStartTime = startTime;
  opt<Time> newEndTime = endTime;

  if (params.startTime.hasValue()) {
    newStartTime = params.startTime.value();
    anythingToUpdate = true;
    updatedFields.emplace_back("start_time");
    updatedParams.push_back(params.startTime.value().toDateTimeString());
  }
  if (params.endTime.hasValue()) {
    newEndTime = params.endTime;
    anythingToUpdate = true;
    updatedFields.emplace_back("end_time");
    updatedParams.push_back(params.endTime.value().toDateTimeString());
  }

  if (newEndTime.hasValue() && newEndTime.value() < newStartTime) {
    throw std::runtime_error("End time cannot be before start time.");
  }

  if (!anythingToUpdate) {
    return;
  }

  std::string updatedFieldsString = StringUtil::join(
      Container::map(updatedFields, [](auto a) { return a + "=?"; }), ",");

  auto query = stringFormat(R"(
    update
      season
    set
      %s
    where
      id=?
  )",
                            updatedFieldsString);

  auto q = _database->sql << query;

  for (const auto &p : updatedParams) {
    q << p;
  }
  q << seasonId;
}

std::vector<std::string>
ETJump::TimerunRepository::getMapsForName(const std::string &map, bool exact) {

  std::string mapFilter = exact ? "map=?" : "map like ?";
  std::string mapSearchString = exact ? map : "%" + map + "%";

  std::vector<std::string> maps;
  _database->sql << stringFormat(R"(
    select
      distinct map
    from record
    where %s
    collate nocase
  )",
                                 mapFilter)
                 << mapSearchString >>
      [&maps](std::string map) { maps.push_back(map); };
  return maps;
}

std::vector<std::string>
ETJump::TimerunRepository::getRunsForName(const std::string &map,
                                          const std::string &run, bool exact,
                                          bool sanitizeResults) {

  std::string runFilter = exact ? "lsanitize(run)=?" : "lsanitize(run) like ?";
  std::string runSearchString = exact ? run : "%" + run + "%";

  std::vector<std::string> runs;
  _database->sql << stringFormat(R"(
    select
      distinct run
    from record
    where %s
      and map = ?
    collate nocase
  )",
                                 runFilter)
                 << runSearchString << map >>
      [&runs, sanitizeResults](const std::string &run) {
        runs.push_back(sanitizeResults ? sanitize(run, true) : run);
      };
  return runs;
}

std::vector<ETJump::Timerun::Record> ETJump::TimerunRepository::getRecords() {
  auto binder = _database->sql << R"(
    select
      season_id,
      map,
      run,
      user_id,
      time,
      checkpoints,
      record_date,
      player_name,
      metadata
    from record
    order by season_id, map, run, time;
  )";

  return getRecordsFromQuery(binder);
}

std::vector<ETJump::Timerun::Record> ETJump::TimerunRepository::getRecords(
    const Timerun::PrintRecordsParams &params) {
  const auto season =
      params.season.hasValue() ? params.season.value() : "Default";
  const std::string &map = params.map;
  const std::string &run = params.run.hasValue() ? params.run.value() : "";
  const bool runSpecified = !run.empty();

  const auto seasons = getSeasonsForName(season, false);

  if (seasons.empty()) {
    throw std::runtime_error(
        stringFormat("No season matches name `%s`", season));
  }

  const auto maps = getMapsForName(map, params.exactMap);
  bool exactMapFound = false;

  if (maps.size() > 1) {
    for (const auto &m : maps) {
      if (m == map) {
        exactMapFound = true;
        break;
      }
    }

    if (!exactMapFound) {
      std::string error = stringFormat(
          "^3records: ^7found %d maps matching ^3%s^7\n", maps.size(), map);

      const int perRow = 3;
      int i = 0;
      for (const auto &m : maps) {
        if (i != 0 && i % perRow == 0) {
          error += "\n";
        }

        error += stringFormat("%-22s", m);
        ++i;
      }

      throw std::runtime_error(error);
    }
  }

  // try to match a single run, so in scenarios where a map has runs
  // 'foo' and 'foobar' and query has 'foo' as the run param,
  // we get the exact match for the run 'foo' instead of exact and
  // partial matches to both 'foo' and 'foobar'
  std::vector<std::string> runs{};
  std::string runPlaceholder;
  std::string runBinder;

  if (runSpecified) {
    runPlaceholder = "and lsanitize(run) like ?";
    runs = getRunsForName(!maps.empty() ? maps[0] : map, run, true, true);
    runBinder = runs.size() == 1 ? runs[0] : "%" + run + "%";
  }

  const std::string seasonPlaceholders = StringUtil::join(
      Container::map(seasons, [](const auto &s) { return "season_id=?"; }),
      " or ");

  const std::string query = stringFormat(R"(
    select
      season_id,
      map,
      run,
      user_id,
      time,
      checkpoints,
      record_date,
      player_name,
      metadata
    from record
    where 
      (%s) and
      map=?
      %s
    collate nocase
    order by season_id, map, run, time asc
  )",
                                         seasonPlaceholders, runPlaceholder);

  auto binder = _database->sql << query;

  for (const auto &s : seasons) {
    binder << s.id;
  }

  binder << StringUtil::toLowerCase(!maps.empty() ? maps[0] : map);

  if (runSpecified) {
    binder << runBinder;
  }

  auto records = getRecordsFromQuery(binder);

  binder >> [&records](int seasonId, std::string map, std::string runName,
                       int userId, int time, std::string checkpointsString,
                       std::string recordDate, std::string playerName,
                       std::string metadataString) {
    const auto record = getRecordFromStandardQueryResult(
        seasonId, std::move(map), std::move(runName), userId, time,
        std::move(checkpointsString), std::move(recordDate),
        std::move(playerName), std::move(metadataString));

    records.push_back(record);
  };

  return records;
}

std::vector<ETJump::Timerun::Season>
ETJump::TimerunRepository::getSeasonsForName(const std::string &name,
                                             bool exact) {
  std::string query;

  std::vector<Timerun::Season> seasons;

  auto handler = [&seasons](int id, const std::string &name,
                            const std::string &startTime,
                            std::unique_ptr<std::string> endTime) {
    seasons.push_back(Timerun::Season{
        id, name, Time::fromString(startTime),
        (endTime ? opt<Time>(Time::fromString(*endTime)) : opt<Time>())});
  };

  if (exact) {
    query = R"(
      select
        id,
        name,
        start_time,
        end_time
      from season
      where name=?
      collate nocase
    )";

    _database->sql << query << name >> handler;
  } else {
    query = R"(
      select
        id,
        name,
        start_time,
        end_time
      from season
      where name like ?
      collate nocase
    )";

    _database->sql << query << "%" + name + "%" >> handler;
  }

  return seasons;
}

ETJump::opt<ETJump::Timerun::Record>
ETJump::TimerunRepository::getRecord(const std::string &map,
                                     const std::string &run, int rank) {
  opt<Timerun::Record> record;

  _database->sql << R"(
    select *
      from (
        select
          season_id,
          map,
          run,
          user_id,
          time,
          checkpoints,
          record_date,
          player_name,
          metadata,
          rank() over (partition by season_id, map, run order by time asc) as rank
        FROM record
        where season_id=1 and map=? and lsanitize(run)=?
      ) as ranked_records
      where rank = ?;
  )" << map << run
                 << rank >>
      [&record](int seasonId, std::string map, std::string runName, int userId,
                int time, std::string checkpointsString, std::string recordDate,
                std::string playerName, std::string metadataString, int rank) {
        record = getRecordFromStandardQueryResult(
            seasonId, map, runName, userId, time, checkpointsString, recordDate,
            playerName, metadataString);
      };

  return record;
}

std::vector<ETJump::Timerun::Season> ETJump::TimerunRepository::getSeasons() {
  std::vector<Timerun::Season> seasons;

  _database->sql << R"(
    select
      id,
      name,
      start_time,
      end_time
    from season;
  )" >>
      [this, &seasons](int id, std::string name, std::string startTimeStr,
                       std::string endTimeStr) {
        auto startTime = Time::fromString(startTimeStr);
        opt<Time> endTime;
        if (endTimeStr.length() != 0) {
          endTime = opt<Time>(Time::fromString(endTimeStr));
        }

        seasons.push_back(Timerun::Season{id, name, startTime, endTime});
      };

  return seasons;
}

void ETJump::TimerunRepository::deleteSeason(const std::string &name) {
  if (name == "default") {
    throw std::runtime_error("Cannot delete default season.");
  }
  int id = 0;
  _database->sql << "select coalesce((select id from season where name=? "
                    "collate nocase), -1);"
                 << name >>
      id;
  if (id < 0) {
    throw std::runtime_error(stringFormat("Season `%s` does not exist.", name));
  }

  // for some reason someone named it something else
  if (id == 1) {
    throw std::runtime_error("Cannot delete default season.");
  }

  _database->sql << "delete from record where season_id=?;" << id;
  _database->sql << "delete from season where id=?" << id;
}

void ETJump::TimerunRepository::tryToMigrateRecords() {
  int count = 0;
  _oldDatabase->sql
          << "select count(*) from sqlite_master where tbl_name='records'" >>
      count;
  if (count == 0) {
    return;
  }

  std::vector<Timerun::Record> oldRecords;

  _oldDatabase->sql << R"(
    select
        id,
        time,
        record_date,
        map,
        run,
        user_id,
        player_name
    from records;
  )" >>
      [&oldRecords](int id, int time, int recordDate, std::string map,
                    std::string run, int userId, std::string playerName) {
        Timerun::Record r{};
        r.seasonId = 1;
        r.map = map;
        r.run = run;
        r.time = time;
        r.recordDate = Time::fromInt(recordDate);
        r.userId = userId;
        r.playerName = playerName;
        r.checkpoints = std::vector<int>(MAX_TIMERUN_CHECKPOINTS,
                                         TIMERUN_CHECKPOINT_NOT_SET);
        r.metadata = {{"mod_version", "unknown(imported)"}};
        oldRecords.push_back(r);
      };

  _database->sql << "begin;";

  for (const auto &r : oldRecords) {
    insertRecord(r);
  }

  _database->sql << "commit;";
}

void ETJump::TimerunRepository::migrate() {
  _database->addMigration(
      // clang-format off
      "initial",
      {R"(
          create table season (
              id integer primary key autoincrement,
              name text not null,
              start_time timestamp not null,
              end_time timestamp null
          );
        )",
       R"(
            insert into season (
              id,
              name,
              start_time,
              end_time
            ) values (
              1,
              'Default',
              '2000-01-01 00:00:00',
              null
            );
          )",
       R"(
            create table record (
              season_id integer not null,
              map text not null,
              run text not null,
              user_id int not null,
              time int not null,
              checkpoints text not null,
              record_date timestamp not null,
              player_name text not null,
              metadata text not null default '',
              primary key (season_id, map, run, user_id),
              foreign key (season_id) references season(id)
            );
          )",
       "create index idx_season_id_map on record(season_id, map);",
       "create index idx_season_id_map_run on record(season_id, map, run);",
       "create index idx_season_id_map_run_user_id on record(season_id, map, run, user_id);"});
  // clang-format on

  _database->applyMigrations();

  int count = 0;
  _database->sql << "select count(*) from record" >> count;

  if (count == 0) {
    tryToMigrateRecords();
  }
}

std::string ETJump::TimerunRepository::serializeMetadata(
    std::map<std::string, std::string> metadata) {
  std::string result;
  for (const auto &kvp : metadata) {
    // NOTE: we're not escaping = so if payload contains = this will fail...
    result += kvp.first + "=" + kvp.second;
  }
  return result;
}

std::vector<ETJump::Timerun::Record>
ETJump::TimerunRepository::getRecordsFromQuery(
    sqlite::database_binder &binder) {
  std::vector<Timerun::Record> records;
  binder >> [&records](int seasonId, std::string map, std::string runName,
                       int userId, int time, std::string checkpointsString,
                       std::string recordDate, std::string playerName,
                       std::string metadataString) {
    auto record = getRecordFromStandardQueryResult(
        seasonId, map, runName, userId, time, checkpointsString, recordDate,
        playerName, metadataString);

    records.push_back(record);
  };
  return records;
}
