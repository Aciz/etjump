#include "g_local.h"
#include <vector>
#include <regex>
#include "etj_printer.h"
#include "etj_operation_result.h"
#include "etj_save_system.h"
#include "etj_entity_utilities.h"
#include "etj_string_utilities.h"
#include "etj_rtv.h"
#include "etj_utilities.h"
#include "etj_chat_replay.h"
#include "etj_session.h"
#include "etj_commands.h"
#include "etj_map_statistics.h"

namespace ETJump {
enum class VotingTypes {
  VoteYes,
  VoteNo,
  RevoteYes,
  RevoteNo,
  VoteRtv,
  RevoteRtv
};

inline constexpr int VOTING_ATTEMPTS = 3;
inline constexpr int VOTING_TIMEOUT = 1000;
} // namespace ETJump

/*
==================
G_SendScore

Sends current scoreboard information
==================
*/
void G_SendScore(gentity_t *ent) {
  char entry[128];
  int i;
  gclient_t *cl;
  int numSorted;
  int team, size, count;
  char buffer[1024];
  char startbuffer[32];

  // send the latest information on all clients
  numSorted = level.numConnectedClients;
  if (numSorted > 64) {
    numSorted = 64;
  }

  i = 0;
  // Gordon: team doesnt actually mean team, ignore...
  for (team = 0; team < 2; team++) {
    *buffer = '\0';
    *startbuffer = '\0';
    if (team == 0) {
      Q_strncpyz(startbuffer,
                 va("sc0 %i %i", level.teamScores[TEAM_AXIS],
                    level.teamScores[TEAM_ALLIES]),
                 32);
    } else {
      Q_strncpyz(startbuffer, "sc1", 32);
    }
    size = strlen(startbuffer) + 1;
    count = 0;

    for (; i < numSorted; i++) {
      int j, totalXP;
      int ping, playerClass;

      cl = &level.clients[level.sortedClients[i]];

      if (g_entities[level.sortedClients[i]].r.svFlags & SVF_POW) {
        continue;
      }

      playerClass = cl->ps.stats[STAT_PLAYER_CLASS];

      if (cl->pers.connected == CON_CONNECTING) {
        ping = -1;
      } else {
        ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
      }

      for (totalXP = 0, j = 0; j < SK_NUM_SKILLS; j++) {
        totalXP += cl->sess.skillpoints[j];
      }

      Com_sprintf(entry, sizeof(entry), " %i %i %i %i %i %i %i",
                  level.sortedClients[i], totalXP, ping,
                  (level.time - cl->pers.enterTime) / 60000,
                  g_entities[level.sortedClients[i]].s.powerups, playerClass,
                  cl->ps.clientNum);

      if (size + strlen(entry) > 1000) {
        break;
      }
      size += strlen(entry);

      Q_strcat(buffer, 1024, entry);
      if (++count >= 32) {
        i++; // we need to redo this client in
             // the next buffer (if we can)
        break;
      }
    }

    if (count > 0 || team == 0) {
      trap_SendServerCommand(ent - g_entities,
                             va("%s %i%s", startbuffer, count, buffer));
    }
  }
}

/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f(gentity_t *ent) {
  ent->client->wantsscore = qtrue;
  //	G_SendScore( ent );
}

/*
==================
CheatsOk
==================
*/
qboolean CheatsOk(gentity_t *ent) {
  if (!g_cheats.integer) {
    trap_SendServerCommand(ent - g_entities,
                           va("print \"Cheats are not enabled "
                              "on this server.\n\""));
    return qfalse;
  }
  if (ent->health <= 0) {
    trap_SendServerCommand(ent - g_entities, va("print \"You must be alive to "
                                                "use this command.\n\""));
    return qfalse;
  }
  return qtrue;
}

/*
==================
ConcatArgs
==================
*/
char *ConcatArgs(int start) {
  int i, c, tlen;
  static char line[MAX_STRING_CHARS];
  int len;
  char arg[MAX_STRING_CHARS];

  len = 0;
  c = trap_Argc();
  for (i = start; i < c; i++) {
    trap_Argv(i, arg, sizeof(arg));
    tlen = strlen(arg);
    if (len + tlen >= MAX_STRING_CHARS - 1) {
      break;
    }
    memcpy(line + len, arg, tlen);
    len += tlen;
    if (i != c - 1) {
      line[len] = ' ';
      len++;
    }
  }

  line[len] = 0;

  return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/

void SanitizeString(char *in, char *out, qboolean fToLower) {
  while (*in) {
    if (*in == 27 || *in == '^') {
      in++; // skip color code
      if (*in) {
        in++;
      }
      continue;
    }

    if (*in < 32) {
      in++;
      continue;
    }

    *out++ = (fToLower) ? tolower(*in++) : *in++;
  }

  *out = 0;
}

void SanitizeConstString(const char *in, char *out, qboolean fToLower) {
  while (*in) {
    if (*in == 27 || *in == '^') {
      in++; // skip color code
      if (*in) {
        in++;
      }
      continue;
    }

    if (*in < 32) {
      in++;
      continue;
    }

    *out++ = (fToLower) ? tolower(*in++) : *in++;
  }

  *out = 0;
}

int CleanStrlen(const char *in) {
  int len = 0;
  while (*in) {
    if (*in == 27 || *in == '^') {
      in++;
      if (*in) {
        in++;
      }
      continue;
    }

    if (*in < 32) {
      in++;
      continue;
    }

    len++;
    in++;
  }
  return len;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString(gentity_t *to, char *s) {
  gclient_t *cl;
  int idnum;
  char s2[MAX_STRING_CHARS];
  char n2[MAX_STRING_CHARS];
  qboolean fIsNumber = qtrue;
  int partialMatchs = 0;
  int partialMatchId = -1;

  // See if its a number or string
  for (idnum = 0; idnum < static_cast<int>(strlen(s)) && s[idnum] != 0;
       idnum++) {
    if (s[idnum] < '0' || s[idnum] > '9') {
      fIsNumber = qfalse;
      break;
    }
  }

  // check for a name match
  SanitizeString(s, s2, qtrue);
  for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++) {
    if (cl->pers.connected != CON_CONNECTED) {
      continue;
    }

    SanitizeString(cl->pers.netname, n2, qtrue);
    if (!strcmp(n2, s2)) {
      return (idnum);
    }

    if (strstr(n2, s2) != NULL) {
      partialMatchs++;
      partialMatchId = idnum;
    }
  }

  // numeric values are just slot numbers
  if (fIsNumber) {
    idnum = Q_atoi(s);
    if (idnum < 0 || idnum >= level.maxclients) {
      CPx(to - g_entities, va("print \"Bad client slot: [lof]%i\n\"", idnum));
      return -1;
    }

    cl = &level.clients[idnum];
    if (cl->pers.connected != CON_CONNECTED) {
      CPx(to - g_entities, va("print \"Client[lof] %i "
                              "[lon]is not active\n\"",
                              idnum));
      return -1;
    }
    return (idnum);
  }

  if (partialMatchs > 1) {
    CPx(to - g_entities, "print \"Several partial matches\n\"");
    return -1;
  }
  if (partialMatchs == 1) {
    return partialMatchId;
  }

  CPx(to - g_entities,
      va("print \"User [lof]%s [lon]is not on the server\n\"", s));
  return (-1);
}

/*
==================
ClientNumbersFromString

Sets plist to an array of integers that represent client numbers that have
names that are a partial match for s. List is terminated by a -1.

Returns number of matching clientids.
==================
*/
int ClientNumbersFromString(const char *s, int *plist) {
  gclient_t *p;
  int i, found = 0;
  char s2[MAX_STRING_CHARS];
  char n2[MAX_STRING_CHARS];
  char *m;
  qboolean is_slot = qtrue;

  *plist = -1;

  // if a number is provided, it might be a slot #
  for (i = 0; i < (int)strlen(s); i++) {
    if (s[i] < '0' || s[i] > '9') {
      is_slot = qfalse;
      break;
    }
  }
  if (is_slot) {
    i = Q_atoi(s);
    if (i >= 0 && i < level.maxclients) {
      p = &level.clients[i];
      if (p->pers.connected == CON_CONNECTED ||
          p->pers.connected == CON_CONNECTING) {

        *plist++ = i;
        *plist = -1;
        return 1;
      }
    }
  }

  // now look for name matches
  SanitizeConstString(s, s2, qtrue);
  if (strlen(s2) < 1) {
    return 0;
  }
  for (i = 0; i < level.maxclients; i++) {
    p = &level.clients[i];
    if (p->pers.connected != CON_CONNECTED &&
        p->pers.connected != CON_CONNECTING) {

      continue;
    }
    SanitizeString(p->pers.netname, n2, qtrue);
    m = strstr(n2, s2);
    if (m != NULL) {
      *plist++ = i;
      found++;
    }
  }
  *plist = -1;
  return found;
}

qboolean G_MatchOnePlayer(int *plist, char *err, int len, team_t filter) {
  gclient_t *cl;
  int *p;
  char line[MAX_NAME_LENGTH + 10];
  int matches = 0;

  err[0] = '\0';
  line[0] = '\0';
  if (plist[0] == -1) {
    Q_strcat(err, len, "no connected player by that name or slot #");
    return qfalse;
  }
  if (plist[1] != -1) {
    Q_strcat(err, len,
             "more than one player name matches. "
             "be more specific or use the slot #:");

    for (p = plist; *p != -1; p++) {
      cl = &level.clients[*p];
      if (cl->pers.connected == CON_CONNECTED ||
          cl->pers.connected == CON_CONNECTING) {
        // ETJump: filtering out specific team
        if (cl->sess.sessionTeam == filter) {
          continue;
        }

        Com_sprintf(line, MAX_NAME_LENGTH + 10, "\n%2i - %s^7", *p,
                    cl->pers.netname);
        if (static_cast<int>(strlen(err) + strlen(line)) > len) {
          break;
        }

        Q_strcat(err, len, line);
        // ETJump: save the last matching result
        // in plist, so we could
        //         use it if we end up having 1
        //         match
        *plist = *p;

        matches++;
      }
    }
    // ETJump: we get one match after filtering out spectators,
    //         plist holds the result
    if (matches == 1) {
      return qtrue;
    }
    // ETJump: no matches after filtering out spectators
    if (!matches) {
      err[0] = '\0';
      Q_strcat(err, len, "no active player by that name or slot #");
    }

    return qfalse;
  }
  return qtrue;
}

// Updates voting stats
namespace ETJump {
void updateVotingInfo(gentity_t *ent, int mapNum, VotingTypes vote) {
  auto client = ent->client;
  const int clientNum = ClientNum(ent);
  const bool isRtvVote = game.rtv->rtvVoteActive();
  auto rtvMaps = game.rtv->getRtvMaps();

  switch (vote) {
    case VotingTypes::VoteYes:
      level.voteInfo.voteYes++;
      client->pers.votingInfo.isVotedYes = true;
      break;
    case VotingTypes::VoteNo:
      level.voteInfo.voteNo++;
      client->pers.votingInfo.isVotedYes = false;
      break;
    case VotingTypes::RevoteYes:
      level.voteInfo.voteYes++;
      level.voteInfo.voteNo--;
      client->pers.votingInfo.isVotedYes = true;
      break;
    case VotingTypes::RevoteNo:
      level.voteInfo.voteYes--;
      level.voteInfo.voteNo++;
      client->pers.votingInfo.isVotedYes = false;
      if (isRtvVote) {
        (*rtvMaps)[ent->client->pers.votingInfo.lastRtvMapVoted].second--;
      }
      break;
    case VotingTypes::VoteRtv:
      (*rtvMaps)[mapNum].second++;
      ent->client->pers.votingInfo.lastRtvMapVoted = mapNum;
      client->pers.votingInfo.isVotedYes = true;
      level.voteInfo.voteYes++;
      break;
    case VotingTypes::RevoteRtv:
      // only reduce the map vote count if we've already voted yes
      // as this gets called on 'vote no' -> 're-vote yes' scenarios too
      if (client->pers.votingInfo.isVotedYes) {
        (*rtvMaps)[ent->client->pers.votingInfo.lastRtvMapVoted].second--;
      } else {
        level.voteInfo.voteNo--;
        level.voteInfo.voteYes++;
      }

      (*rtvMaps)[mapNum].second++;
      ent->client->pers.votingInfo.lastRtvMapVoted = mapNum;
      client->pers.votingInfo.isVotedYes = true;
      break;
  }

  client->pers.votingInfo.time = level.time;
  client->pers.votingInfo.attempts++;

  if (client->pers.votingInfo.isVotedYes) {
    trap_SendServerCommand(clientNum, "voted yes");
  } else {
    trap_SendServerCommand(clientNum, "voted no");
  }

  // note: we must always send this during rtv to update map vote counts,
  // as someone might vote for a map initially, but then re-vote no
  if (isRtvVote) {
    game.rtv->setRtvConfigstrings();
  } else {
    trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteInfo.voteYes));
  }

  trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteInfo.voteNo));
}
} // namespace ETJump

/*
=================
Cmd_ListBotGoals_f

=================
*/

void Cmd_ListBotGoals_f(gentity_t *ent) {
  int i;
  team_t t;

  if (!CheatsOk(ent)) {
    return;
  }

  int ti = TEAM_AXIS;
  for (ti = TEAM_AXIS; ti <= TEAM_ALLIES; ti++) {
    t = static_cast<team_t>(ti);
    gentity_t *list = g_entities, *targ;

    G_Printf("\n%s bot goals\n=====================\n",
             (t == TEAM_AXIS ? "Axis" : "Allies"));

    for (i = 0; i < level.num_entities; i++, list++) {
      if (!list->inuse) {
        continue;
      }

      if (!(list->aiInactive & (1 << t))) {
        G_Printf("%s (%s)",
                 (list->scriptName
                      ? list->scriptName
                      : (list->targetname ? list->targetname : "NONE")),
                 list->classname);
        if (list->target_ent) {
          targ = list->target_ent;
          G_Printf(" -> ");
          G_Printf("%s (%s)",
                   (targ->scriptName
                        ? targ->scriptName
                        : (targ->targetname ? targ->targetname : "NONE")),
                   targ->classname);
        }
        G_Printf("\n");
      }
    }
  }
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f(gentity_t *ent) {
  char *name, *amt;
  int i;
  qboolean give_all;
  int amount;
  qboolean hasAmount = qfalse;
  if (!CheatsOk(ent)) {
    return;
  }

  //----(SA)	check for an amount (like "give health 30")
  amt = ConcatArgs(2);
  if (*amt) {
    hasAmount = qtrue;
  }
  amount = Q_atoi(amt);
  //----(SA)	end

  name = ConcatArgs(1);

  if (Q_stricmp(name, "all") == 0) {
    give_all = qtrue;
  } else {
    give_all = qfalse;
  }

  if (Q_stricmpn(name, "skill", 5) == 0) {
    if (hasAmount) {
      if (amount >= 0 && amount < SK_NUM_SKILLS) {
        G_AddSkillPoints(ent, static_cast<skillType_t>(amount), 20);
        G_DebugAddSkillPoints(ent, static_cast<skillType_t>(amount), 20,
                              "give skill");
      }
    } else {
      // bumps all skills with 1 level
      for (i = 0; i < SK_NUM_SKILLS; i++) {
        G_AddSkillPoints(ent, static_cast<skillType_t>(i), 20);
        G_DebugAddSkillPoints(ent, static_cast<skillType_t>(i), 20,
                              "give skill");
      }
    }
    return;
  }

  if (Q_stricmpn(name, "medal", 5) == 0) {
    for (i = 0; i < SK_NUM_SKILLS; i++) {
      if (!ent->client->sess.medals[i]) {
        ent->client->sess.medals[i] = 1;
      }
    }
    ClientUserinfoChanged(ClientNum(ent));
    return;
  }

  if (give_all || Q_stricmpn(name, "health", 6) == 0) {
    //----(SA)	modified
    if (amount) {
      // go through proper damage routine for negative give health,
      // so we set up things correctly in case this kills us
      if (amount < 0) {
        // ignore nofatigue permanent adrenaline
        if (ent->client && ent->client->pers.nofatigue) {
          amount *= 2;
        }

        G_Damage(ent, ent, ent, nullptr, nullptr, std::abs(amount),
                 DAMAGE_NO_PROTECTION, MOD_UNKNOWN);
      } else {
        ent->health += amount;
      }
    } else {
      ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
    }
    if (!give_all) {
      return;
    }
  }

  if (give_all || Q_stricmp(name, "weapons") == 0) {
    for (i = 0; i < WP_NUM_WEAPONS; i++) {
      if (BG_WeaponInWolfMP(i)) {
        COM_BitSet(ent->client->ps.weapons, i);
      }
    }

    if (!give_all) {
      return;
    }
  }

  if (give_all || Q_stricmpn(name, "ammo", 4) == 0) {
    if (amount) {
      if (ent->client->ps.weapon && ent->client->ps.weapon != WP_SATCHEL &&
          ent->client->ps.weapon != WP_SATCHEL_DET) {
        Add_Ammo(ent, ent->client->ps.weapon, amount, qtrue);
      }
    } else {
      for (i = 1; i < WP_NUM_WEAPONS; i++) {
        if (COM_BitCheck(ent->client->ps.weapons, i) && i != WP_SATCHEL &&
            i != WP_SATCHEL_DET) {
          Add_Ammo(ent, i, 9999, qtrue);
        }
      }
    }

    if (!give_all) {
      return;
    }
  }

  //	"give allammo <n>" allows you to give a specific amount of ammo
  // to /all/
  // weapons while 	allowing "give ammo <n>" to only give to the
  // selected weap.
  if (Q_stricmpn(name, "allammo", 7) == 0 && amount) {
    for (i = 1; i < WP_NUM_WEAPONS; i++) Add_Ammo(ent, i, amount, qtrue);

    if (!give_all) {
      return;
    }
  }

  //---- (SA) Wolf keys
  if (give_all || Q_stricmp(name, "keys") == 0) {
    ent->client->ps.stats[STAT_KEYS] = (1 << KEY_NUM_KEYS) - 2;
    if (!give_all) {
      return;
    }
  }
  //---- (SA) end
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f(gentity_t *ent) {
  if (!g_cheats.integer) {
    if (ent->client->sess.timerunActive) {
      Printer::center(ent, "You cannot use cheats while timerun is active.");
      return;
    }

    if (level.noGod) {
      Printer::center(ent, "^3god ^7has been disabled on this map.");
      return;
    }
  }

  if (!CheatsOk(ent)) {
    return;
  }

  const char *msg;
  const char *arg = ConcatArgs(1);

  if (!Q_stricmp(arg, "on") || Q_atoi(arg)) {
    ent->flags |= FL_GODMODE;
  } else if (!Q_stricmp(arg, "off") || !Q_stricmp(arg, "0")) {
    ent->flags &= ~FL_GODMODE;
  } else {
    ent->flags ^= FL_GODMODE;
  }
  if (!(ent->flags & FL_GODMODE)) {
    msg = "godmode OFF\n";
  } else {
    msg = "godmode ON\n";
  }

  trap_SendServerCommand(ClientNum(ent), va("print \"%s\"", msg));
}

/*
==================
Cmd_Nofatigue_f

Sets client to nofatigue

argv(0) nofatigue
==================
*/

void Cmd_Nofatigue_f(gentity_t *ent) {
  const char *msg;

  char *name = ConcatArgs(1);

  if (!CheatsOk(ent)) {
    return;
  }

  if (!Q_stricmp(name, "on") || Q_atoi(name)) {
    ent->flags |= FL_NOFATIGUE;
  } else if (!Q_stricmp(name, "off") || !Q_stricmp(name, "0")) {
    ent->flags &= ~FL_NOFATIGUE;
  } else {
    ent->flags ^= FL_NOFATIGUE;
  }

  if (!(ent->flags & FL_NOFATIGUE)) {
    msg = "nofatigue OFF\n";
  } else {
    msg = "nofatigue ON\n";
  }

  trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f(gentity_t *ent) {
  const char *msg;

  if (!CheatsOk(ent)) {
    return;
  }

  ent->flags ^= FL_NOTARGET;
  if (!(ent->flags & FL_NOTARGET)) {
    msg = "notarget OFF\n";
  } else {
    msg = "notarget ON\n";
  }

  trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

void capitalizeWithColor(std::string &str) {
  if (str.length() > 0) {
    unsigned char index = str[0] == '^' ? 2 : 0;
    str[index] = toupper(str[index]);
  }
}

namespace ETJump {
OperationResult canNoclip(gentity_t *ent) {
  if (!ent || !ent->client) {
    return {false, "^7Non-player entities cannot use ^3%s^7.\n"};
  }

  if (!g_cheats.integer) {
    if (ent->client->sess.timerunActive) {
      return {false, "^7Cannot use ^3%s ^7while timer is running.\n"};
    }

    if (ent->client->sess.deathrunFlags &
        static_cast<int>(DeathrunFlags::Active)) {
      return {false, "^7Cannot use ^3%s ^7while death run is active.\n"};
    }

    if (Utilities::inNoNoclipArea(ent)) {
      return {false, "^7You cannot ^3%s ^7inside this area.\n"};
    }

    if (ent->client->pers.noclipCount == 0 && !ent->client->noclip) {
      if (!g_noclip.integer && !CheatsOk(ent)) {
        return {false, "^7You can no longer use ^3%s^7.\n"};
      }
    }
  }

  if (ent->client->ps.eFlags & EF_DEAD) {
    return {false, "^7Cannot use ^3%s ^7in this state.\n"};
  }

  return {true, ""};
}

void decreaseNoclipCount(gentity_t *ent, const std::string &action) {
  if (!ent || !ent->client) {
    return;
  }

  if (ent->client->pers.noclipCount > 0) {
    --ent->client->pers.noclipCount;
    Printer::center(
        ClientNum(ent),
        ETJump::stringFormat("^7You may use ^3%s ^2%d^7 more times.\n", action,
                             ent->client->pers.noclipCount));
  }
}

const float MaxAxisOffset = 4096.f;
// offsets player's position by given vector if noclip is available
void setPlayerOffset(gentity_t *ent) {
  static char buffer[64];
  const int clientNum = ClientNum(ent);

  if (trap_Argc() != 4) {
    Printer::console(
        clientNum,
        "^3Usage: ^7setoffset x y z\n"
        "Changes your position into the direction of X Y Z vector.\n");
    return;
  }

  auto result = canNoclip(ent);

  if (!result.success) {
    std::string str = ETJump::stringFormat(result.message, "setoffset");
    capitalizeWithColor(str);
    Printer::console(clientNum, str);
    return;
  }

  vec3_t dst{};
  VectorCopy(ent->client->ps.origin, dst);

  for (auto i = 0; i < 3; i++) {
    trap_Argv(i + 1, buffer, sizeof buffer);
    const float value = Q_atof(buffer);
    dst[i] += std::clamp(value, -MaxAxisOffset, MaxAxisOffset);
  }

  // check if there is a non-noclippable content between our position and
  // current target
  trace_t trace;
  trap_TraceCapsule(&trace, ent->client->ps.origin, ent->client->ps.mins,
                    ent->client->ps.maxs, dst, ent->client->ps.clientNum,
                    CONTENTS_NONOCLIP);

  if (!g_cheats.integer && level.noNoclip == (trace.fraction == 1.0f)) {
    Printer::console(clientNum, "^7You cannot ^3setoffset ^7to this area.\n");
    return;
  }

  decreaseNoclipCount(ent, "setoffset");
  ent->client->setoffsetThisLife = true;

  // reset speed
  VectorClear(ent->client->ps.velocity);
  DirectTeleport(ent, dst, ent->client->ps.viewangles);
}

void interruptRun(gentity_t *ent) {
  auto clientNum = ClientNum(ent);

  if (ent->client->sess.timerunActive) {
    InterruptRun(ent);
    Printer::console(clientNum, "Timerun interrupted.\n");
  } else {
    Printer::console(clientNum, "No timerun currently active.\n");
  }
}

bool checkTrackerIndex(int clientNum, int idx, char *buffer, bool noIndex) {
  if (((idx < 1 || idx > MAX_PROGRESSION_TRACKERS) && !noIndex) ||
      !(std::regex_match(buffer, std::regex("^-*\\d+")))) {
    Printer::console(
        clientNum,
        "^3Error: ^7Invalid index specified. Valid range is ^31-50^7.\n");
    return false;
  }

  return true;
}

bool checkTrackerValue(int clientNum, char *value) {
  if (!std::regex_match(value, std::regex("^[+-]?\\d+"))) {
    Printer::console(clientNum,
                     "^3Error: ^7Specified value is not an integer.\n");
    return false;
  }

  return true;
}

void printTracker(gentity_t *ent) {
  std::string printTrackerMsg;
  auto clientNum = ClientNum(ent);
  static char buffer[16];
  int i;

  if (g_debugTrackers.integer <= 0) {
    Printer::console(clientNum,
                     "^3Error: ^7Tracker debugging is not enabled.\n");
    return;
  }

  if (trap_Argc() < 2) {
    printTrackerMsg = stringFormat("Index: ^3%i ^7value: ^2%i\n", 1,
                                   ent->client->sess.progression[0]);
    Printer::console(clientNum, printTrackerMsg);
    return;
  }

  trap_Argv(1, buffer, sizeof buffer);

  if (!Q_stricmp("all", buffer)) {
    for (i = 0; i < MAX_PROGRESSION_TRACKERS; i++) {
      printTrackerMsg = stringFormat("Index: ^3%i ^7value: ^2%i\n", i + 1,
                                     ent->client->sess.progression[i]);
      Printer::console(clientNum, printTrackerMsg);
    }
  } else {
    int numArgs = trap_Argc();

    for (i = 1; i < numArgs; i++) {
      trap_Argv(i, buffer, sizeof buffer);
      int idx = Q_atoi(buffer);

      if (checkTrackerIndex(clientNum, idx, buffer, false)) {
        printTrackerMsg = stringFormat("Index: ^3%i ^7value: ^2%i\n", idx,
                                       ent->client->sess.progression[idx - 1]);
        Printer::console(clientNum, printTrackerMsg);
      }
    }
  }
}

void setTracker(gentity_t *ent) {
  std::string setTrackerMsg;
  auto clientNum = ClientNum(ent);
  static char bufferIndex[16];
  static char bufferValue[16];
  int i, value;
  bool noIndex = false;

  // Check this first before g_debugTrackers so help can be printed even
  // when debugging isn't enabled.
  if (trap_Argc() < 2 || trap_Argc() > 3) {
    Printer::console(clientNum,
                     "^3Usage: ^7tracker_set <index|all> <value>\n"
                     "Sets your tracker value on specified index to N. If "
                     "index isn't specified, defaults to index 1.\n");
    return;
  }

  if (g_debugTrackers.integer <= 0) {
    Printer::console(clientNum,
                     "^3Error: ^7Tracker debugging is not enabled.\n");
    return;
  }

  trap_Argv(1, bufferIndex, sizeof bufferIndex);
  int idx = Q_atoi(bufferIndex);

  if (!Q_stricmp("all", bufferIndex)) {
    if (trap_Argc() == 2) {
      Printer::console(clientNum, "^3Error: ^7value not specified.\n");
      return;
    }

    trap_Argv(2, bufferValue, sizeof bufferValue);
    value = Q_atoi(bufferValue);

    if (checkTrackerValue(clientNum, bufferValue)) {
      for (i = 0; i < MAX_PROGRESSION_TRACKERS; i++) {
        ent->client->sess.progression[i] = value;
      }

      setTrackerMsg = stringFormat("^7Set tracker value on all "
                                   "indices to ^2%i^7.\n",
                                   value);
      Printer::console(clientNum, setTrackerMsg);
    }
  }

  else {
    if (trap_Argc() == 2) {
      noIndex = true;
      if (checkTrackerIndex(clientNum, idx, bufferIndex, noIndex)) {
        ent->client->sess.progression[0] = idx; // No index specified, use it
                                                // for value on index 1
        setTrackerMsg = stringFormat("^7Tracker set - index: ^31 "
                                     "^7value: ^2%i\n",
                                     idx);
        Printer::console(clientNum, setTrackerMsg);
        return;
      }
    }

    if (trap_Argc() == 3) {
      trap_Argv(2, bufferValue, sizeof bufferValue);
      noIndex = false;

      if (!checkTrackerIndex(clientNum, idx, bufferIndex, noIndex)) {
        checkTrackerValue(clientNum,
                          bufferValue); // Still checking
                                        // value for more
                                        // accurate feedback
                                        // to user
        return;
      }

      if (checkTrackerValue(clientNum, bufferValue)) {
        value = Q_atoi(bufferValue);
        ent->client->sess.progression[idx - 1] = value;
        setTrackerMsg = stringFormat("^7Tracker set - index: ^3%i "
                                     "^7value: ^2%i\n",
                                     idx, value);
        Printer::console(clientNum, setTrackerMsg);
      }
    }
  }
}

void clearSaves(gentity_t *ent) {
  auto clientNum = ClientNum(ent);
  saveSystem->resetSavedPositions(ent);
  Printer::center(clientNum, "^7Your saves were removed.\n");
}
} // namespace ETJump

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f(gentity_t *ent) {
  auto *name = ConcatArgs(1);
  auto clientNum = ClientNum(ent);
  auto result = ETJump::canNoclip(ent);

  if (!result.success) {
    std::string str = ETJump::stringFormat(result.message, "noclip");
    capitalizeWithColor(str);
    Printer::center(clientNum, str);
    return;
  }

  if (!Q_stricmp(name, "on") || Q_atoi(name)) {
    ent->client->noclip = qtrue;
  } else if (!Q_stricmp(name, "off") || !Q_stricmp(name, "0")) {
    ent->client->noclip = qfalse;
  } else {
    ent->client->noclip = !ent->client->noclip ? qtrue : qfalse;
  }

  if (ent->client->noclip) {
    ent->client->noclipThisLife = true;
    ETJump::decreaseNoclipCount(ent, "noclip");
  }
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f(gentity_t *ent) {
  if (ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
      (ent->client->ps.pm_flags & PMF_LIMBO) || ent->health <= 0 ||
      level.match_pause != PAUSE_NONE) {
    return;
  }

  if (ent->client->sess.lastKillTime + 1000 > level.time) {
    return;
  }

  ent->flags &= ~FL_GODMODE;
  ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
  ent->client->ps.persistant[PERS_HWEAPON_USE] =
      0; // TTimo - if using /kill while at MG42
  player_die(ent, ent, ent, (g_gamestate.integer == GS_PLAYING) ? 100000 : 135,
             MOD_SUICIDE);
  ent->client->sess.lastKillTime = level.time;
}

void G_TeamDataForString(const char *teamstr, int clientNum, team_t *team,
                         spectatorState_t *sState, int *specClient) {
  *sState = SPECTATOR_NOT;
  if (!Q_stricmp(teamstr,
                 "follow1")) // follow player 1 as a spectator (we do
                             // require at least 1 playing client)
  {
    *team = TEAM_SPECTATOR;

    if (TeamCount(clientNum, TEAM_AXIS) + TeamCount(clientNum, TEAM_ALLIES) >
        0) {
      *sState = SPECTATOR_FOLLOW;
    } else {
      *sState = SPECTATOR_FREE;
    }

    if (specClient) {
      *specClient = -1;
    }
  } else if (!Q_stricmp(teamstr,
                        "follow2")) // follow player 2 as a spectator (we do
                                    // require at least 2 playing clients)
  {
    int specClientNum = -2;
    int playerCount =
        TeamCount(clientNum, TEAM_AXIS) + TeamCount(clientNum, TEAM_ALLIES);

    *team = TEAM_SPECTATOR;
    if (playerCount > 1) {
      *sState = SPECTATOR_FOLLOW;
    }
    // if there is no 2nd player to follow, follow the first one
    else if (playerCount > 0) {
      *sState = SPECTATOR_FOLLOW;
      specClientNum = -1;
    } else {
      *sState = SPECTATOR_FREE;
    }

    if (specClient) {
      *specClient = specClientNum;
    }
  } else if (!Q_stricmp(teamstr, "spectator") || !Q_stricmp(teamstr, "s")) {
    *team = TEAM_SPECTATOR;
    *sState = SPECTATOR_FREE;
  } else if (!Q_stricmp(teamstr, "red") || !Q_stricmp(teamstr, "r") ||
             !Q_stricmp(teamstr, "axis")) {
    *team = TEAM_AXIS;
  } else if (!Q_stricmp(teamstr, "blue") || !Q_stricmp(teamstr, "b") ||
             !Q_stricmp(teamstr, "allies")) {
    *team = TEAM_ALLIES;
  } else {
    *team = PickTeam(clientNum);
    if (!G_teamJoinCheck(*team, &g_entities[clientNum])) {
      *team = static_cast<team_t>((TEAM_AXIS | TEAM_ALLIES) & ~*team);
    }
  }
}

/*
=================
SetTeam
=================
*/
qboolean SetTeam(gentity_t *ent, const char *s, qboolean force, weapon_t w1,
                 weapon_t w2, qboolean setweapons) {
  team_t team, oldTeam;
  gclient_t *client;
  int clientNum;
  spectatorState_t specState;
  int specClient;

  //
  // see what change is requested
  //
  client = ent->client;

  clientNum = client - level.clients;
  specClient = 0;

  G_TeamDataForString(s, client - level.clients, &team, &specState,
                      &specClient);

  if (team != TEAM_SPECTATOR) {
    // Ensure the player can join
    if (!G_teamJoinCheck(team, ent)) {
      // Leave them where they were before the command
      // was issued
      return (qfalse);
    }
  }

  if (g_maxGameClients.integer > 0 &&
      level.numNonSpectatorClients >= g_maxGameClients.integer) {
    team = TEAM_SPECTATOR;
  }

  //
  // decide if we will allow the change
  //
  oldTeam = client->sess.sessionTeam;
  if (team == oldTeam && team != TEAM_SPECTATOR) {
    return qfalse;
  }

  if (oldTeam != TEAM_SPECTATOR) {
    ETJump::saveSystem->storeTeamQuickDeployPosition(ent, oldTeam);
  }

  // DHM - Nerve
  // OSP
  if (team != TEAM_SPECTATOR) {
    client->pers.initialSpawn = qfalse;
  }

  if (oldTeam != TEAM_SPECTATOR) {
    if (!(ent->client->ps.pm_flags & PMF_LIMBO)) {
      // Kill him (makes sure he loses flags, etc)
      ent->flags &= ~FL_GODMODE;
      ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
      player_die(ent, ent, ent, 100000, MOD_SWITCHTEAM);
    }
  }
  // they go to the end of the line for tournements
  if (team == TEAM_SPECTATOR) {
    client->sess.spectatorTime = level.time;
    client->pers.invite = 0;
  }

  G_LeaveTank(ent, qfalse);
  if (g_landminetimeout.integer) {
    G_ExplodeMines(ent);
  }
  G_FadeItems(ent, MOD_SATCHEL);

  // remove ourself from teamlists
  {
    int i;
    mapEntityData_t *mEnt;
    mapEntityData_Team_t *teamList;

    for (i = 0; i < 2; i++) {
      teamList = &mapEntityData[i];

      if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) !=
          NULL) {
        G_FreeMapEntityData(teamList, mEnt);
      }

      mEnt = G_FindMapEntityDataSingleClient(teamList, NULL, ent->s.number, -1);

      while (mEnt) {
        mapEntityData_t *mEntFree = mEnt;

        mEnt =
            G_FindMapEntityDataSingleClient(teamList, mEnt, ent->s.number, -1);

        G_FreeMapEntityData(teamList, mEntFree);
      }
    }
  }
  client->sess.spec_team = 0;
  client->sess.sessionTeam = team;
  client->sess.spectatorState = specState;
  client->sess.spectatorClient = specClient;
  client->pers.ready = qfalse;
  client->sess.lastTeamSwitch = level.time;

  // (l)users will spam spec messages... honest!
  if (team != oldTeam) {
    gentity_t *tent = G_PopupMessage(PM_TEAM);
    tent->s.effect2Time = team;
    tent->s.effect3Time = clientNum;
    tent->s.density = 0;
  }

  if (setweapons) {
    G_SetClientWeapons(ent, w1, w2, qfalse);
  }

  // get and distribute relevent paramters
  G_UpdateCharacter(
      client); // FIXME : doesn't ClientBegin take care of this already?
  ClientUserinfoChanged(clientNum);

  ClientBegin(clientNum);

  G_verifyMatchState(oldTeam);

  // Reset stats when changing teams
  if (team != oldTeam) {
    G_deleteStats(clientNum);
  }

  G_UpdateSpawnCounts();

  if (g_gamestate.integer == GS_PLAYING &&
      (client->sess.sessionTeam == TEAM_AXIS ||
       client->sess.sessionTeam == TEAM_ALLIES)) {
    int i;
    int x = client->sess.sessionTeam - TEAM_AXIS;

    for (i = 0; i < MAX_COMMANDER_TEAM_SOUNDS; i++) {
      if (level.commanderSounds[x][i].index) {
        gentity_t *tent =
            G_TempEntity(client->ps.origin, EV_GLOBAL_CLIENT_SOUND);
        tent->s.eventParm = level.commanderSounds[x][i].index - 1;
        tent->s.teamNum = clientNum;
      }
    }
  }

  ent->client->pers.autofireteamCreateEndTime = 0;
  ent->client->pers.autofireteamJoinEndTime = 0;

  if (client->sess.sessionTeam == TEAM_AXIS ||
      client->sess.sessionTeam == TEAM_ALLIES) {
    if (g_autoFireteams.integer) {
      fireteamData_t *ft = G_FindFreePublicFireteam(client->sess.sessionTeam);

      if (ft) {
        trap_SendServerCommand(ent - g_entities, "aftj -1");
        ent->client->pers.autofireteamJoinEndTime = level.time;

        //				G_AddClientToFireteam(
        // ent-g_entities,
        // ft->joinOrder[0] );
      } else {
        trap_SendServerCommand(ent - g_entities, "aftc -1");
        ent->client->pers.autofireteamCreateEndTime = level.time;
      }
    }
  }

  if (ent->client->sess.timerunActive) {
    if (team == TEAM_SPECTATOR || !ent->client->sess.runSpawnflags ||
        ent->client->sess.runSpawnflags &
            static_cast<int>(ETJump::TimerunSpawnflags::ResetTeamChange)) {
      InterruptRun(ent);
    }
  }

  return qtrue;
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing(gentity_t *ent) {
  // ATVI Wolfenstein Misc #474
  // divert behaviour if TEAM_SPECTATOR, moved the code from
  // SpectatorThink to put back into free fly correctly (I am not sure
  // this can be called in non-TEAM_SPECTATOR situation, better be safe)
  if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
    // drop to free floating, somewhere above the current
    // position (that's the client you were following)
    vec3_t pos, angle;
    gclient_t *client = ent->client;
    VectorCopy(client->ps.origin, pos);
    //		pos[2] += 16; // Gordon: removing for now
    VectorCopy(client->ps.viewangles, angle);
    // Need this as it gets spec mode reset properly
    SetTeam(ent, "s", qtrue, static_cast<weapon_t>(-1),
            static_cast<weapon_t>(-1), qfalse);
    VectorCopy(pos, client->ps.origin);
    SetClientViewAngle(ent, angle);
  } else {
    // legacy code, FIXME: useless?
    // Gordon: no this is for limbo i'd guess
    ent->client->sess.spectatorState = SPECTATOR_FREE;
    ent->client->ps.clientNum = ent - g_entities;
  }
}

int G_NumPlayersWithWeapon(weapon_t weap, team_t team) {
  int i, j, cnt = 0;

  for (i = 0; i < level.numConnectedClients; i++) {
    j = level.sortedClients[i];

    if (level.clients[j].sess.playerType != PC_SOLDIER) {
      continue;
    }

    if (level.clients[j].sess.sessionTeam != team) {
      continue;
    }

    if (level.clients[j].sess.latchPlayerWeapon != weap &&
        level.clients[j].sess.playerWeapon != weap) {
      continue;
    }

    cnt++;
  }

  return cnt;
}

int G_NumPlayersOnTeam(team_t team) {
  int i, j, cnt = 0;

  for (i = 0; i < level.numConnectedClients; i++) {
    j = level.sortedClients[i];

    if (level.clients[j].sess.sessionTeam != team) {
      continue;
    }

    cnt++;
  }

  return cnt;
}

qboolean G_IsHeavyWeapon(weapon_t weap) {
  int i;

  for (i = 0; i < NUM_HEAVY_WEAPONS; i++) {
    if (bg_heavyWeapons[i] == weap) {
      return qtrue;
    }
  }

  return qfalse;
}

int G_TeamCount(gentity_t *ent, weapon_t weap) {
  int i, j, cnt;

  if (weap == -1) // we aint checking for a weapon, so always include ourselves
  {
    cnt = 1;
  } else // we ARE checking for a weapon, so ignore ourselves
  {
    cnt = 0;
  }

  for (i = 0; i < level.numConnectedClients; i++) {
    j = level.sortedClients[i];

    if (j == ent - g_entities) {
      continue;
    }

    if (level.clients[j].sess.sessionTeam != ent->client->sess.sessionTeam) {
      continue;
    }

    if (weap != -1) {
      if (level.clients[j].sess.playerWeapon != weap &&
          level.clients[j].sess.latchPlayerWeapon != weap) {
        continue;
      }
    }

    cnt++;
  }

  return cnt;
}

qboolean G_IsWeaponDisabled(gentity_t *ent, weapon_t weapon) {

  if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
    return qtrue;
  }

  return qfalse;
}

void G_SetClientWeapons(gentity_t *ent, weapon_t w1, weapon_t w2,
                        qboolean updateclient) {
  qboolean changed = qfalse;

  if (ent->client->sess.latchPlayerWeapon2 != w2) {
    ent->client->sess.latchPlayerWeapon2 = w2;
    changed = qtrue;
  }

  if (!G_IsWeaponDisabled(ent, w1)) {
    if (ent->client->sess.latchPlayerWeapon != w1) {
      ent->client->sess.latchPlayerWeapon = w1;
      changed = qtrue;
    }
  } else {
    if (ent->client->sess.latchPlayerWeapon != 0) {
      ent->client->sess.latchPlayerWeapon = 0;
      changed = qtrue;
    }
  }

  if (updateclient && changed) {
    ClientUserinfoChanged(ent - g_entities);
  }
}

/*
=================
G_GetDefaultWeaponForClass
Returns default weapons for class
=================
*/
weapon_t G_GetDefaultWeaponForClass(gentity_t *ent, char *s, bool primary) {
  weapon_t weapon = WP_NONE;
  // sessionTeam isn't set when this is called, so compare against team
  // string
  if (!Q_stricmp(s, "red") || !Q_stricmp(s, "r") || !Q_stricmp(s, "axis")) {
    switch (ent->client->sess.latchPlayerType) {
      case PC_SOLDIER:
      case PC_MEDIC:
      case PC_ENGINEER:
      case PC_FIELDOPS:
        weapon = primary ? WP_MP40 : WP_LUGER;
        break;
      case PC_COVERTOPS:
        weapon = primary ? WP_STEN : WP_SILENCER;
        break;
      default:
        break;
    }
  } else if (!Q_stricmp(s, "blue") || !Q_stricmp(s, "b") ||
             !Q_stricmp(s, "allies")) {
    switch (ent->client->sess.latchPlayerType) {
      case PC_SOLDIER:
      case PC_MEDIC:
      case PC_ENGINEER:
      case PC_FIELDOPS:
        weapon = primary ? WP_THOMPSON : WP_COLT;
        break;
      case PC_COVERTOPS:
        weapon = primary ? WP_STEN : WP_SILENCED_COLT;
        break;
      default:
        break;
    }
  }

  return weapon;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f(gentity_t *ent) {
  char s[MAX_TOKEN_CHARS] = "\0";
  char ptype[4];
  char weap[4], weap2[4];
  int argc = 0;
  weapon_t w, w2;

  argc = trap_Argc();
  if (argc > 1) {
    trap_Argv(1, s, sizeof(s));
  }

  if (ClientIsFlooding(ent)) {
    return;
  }

  if (trap_Argc() < 2) {
    CP("print \"usage: team <b|r|s|none>\n\"");
    return;
  }

  trap_Argv(1, s, sizeof(s));
  trap_Argv(2, ptype, sizeof(ptype));
  trap_Argv(3, weap, sizeof(weap));
  trap_Argv(4, weap2, sizeof(weap2));

  w = static_cast<weapon_t>(Q_atoi(weap));
  w2 = static_cast<weapon_t>(Q_atoi(weap2));

  ent->client->sess.latchPlayerType = Q_atoi(ptype);
  if (ent->client->sess.latchPlayerType < PC_SOLDIER ||
      ent->client->sess.latchPlayerType > PC_COVERTOPS) {
    ent->client->sess.latchPlayerType = PC_SOLDIER;
  }

  // if weapons are not specified, set default weapons for class
  if (!w) {
    w = G_GetDefaultWeaponForClass(ent, s, true);
  }
  if (!w2) {
    w2 = G_GetDefaultWeaponForClass(ent, s, false);
  }

  if (!SetTeam(ent, s, qfalse, w, w2, qtrue)) {
    G_SetClientWeapons(ent, w, w2, qtrue);
  }
}

// wrapper for Cmd_Team_f
void Cmd_Team2_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue) {
  Cmd_Team_f(ent);
}

void Cmd_ResetSetup_f(gentity_t *ent) {
  qboolean changed = qfalse;

  if (!ent || !ent->client) {
    return;
  }

  ent->client->sess.latchPlayerType = ent->client->sess.playerType;

  if (ent->client->sess.latchPlayerWeapon != ent->client->sess.playerWeapon) {
    ent->client->sess.latchPlayerWeapon = ent->client->sess.playerWeapon;
    changed = qtrue;
  }

  if (ent->client->sess.latchPlayerWeapon2 != ent->client->sess.playerWeapon2) {
    ent->client->sess.latchPlayerWeapon2 = ent->client->sess.playerWeapon2;
    changed = qtrue;
  }

  if (changed) {
    ClientUserinfoChanged(ent - g_entities);
  }
}

void Cmd_SetClass_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue) {}

void Cmd_SetWeapons_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue) {
}

// START Mad Doc - TDF
/*
=================
Cmd_TeamBot_f
=================
*/
void Cmd_TeamBot_f(gentity_t *foo) {
  char ptype[4], weap[4], fireteam[4];
  char entNumStr[4];
  int entNum;
  const char *weapon;
  char weaponBuf[MAX_INFO_STRING];
  char userinfo[MAX_INFO_STRING];

  gentity_t *ent;

  trap_Argv(1, entNumStr, sizeof(entNumStr));
  entNum = Q_atoi(entNumStr);

  ent = g_entities + entNum;

  trap_Argv(3, ptype, sizeof(ptype));
  trap_Argv(4, weap, sizeof(weap));
  trap_Argv(5, fireteam, sizeof(fireteam));

  ent->client->sess.latchPlayerType = Q_atoi(ptype);
  ent->client->sess.latchPlayerWeapon = Q_atoi(weap);
  ent->client->sess.latchPlayerWeapon2 = 0;
  ent->client->sess.playerType = Q_atoi(ptype);
  ent->client->sess.playerWeapon = Q_atoi(weap);

  // remove any weapon info from the userinfo, so SetWolfSpawnWeapons()
  // doesn't reset the weapon as that
  trap_GetUserinfo(entNum, userinfo, sizeof(userinfo));

  weapon = Info_ValueForKey(userinfo, "pWeapon");
  if (weapon[0]) {
    Q_strncpyz(weaponBuf, weapon, sizeof(weaponBuf));
    Info_RemoveKey(userinfo, "pWeapon");
    trap_SetUserinfo(entNum, userinfo);
  }

  SetWolfSpawnWeapons(ent->client);
}

// END Mad Doc - TDF
/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue) {
  int i;
  char arg[MAX_TOKEN_CHARS];

  if (trap_Argc() != 2) {
    if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
      StopFollowing(ent);
    }
    return;
  }

  if (ent->client->ps.pm_flags & PMF_LIMBO) {
    CP("cpm \"Can't issue a follow command while in "
       "limbo.\n\"");
    CP("cpm \"Hit FIRE to switch between teammates.\n\"");
    return;
  }

  trap_Argv(1, arg, sizeof(arg));
  i = ClientNumberFromString(ent, arg);
  if (i == -1) {
    if (!Q_stricmp(arg, "allies")) {
      i = TEAM_ALLIES;
    } else if (!Q_stricmp(arg, "axis")) {
      i = TEAM_AXIS;
    } else {
      return;
    }

    if (!TeamCount(ent - g_entities, i)) {
      CP(va("print \"The %s team %s empty!  Follow "
            "command ignored.\n\"",
            aTeams[i],
            ((ent->client->sess.sessionTeam != i) ? "is" : "would be")));
      return;
    }

    // Allow for simple toggle
    if (ent->client->sess.spec_team != i) {
      if (teamInfo[i].spec_lock && !(ent->client->sess.spec_invite & i)) {
        CP(va("print \"Sorry, the %s team is "
              "locked from spectators.\n\"",
              aTeams[i]));
      } else {
        ent->client->sess.spec_team = i;
        CP(va("print \"Spectator follow is now "
              "locked on the %s team.\n\"",
              aTeams[i]));
        Cmd_FollowCycle_f(ent, 1);
      }
    } else {
      ent->client->sess.spec_team = 0;
      CP(va("print \"%s team spectating is now "
            "disabled.\n\"",
            aTeams[i]));
    }

    return;
  }

  // can't follow self
  if (&level.clients[i] == ent->client) {
    return;
  }

  // can't follow another spectator
  if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR) {
    return;
  }
  if (level.clients[i].ps.pm_flags & PMF_LIMBO) {
    return;
  }

  if (!G_AllowFollow(ent, g_entities + i)) {
    CP(va("print \"Sorry, player %s ^7is locked from "
          "spectators.\n\"",
          level.clients[i].pers.netname));
    return;
  }

  // first set them to spectator
  if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
    SetTeam(ent, "spectator", qfalse, static_cast<weapon_t>(-1),
            static_cast<weapon_t>(-1), qfalse);
  }

  ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
  ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f(gentity_t *ent, int dir) {
  int clientNum;
  int original;

  // first set them to spectator
  if ((ent->client->sess.spectatorState == SPECTATOR_NOT) &&
      (!(ent->client->ps.pm_flags & PMF_LIMBO))) // JPW NERVE for limbo state
  {
    SetTeam(ent, "spectator", qfalse, static_cast<weapon_t>(-1),
            static_cast<weapon_t>(-1), qfalse);
  }

  if (dir != 1 && dir != -1) {
    G_Error("Cmd_FollowCycle_f: bad dir %i", dir);
  }

  // if dedicated follow client, just switch between the two auto
  // clients
  if (ent->client->sess.spectatorClient < 0) {
    if (ent->client->sess.spectatorClient == -1) {
      ent->client->sess.spectatorClient = -2;
    } else if (ent->client->sess.spectatorClient == -2) {
      ent->client->sess.spectatorClient = -1;
    }
    return;
  }

  clientNum = ent->client->sess.spectatorClient;
  original = clientNum;
  do {
    clientNum += dir;
    if (clientNum >= level.maxclients) {
      clientNum = 0;
    }
    if (clientNum < 0) {
      clientNum = level.maxclients - 1;
    }

    // can only follow connected clients
    if (level.clients[clientNum].pers.connected != CON_CONNECTED) {
      continue;
    }

    // can't follow another spectator
    if (level.clients[clientNum].sess.sessionTeam == TEAM_SPECTATOR) {
      continue;
    }

    // JPW NERVE -- couple extra checks for limbo mode
    if (ent->client->ps.pm_flags & PMF_LIMBO) {
      if (level.clients[clientNum].ps.pm_flags & PMF_LIMBO) {
        continue;
      }
      if (level.clients[clientNum].sess.sessionTeam !=
          ent->client->sess.sessionTeam) {
        continue;
      }
    }

    if (level.clients[clientNum].ps.pm_flags & PMF_LIMBO) {
      continue;
    }

    // OSP
    if (!G_DesiredFollow(ent, g_entities + clientNum)) {
      continue;
    }

    // this is good, we can use it
    ent->client->sess.spectatorClient = clientNum;
    ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
    return;
  } while (clientNum != original);

  // leave it where it was
}

/*======================
G_EntitySound
    Mad Doc xkan, 11/06/2002 -

    Plays a sound (wav file or sound script) on this entity

    Note that calling G_AddEvent(..., EV_GENERAL_SOUND, ...) has the danger of
    the event never getting through to the client because the entity might not
    be visible (unless it has the SVF_BROADCAST flag), so if you want to make
sure the sound is heard, call this function instead.
======================*/
void G_EntitySound(
    gentity_t *ent,      // entity to play the sound on
    const char *soundId, // sound file name or sound script ID
    int volume)          // sound volume, only applies to sound file name call
                         //   for sound script, volume is currently always 127.
{
  trap_SendServerCommand(
      -1, va("entitySound %d %s %d %i %i %i normal", ent->s.number, soundId,
             volume, (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1],
             (int)ent->s.pos.trBase[2]));
}

/*======================
G_EntitySoundNoCut
    Mad Doc xkan, 1/16/2003 -

    Similar to G_EntitySound, but do not cut this sound off

======================*/
void G_EntitySoundNoCut(
    gentity_t *ent,      // entity to play the sound on
    const char *soundId, // sound file name or sound script ID
    int volume)          // sound volume, only applies to sound file name call
                         //   for sound script, volume is currently always 127.
{
  trap_SendServerCommand(
      -1, va("entitySound %d %s %d %i %i %i noCut", ent->s.number, soundId,
             volume, (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1],
             (int)ent->s.pos.trBase[2]));
}

/*
==================
G_Say
==================
*/
void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color,
             const char *name, const char *message, qboolean localize,
             qboolean encoded) {
  const char *cmd;
  auto clientNum = ClientNum(ent);
  auto otherClientNum = ClientNum(other);

  if (!other || !other->inuse || !other->client) {
    return;
  }

  if (mode == SAY_TEAM && !OnSameTeam(ent, other)) {
    return;
  }

  if (mode == SAY_BUDDY) {
    if (ent->s.clientNum != other->s.clientNum) {
      fireteamData_t *ft1, *ft2;
      if (!G_IsOnFireteam(otherClientNum, &ft1)) {
        return;
      }
      if (!G_IsOnFireteam(clientNum, &ft2)) {
        return;
      }
      if (ft1 != ft2) {
        return;
      }
    }
  }

  if (encoded) {
    cmd = (mode == SAY_TEAM || mode == SAY_BUDDY || mode == SAY_ADMIN)
              ? "enc_tchat"
              : "enc_chat";
  } else {
    cmd = (mode == SAY_TEAM || mode == SAY_BUDDY || mode == SAY_ADMIN) ? "tchat"
                                                                       : "chat";
  }

  trap_SendServerCommand(otherClientNum,
                         va("%s \"%s%c%c%s\" %i %i", cmd, name, Q_COLOR_ESCAPE,
                            color, message, clientNum, localize));
}

void G_Say(gentity_t *ent, gentity_t *target, int mode, qboolean encoded,
           char *chatText) {
  int j, len;
  gentity_t *other;
  int color;
  char name[64];
  // don't let text be too long for malicious reasons
  char text[MAX_CHAT_TEXT];
  const char *escapedName = nullptr;
  qboolean localize = qfalse;
  const char *printText = nullptr;
  const int clientNum = ClientNum(ent);

  switch (mode) {
    default:
    case SAY_ALL:
      G_LogPrintf("say: %s: %s\n", ent->client->pers.netname, chatText);
      Com_sprintf(name, sizeof(name), "%s^7: ", ent->client->pers.netname);
      color = COLOR_GREEN;
      break;
    case SAY_TEAM:
      localize = qtrue;
      G_LogPrintf("sayteam: %s: %s\n", ent->client->pers.netname, chatText);
      Com_sprintf(name, sizeof(name),
                  "[lof](%s^7): ", ent->client->pers.netname);
      color = COLOR_CYAN;
      break;
    case SAY_BUDDY:
      localize = qtrue;
      Com_sprintf(name, sizeof(name),
                  "[lof](%s^7): ", ent->client->pers.netname);
      color = COLOR_YELLOW;
      break;
    case SAY_ADMIN:
      Printer::logAdminLn(ETJump::stringFormat(
          "adminchat: %i %s: %s", ent->s.number,
          ETJump::sanitize(ent->client->pers.netname), chatText));
      Com_sprintf(name, sizeof(name),
                  "^A> ^7%s^7: ", ent->client->pers.netname);
      color = COLOR_LTORANGE;
  }

  len = sizeof(text);
  Q_strncpyz(text, chatText, len);

  // if chat message is too long, e.g. being sent from console
  // cut it and put ellipsis at the end
  if (Q_strnlen(chatText, MAX_SAY_TEXT) > len) {
    text[len - 2] = '.';
    text[len - 3] = '.';
    text[len - 4] = '.';
  }

  printText = text;
  escapedName = EscapeString(name);

  if (g_chatOptions.integer & CHAT_OPTIONS_INTERPOLATE_NAME_TAGS) {
    printText = interpolateNametags(text, color);
  }

  if (mode == SAY_ALL) {
    game.chatReplay->createChatMessage(clientNum, escapedName, printText,
                                       localize, encoded);
  }

  if (target) {
    if (!COM_BitCheck(target->client->sess.ignoreClients, clientNum)) {
      G_SayTo(ent, target, mode, color, escapedName, printText, localize,
              encoded);
    }
    return;
  }

  // send it to all the appropriate clients
  for (j = 0; j < level.numConnectedClients; j++) {
    other = &g_entities[level.sortedClients[j]];

    if (mode == SAY_ADMIN &&
        !ETJump::session->HasPermission(other, CommandFlags::ADMINCHAT)) {
      continue;
    }

    if (!COM_BitCheck(other->client->sess.ignoreClients, clientNum)) {
      G_SayTo(ent, other, mode, color, escapedName, printText, localize,
              encoded);
    }
  }

  AdminCommandCheck(ent);
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f(gentity_t *ent, int mode, qboolean arg0, qboolean encoded) {
  if (trap_Argc() < 2 && !arg0) {
    return;
  }
  G_Say(ent, nullptr, mode, encoded, ConcatArgs(((arg0) ? 0 : 1)));
}

// NERVE - SMF
void G_VoiceTo(gentity_t *ent, gentity_t *other, int mode, vsayCmd_t *vsay,
               qboolean voiceonly) {
  int color;
  const char *cmd;

  if (!other) {
    return;
  }
  if (!other->inuse) {
    return;
  }
  if (!other->client) {
    return;
  }
  if (mode == SAY_TEAM && !OnSameTeam(ent, other)) {
    return;
  }

  // send only to people who have the sender on their buddy list
  if (mode == SAY_BUDDY) {
    if (ent->s.clientNum != other->s.clientNum) {
      fireteamData_t *ft1, *ft2;
      if (!G_IsOnFireteam(other - g_entities, &ft1)) {
        return;
      }
      if (!G_IsOnFireteam(ent - g_entities, &ft2)) {
        return;
      }
      if (ft1 != ft2) {
        return;
      }
    }
  }

  if (mode == SAY_TEAM) {
    color = COLOR_CYAN;
    cmd = "vtchat";
  } else if (mode == SAY_BUDDY) {
    color = COLOR_YELLOW;
    cmd = "vbchat";
  } else {
    color = COLOR_GREEN;
    cmd = "vchat";
  }

  if (mode == SAY_TEAM || mode == SAY_BUDDY) {
    CPx(other - g_entities, va("%s %d %d %d %s %i %i %i %i %f \"%s\"", cmd,
                               voiceonly, ClientNum(ent), color, vsay->id,
                               static_cast<int>(ent->s.pos.trBase[0]),
                               static_cast<int>(ent->s.pos.trBase[1]),
                               static_cast<int>(ent->s.pos.trBase[2]),
                               vsay->variant, vsay->random, vsay->custom));
  } else {
    CPx(other - g_entities,
        va("%s %d %d %d %s %i %f \"%s\"", cmd, voiceonly, ClientNum(ent), color,
           vsay->id, vsay->variant, vsay->random, vsay->custom));
  }
}

void G_Voice(gentity_t *ent, gentity_t *target, int mode, vsayCmd_t *vsay,
             qboolean voiceonly) {
  int j;
  gentity_t *other;

  // DHM - Nerve :: Don't allow excessive spamming of voice chats
  ent->voiceChatSquelch -= (level.time - ent->voiceChatPreviousTime);
  ent->voiceChatPreviousTime = level.time;

  if (ent->voiceChatSquelch < 0) {
    ent->voiceChatSquelch = 0;
  }

  // Only do the spam check for MP
  if (ent->voiceChatSquelch >= 30000) {
    trap_SendServerCommand(ent - g_entities,
                           "cpm \"^1Spam Protection^7: VoiceChat ignored\n\"");
    return;
  }

  if (g_voiceChatsAllowed.integer) {
    ent->voiceChatSquelch += (34000 / g_voiceChatsAllowed.integer);
  } else {
    return;
  }
  // dhm

  // OSP - Charge for the lame spam!
  /*if(mode == SAY_ALL && (!Q_stricmp(id, "DynamiteDefused") ||
  !Q_stricmp(id, "DynamitePlanted"))) { return;
  }*/

  if (target) {
    if (!COM_BitCheck(target->client->sess.ignoreClients, ent - g_entities)) {
      G_VoiceTo(ent, target, mode, vsay, voiceonly);
    }
    return;
  }

  // echo the text to the console
  if (g_dedicated.integer) {
    G_Printf("voice: %s %s\n", ent->client->pers.netname, vsay->id);
  }

  if (mode == SAY_BUDDY) {
    char buffer[32];
    int cls = -1, i, cnt, num;
    qboolean allowclients[MAX_CLIENTS];

    memset(allowclients, 0, sizeof(allowclients));

    trap_Argv(1, buffer, 32);

    cls = Q_atoi(buffer);

    trap_Argv(2, buffer, 32);
    cnt = Q_atoi(buffer);
    if (cnt > MAX_CLIENTS) {
      cnt = MAX_CLIENTS;
    }

    for (i = 0; i < cnt; i++) {
      trap_Argv(3 + i, buffer, 32);

      num = Q_atoi(buffer);
      if (num < 0) {
        continue;
      }
      if (num >= MAX_CLIENTS) {
        continue;
      }

      allowclients[num] = qtrue;
    }

    for (j = 0; j < level.numConnectedClients; j++) {

      if (level.sortedClients[j] != ent->s.clientNum) {
        if (cls != -1 &&
            cls != level.clients[level.sortedClients[j]].sess.playerType) {
          continue;
        }
      }

      if (cnt) {
        if (!allowclients[level.sortedClients[j]]) {
          continue;
        }
      }

      G_VoiceTo(ent, &g_entities[level.sortedClients[j]], mode, vsay,
                voiceonly);
    }
  } else {

    // send it to all the apropriate clients
    for (j = 0; j < level.numConnectedClients; j++) {
      other = &g_entities[level.sortedClients[j]];
      if (!COM_BitCheck(other->client->sess.ignoreClients, ent - g_entities)) {
        G_VoiceTo(ent, other, mode, vsay, voiceonly);
      }
    }
  }
}

/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f(gentity_t *ent, int mode, qboolean arg0,
                        qboolean voiceonly) {

  vsayCmd_t vsay;
  auto id = 1, cust = 2;
  char variant[2];

  memset(&vsay, 0, sizeof(vsay));
  // get random seed beforehand to keep it same for all clients
  vsay.random = random();

  if (mode != SAY_BUDDY) {

    if (trap_Argc() < 2 && !arg0) {
      return;
    }

    trap_Argv(1, variant, sizeof(variant));

    if (Q_isnumeric(variant[0])) {
      id = 2;
      cust = 3;
      vsay.variant = Q_atoi(variant);
    }

    trap_Argv(id, vsay.id, sizeof(vsay.id));
    Q_strncpyz(vsay.custom, ConcatArgs(cust), sizeof(vsay.custom));

  } else {
    char buffer[16];
    int skipArgs;

    // if fireteam members are selected with 'selectbuddy', 2nd arg contains
    // the number of clients that are selected, followed by the clientnums
    // of the selected fireteam members = offset arg parsing by this amount
    trap_Argv(2, buffer, sizeof(buffer));
    skipArgs = Q_atoi(buffer);

    if (skipArgs < 0) {
      skipArgs = 0;
    }

    if (trap_Argc() < 3 + skipArgs && !arg0) {
      return;
    }

    trap_Argv(3 + skipArgs, variant, sizeof(variant));

    if (Q_isnumeric(variant[0])) {
      id = 4 + skipArgs;
      cust = 5 + skipArgs;
      vsay.variant = Q_atoi(variant);
    } else {
      id = 3 + skipArgs;
      cust = 4 + skipArgs;
    }

    trap_Argv(id, vsay.id, sizeof(vsay.id));
    Q_strncpyz(vsay.custom, ConcatArgs(cust), sizeof(vsay.custom));
  }

  G_Voice(ent, nullptr, mode, &vsay, voiceonly);
}

// TTimo gcc: defined but not used
#if 0
/*
==================
Cmd_VoiceTell_f
==================
*/
static void Cmd_VoiceTell_f(gentity_t *ent, qboolean voiceonly)
{
	int       targetNum;
	gentity_t *target;
	char      *id;
	char      arg[MAX_TOKEN_CHARS];

	if (trap_Argc() < 2)
	{
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	targetNum = Q_atoi(arg);
	if (targetNum < 0 || targetNum >= level.maxclients)
	{
		return;
	}

	target = &g_entities[targetNum];
	if (!target || !target->inuse || !target->client)
	{
		return;
	}

	id = ConcatArgs(2);

	G_LogPrintf("vtell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, id);
	G_Voice(ent, target, SAY_TELL, id, voiceonly);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if (ent != target && !(ent->r.svFlags & SVF_BOT))
	{
		G_Voice(ent, ent, SAY_TELL, id, voiceonly);
	}
}
#endif

// TTimo gcc: defined but not used
#if 0
/*
==================
Cmd_VoiceTaunt_f
==================
*/
static void Cmd_VoiceTaunt_f(gentity_t *ent)
{
	gentity_t *who;
	int       i;

	if (!ent->client)
	{
		return;
	}

	// insult someone who just killed you
	if (ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == ent->s.number)
	{
		// i am a dead corpse
		if (!(ent->enemy->r.svFlags & SVF_BOT))
		{
//			G_Voice( ent, ent->enemy, SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		if (!(ent->r.svFlags & SVF_BOT))
		{
//			G_Voice( ent, ent,        SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		ent->enemy = NULL;
		return;
	}
	// insult someone you just killed
	if (ent->client->lastkilled_client >= 0 && ent->client->lastkilled_client != ent->s.number)
	{
		who = g_entities + ent->client->lastkilled_client;
		if (who->client)
		{
			// who is the person I just killed
			if (who->client->lasthurt_mod == MOD_GAUNTLET)
			{
				if (!(who->r.svFlags & SVF_BOT))
				{
//					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );	// and I killed them with a gauntlet
				}
				if (!(ent->r.svFlags & SVF_BOT))
				{
//					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );
				}
			}
			else
			{
				if (!(who->r.svFlags & SVF_BOT))
				{
//					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );	// and I killed them with something else
				}
				if (!(ent->r.svFlags & SVF_BOT))
				{
//					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );
				}
			}
			ent->client->lastkilled_client = -1;
			return;
		}
	}

	if (g_gametype.integer >= GT_TEAM)
	{
		// praise a team mate who just got a reward
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			who = g_entities + i;
			if (who->client && who != ent && who->client->sess.sessionTeam == ent->client->sess.sessionTeam)
			{
				if (who->client->rewardTime > level.time)
				{
					if (!(who->r.svFlags & SVF_BOT))
					{
//						G_Voice( ent, who, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					if (!(ent->r.svFlags & SVF_BOT))
					{
//						G_Voice( ent, ent, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					return;
				}
			}
		}
	}

	// just say something
//	G_Voice( ent, NULL, SAY_ALL, VOICECHAT_TAUNT, qfalse );
}
// -NERVE - SMF
#endif

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f(gentity_t *ent) {
  trap_SendServerCommand(ent - g_entities,
                         va("print \"%s\n\"", vtos(ent->s.origin)));
}

/*
==================
checkVoteConditions
Checks if a vote can be called
==================
*/
bool checkVoteConditions(gentity_t *ent, int clientNum) {
  std::string voteError;

  // Setting g_enableVote 0 sets this flag as well
  if (voteFlags.integer == VOTING_DISABLED) {
    Printer::popup(clientNum, "Voting is not enabled on this server.");
    return false;
  }

  if (ent->client->sess.muted && g_mute.integer & 2) {
    Printer::popup(clientNum,
                   "^3callvote: ^7not allowed to call a vote while muted.");
    return false;
  }

  if (ent->client->sess.sessionTeam == TEAM_SPECTATOR &&
      g_spectatorVote.integer < 2) {
    Printer::popup(
        clientNum,
        "^3callvote: ^7you are not allowed to call a vote as a spectator.");
    return false;
  }

  if (level.voteInfo.voteTime) {
    Printer::popup(clientNum, "A vote is already in progress.");
    return false;
  }

  if (level.intermissiontime) {
    Printer::popup(clientNum, "Cannot callvote during intermission.");
    return false;
  }

  if (vote_limit.integer > 0 &&
      ent->client->pers.voteCount >= vote_limit.integer) {
    voteError = ETJump::stringFormat(
        "You have already called the maximum number of votes (%d).",
        vote_limit.integer);
    Printer::popup(clientNum, voteError);
    return false;
  }

  if (level.time - level.startTime < g_disableVoteAfterMapChange.integer) {
    const int remainingTime = std::ceil(
        (g_disableVoteAfterMapChange.integer - (level.time - level.startTime)) /
        1000.0);
    voteError = "You must wait " + ETJump::getSecondsString(remainingTime) +
                " before voting after a map change.";
    Printer::popup(clientNum, voteError);
    return false;
  }

  if (level.time - ent->client->lastVoteTime < g_voteCooldown.integer * 1000) {
    const int voteCooldown =
        std::ceil(((g_voteCooldown.integer * 1000) -
                   (level.time - ent->client->lastVoteTime)) /
                  1000.0);
    voteError = "^3callvote:^7 you must wait " +
                ETJump::getSecondsString(voteCooldown) +
                " before voting again.";
    Printer::chat(clientNum, voteError);
    return false;
  }

  return true;
}

namespace ETJump {
void setCvString(char *voteMsg, char *voteArg) {
  const auto voteStringFormat = voteArg[0] ? "%s %s" : "%s";
  std::string voteString;
  const auto func = level.voteInfo.vote_fn;

  if (func == G_DevMap_v_Wrapper) {
    voteString = stringFormat("%s (cheats enabled)", voteArg);
    Q_strncpyz(voteArg, voteString.c_str(), MAX_STRING_TOKENS);
  } else if (func == G_RandomMap_v || func == G_RockTheVote_v) {
    if (voteArg[0]) {
      voteString = stringFormat("from %s", voteArg);
      Q_strncpyz(voteArg, voteString.c_str(), MAX_STRING_TOKENS);
    }
  } else if (func == G_AutoRtv_v) {
    const int voteArgInt = Q_atoi(voteArg);

    if (voteArgInt == 0) {
      Q_strncpyz(voteArg, "Off", MAX_STRING_TOKENS);
    } else {
      voteString = stringFormat("%s", getMinutesString(voteArgInt));
      Q_strncpyz(voteArg, voteString.c_str(), MAX_STRING_TOKENS);
    }
  } else if (func == G_PortalPredict_v) {
    voteString = stringFormat("%s", Q_atoi(voteArg) ? "ON" : "OFF", voteMsg);
    Q_strncpyz(voteArg, voteString.c_str(), MAX_STRING_TOKENS);
  }

  Com_sprintf(level.voteInfo.voteString, sizeof(level.voteInfo.voteString),
              voteStringFormat, voteMsg, voteArg);
}

bool isValidVoteString(const std::string &str) {
  return std::none_of(
      tokenDelimiters.cbegin(), tokenDelimiters.cend(),
      [&](const char token) { return StringUtil::contains(str, token); });
}

} // namespace ETJump

/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue) {
  int i;
  char arg1[MAX_STRING_TOKENS];
  char arg2[MAX_STRING_TOKENS];
  const int clientNum = ClientNum(ent);

  if (!checkVoteConditions(ent, clientNum)) {
    return;
  }

  // make sure it is a valid command to vote on
  trap_Argv(1, arg1, sizeof(arg1));
  trap_Argv(2, arg2, sizeof(arg2));

  if (!ETJump::isValidVoteString(arg1) || !ETJump::isValidVoteString(arg2)) {
    Printer::popup(clientNum, "Invalid vote string.");
    return;
  }

  if ((i = G_voteCmdCheck(ent, arg1, arg2)) != G_OK) {
    if (i == G_NOTFOUND) {
      if (arg1[0]) {
        std::string errorMessage = ETJump::stringFormat(
            "\n^3Unknown vote command: ^7%s %s\n", arg1, arg2);
        Printer::console(clientNum, errorMessage);
      }
      G_voteHelp(ent, qtrue);
    }
    return;
  }

  ETJump::setCvString(arg1, arg2);

  if (level.voteInfo.vote_fn == ETJump::G_RockTheVote_v) {
    game.rtv->setRtvStatus(true);
    trap_SendServerCommand(clientNum, "openRtvMenu");
  }

  // Zero: NOTE! if we call a randommap vote with a custom map type
  // it only changes the clientside info text, not everything.

  // rtv starts with no initial votes as the caller hasn't picked a map yet
  if (game.rtv->rtvVoteActive()) {
    level.voteInfo.voteYes = 0;
  } else {
    level.voteInfo.voteYes = 1;
    trap_SendServerCommand(clientNum, "voted yes");
  }

  std::string calledVoteString = ETJump::stringFormat(
      "%s^7 called a vote. Voting for: %s\n", ent->client->pers.netname,
      level.voteInfo.voteString);
  Printer::consoleAll(calledVoteString);

  calledVoteString =
      ETJump::stringFormat("%s^7 called a vote.", ent->client->pers.netname);
  Printer::centerAll(calledVoteString);

  G_LogPrintf("%s called a vote. Voting for: %s\n", ent->client->pers.netname,
              level.voteInfo.voteString);

  G_globalSound("sound/misc/vote.wav");

  level.voteInfo.voteTime = level.time;
  level.voteInfo.voteNo = 0;
  level.voteInfo.voter_cn = clientNum;
  level.voteInfo.voter_team = ent->client->sess.sessionTeam;

  ent->client->lastVoteTime = level.time;

  for (i = 0; i < level.numConnectedClients; i++) {
    level.clients[level.sortedClients[i]].ps.eFlags &= ~EF_VOTED;
  }

  ent->client->pers.voteCount++;
  if (!game.rtv->rtvVoteActive()) {
    ent->client->ps.eFlags |= EF_VOTED;
  }

  trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.voteInfo.voteTime));
  trap_SetConfigstring(CS_VOTE_STRING, level.voteInfo.voteString);

  if (game.rtv->rtvVoteActive()) {
    game.rtv->setRtvConfigstrings();
  } else {
    trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteInfo.voteYes));
  }

  trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteInfo.voteNo));
}

static const char *yesMsgs[] = {"yes", "y", "1"};
static const char *noMsgs[] = {"no", "n", "0"};

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f(gentity_t *ent) {
  char voteCmd[64]; // either 'vote' or 'rtvVote'
  char voteArg[64];
  auto *client = ent->client;
  const int clientNum = ClientNum(ent);
  const bool isRtvVote = game.rtv->rtvVoteActive();

  static const auto votedYes = [](const std::string &msg) {
    return std::any_of(std::begin(yesMsgs), std::end(yesMsgs),
                       [&msg](const char *ym) { return ym == msg; });
  };

  static const auto votedNo = [](const std::string &msg) {
    return std::any_of(std::begin(noMsgs), std::end(noMsgs),
                       [&msg](const char *nm) { return nm == msg; });
  };

  const auto printVoteMsgs = [&] {
    std::string voteMsgs = "^7Invalid vote argument.\n";
    voteMsgs += "  ^7Valid arguments for '^2Yes^7' are: ^3<" +
                ETJump::StringUtil::join(yesMsgs, "|") + "^3>\n";
    voteMsgs += "  ^7Valid arguments for '^1No^7' are: ^3<" +
                ETJump::StringUtil::join(noMsgs, "|") + "^3>\n";
    Printer::console(clientNum, voteMsgs);
  };

  const auto printRtvVoteMsgs = [&] {
    const auto *rtvMaps = game.rtv->getRtvMaps();
    std::string voteMsgs = "^7Invalid vote argument.\n";
    voteMsgs += ETJump::stringFormat(
        "^7Valid arguments for this vote are:\n  ^3rtvVote 1-%i\n",
        rtvMaps->size());
    voteMsgs += "  ^7Open map selection with: ^3vote <" +
                ETJump::StringUtil::join(yesMsgs, "|") + ">\n";
    voteMsgs += "  ^7Vote '^1No^7' to keep current map with: ^3vote <" +
                ETJump::StringUtil::join(noMsgs, "|") + ">\n";
    Printer::console(clientNum, voteMsgs);
  };

  static const auto cancelVote = [&] {
    level.voteInfo.voteCanceled = qtrue;
    level.voteInfo.voteNo = level.numConnectedClients;
    level.voteInfo.voteYes = 0;

    Printer::popupAll("^7Vote canceled by caller.");
  };

  trap_Argv(0, voteCmd, sizeof(voteCmd));
  const std::string voteCmdStr = ETJump::sanitize(std::string(voteCmd), true);
  const bool isRtvVoteCmd = voteCmdStr == "rtvvote";

  trap_Argv(1, voteArg, sizeof(voteArg));
  const std::string voteArgStr = ETJump::sanitize(std::string(voteArg), true);

  if (ent->client->pers.applicationEndTime > level.time) {

    gclient_t *cl = g_entities[ent->client->pers.applicationClient].client;

    if (!cl) {
      return;
    }
    if (cl->pers.connected != CON_CONNECTED) {
      return;
    }

    if (votedYes(voteArgStr)) {
      trap_SendServerCommand(clientNum, "application -4");
      trap_SendServerCommand(ent->client->pers.applicationClient,
                             "application -3");

      G_AddClientToFireteam(ent->client->pers.applicationClient, clientNum);
    } else if (votedNo(voteArgStr)) {
      trap_SendServerCommand(clientNum, "application -4");
      trap_SendServerCommand(ent->client->pers.applicationClient,
                             "application -2");
    } else {
      printVoteMsgs();
      return;
    }

    ent->client->pers.applicationEndTime = 0;
    ent->client->pers.applicationClient = -1;

    return;
  }

  ent->client->pers.applicationEndTime = 0;
  ent->client->pers.applicationClient = -1;

  if (ent->client->pers.invitationEndTime > level.time) {

    gclient_t *cl = g_entities[ent->client->pers.invitationClient].client;
    if (!cl) {
      return;
    }
    if (cl->pers.connected != CON_CONNECTED) {
      return;
    }

    if (votedYes(voteArgStr)) {
      trap_SendServerCommand(clientNum, "invitation -4");
      trap_SendServerCommand(ent->client->pers.invitationClient,
                             "invitation -3");

      G_AddClientToFireteam(clientNum, ent->client->pers.invitationClient);
    } else if (votedNo(voteArgStr)) {
      trap_SendServerCommand(clientNum, "invitation -4");
      trap_SendServerCommand(ent->client->pers.invitationClient,
                             "invitation -2");
    } else {
      printVoteMsgs();
      return;
    }

    ent->client->pers.invitationEndTime = 0;
    ent->client->pers.invitationClient = -1;

    return;
  }

  ent->client->pers.invitationEndTime = 0;
  ent->client->pers.invitationClient = -1;

  if (ent->client->pers.propositionEndTime > level.time) {
    gclient_t *cl = g_entities[ent->client->pers.propositionClient].client;
    if (!cl) {
      return;
    }
    if (cl->pers.connected != CON_CONNECTED) {
      return;
    }

    if (votedYes(voteArgStr)) {
      trap_SendServerCommand(clientNum, "proposition -4");
      trap_SendServerCommand(ent->client->pers.propositionClient2,
                             "proposition -3");

      G_InviteToFireTeam(clientNum, ent->client->pers.propositionClient);
    } else if (votedNo(voteArgStr)) {
      trap_SendServerCommand(clientNum, "proposition -4");
      trap_SendServerCommand(ent->client->pers.propositionClient2,
                             "proposition -2");
    } else {
      printVoteMsgs();
      return;
    }

    ent->client->pers.propositionEndTime = 0;
    ent->client->pers.propositionClient = -1;
    ent->client->pers.propositionClient2 = -1;

    return;
  }

  if (ent->client->pers.autofireteamEndTime > level.time) {
    fireteamData_t *ft;

    if (votedYes(voteArgStr)) {
      trap_SendServerCommand(clientNum, "aft -2");

      if (G_IsFireteamLeader(clientNum, &ft)) {
        ft->priv = qtrue;
      }
    } else if (votedNo(voteArgStr)) {
      trap_SendServerCommand(clientNum, "aft -2");
    } else {
      printVoteMsgs();
      return;
    }

    ent->client->pers.autofireteamEndTime = 0;

    return;
  }

  if (ent->client->pers.autofireteamCreateEndTime > level.time) {

    if (votedYes(voteArgStr)) {
      trap_SendServerCommand(clientNum, "aftc -2");

      G_RegisterFireteam(clientNum);
    } else if (votedNo(voteArgStr)) {
      trap_SendServerCommand(clientNum, "aftc -2");
    } else {
      printVoteMsgs();
      return;
    }

    ent->client->pers.autofireteamCreateEndTime = 0;

    return;
  }

  if (ent->client->pers.autofireteamJoinEndTime > level.time) {
    if (votedYes(voteArgStr)) {
      fireteamData_t *ft;

      trap_SendServerCommand(clientNum, "aftj -2");

      ft = G_FindFreePublicFireteam(ent->client->sess.sessionTeam);
      if (ft) {
        G_AddClientToFireteam(clientNum, ft->joinOrder[0]);
      }
    } else if (votedNo(voteArgStr)) {
      trap_SendServerCommand(clientNum, "aftj -2");
    } else {
      printVoteMsgs();
      return;
    }

    ent->client->pers.autofireteamCreateEndTime = 0;

    return;
  }

  ent->client->pers.propositionEndTime = 0;
  ent->client->pers.propositionClient = -1;
  ent->client->pers.propositionClient2 = -1;

  if (!level.voteInfo.voteTime) {
    Printer::console(clientNum, "No vote in progress.\n");
    return;
  }

  // this is sent as + 1 from cgame for user convenience
  // e.g. /rtvVote 3 = vote for 3rd map on the list
  int mapNum = Q_atoi(voteArg) - 1;
  const size_t maxMaps = game.rtv->getRtvMaps()->size() - 1;

  if (isRtvVote && isRtvVoteCmd) {
    if (!std::isdigit(voteArg[0]) || mapNum < 0 || mapNum > maxMaps) {
      printRtvVoteMsgs();
      return;
    }

    mapNum = std::clamp(mapNum, 0, static_cast<int>(maxMaps));
  }

  if (ent->client->ps.eFlags & EF_VOTED) {
    // If the caller decides to hit f2 after calling the vote, cancel it.
    if (clientNum == level.voteInfo.voter_cn) {
      if (isRtvVote) {
        if (!isRtvVoteCmd) {
          if (votedYes(voteArgStr)) {
            trap_SendServerCommand(clientNum, "openRtvMenu");
          } else if (votedNo(voteArgStr)) {
            cancelVote();
          } else {
            printRtvVoteMsgs();
          }
          return;
        }
      } else {
        if (votedYes(voteArgStr)) {
          return; // do nothing
        } else if (votedNo(voteArgStr)) {
          cancelVote();
          return;
        } else {
          printVoteMsgs();
          return;
        }
      }
    }

    if (isRtvVote) {
      // allow opening the menu with 'vote yes' even when past re-vote period
      if (!isRtvVoteCmd && votedYes(voteArgStr)) {
        trap_SendServerCommand(clientNum, "openRtvMenu");
        return;
      }
      // don't take away re-vote attempts if we re-vote the same map on rtv
      if (client->pers.votingInfo.lastRtvMapVoted == mapNum) {
        return;
      }
    }

    // so it's first 10s if vote time set to 30s
    const int allowedRevoteTimeRange =
        level.voteInfo.voteTime + (VOTE_TIME / 3);

    // don't allow to re-vote anymore if time range is passed or
    // there are no attempts left
    if (level.time > allowedRevoteTimeRange ||
        client->pers.votingInfo.attempts > ETJump::VOTING_ATTEMPTS) {
      if (client->pers.votingInfo.time + ETJump::VOTING_TIMEOUT > level.time) {
        // stops excessive spam from server if user keeps voting in timeouts
        if (!client->pers.votingInfo.isWarned) {
          client->pers.votingInfo.isWarned = true;
          Printer::popup(clientNum, "You can't re-vote on this vote anymore.");
        }
        return;
      }

      client->pers.votingInfo.time = level.time;
      client->pers.votingInfo.isWarned = false;
      return;
    }

    // defaults to 2s timeout
    if (client->pers.votingInfo.time + ETJump::VOTING_TIMEOUT > level.time) {
      // stops excessive spam from server if user keeps voting in timeouts
      if (!client->pers.votingInfo.isWarned) {
        client->pers.votingInfo.isWarned = true;
        // if vote timeout ever gets changed to other than 1s,
        // this will need to be adjusted!
        Printer::popup(
            clientNum,
            ETJump::stringFormat(
                "^7You must wait for ^3%s ^7before re-voting.",
                ETJump::getSecondsString(ETJump::VOTING_TIMEOUT / 1000)));
      }
      return;
    }

    client->pers.votingInfo.isWarned = false;

    // allow re-vote
    if (isRtvVote) {
      if (isRtvVoteCmd) {
        ETJump::updateVotingInfo(ent, mapNum, ETJump::VotingTypes::RevoteRtv);
      } else {
        // 'vote yes' is handled before the re-vote period check
        // to always allow opening the menu, even when past re-vote period
        if (votedNo(voteArgStr)) {
          if (client->pers.votingInfo.isVotedYes) {
            ETJump::updateVotingInfo(ent, mapNum,
                                     ETJump::VotingTypes::RevoteNo);
          }
        } else {
          printRtvVoteMsgs();
          return;
        }
      }
    } else {
      if (votedYes(voteArgStr)) {
        if (!client->pers.votingInfo.isVotedYes) {
          ETJump::updateVotingInfo(ent, mapNum, ETJump::VotingTypes::RevoteYes);
        }
      } else if (votedNo(voteArgStr)) {
        if (client->pers.votingInfo.isVotedYes) {
          ETJump::updateVotingInfo(ent, mapNum, ETJump::VotingTypes::RevoteNo);
        }
      } else {
        printVoteMsgs();
        return;
      }
    }

    Printer::popup(clientNum, ETJump::stringFormat(
                                  "Vote cast, you can change your vote %s.",
                                  ETJump::getPluralizedString(
                                      ETJump::VOTING_ATTEMPTS + 1 -
                                          client->pers.votingInfo.attempts,
                                      "time")));
    return;
  }

  if (ent->client->sess.sessionTeam == TEAM_SPECTATOR &&
      !g_spectatorVote.integer) {
    Printer::popup(clientNum, "You are not allowed to vote as a spectator.");
    return;
  }

  // we need to do all this handling here because the initial vote caller
  // hasn't actually voted yet on rtv vote (so we skip the code above),
  // and it's possible they just hit esc to close the menu without voting
  if (isRtvVote) {
    if (!isRtvVoteCmd) {
      if (votedYes(voteArgStr)) {
        trap_SendServerCommand(clientNum, "openRtvMenu");
        return;
      } else if (votedNo(voteArgStr)) {
        if (level.voteInfo.voter_cn == clientNum) {
          cancelVote();
          return;
        } else {
          ETJump::updateVotingInfo(ent, mapNum, ETJump::VotingTypes::VoteNo);
        }
      } else {
        printRtvVoteMsgs();
        return;
      }
    } else {
      ETJump::updateVotingInfo(ent, mapNum, ETJump::VotingTypes::VoteRtv);
    }
  } else {
    // regular vote
    if (votedYes(voteArgStr)) {
      ETJump::updateVotingInfo(ent, mapNum, ETJump::VotingTypes::VoteYes);
    } else if (votedNo(voteArgStr)) {
      ETJump::updateVotingInfo(ent, mapNum, ETJump::VotingTypes::VoteNo);
    } else {
      printVoteMsgs();
      return;
    }
  }

  Printer::console(clientNum, "Vote cast.\n");

  ent->client->ps.eFlags |= EF_VOTED;
  level.voteInfo.voteCanceled = qfalse;

  CalculateRanks();
}

qboolean G_canPickupMelee(gentity_t *ent) {
  // JPW NERVE -- no "melee" weapons in net play
  return qfalse;
}
// jpw

/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f(gentity_t *ent) {
  if (!g_cheats.integer) {
    Printer::console(ent, "Cheats are not enabled on this server.\n");
    return;
  }

  const int argc = trap_Argc();

  if (argc != 4 && argc != 7) {
    Printer::console(
        ent, "Usage:\nsetviewpos x y z\nsetviewpos x y z pitch yaw roll\n");
    return;
  }

  vec3_t origin, angles;
  char buffer[MAX_TOKEN_CHARS];

  for (int i = 0; i < 3; i++) {
    trap_Argv(i + 1, buffer, sizeof(buffer));
    origin[i] = Q_atof(buffer);

    if (argc == 4) {
      angles[i] = ent->client->ps.viewangles[i];
    } else {
      trap_Argv(i + 4, buffer, sizeof(buffer));
      angles[i] = AngleNormalize180(Q_atof(buffer));
    }
  }

  DirectTeleport(ent, origin, angles);
}

/*
=================
Cmd_StartCamera_f
=================
*/
void Cmd_StartCamera_f(gentity_t *ent) {

  if (ent->client->cameraPortal) {
    G_FreeEntity(ent->client->cameraPortal);
  }
  ent->client->cameraPortal = G_Spawn();

  ent->client->cameraPortal->s.eType = ET_CAMERA;
  ent->client->cameraPortal->s.apos.trType = TR_STATIONARY;
  ent->client->cameraPortal->s.apos.trTime = 0;
  ent->client->cameraPortal->s.apos.trDuration = 0;
  VectorClear(ent->client->cameraPortal->s.angles);
  VectorClear(ent->client->cameraPortal->s.apos.trDelta);
  G_SetOrigin(ent->client->cameraPortal, ent->r.currentOrigin);
  VectorCopy(ent->r.currentOrigin, ent->client->cameraPortal->s.origin2);

  ent->client->cameraPortal->s.frame = 0;

  ent->client->cameraPortal->r.svFlags |= (SVF_PORTAL | SVF_SINGLECLIENT);
  ent->client->cameraPortal->r.singleClient = ent->client->ps.clientNum;

  ent->client->ps.eFlags |= EF_VIEWING_CAMERA;
  ent->s.eFlags |= EF_VIEWING_CAMERA;

  VectorCopy(ent->r.currentOrigin,
             ent->client->cameraOrigin); // backup our origin

  // (SA) trying this in client to avoid 1 frame of player drawing
  //	ent->client->ps.eFlags |= EF_NODRAW;
  //	ent->s.eFlags |= EF_NODRAW;
}

/*
=================
Cmd_StopCamera_f
=================
*/
void Cmd_StopCamera_f(gentity_t *ent) {
  if (ent->client->cameraPortal &&
      (ent->client->ps.eFlags & EF_VIEWING_CAMERA)) {

    // go back into noclient mode
    G_FreeEntity(ent->client->cameraPortal);
    ent->client->cameraPortal = NULL;

    ent->s.eFlags &= ~EF_VIEWING_CAMERA;
    ent->client->ps.eFlags &= ~EF_VIEWING_CAMERA;
  }
}

/*
=================
Cmd_SetCameraOrigin_f
=================
*/
void Cmd_SetCameraOrigin_f(gentity_t *ent) {
  char buffer[MAX_TOKEN_CHARS];
  int i;
  vec3_t origin;

  if (trap_Argc() != 4) {
    return;
  }

  for (i = 0; i < 3; i++) {
    trap_Argv(i + 1, buffer, sizeof(buffer));
    origin[i] = Q_atof(buffer);
  }

  if (ent->client->cameraPortal) {
    // G_SetOrigin( ent->client->cameraPortal, origin );	// set
    // our origin
    VectorCopy(origin, ent->client->cameraPortal->s.origin2);
    trap_LinkEntity(ent->client->cameraPortal);
    //	G_SetOrigin( ent, origin );	// set our origin
    //	VectorCopy( origin, ent->client->ps.origin );
  }
}

extern gentity_t *BotFindEntityForName(char *name);

extern vec3_t playerMins;
extern vec3_t playerMaxs;

qboolean G_TankIsOccupied(gentity_t *ent) {
  if (!ent->tankLink) {
    return qfalse;
  }

  return qtrue;
}

qboolean G_TankIsMountable(gentity_t *ent, gentity_t *other) {
  if (!(ent->spawnflags & 128)) {
    return qfalse;
  }

  if (level.disableTankEnter) {
    return qfalse;
  }

  if (G_TankIsOccupied(ent)) {
    return qfalse;
  }

  if (ent->health <= 0) {
    return qfalse;
  }

  if (other->client->ps.weaponDelay) {
    return qfalse;
  }

  return qtrue;
}

// Rafael
/*
==================
Cmd_Activate_f
==================
*/
qboolean Do_Activate2_f(gentity_t *ent, gentity_t *traceEnt) {
  qboolean found = qfalse;

  if (ent->client->sess.playerType == PC_COVERTOPS &&
      !ent->client->ps.powerups[PW_OPS_DISGUISED] && ent->health > 0) {
    if (!ent->client->ps.powerups[PW_BLUEFLAG] &&
        !ent->client->ps.powerups[PW_REDFLAG]) {
      if (traceEnt->s.eType == ET_CORPSE) {
        if (level.time - traceEnt->s.time >= DEFAULT_SV_FRAMETIME &&
            BODY_TEAM(traceEnt) < 4 &&
            BODY_TEAM(traceEnt) != ent->client->sess.sessionTeam) {
          found = qtrue;

          if (BODY_VALUE(traceEnt) >= 250) {

            traceEnt->nextthink = traceEnt->timestamp + BODY_TIME;

            ent->client->ps.powerups[PW_OPS_DISGUISED] = 1;
            ent->client->ps.powerups[PW_OPS_CLASS_1] = BODY_CLASS(traceEnt) & 1;
            ent->client->ps.powerups[PW_OPS_CLASS_2] = BODY_CLASS(traceEnt) & 2;
            ent->client->ps.powerups[PW_OPS_CLASS_3] = BODY_CLASS(traceEnt) & 4;

            BODY_TEAM(traceEnt) += 4;
            traceEnt->activator = ent;

            traceEnt->s.time2 = 1;

            // sound effect
            G_AddEvent(ent, EV_DISGUISE_SOUND, 0);

            G_AddSkillPoints(ent, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS,
                             5.f);
            G_DebugAddSkillPoints(
                ent, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 5,
                "stealing "
                "uniform");

            Q_strncpyz(ent->client->disguiseNetname,
                       g_entities[traceEnt->s.clientNum].client->pers.netname,
                       sizeof(ent->client->disguiseNetname));
            ent->client->disguiseRank =
                g_entities[traceEnt->s.clientNum].client
                    ? g_entities[traceEnt->s.clientNum].client->sess.rank
                    : 0;

            ClientUserinfoChanged(ent->s.clientNum);
          } else {
            BODY_VALUE(traceEnt) += 5;
            traceEnt->s.time = level.time;
          }
        }
      }
    }
  }

  return found;
}

// TAT 1/14/2003 - extracted out the functionality of Cmd_Activate_f from
// finding the object to use
//		so we can force bots to use items, without worrying that they
// are looking EXACTLY at the target
qboolean Do_Activate_f(gentity_t *ent, gentity_t *traceEnt) {
  qboolean found = qfalse;
  qboolean walking = qfalse;
  vec3_t forward; //, offset, end;
  // trace_t		tr;

  // specs can't use
  if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
    return qfalse;
  }

  // Arnout: invisible entities can't be used

  if (traceEnt->entstate == STATE_INVISIBLE ||
      traceEnt->entstate == STATE_UNDERCONSTRUCTION) {
    return qfalse;
  }

  if (ent->client->pers.cmd.buttons & BUTTON_WALKING) {
    walking = qtrue;
  }

  if (traceEnt->classname) {
    traceEnt->flags &= ~FL_SOFTACTIVATE; // FL_SOFTACTIVATE will be set if the
                                         // user is holding 'walk' key

    if (traceEnt->s.eType == ET_ALARMBOX) {
      trace_t trace;

      if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
        return qfalse;
      }

      memset(&trace, 0, sizeof(trace));

      if (traceEnt->use) {
        G_UseEntity(traceEnt, ent, 0);
      }
      found = qtrue;
    } else if (traceEnt->s.eType == ET_ITEM) {
      trace_t trace;

      if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
        return qfalse;
      }

      memset(&trace, 0, sizeof(trace));

      if (traceEnt->touch) {
        if (ent->client->pers.autoActivate == PICKUP_ACTIVATE) {
          ent->client->pers.autoActivate = PICKUP_FORCE; //----(SA)
                                                         // force
                                                         // pickup
        }
        traceEnt->active = qtrue;
        traceEnt->touch(traceEnt, ent, &trace);
      }

      found = qtrue;
    } else if (traceEnt->s.eType == ET_MOVER &&
               G_TankIsMountable(traceEnt, ent)) {
      G_Script_ScriptEvent(traceEnt, "mg42", "mount");
      ent->tagParent = traceEnt->nextTrain;
      Q_strncpyz(ent->tagName, "tag_player", MAX_QPATH);
      ent->backupWeaponTime = ent->client->ps.weaponTime;
      ent->client->ps.weaponTime = traceEnt->backupWeaponTime;
      ent->client->pmext.weapHeat[WP_DUMMY_MG42] =
          static_cast<float>(traceEnt->mg42weapHeat);
      ent->client->ps.ammo[WP_DUMMY_MG42] = traceEnt->mg42weapHeat;

      ent->tankLink = traceEnt;
      traceEnt->tankLink = ent;

      G_ProcessTagConnect(ent, qtrue);
      found = qtrue;
    } else if (G_EmplacedGunIsMountable(traceEnt, ent)) {
      gclient_t *cl = &level.clients[ent->s.clientNum];
      vec3_t point;

      AngleVectors(traceEnt->s.apos.trBase, forward, NULL, NULL);
      VectorMA(traceEnt->r.currentOrigin, -36, forward, point);
      point[2] = ent->r.currentOrigin[2];

      // Save initial position
      VectorCopy(point, ent->TargetAngles);

      // Zero out velocity
      VectorCopy(vec3_origin, ent->client->ps.velocity);
      VectorCopy(vec3_origin, ent->s.pos.trDelta);

      traceEnt->active = qtrue;
      ent->active = qtrue;
      traceEnt->r.ownerNum = ent->s.number;
      VectorCopy(traceEnt->s.angles, traceEnt->TargetAngles);
      traceEnt->s.otherEntityNum = ent->s.number;

      cl->pmext.harc = traceEnt->harc;
      cl->pmext.varc = traceEnt->varc;
      VectorCopy(traceEnt->s.angles, cl->pmext.centerangles);
      cl->pmext.centerangles[PITCH] =
          AngleNormalize180(cl->pmext.centerangles[PITCH]);
      cl->pmext.centerangles[YAW] =
          AngleNormalize180(cl->pmext.centerangles[YAW]);
      cl->pmext.centerangles[ROLL] =
          AngleNormalize180(cl->pmext.centerangles[ROLL]);

      ent->backupWeaponTime = ent->client->ps.weaponTime;
      ent->client->ps.weaponTime = traceEnt->backupWeaponTime;
      ent->client->pmext.weapHeat[WP_DUMMY_MG42] =
          static_cast<float>(traceEnt->mg42weapHeat);
      ent->client->ps.ammo[WP_DUMMY_MG42] = traceEnt->mg42weapHeat;

      G_UseTargets(traceEnt,
                   ent); //----(SA)	added for Mike so
                         // mounting an MG42 can be a
                         // trigger event (let me know if there's
                         // any issues with this)
      found = qtrue;
    } else if (((Q_stricmp(traceEnt->classname, "func_door") == 0) ||
                (Q_stricmp(traceEnt->classname, "func_door_rotating") == 0))) {
      if (walking) {
        traceEnt->flags |= FL_SOFTACTIVATE; // no noise
      }
      G_TryDoor(traceEnt, ent,
                ent); // (door,other,activator)
      found = qtrue;
    } else if ((Q_stricmp(traceEnt->classname, "team_WOLF_checkpoint") == 0)) {
      if (traceEnt->count != ent->client->sess.sessionTeam) {
        traceEnt->health++;
      }
      found = qtrue;
    } else if (!Q_stricmp(traceEnt->classname, "func_button") &&
               !traceEnt->active) {
      if (ETJump::activateButton(traceEnt)) {
        Use_BinaryMover(traceEnt, ent, ent);
        traceEnt->active = qtrue;
        found = qtrue;
      }
    } else if (!Q_stricmp(traceEnt->classname, "func_invisible_user")) {
      if (walking) {
        traceEnt->flags |= FL_SOFTACTIVATE; // no noise
      }
      G_UseEntity(traceEnt, ent, ent);
      found = qtrue;
    } else if (!Q_stricmp(traceEnt->classname, "props_footlocker")) {
      G_UseEntity(traceEnt, ent, ent);
      found = qtrue;
    }
  }

  return found;
}

void G_LeaveTank(gentity_t *ent, qboolean position) {
  gentity_t *tank;

  // found our tank (or whatever)
  vec3_t axis[3];
  vec3_t pos;
  trace_t tr;

  tank = ent->tankLink;
  if (!tank) {
    return;
  }

  if (position) {

    AnglesToAxis(tank->s.angles, axis);

    VectorMA(ent->client->ps.origin, 128, axis[1], pos);
    trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

    if (tr.startsolid) {
      // try right
      VectorMA(ent->client->ps.origin, -128, axis[1], pos);
      trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

      if (tr.startsolid) {
        // try back
        VectorMA(ent->client->ps.origin, -224, axis[0], pos);
        trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

        if (tr.startsolid) {
          // try front
          VectorMA(ent->client->ps.origin, 224, axis[0], pos);
          trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

          if (tr.startsolid) {
            // give up
            return;
          }
        }
      }
    }

    VectorClear(
        ent->client->ps.velocity); // Gordon: dont want them to fly away ;D
    TeleportPlayer(ent, pos, ent->client->ps.viewangles);
  }

  tank->mg42weapHeat =
      static_cast<int>(ent->client->pmext.weapHeat[WP_DUMMY_MG42]);
  tank->backupWeaponTime = ent->client->ps.weaponTime;
  ent->client->ps.weaponTime = ent->backupWeaponTime;

  G_Script_ScriptEvent(tank, "mg42", "unmount");
  ent->tagParent = NULL;
  *ent->tagName = '\0';
  ent->s.eFlags &= ~EF_MOUNTEDTANK;
  ent->client->ps.eFlags &= ~EF_MOUNTEDTANK;
  tank->s.powerups = -1;

  tank->tankLink = NULL;
  ent->tankLink = NULL;
}

void Cmd_Activate_f(gentity_t *ent) {
  trace_t tr;
  vec3_t end;
  gentity_t *traceEnt;
  vec3_t forward, right, up, offset;
  //	int			activatetime = level.time;
  qboolean found = qfalse;
  qboolean pass2 = qfalse;
  int i;

  if (ent->health <= 0) {
    return;
  }

  if (ent->s.weapon == WP_MORTAR_SET || ent->s.weapon == WP_MOBILE_MG42_SET) {
    return;
  }

  if (ent->active) {
    if (ent->client->ps.persistant[PERS_HWEAPON_USE]) {
      // DHM - Nerve :: Restore original position if
      // current position is bad
      trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs,
                 ent->r.currentOrigin, ent->s.number, MASK_PLAYERSOLID);
      if (tr.startsolid) {
        VectorCopy(ent->TargetAngles, ent->client->ps.origin);
        VectorCopy(ent->TargetAngles, ent->r.currentOrigin);
        ent->r.contents = CONTENTS_CORPSE; // DHM - this will
                                           // correct itself
                                           // in
                                           // ClientEndFrame
      }

      ent->client->ps.eFlags &= ~EF_MG42_ACTIVE; // DHM - Nerve :: unset flag
      ent->client->ps.eFlags &= ~EF_AAGUN_ACTIVE;

      ent->client->ps.persistant[PERS_HWEAPON_USE] = 0;
      ent->active = qfalse;

      for (i = 0; i < level.num_entities; i++) {
        if (g_entities[i].s.eType == ET_MG42_BARREL &&
            g_entities[i].r.ownerNum == ent->s.number) {
          g_entities[i].mg42weapHeat =
              static_cast<int>(ent->client->pmext.weapHeat[WP_DUMMY_MG42]);
          g_entities[i].backupWeaponTime = ent->client->ps.weaponTime;
          break;
        }
      }
      ent->client->ps.weaponTime = ent->backupWeaponTime;
    } else {
      ent->active = qfalse;
    }
    return;
  } else if (ent->client->ps.eFlags & EF_MOUNTEDTANK &&
             ent->s.eFlags & EF_MOUNTEDTANK && !level.disableTankExit) {
    G_LeaveTank(ent, qtrue);
    return;
  }

  AngleVectors(ent->client->ps.viewangles, forward, right, up);

  VectorCopy(ent->client->ps.origin, offset);
  offset[2] += ent->client->ps.viewheight;

  // lean
  if (ent->client->ps.leanf) {
    VectorMA(offset, ent->client->ps.leanf, right, offset);
  }

  // VectorMA( offset, 256, forward, end );
  VectorMA(offset, 96, forward, end);

  trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number,
             (CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY |
              CONTENTS_CORPSE));

  if (tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD) {
    trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number,
               (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE |
                CONTENTS_MISSILECLIP | CONTENTS_TRIGGER));
    pass2 = qtrue;
  }

tryagain:

  if (tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD) {
    return;
  }

  traceEnt = &g_entities[tr.entityNum];

  found = Do_Activate_f(ent, traceEnt);

  if (!found && !pass2) {
    pass2 = qtrue;
    trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number,
               (CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY |
                CONTENTS_CORPSE | CONTENTS_TRIGGER));
    goto tryagain;
  }

  if (!found) {
    ETJump::longRangeActivate(ent);
  }
}

void Cmd_Activate2_f(gentity_t *ent) {
  trace_t tr;
  vec3_t end;
  gentity_t *traceEnt;
  vec3_t forward, right, up, offset;
  //	int			activatetime = level.time;
  qboolean found = qfalse;
  qboolean pass2 = qfalse;

  if (ent->client->sess.playerType != PC_COVERTOPS) {
    return;
  }

  AngleVectors(ent->client->ps.viewangles, forward, right, up);
  CalcMuzzlePointForActivate(ent, forward, right, up, offset);
  VectorMA(offset, 96, forward, end);

  trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number,
             (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE));

  if (tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD) {
    trap_Trace(
        &tr, offset, NULL, NULL, end, ent->s.number,
        (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
    pass2 = qtrue;
  }

tryagain:

  if (tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD) {
    return;
  }

  traceEnt = &g_entities[tr.entityNum];

  found = Do_Activate2_f(ent, traceEnt);

  if (!found && !pass2) {
    pass2 = qtrue;
    trap_Trace(
        &tr, offset, NULL, NULL, end, ent->s.number,
        (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
    goto tryagain;
  }
}

namespace ETJump {
bool allowQuickFollow(gentity_t *ent, gentity_t *traceEnt) {
  if (!EntityUtilities::isPlayer(traceEnt)) {
    return false;
  }
  if (!G_AllowFollow(ent, traceEnt)) {
    auto clientNum = ClientNum(ent);
    std::string specLockMsg = ETJump::stringFormat(
        "%s is speclocked.", traceEnt->client->pers.netname);
    Printer::popup(clientNum, specLockMsg);
    return false;
  }
  if (!ent->client->pers.quickFollow) {
    return false;
  }
  if (traceEnt->client->pers.hideMe) {
    return false;
  }
  return true;
}

void longRangeActivate(gentity_t *ent) {
  trace_t tr;
  vec3_t end;
  gentity_t *traceEnt;
  vec3_t forward, right, up, offset;
  const auto MAX_DISTANCE = 1 << 16;
  AngleVectors(ent->client->ps.viewangles, forward, right, up);
  VectorCopy(ent->client->ps.origin, offset);
  offset[2] += ent->client->ps.viewheight;
  VectorMA(offset, MAX_DISTANCE, forward, end);

  trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number,
             CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_MISSILECLIP |
                 CONTENTS_BODY | CONTENTS_CORPSE);

  if (tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD) {
    return;
  }

  traceEnt = &g_entities[tr.entityNum];

  if (ETJump::allowQuickFollow(ent, traceEnt)) {
    if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
      SetTeam(ent, "spectator", qfalse, static_cast<weapon_t>(-1),
              static_cast<weapon_t>(-1), qfalse);
    }

    ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
    ent->client->sess.spectatorClient = ClientNum(traceEnt);
  }
}
} // namespace ETJump

/*
============================
Cmd_ClientMonsterSlickAngle
============================
*/
/*
void Cmd_ClientMonsterSlickAngle (gentity_t *clent) {

    char s[MAX_STRING_CHARS];
    int	entnum;
    int angle;
    gentity_t *ent;
    vec3_t	dir, kvel;
    vec3_t	forward;

    if (trap_Argc() != 3) {
        G_Printf( "ClientDamage command issued with incorrect number of
args\n"
);
    }

    trap_Argv( 1, s, sizeof( s ) );
    entnum = Q_atoi(s);
    ent = &g_entities[entnum];

    trap_Argv( 2, s, sizeof( s ) );
    angle = Q_atoi(s);

    // sanity check (also protect from cheaters)
    if (g_gametype.integer != GT_SINGLE_PLAYER && entnum != clent->s.number) {
        trap_DropClient( clent->s.number, "Dropped due to illegal
ClientMonsterSlick command\n" ); return;
    }

    VectorClear (dir);
    dir[YAW] = angle;
    AngleVectors (dir, forward, NULL, NULL);

    VectorScale (forward, 32, kvel);
    VectorAdd (ent->client->ps.velocity, kvel, ent->client->ps.velocity);
}
*/

void G_UpdateSpawnCounts(void) {
  int i, j;
  char cs[MAX_STRING_CHARS];
  int current, count, team;

  for (i = 0; i < level.numspawntargets; i++) {
    trap_GetConfigstring(CS_MULTI_SPAWNTARGETS + i, cs, sizeof(cs));

    current = Q_atoi(Info_ValueForKey(cs, "c"));
    team = Q_atoi(Info_ValueForKey(cs, "t")) & ~256;

    count = 0;
    for (j = 0; j < level.numConnectedClients; j++) {
      gclient_t *client = &level.clients[level.sortedClients[j]];

      if (client->sess.sessionTeam != TEAM_AXIS &&
          client->sess.sessionTeam != TEAM_ALLIES) {
        continue;
      }

      if (client->sess.sessionTeam == team &&
          client->sess.spawnObjectiveIndex == i + 1) {
        count++;
        continue;
      }

      if (client->sess.spawnObjectiveIndex == 0) {
        if (client->sess.autoSpawnObjectiveIndex > 0) {
          if (client->sess.sessionTeam == team &&
              client->sess.autoSpawnObjectiveIndex == i + 1) {
            count++;
            continue;
          }
        } else {
          if (client->sess.sessionTeam == TEAM_AXIS) {
            if (level.axisAutoSpawn == i) {
              count++;
              continue;
            }
          } else {
            if (level.alliesAutoSpawn == i) {
              count++;
              continue;
            }
          }
        }
      }
    }

    if (count == current) {
      continue;
    }

    Info_SetValueForKey(cs, "c", va("%i", count));
    trap_SetConfigstring(CS_MULTI_SPAWNTARGETS + i, cs);
  }
}

/*
============
Cmd_SetSpawnPoint_f
============
*/
void SetPlayerSpawn(gentity_t *ent, int spawn, qboolean update) {
  ent->client->sess.spawnObjectiveIndex = spawn;
  if (ent->client->sess.spawnObjectiveIndex >= MAX_MULTI_SPAWNTARGETS ||
      ent->client->sess.spawnObjectiveIndex < 0) {
    ent->client->sess.spawnObjectiveIndex = 0;
  }

  if (update) {
    G_UpdateSpawnCounts();
  }
}

void Cmd_SetSpawnPoint_f(gentity_t *ent) {
  char arg[MAX_TOKEN_CHARS];
  int val, i;

  if (trap_Argc() != 2) {
    return;
  }

  trap_Argv(1, arg, sizeof(arg));
  val = Q_atoi(arg);

  if (ent->client) {
    SetPlayerSpawn(ent, val, qtrue);
  }

  //	if( ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
  //!(ent->client->ps.pm_flags & PMF_LIMBO) ) { 		return;
  //	}

  for (i = 0; i < level.numLimboCams; i++) {
    int x = (g_entities[level.limboCams[i].targetEnt].count -
             CS_MULTI_SPAWNTARGETS) +
            1;
    if (level.limboCams[i].spawn && x == val) {
      VectorCopy(level.limboCams[i].origin, ent->s.origin2);
      ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
      trap_SendServerCommand(
          ent - g_entities,
          va("portalcampos %i %i %i %i %i %i %i %i", val - 1,
             (int)level.limboCams[i].origin[0],
             (int)level.limboCams[i].origin[1],
             (int)level.limboCams[i].origin[2],
             (int)level.limboCams[i].angles[0],
             (int)level.limboCams[i].angles[1],
             (int)level.limboCams[i].angles[2],
             level.limboCams[i].hasEnt ? level.limboCams[i].targetEnt : -1));
      break;
    }
  }
}

/*
============
Cmd_SetSniperSpot_f
============
*/
void Cmd_SetSniperSpot_f(gentity_t *clent) {
  gentity_t *spot;

  vmCvar_t cvar_mapname;
  char filename[MAX_QPATH];
  fileHandle_t f;
  char buf[1024];

  if (!g_cheats.integer) {
    return;
  }
  if (!trap_Cvar_VariableIntegerValue("cl_running")) {
    return; // only allow locally playing client
  }
  if (clent->s.number != 0) {
    return; // only allow locally playing client
  }
  // drop a sniper spot here
  spot = G_Spawn();
  spot->classname = "bot_sniper_spot";
  VectorCopy(clent->r.currentOrigin, spot->s.origin);
  VectorCopy(clent->client->ps.viewangles, spot->s.angles);
  spot->aiTeam = clent->client->sess.sessionTeam;

  // output to text file
  trap_Cvar_Register(&cvar_mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);

  Com_sprintf(filename, sizeof(filename), "maps/%s.botents",
              cvar_mapname.string);
  if (trap_FS_FOpenFile(filename, &f, FS_APPEND) < 0) {
    G_Error("Cmd_SetSniperSpot_f: cannot open %s for writing", filename);
  }

  Com_sprintf(buf, sizeof(buf),
              "{\n\"classname\" \"%s\"\n\"origin\" \"%.3f %.3f "
              "%.3f\"\n\"angles\" \"%.2f %.2f %.2f\"\n\"aiTeam\" \"%i\"\n}\n\n",
              spot->classname, spot->s.origin[0], spot->s.origin[1],
              spot->s.origin[2], spot->s.angles[0], spot->s.angles[1],
              spot->s.angles[2], spot->aiTeam);
  trap_FS_Write(buf, strlen(buf), f);

  trap_FS_FCloseFile(f);

  G_Printf("dropped sniper spot\n");

  return;
}

void G_PrintAccuracyLog(gentity_t *ent);

/*
============
Cmd_SetWayPoint_f
============
*/
/*void Cmd_SetWayPoint_f( gentity_t *ent ) {
    char	arg[MAX_TOKEN_CHARS];
    vec3_t	forward, muzzlePoint, end, loc;
    trace_t	trace;

    if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
        trap_SendServerCommand( ent-g_entities, "print \"Not allowed to set
waypoints as spectator.\n\"" ); return;
    }

    if ( trap_Argc() != 2 ) {
        return;
    }

    trap_Argv( 1, arg, sizeof( arg ) );

    AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );

    VectorCopy( ent->r.currentOrigin, muzzlePoint );
    muzzlePoint[2] += ent->client->ps.viewheight;

    VectorMA( muzzlePoint, 8192, forward, end );
    trap_Trace( &trace, muzzlePoint, NULL, NULL, end, ent->s.number, MASK_SHOT
);

    if( trace.surfaceFlags & SURF_NOIMPACT )
        return;

    VectorCopy( trace.endpos, loc );

    G_SetWayPoint( ent, Q_atoi(arg), loc );
}*/

/*
============
Cmd_ClearWayPoint_f
============
*/
/*void Cmd_ClearWayPoint_f( gentity_t *ent ) {

    if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
        trap_SendServerCommand( ent-g_entities, "print \"Not allowed to clear
waypoints as spectator.\n\"" ); return;
    }

    G_RemoveWayPoint( ent->client );
}*/

void Cmd_WeaponStat_f(gentity_t *ent) {
  char buffer[16];
  extWeaponStats_t stat;

  if (!ent || !ent->client) {
    return;
  }

  if (trap_Argc() != 2) {
    return;
  }
  trap_Argv(1, buffer, 16);
  stat = static_cast<extWeaponStats_t>(Q_atoi(buffer));
  if (stat >= 0 && stat < WS_MAX) {
    trap_SendServerCommand(ent - g_entities,
                           va("rws %i %i",
                              ent->client->sess.aWeaponStats[stat].atts,
                              ent->client->sess.aWeaponStats[stat].hits));
  }
}

void Cmd_IntermissionWeaponStats_f(gentity_t *ent) {
  char buffer[1024];
  int i, clientNum;

  if (!ent || !ent->client) {
    return;
  }

  trap_Argv(1, buffer, sizeof(buffer));

  clientNum = Q_atoi(buffer);
  if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
    return;
  }

  Q_strncpyz(buffer, "imws ", sizeof(buffer));
  for (i = 0; i < WS_MAX; i++) {
    Q_strcat(buffer, sizeof(buffer),
             va("%i %i %i ", level.clients[clientNum].sess.aWeaponStats[i].atts,
                level.clients[clientNum].sess.aWeaponStats[i].hits,
                level.clients[clientNum].sess.aWeaponStats[i].kills));
  }

  trap_SendServerCommand(ent - g_entities, buffer);
}

void G_MakeReady(gentity_t *ent) {
  ent->client->ps.eFlags |= EF_READY;
  ent->s.eFlags |= EF_READY;
  // rain - #105 - moved this set here
  ent->client->pers.ready = qtrue;
}

void G_MakeUnready(gentity_t *ent) {
  ent->client->ps.eFlags &= ~EF_READY;
  ent->s.eFlags &= ~EF_READY;
  // rain - #105 - moved this set here
  ent->client->pers.ready = qfalse;
}

void Cmd_IntermissionReady_f(gentity_t *ent) {
  if (!ent || !ent->client) {
    return;
  }

  G_MakeReady(ent);
}

void Cmd_IntermissionPlayerKillsDeaths_f(gentity_t *ent) {
  char buffer[1024];
  int i;

  if (!ent || !ent->client) {
    return;
  }

  Q_strncpyz(buffer, "impkd ", sizeof(buffer));
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (g_entities[i].inuse) {
      Q_strcat(buffer, sizeof(buffer),
               va("%i %i ", level.clients[i].sess.kills,
                  level.clients[i].sess.deaths));
    } else {
      Q_strcat(buffer, sizeof(buffer), "0 0 ");
    }
  }

  trap_SendServerCommand(ent - g_entities, buffer);
}

void G_CalcClientAccuracies(void) {
  int i, j;
  int shots, hits;

  for (i = 0; i < MAX_CLIENTS; i++) {
    shots = 0;
    hits = 0;

    if (g_entities[i].inuse) {
      for (j = 0; j < WS_MAX; j++) {
        shots += level.clients[i].sess.aWeaponStats[j].atts;
        hits += level.clients[i].sess.aWeaponStats[j].hits;
      }

      level.clients[i].acc = shots ? (100 * hits) / (float)shots : 0;
    } else {
      level.clients[i].acc = 0;
    }
  }
}

void Cmd_IntermissionWeaponAccuracies_f(gentity_t *ent) {
  char buffer[1024];
  int i;

  if (!ent || !ent->client) {
    return;
  }

  G_CalcClientAccuracies();

  Q_strncpyz(buffer, "imwa ", sizeof(buffer));
  for (i = 0; i < MAX_CLIENTS; i++) {
    Q_strcat(buffer, sizeof(buffer), va("%i ", (int)level.clients[i].acc));
  }

  trap_SendServerCommand(ent - g_entities, buffer);
}

void Cmd_SelectedObjective_f(gentity_t *ent) {
  int i, val;
  char buffer[16];
  vec_t dist, neardist = 0;
  int nearest = -1;

  if (!ent || !ent->client) {
    return;
  }

  if (trap_Argc() != 2) {
    return;
  }
  trap_Argv(1, buffer, 16);
  val = Q_atoi(buffer) + 1;

  // if limbo cams aren't present, use spectator spawnpoint by default,
  // so we don't get limbo cams potentially in void
  if (!level.numLimboCams) {
    VectorCopy(level.intermission_origin, ent->s.origin2);
    ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
    trap_SendServerCommand(
        ClientNum(ent),
        va("portalcampos %i %.0f %.0f %.0f %.0f %.0f %.0f %i", val - 1,
           level.intermission_origin[0], level.intermission_origin[1],
           level.intermission_origin[2], level.intermission_angle[0],
           level.intermission_angle[1], level.intermission_angle[2], -1));
    return;
  }

  for (i = 0; i < level.numLimboCams; i++) {
    if (!level.limboCams[i].spawn && level.limboCams[i].info == val) {
      if (!level.limboCams[i].hasEnt) {
        VectorCopy(level.limboCams[i].origin, ent->s.origin2);
        ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
        trap_SendServerCommand(
            ClientNum(ent),
            va("portalcampos %i %.0f %.0f %.0f %.0f %.0f %.0f %i", val - 1,
               level.limboCams[i].origin[0], level.limboCams[i].origin[1],
               level.limboCams[i].origin[2], level.limboCams[i].angles[0],
               level.limboCams[i].angles[1], level.limboCams[i].angles[2],
               level.limboCams[i].hasEnt ? level.limboCams[i].targetEnt : -1));
        break;
      } else {
        dist = VectorDistanceSquared(
            level.limboCams[i].origin,
            g_entities[level.limboCams[i].targetEnt].r.currentOrigin);
        if (nearest == -1 || dist < neardist) {
          nearest = i;
          neardist = dist;
        }
      }
    }
  }

  if (nearest != -1) {
    i = nearest;

    VectorCopy(level.limboCams[i].origin, ent->s.origin2);
    ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
    trap_SendServerCommand(
        ClientNum(ent),
        va("portalcampos %i %.0f %.0f %.0f %.0f %.0f %.0f %i", val - 1,
           level.limboCams[i].origin[0], level.limboCams[i].origin[1],
           level.limboCams[i].origin[2], level.limboCams[i].angles[0],
           level.limboCams[i].angles[1], level.limboCams[i].angles[2],
           level.limboCams[i].hasEnt ? level.limboCams[i].targetEnt : -1));
  }
}

void Cmd_Ignore_f(gentity_t *ent) {
  char cmd[MAX_TOKEN_CHARS];
  int clientNum;

  trap_Argv(1, cmd, sizeof(cmd));

  if (!*cmd) {
    trap_SendServerCommand(ent - g_entities,
                           "print \"usage: Ignore <clientname>.\n\"\n");
    return;
  }

  if ((clientNum = ClientNumberFromString(ent, cmd)) == -1) {
    return;
  }

  if (clientNum != MAX_CLIENTS) {
    COM_BitSet(ent->client->sess.ignoreClients, clientNum);
  }
}

void Cmd_TicketTape_f(void) {
  /*	char	cmd[MAX_TOKEN_CHARS];

      trap_Argv( 1, cmd, sizeof( cmd ) );

      trap_SendServerCommand( -1, va( "tt \"LANDMINES SPOTTED <STOP>
     CHECK COMMAND MAP FOR DETAILS <STOP>\"\n", cmd ));*/
}

void Cmd_UnIgnore_f(gentity_t *ent) {
  char cmd[MAX_TOKEN_CHARS];
  int clientNum;

  trap_Argv(1, cmd, sizeof(cmd));

  if (!*cmd) {
    trap_SendServerCommand(ent - g_entities,
                           "print \"usage: Unignore <clientname>.\n\"\n");
    return;
  }

  if ((clientNum = ClientNumberFromString(ent, cmd)) == -1) {
    return;
  }

  if (clientNum != MAX_CLIENTS) {
    COM_BitClear(ent->client->sess.ignoreClients, clientNum);
  }
}

void Cmd_Goto_f(gentity_t *ent) {
  int clientNum;
  char cmd[MAX_TOKEN_CHARS];
  gentity_t *other;

  if (!g_goto.integer) {
    CP("print \"Goto is disabled on this server.\n\"");
    return;
  }

  if (!g_cheats.integer && level.noGoto) {
    CP("print \"Goto is disabled on this map.\n\"");
    return;
  }

  if (!ent->client->sess.gotoAllowed) {
    CP("print \"You are not allowed to use goto.\n\"");
    return;
  }

  if (!g_cheats.integer && ent->client->sess.timerunActive) {
    CP("print \"Goto is disabled while timerun is active.\n\"");
    return;
  }

  if (trap_Argc() != 2) {
    CP("print \"^7Usage: ^3goto ^7<Player ID> | <Partname> | "
       "<Name>\n\"");
    return;
  }

  trap_Argv(1, cmd, sizeof(cmd));
  if ((clientNum = ClientNumberFromString(ent, cmd)) == -1) {
    return;
  }

  other = g_entities + clientNum;

  if (clientNum == ent - g_entities) {
    CP("print \"^7You can not ^3goto ^7yourself!\n\"");
    return;
  }

  if (other->client->sess.sessionTeam == TEAM_SPECTATOR) {
    CP("print \"^7You can not ^3goto^7 a spectator!\n\"");
    return;
  }

  if (other->client->sess.noGoto) {
    CP("print \"^7Target player has disabled ^3goto^7!\n\"");
    CP("print \"^7You need to ask the other player to enable "
       "goto by using "
       "^3/nogoto^7!\n\"");
    return;
  }

  if (VectorLengthSquared(other->client->ps.velocity) > 0) {
    CP("print \" ^7You can not ^3goto ^7 moving player!\n\"");
    return;
  }

  if (ent->client->sess.sessionTeam != other->client->sess.sessionTeam) {
    const weapon_t w = static_cast<weapon_t>(-1);
    if (other->client->sess.sessionTeam == TEAM_AXIS) {
      SetTeam(ent, "r", qfalse, w, w, qtrue);

    } else if (other->client->sess.sessionTeam == TEAM_ALLIES) {
      SetTeam(ent, "b", qfalse, w, w, qtrue);
    }
  }

  Utilities::copyStanceFromClient(ent, other);
  VectorCopy(other->client->ps.origin, ent->client->ps.origin);
  VectorClear(ent->client->ps.velocity);

  Printer::popup(ClientNum(ent),
                 ETJump::stringFormat("%s ^7-> %s", ent->client->pers.netname,
                                      other->client->pers.netname));
  Printer::popup(ClientNum(other),
                 ETJump::stringFormat("%s ^7-> %s", ent->client->pers.netname,
                                      other->client->pers.netname));
}

void Cmd_Call_f(gentity_t *ent) {
  int clientNum;
  char cmd[MAX_TOKEN_CHARS];
  gentity_t *other;

  if (!g_goto.integer) {
    CP("print \"Call is disabled on this server.\n\"");
    return;
  }

  if (!g_cheats.integer && level.noGoto) {
    CP("print \"Call is disabled on this map.\n\"");
    return;
  }

  if (!ent->client->sess.gotoAllowed) {
    CP("print \"You are not allowed to use call.\n\"");
    return;
  }

  if (trap_Argc() != 2) {
    CP("print \"^7Usage: ^3call ^7<Player ID> | <Partname> | "
       "<Name>\n\"");
    return;
  }

  trap_Argv(1, cmd, sizeof(cmd));
  if ((clientNum = ClientNumberFromString(ent, cmd)) == -1) {
    return;
  }

  other = g_entities + clientNum;

  if (clientNum == ent - g_entities) {
    CP("print \"^7You can not ^3call ^7yourself!\n\"");
    return;
  }

  if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
    CP("print \"^7You can not ^3call^7 as a spectator!\n\"");
    return;
  }

  if (!g_cheats.integer && other->client->sess.timerunActive) {
    CP("print \"^7You can not ^3call^7 a timerunning "
       "player!\n\"");
    return;
  }

  if (other->client->sess.noCall) {
    CP("print \"^7Target player has disabled ^3call^7!\n\"");
    CP("print \"^7You need to ask the other player to enable "
       "call by using "
       "^3/nocall^7!\n\"");
    return;
  }

  if (VectorLengthSquared(other->client->ps.velocity) > 0) {
    CP("print \"^7You can not ^3call ^7a moving player!\n\"");
    return;
  }

  if (other->client->sess.sessionTeam == TEAM_SPECTATOR &&
      other->client->sess.sessionTeam != ent->client->sess.sessionTeam) {
    const weapon_t w = static_cast<weapon_t>(-1);
    if (ent->client->sess.sessionTeam == TEAM_AXIS) {
      SetTeam(other, "r", qfalse, w, w, qtrue);

    } else if (ent->client->sess.sessionTeam == TEAM_ALLIES) {
      SetTeam(other, "b", qfalse, w, w, qtrue);
    }
  }

  Utilities::copyStanceFromClient(other, ent);
  VectorClear(other->client->ps.velocity);
  VectorCopy(ent->client->ps.origin, other->client->ps.origin);

  Printer::popup(ClientNum(ent),
                 ETJump::stringFormat("%s ^7-> %s", other->client->pers.netname,
                                      ent->client->pers.netname));
  Printer::popup(ClientNum(other),
                 ETJump::stringFormat("%s ^7-> %s", other->client->pers.netname,
                                      ent->client->pers.netname));
}

void Cmd_PrivateMessage_f(gentity_t *ent) {
  int clientNum = -1;
  char cmd[MAX_TOKEN_CHARS] = "\0";
  gentity_t *other = nullptr;
  char *msg = nullptr;
  auto selfNum = ClientNum(ent);

  if (trap_Argc() < 3) {
    Printer::console(selfNum, "^7usage: ^3m ^7<name> <message>.\n");
    return;
  }

  if (ent && ent->client->sess.muted && g_mute.integer & 1) {
    Printer::console(selfNum, "^3NOTE: ^7You are muted.\n");
    return;
  }

  trap_Argv(1, cmd, sizeof(cmd));
  if ((clientNum = ClientNumberFromString(ent, cmd)) == -1) {
    return;
  }

  other = g_entities + clientNum;
  const int otherNum = ClientNum(other);

  if (!ent) {
    msg = ConcatArgs(2);
    Printer::chat(ClientNum(other),
                  va("^7Private message from server console: ^3%s", msg));
    trap_SendServerCommand(otherNum, "pmFlashWindow");

    G_Printf("Private message to %s^7: ^3%s\n", other->client->pers.netname,
             msg);
    return;
  }

  if (!COM_BitCheck(other->client->sess.ignoreClients, ClientNum(ent))) {
    msg = ConcatArgs(2);
    Printer::chat(otherNum, va("^7Private message from %s^7: ^3%s",
                               ent->client->pers.netname, msg));
    trap_SendServerCommand(otherNum, "pmFlashWindow");

    if (ent) {
      Printer::chat(selfNum, va("^7Private message to %s^7: ^3%s",
                                other->client->pers.netname, msg));
    }
  } else {
    Printer::console(selfNum,
                     va("Private message to %s was ignored by the player.\n",
                        other->client->pers.netname));
  }
}

void Cmd_noGoto_f(gentity_t *ent) {
  const char *msg;
  if (ent->client->sess.noGoto == qtrue) {
    ent->client->sess.noGoto = qfalse;
    msg = "enabled";
  } else {
    ent->client->sess.noGoto = qtrue;
    msg = "disabled";
  }
  CP(va("print \"^7You have %s ^3goto^7.\n\"", msg));
}

void Cmd_noCall_f(gentity_t *ent) {
  const char *msg;
  if (ent->client->sess.noCall == qtrue) {
    ent->client->sess.noCall = qfalse;
    msg = "enabled";
  } else {
    ent->client->sess.noCall = qtrue;
    msg = "disabled";
  }
  CP(va("print \"^7You have %s ^3call^7.\n\"", msg));
}

/*
 * Figures out if we are allowed to follow a given client.
 */
qboolean G_AllowFollow(gentity_t *ent, gentity_t *other) {
  // Check if target is speclocked, if it is check if we're invited
  return (!other->client->sess.specLocked ||
          COM_BitCheck(other->client->sess.specInvitedClients, ClientNum(ent)))
             ? qtrue
             : qfalse;
}

/*
 * Figures out if we are allowed & want to follow a given client.
 */
qboolean G_DesiredFollow(gentity_t *ent, gentity_t *other) {
  return (G_AllowFollow(ent, other) &&
          (ent->client->sess.spec_team == 0 ||
           ent->client->sess.spec_team == other->client->sess.sessionTeam))
             ? qtrue
             : qfalse;
}

namespace ETJump {
static int getPlayerClassId(const std::string string) {
  switch (string[0]) {
    case 'm':
      return PC_MEDIC;
    case 'e':
      return PC_ENGINEER;
    case 'f':
      return PC_FIELDOPS;
    case 'c':
      return PC_COVERTOPS;
    case 's':
    default:
      return PC_SOLDIER;
  }
}

static char getPlayerClassSymbol(const int num) {
  switch (num) {
    case PC_MEDIC:
      return 'm';
    case PC_ENGINEER:
      return 'e';
    case PC_FIELDOPS:
      return 'f';
    case PC_COVERTOPS:
      return 'c';
    case PC_SOLDIER:
    default:
      return 's';
  }
}

static const char *getPlayerTeamName(const int teamNum) {
  switch (teamNum) {
    case TEAM_AXIS:
      return "Axis";
    case TEAM_ALLIES:
      return "Allied";
    default:
      return "unknown";
  }
}

struct classLoadout {
  const int classId;
  const int weaponSlot;
  const char *description;
};

classLoadout availableLoadouts[]{
    {PC_MEDIC, 1, "Medic with SMG"},
    {PC_MEDIC, 2, "Medic with Rifle"},
    {PC_MEDIC, 3, "Medic with Sniper Rifle"},
    {PC_ENGINEER, 1, "Engineer with SMG"},
    {PC_ENGINEER, 2, "Engineer with Rifle"},
    {PC_ENGINEER, 3, "Engineer with Sniper Rifle"},
    {PC_FIELDOPS, 1, "Field ops with SMG"},
    {PC_FIELDOPS, 2, "Field ops with Rifle"},
    {PC_FIELDOPS, 3, "Field ops with Sniper Rifle"},
    {PC_COVERTOPS, 1, "Covert ops with Sten"},
    {PC_COVERTOPS, 2, "Covert ops with FG42"},
    {PC_COVERTOPS, 3, "Covert ops with Sniper Rifle"},
    {PC_COVERTOPS, 4, "Covert ops with Rifle"},
    {PC_SOLDIER, 1, "Soldier with SMG"},
    {PC_SOLDIER, 2, "Soldier with MG42"},
    {PC_SOLDIER, 3, "Soldier with Flamethrower"},
    {PC_SOLDIER, 4, "Soldier with Panzerfaust"},
    {PC_SOLDIER, 5, "Soldier with Mortar"},
    {PC_SOLDIER, 6, "Soldier with Rifle"},
    {PC_SOLDIER, 7, "Soldier with Sniper Rifle"},
};
} // namespace ETJump

void Cmd_Class_f(gentity_t *ent) {
  const auto args = GetArgs();
  const auto clientNum = ClientNum(ent);
  const int DEFAULT_WEAPON_SLOT = 1; // weapon argument default value

  // not enough arguments were specified, generate usage text
  if (args->size() < 2) {
    std::string usageText{"^3Usage:\n"};

    for (auto &loadout : ETJump::availableLoadouts) {
      usageText += ETJump::stringFormat(
          "  ^7%-30s ^9/class %c %i\n", loadout.description,
          ETJump::getPlayerClassSymbol(loadout.classId), loadout.weaponSlot);
    }

    Printer::console(clientNum, usageText);
    return;
  }

  // get class id
  const auto classId = ETJump::getPlayerClassId((*args)[1]);

  // get weapon slot
  int weaponSlot = DEFAULT_WEAPON_SLOT;
  if (args->size() > 2) {
    try {
      int parsedValue = std::stoi((*args)[2]);
      weaponSlot = std::max(std::min(parsedValue, MAX_WEAPS_PER_CLASS),
                            DEFAULT_WEAPON_SLOT);
    }
    // suppress error and use DEFAULT_WEAPON_SLOT
    catch (const std::invalid_argument &) {
    } catch (const std::out_of_range &) {
    }
  }
  // out of bounds check - if no weapon is set in specified slot
  const auto classInfo =
      BG_GetPlayerClassInfo(ent->client->sess.sessionTeam, classId);
  if (classInfo->classWeapons[weaponSlot - 1] == WP_NONE) {
    weaponSlot = DEFAULT_WEAPON_SLOT;
  }

  // fetch weapons
  const auto primaryWeapon = classInfo->classWeapons[weaponSlot - 1];
  const auto secondaryWeapon =
      static_cast<weapon_t>(ent->client->sess.latchPlayerWeapon2);

  // display center print message
  for (auto &loadout : ETJump::availableLoadouts) {
    if (loadout.classId == classId && loadout.weaponSlot == weaponSlot) {
      Printer::center(clientNum,
                      ETJump::stringFormat("You will spawn as an %s %s",
                                           ETJump::getPlayerTeamName(
                                               ent->client->sess.sessionTeam),
                                           loadout.description),
                      false);
      break;
    }
  }

  // set loadout
  ent->client->sess.latchPlayerType = classId;
  G_SetClientWeapons(ent, primaryWeapon, secondaryWeapon, qtrue);
}

// Little easter egg :]
void Cmd_shrug_f(gentity_t *ent) {
  BG_AnimScriptEvent(&ent->client->ps,
                     ent->client->pers.character->animModelInfo,
                     ANIM_ET_NOPOWER, qtrue, qfalse);
}

// sends back timerun specific information, used to restore runtimer after
// cgame restart
void Cmd_timerunStatus_f(gentity_t *ent) {
  if (level.hasTimerun) {
    trap_SendServerCommand(ClientNum(ent), "hasTimerun 1");
  } else {
    trap_SendServerCommand(ClientNum(ent), "hasTimerun 0");
  }

  TimerunConnectNotify(ent);
}

/*
=================
Cmd_SwapPlacesWithBot_f
=================
*/
void Cmd_SwapPlacesWithBot_f(gentity_t *ent, int botNum) {
  gentity_t *botent;
  gclient_t cl, *client;
  clientPersistant_t saved;
  clientSession_t sess;
  int persistant[MAX_PERSISTANT];
  //
  client = ent->client;
  //
  botent = &g_entities[botNum];
  if (!botent->client) {
    return;
  }
  // if this bot is dead
  if (botent->health <= 0 && (botent->client->ps.pm_flags & PMF_LIMBO)) {
    trap_SendServerCommand(ent - g_entities, "print \"Bot is in limbo mode, "
                                             "cannot swap places.\n\"");
    return;
  }
  //
  if (client->sess.sessionTeam != botent->client->sess.sessionTeam) {
    trap_SendServerCommand(ent - g_entities,
                           "print \"Bot is on different team, "
                           "cannot swap places.\n\"");
    return;
  }
  //
  // copy the client information
  cl = *botent->client;
  //
  G_DPrintf("Swapping places: %s in for %s\n", ent->client->pers.netname,
            botent->client->pers.netname);
  // kill the bot
  botent->flags &= ~FL_GODMODE;
  botent->client->ps.stats[STAT_HEALTH] = botent->health = 0;
  player_die(botent, ent, ent, 100000, MOD_SWAP_PLACES);
  // make sure they go into limbo mode right away, and dont show a
  // corpse
  limbo(botent, qfalse);
  // respawn the player
  ent->client->ps.pm_flags &= ~PMF_LIMBO; // JPW NERVE turns off limbo
  // copy the location
  VectorCopy(cl.ps.origin, ent->s.origin);
  VectorCopy(cl.ps.viewangles, ent->s.angles);
  // copy session data, so we spawn in as the same class
  // save items
  saved = client->pers;
  sess = client->sess;
  memcpy(persistant, ent->client->ps.persistant, sizeof(persistant));
  // give them the right weapons/etc
  *client = cl;
  client->sess = sess;
  client->sess.playerType = ent->client->sess.latchPlayerType =
      cl.sess.playerType;
  client->sess.playerWeapon = ent->client->sess.latchPlayerWeapon =
      cl.sess.playerWeapon;
  client->sess.playerWeapon2 = ent->client->sess.latchPlayerWeapon2 =
      cl.sess.playerWeapon2;
  // spawn them in
  ClientSpawn(ent, qtrue);
  // restore items
  client->pers = saved;
  memcpy(ent->client->ps.persistant, persistant, sizeof(persistant));
  client->ps = cl.ps;
  client->ps.clientNum = ent->s.number;
  ent->health = client->ps.stats[STAT_HEALTH];
  SetClientViewAngle(ent, cl.ps.viewangles);
  // make sure they dont respawn immediately after they die
  client->pers.lastReinforceTime = 0;
}

typedef struct {
  const char *cmd;
  qboolean floodProtected;
  void (*function)(gentity_t *ent);
} command_t;

static const command_t anyTimeCommands[] = {
    {"score", qfalse, Cmd_Score_f},
    {"vote", qtrue, Cmd_Vote_f},
    {"fireteam", qfalse, Cmd_FireTeam_MP_f},
    {"showstats", qfalse, G_PrintAccuracyLog},
    {"ignore", qfalse, Cmd_Ignore_f},
    {"unignore", qfalse, Cmd_UnIgnore_f},
    {"obj", qfalse, Cmd_SelectedObjective_f},
    {"impkd", qfalse, Cmd_IntermissionPlayerKillsDeaths_f},
    {"imwa", qfalse, Cmd_IntermissionWeaponAccuracies_f},
    {"imws", qfalse, Cmd_IntermissionWeaponStats_f},
    {"imready", qfalse, Cmd_IntermissionReady_f},
    {"ws", qfalse, Cmd_WeaponStat_f},
    {"rs", qfalse, Cmd_ResetSetup_f},
    {"m", qtrue, Cmd_PrivateMessage_f},
    {"nogoto", qfalse, Cmd_noGoto_f},
    {"nocall", qfalse, Cmd_noCall_f},
    {"rtvVote", qfalse, Cmd_Vote_f},
};

static const command_t noIntermissionCommands[] = {
    {"class", qfalse, Cmd_Class_f},
    {"give", qfalse, Cmd_Give_f},
    {"god", qfalse, Cmd_God_f},
    {"notarget", qfalse, Cmd_Notarget_f},
    {"noclip", qfalse, Cmd_Noclip_f},
    {"kill", qtrue, Cmd_Kill_f},
    {"team", qtrue, Cmd_Team_f},
    {"where", qfalse, Cmd_Where_f},
    //{ "startcamera",		qfalse,	Cmd_StartCamera_f },
    {"stopcamera", qfalse, Cmd_StopCamera_f},
    {"setcameraorigin", qfalse, Cmd_SetCameraOrigin_f},
    {"setviewpos", qfalse, Cmd_SetViewpos_f},
    {"setspawnpt", qfalse, Cmd_SetSpawnPoint_f},

    // tj related cmds
    {"goto", qtrue, Cmd_Goto_f},
    {"call", qtrue, Cmd_Call_f},
    {"iwant", qtrue, Cmd_Call_f},
    //{ "load",				qfalse,	Cmd_Load_f },
    //{ "backup",             qfalse, Cmd_BackupLoad_f },
    //{ "save",				qfalse,	Cmd_Save_f },
    {"shrug", qfalse, Cmd_shrug_f},
    //{ "savereset",          qfalse, Cmd_SaveReset_f },
    {"timerun_status", qfalse, Cmd_timerunStatus_f},
    {"setoffset", qtrue, ETJump::setPlayerOffset},
    {"interruptRun", qtrue, ETJump::interruptRun},
    {"tracker_print", qtrue, ETJump::printTracker},
    {"tracker_set", qtrue, ETJump::setTracker},
    {"clearsaves", qtrue, ETJump::clearSaves},
};

qboolean ClientIsFlooding(gentity_t *ent) {
  if (!ent->client || !g_floodprotection.integer) {
    return qfalse;
  }

  if (level.time - ent->client->sess.thresholdTime > 30000) {
    ent->client->sess.thresholdTime = level.time;
  }

  if (level.time < ent->client->sess.nextReliableTime) {
    return qtrue;
  }

  if (level.time - ent->client->sess.thresholdTime <= 30000 &&
      ent->client->sess.numReliableCmds > g_floodlimit.integer) {
    ent->client->sess.nextReliableTime = level.time + g_floodwait.integer;
    return qtrue;
  }

  ent->client->sess.numReliableCmds++;
  // delay between each command (values >0 break batch of commands)
  ent->client->sess.nextReliableTime = level.time + 0;

  return qfalse;
}

void ClientCommand(int clientNum) {
  gentity_t *ent;
  char cmd[MAX_TOKEN_CHARS];
  int i;
  qboolean enc = qfalse; // QP-encoded chat commands
  fireteamData_t *ft;

  ent = g_entities + clientNum;

  if (!ent->client) {
    return; // not fully in game yet
  }
  trap_Argv(0, cmd, sizeof(cmd));

  if (OnClientCommand(ent)) {
    return;
  }

  if (ent->client->pers.connected != CON_CONNECTED) {
    return;
  }

  // Let's handle rest of the commands after checking if we're really
  // connected.

  // handle say/vsay commands
  enc = !Q_stricmp(cmd, "enc_say") ? qtrue : qfalse;
  if (!Q_stricmp(cmd, "say") || enc) {
    if (ClientIsFlooding(ent)) {
      CP(va("print \"^1Spam Protection:^7 command %s^7 "
            "ignored\n\"",
            cmd));
    } else if (!ent->client->sess.muted) {
      Cmd_Say_f(ent, SAY_ALL, qfalse, enc);
    }
    return;
  }

  enc = !Q_stricmp(cmd, "enc_say_team") ? qtrue : qfalse;
  if (!Q_stricmp(cmd, "say_team") || enc) {
    if (ClientIsFlooding(ent)) {
      CP(va("print \"^1Spam Protection:^7 command %s^7 "
            "ignored\n\"",
            cmd));
    } else if (!ent->client->sess.muted) {
      Cmd_Say_f(ent, SAY_TEAM, qfalse, enc);
    }
    return;
  }

  if (!Q_stricmp(cmd, "vsay")) {
    if (ClientIsFlooding(ent)) {
      CP(va("print \"^1Spam Protection:^7 command %s^7 "
            "ignored\n\"",
            cmd));
    } else if (!ent->client->sess.muted) {
      Cmd_Voice_f(ent, SAY_ALL, qfalse, qfalse);
    }
    return;
  }

  if (!Q_stricmp(cmd, "vsay_team")) {
    if (ClientIsFlooding(ent)) {
      CP(va("print \"^1Spam Protection:^7 command %s^7 "
            "ignored\n\"",
            cmd));
    } else if (!ent->client->sess.muted) {
      Cmd_Voice_f(ent, SAY_TEAM, qfalse, qfalse);
    }
    return;
  }

  enc = !Q_stricmp(cmd, "enc_say_buddy") ? qtrue : qfalse;
  if (!Q_stricmp(cmd, "say_buddy") || enc) {
    if (!G_IsOnFireteam(clientNum, &ft)) {
      return;
    }

    if (ClientIsFlooding(ent)) {
      CP(va("print \"^1Spam Protection:^7 command %s^7 "
            "ignored\n\"",
            cmd));
    } else if (!ent->client->sess.muted) {
      Cmd_Say_f(ent, SAY_BUDDY, qfalse, enc);
    }
    return;
  }

  if (!Q_stricmp(cmd, "vsay_buddy")) {
    if (!G_IsOnFireteam(clientNum, &ft)) {
      return;
    }

    if (ClientIsFlooding(ent)) {
      CP(va("print \"^1Spam Protection:^7 command %s^7 "
            "ignored\n\"",
            cmd));
    } else if (!ent->client->sess.muted) {
      Cmd_Voice_f(ent, SAY_BUDDY, qfalse, qfalse);
    }
    return;
  }

  enc = !Q_stricmp(cmd, "enc_say_admin") ? qtrue : qfalse;
  if (!Q_stricmp(cmd, "say_admin") || !Q_stricmp(cmd, "ma") || enc) {
    if (!g_adminChat.integer) {
      Printer::chat(clientNum, "Adminchat is disabled on this server.");
    } else if (!ETJump::session->HasPermission(ent, CommandFlags::ADMINCHAT)) {
      Printer::chat(
          clientNum,
          "You don't have permission to use adminchat on this server.");
    } else if (ClientIsFlooding(ent)) {
      Printer::console(clientNum,
                       ETJump::stringFormat(
                           "^1Spam Protection: ^7command %s ^7ignored\n", cmd));
    } else if (!ent->client->sess.muted) {
      Cmd_Say_f(ent, SAY_ADMIN, qfalse, enc);
    }
    return;
  }

  // forcetapout
  if (!Q_stricmp(cmd, "forcetapout")) {
    if (ent->client->ps.stats[STAT_HEALTH] <= 0 &&
        (ent->client->sess.sessionTeam == TEAM_AXIS ||
         ent->client->sess.sessionTeam == TEAM_ALLIES)) {
      limbo(ent, qtrue);
    }
    return;
  }

  // OSP - do these outside as we don't want to advertise it in the help
  // screen
  if (!Q_stricmp(cmd, "wstats")) {
    G_statsPrint(ent, 1);
    return;
  }
  if (!Q_stricmp(cmd, "sgstats")) // Player game stats
  {
    G_statsPrint(ent, 2);
    return;
  }
  if (!Q_stricmp(cmd, "stshots")) // "Topshots" accuracy rankings
  {
    G_weaponStatsLeaders_cmd(ent, qtrue, qtrue);
    return;
  }

  // regular anytime commands
  for (i = 0; i < static_cast<int>(sizeof(anyTimeCommands) /
                                   sizeof(anyTimeCommands[0]));
       i++) {
    if (!Q_stricmp(cmd, anyTimeCommands[i].cmd)) {
      if (anyTimeCommands[i].floodProtected && ClientIsFlooding(ent)) {
        CP(va("print \"^1Spam Protection:^7 "
              "command %s^7 ignored\n\"",
              cmd));
      } else {
        anyTimeCommands[i].function(ent);
      }
      return;
    }
  }

  // ignore all other commands when at intermission
  if (level.intermissiontime) {
    CPx(clientNum, va("print \"^3%s^7 not allowed during "
                      "intermission.\n\"",
                      cmd));
    return;
  }

  // follow
  if (!Q_stricmp(cmd, "follownext")) {
    Cmd_FollowCycle_f(ent, 1);
    return;
  }
  if (!Q_stricmp(cmd, "followprev")) {
    Cmd_FollowCycle_f(ent, -1);
    return;
  }

  if (!Q_stricmp(cmd, "mod_information")) {
    Printer::console(ent,
                     va("%s %s %s", GAME_NAME, GAME_VERSION_DATED, __TIME__));
    return;
  }

  if (OnConnectedClientCommand(ent)) {
    return;
  }

  // regular no intermission commands
  for (i = 0; i < static_cast<int>(sizeof(noIntermissionCommands) /
                                   sizeof(noIntermissionCommands[0]));
       i++) {
    if (!Q_stricmp(cmd, noIntermissionCommands[i].cmd)) {
      if (noIntermissionCommands[i].floodProtected && ClientIsFlooding(ent)) {
        CP(va("print \"^1Spam Protection:^7 "
              "command %s^7 ignored\n\"",
              cmd));
      } else {
        noIntermissionCommands[i].function(ent);
      }
      return;
    }
  }

  if (G_commandCheck(ent, cmd, qtrue)) {
    return;
  }

  // if we execute a `vid_restart`, UI will send these while cgame isn't loaded,
  // and they end up here as unrecognized console commands
  if (!Q_stricmp(cmd, "forceMaplistRefresh") ||
      !Q_stricmp(cmd, "forceCustomvoteRefresh")) {
    return;
  }

  CP(va("print \"Unknown command %s^7.\n\"", cmd));
}

// A replacement for trap_Argc() that can correctly handle
//   say "!cmd arg0 arg1"
// as well as
//   say !cmd arg0 arg1
// The client uses the latter for messages typed in the console
// and the former when the message is typed in the chat popup
int Q_SayArgc() {
  int c = 1;
  char *s;

  s = ConcatArgs(0);
  if (!*s) {
    return 0;
  }
  while (*s) {
    if (*s == ' ') {
      s++;
      if (*s != ' ') {
        c++;
        continue;
      }
      while (*s && *s == ' ') s++;
      c++;
    }
    s++;
  }
  return c;
}

// A replacement for trap_Argv() that can correctly handle
//   say "!cmd arg0 arg1"
// as well as
//   say !cmd arg0 arg1
// The client uses the latter for messages typed in the console
// and the former when the message is typed in the chat popup
qboolean Q_SayArgv(int n, char *buffer, int bufferLength) {
  int bc = 1;
  int c = 0;
  char *s;

  if (bufferLength < 1) {
    return qfalse;
  }
  if (n < 0) {
    return qfalse;
  }
  *buffer = '\0';
  s = ConcatArgs(0);
  while (*s) {
    if (c == n) {
      while (*s && (bc < bufferLength)) {
        if (*s == ' ') {
          *buffer = '\0';
          return qtrue;
        }
        *buffer = *s;
        buffer++;
        s++;
        bc++;
      }
      *buffer = '\0';
      return qtrue;
    }
    if (*s == ' ') {
      s++;
      if (*s != ' ') {
        c++;
        continue;
      }
      while (*s && *s == ' ') s++;
      c++;
    }
    s++;
  }
  return qfalse;
}

char *Q_SayConcatArgs(int start) {
  char *s;
  int c = 0;

  s = ConcatArgs(0);
  while (*s) {
    if (c == start) {
      return s;
    }
    if (*s == ' ') {
      s++;
      if (*s != ' ') {
        c++;
        continue;
      }
      while (*s && *s == ' ') s++;
      c++;
    }
    s++;
  }
  return s;
}

void DecolorString(char *in, char *out) {
  while (*in) {
    if (*in == 27 || *in == '^') {
      in++; // skip color code
      if (*in) {
        in++;
      }
      continue;
    }
    *out++ = *in++;
  }
  *out = 0;
}
