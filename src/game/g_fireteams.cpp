#include "g_local.h"
#include "etj_printer.h"
#include "etj_string_utilities.h"
#include "etj_entity_utilities.h"
#include "etj_local.h"
#include "etj_fireteam_countdown.h"
#include "etj_portalgun_shared.h"

// Gordon
// What we need....
// invite <clientname|number>
// apply <fireteamname|number>
// create <name>
// leave

// player can only be on single fire team
// only leader can invite
// leaving a team causes the first person to join the team after the leader to
// become leader 32 char limit on fire team names, mebe reduce to 16..

// Application commad overview
//
// clientNum < 0 = special, otherwise is client that the command refers to
// -1 = Application sent
// -2 = Application Failed
// -3 = Application Approved
// -4 = Response sent

// Invitation commad overview
//
// clientNum < 0 = special, otherwise is client that the command refers to
// -1 = Invitation sent
// -2 = Invitation Rejected
// -3 = Invitation Accepted
// -4 = Response sent

// Proposition commad overview
//
// clientNum < 0 = special, otherwise is client that the command refers to
// -1 = Proposition sent
// -2 = Proposition Rejected
// -3 = Proposition Accepted
// -4 = Response sent

// Auto fireteam priv/pub
//
// -1 = ask
// -2 = confirmed
//

// Configstring for each fireteam "\\n\\%NAME%\\l\\%LEADERNUM%\\c\\%CLIENTS%"
// clients "compressed" using hex

#define G_ClientPrintAndReturn(entityNum, text)                                \
  trap_SendServerCommand(entityNum, "cpm \"" text "\"\n");                     \
  return

// Utility functions
fireteamData_t *G_FindFreeFireteam() {
  int i;

  for (i = 0; i < MAX_FIRETEAMS; i++) {
    if (!level.fireTeams[i].inuse) {
      return &level.fireTeams[i];
    }
  }

  return NULL;
}

team_t G_GetFireteamTeam(fireteamData_t *ft) {
  if (!ft->inuse) {
    return static_cast<team_t>(-1);
  }

  if (ft->joinOrder[0] == -1 || !g_entities[(int)ft->joinOrder[0]].client) {
    G_Error("G_GetFireteamTeam: Fireteam leader is invalid\n");
  }

  return g_entities[(int)ft->joinOrder[0]].client->sess.sessionTeam;
}
// Confusing name, actually counts all fireteams
int G_CountTeamFireteams(team_t team) {
  int i, cnt = 0;

  for (i = 0; i < MAX_FIRETEAMS; i++) {
    // Zero: let's count all fireteams, not just team fireteams
    if (G_GetFireteamTeam(&level.fireTeams[i]) != -1) {
      cnt++;
    }
  }

  return cnt;
}

void G_UpdateFireteamConfigString(fireteamData_t *ft) {
  char buffer[128];
  int i;
  int clnts[2] = {0, 0};

  if (!ft->inuse) {
    Com_sprintf(buffer, 128, "\\id\\-1");
  } else {
    for (i = 0; i < MAX_CLIENTS; i++) {
      if (ft->joinOrder[i] != -1) {
        COM_BitSet(clnts, ft->joinOrder[i]);
      }
    }

    Com_sprintf(buffer, 128, R"(\id\%i\l\%i\sl\%i\ng\%i\tj\%i\c\%.8x%.8x)",
                ft->ident - 1, ft->joinOrder[0], ft->saveLimit, ft->noGhost,
                ft->teamJumpMode, clnts[1], clnts[0]);
    // G_Printf(va("%s\n", buffer));
  }

  trap_SetConfigstring(CS_FIRETEAMS + (ft - level.fireTeams), buffer);
}

qboolean G_IsOnFireteam(int entityNum, fireteamData_t **teamNum) {
  int i, j;

  if ((entityNum < 0 || entityNum >= MAX_CLIENTS) ||
      !g_entities[entityNum].client) {
    G_Error("G_IsOnFireteam: invalid client");
  }

  for (i = 0; i < MAX_FIRETEAMS; i++) {
    if (!level.fireTeams[i].inuse) {
      continue;
    }

    for (j = 0; j < MAX_CLIENTS; j++) {
      if (level.fireTeams[i].joinOrder[j] == -1) {
        break;
      }

      if (level.fireTeams[i].joinOrder[j] == entityNum) {
        if (teamNum) {
          *teamNum = &level.fireTeams[i];
        }
        return qtrue;
      }
    }
  }

  if (teamNum) {
    *teamNum = NULL;
  }
  return qfalse;
}

qboolean G_IsFireteamLeader(int entityNum, fireteamData_t **teamNum) {
  int i;

  if ((entityNum < 0 || entityNum >= MAX_CLIENTS) ||
      !g_entities[entityNum].client) {
    G_Error("G_IsFireteamLeader: invalid client");
  }

  for (i = 0; i < MAX_FIRETEAMS; i++) {
    if (!level.fireTeams[i].inuse) {
      continue;
    }

    if (level.fireTeams[i].joinOrder[0] == entityNum) {
      if (teamNum) {
        *teamNum = &level.fireTeams[i];
      }
      return qtrue;
    }
  }

  if (teamNum) {
    *teamNum = nullptr;
  }
  return qfalse;
}

int G_FindFreeFireteamIdent(team_t team) {
  bool freeIdent[MAX_FIRETEAMS];
  int i;

  // this was memset, which is wrong since it works on bytes
  // we need to set all elements to true initially instead
  std::fill_n(freeIdent, MAX_FIRETEAMS, true);

  for (i = 0; i < MAX_FIRETEAMS; i++) {
    if (!level.fireTeams[i].inuse) {
      continue;
    }
    // Set every team that is inuse not free
    freeIdent[level.fireTeams[i].ident - 1] = false;
  }

  for (i = 0; i < (MAX_FIRETEAMS); i++) {
    if (freeIdent[i]) {
      return i;
    }
  }

  // Gordon: this should never happen
  return -1;
}

// Should be the only function that ever creates a fireteam
void G_RegisterFireteam(int entityNum) {
  fireteamData_t *ft;
  gentity_t *leader;
  int count, ident;

  if (entityNum < 0 || entityNum >= MAX_CLIENTS) {
    G_Error("G_RegisterFireteam: invalid client");
  }

  leader = &g_entities[entityNum];
  if (!leader->client) {
    G_Error("G_RegisterFireteam: attempting to register a "
            "Fireteam to an "
            "entity with no client\n");
  }

  if (G_IsOnFireteam(entityNum, nullptr)) {
    G_ClientPrintAndReturn(entityNum,
                           "You are already on a fireteam, leave it first");
  }

  if ((ft = G_FindFreeFireteam()) == nullptr) {
    G_ClientPrintAndReturn(entityNum, "No free fireteams available");
  }

  count = G_CountTeamFireteams(leader->client->sess.sessionTeam);
  if (count >= MAX_FIRETEAMS) {
    G_ClientPrintAndReturn(entityNum,
                           "There is a maximum number of fireteams in use.");
  }

  ident = G_FindFreeFireteamIdent(leader->client->sess.sessionTeam) + 1;
  if (ident == 0) {
    G_ClientPrintAndReturn(entityNum, "Um, something is broken, spoink Gordon");
  }

  // good to go now, i hope!
  ft->inuse = qtrue;
  memset(ft->joinOrder, -1, sizeof(level.fireTeams[0].joinOrder));
  ft->joinOrder[0] = static_cast<char>(leader - g_entities);
  ft->ident = ident;
  ft->saveLimit = FT_SAVELIMIT_NOT_SET;
  ft->teamJumpMode = qfalse;
  ft->noGhost = false;

  if (g_autoFireteams.integer) {
    ft->priv = qfalse;

    trap_SendServerCommand(entityNum, "aft -1");
    leader->client->pers.autofireteamEndTime = level.time + 20500;
  } else {
    ft->priv = qfalse;
  }

  G_UpdateFireteamConfigString(ft);
}

// only way a client should ever join a fireteam, other than creating one
void G_AddClientToFireteam(int entityNum, int leaderNum) {
  fireteamData_t *ft;
  int i;

  if ((entityNum < 0 || entityNum >= MAX_CLIENTS) ||
      !g_entities[entityNum].client) {
    G_Error("G_AddClientToFireteam: invalid client");
  }

  if ((leaderNum < 0 || leaderNum >= MAX_CLIENTS) ||
      !g_entities[leaderNum].client) {
    G_Error("G_AddClientToFireteam: invalid client");
  }

  if (!G_IsFireteamLeader(leaderNum, &ft)) {
    G_ClientPrintAndReturn(
        entityNum, "The leader has now left the Fireteam you applied to");
  }

  if (G_IsOnFireteam(entityNum, NULL)) {
    G_ClientPrintAndReturn(entityNum, "You are already on a fireteam");
  }

  for (i = 0; i < MAX_CLIENTS; i++) {
    // Zero: changed to 20 instead of 6
    if (i >= MAX_FIRETEAM_USERS) {
      G_ClientPrintAndReturn(entityNum,
                             "Too many players already on this Fireteam");
      return;
    }

    if (ft->joinOrder[i] == -1) {
      gentity_t *otherEnt = g_entities + entityNum;
      // found a free position
      ft->joinOrder[i] = static_cast<char>(entityNum);

      otherEnt->client->sess.saveLimitFt = ft->saveLimit;

      if (ft->noGhost) {
        if (otherEnt->client->pers.hideMe) {
          otherEnt->client->pers.hideMe = false;
          Printer::popup(
              otherEnt,
              "Fireteam ^3noghost ^7is enabled, disabling ^3etj_hideMe\n");
        }

        otherEnt->client->ftNoGhostThisLife = true;

        if (otherEnt->client->sess.timerunActive &&
            !(otherEnt->client->sess.runSpawnflags &
              static_cast<int>(ETJump::TimerunSpawnflags::AllowFTNoGhost))) {
          Printer::popup(otherEnt, "Fireteam ^3noghost ^7is not allowed in "
                                   "this timerun, interrupting!\n");
          InterruptRun(otherEnt);
        }
      }

      if (level.portalTeam == ETJump::PORTAL_TEAM_FT &&
          ETJump::EntityUtilities::clearPortals(otherEnt)) {
        Printer::center(otherEnt, "Your portal gun portals have been reset.");
        Printer::console(otherEnt,
                         "Your portals have been reset due to ^3'portalteam' "
                         "^7setting of the current map.\n");
      }

      G_UpdateFireteamConfigString(ft);

      return;
    }
  }
}

// The only way a client should be removed from a fireteam
void G_RemoveClientFromFireteams(int entityNum, qboolean update,
                                 qboolean print) {
  fireteamData_t *ft;
  int i, j;

  if ((entityNum < 0 || entityNum >= MAX_CLIENTS) ||
      !g_entities[entityNum].client) {
    G_Error("G_RemoveClientFromFireteams: invalid client");
  }

  if (G_IsOnFireteam(entityNum, &ft)) {
    for (i = 0; i < MAX_CLIENTS; i++) {
      if (ft->joinOrder[i] == entityNum) {
        if (i == 0) {
          if (ft->joinOrder[1] == -1) {
            ft->inuse = qfalse;
            ft->ident = -1;
          } else {
            // TODO: Inform
            // client of
            // promotion to
            // leader
          }
        }
        for (j = i; j < MAX_CLIENTS - 1; j++) {
          ft->joinOrder[j] = ft->joinOrder[j + 1];
        }
        ft->joinOrder[MAX_CLIENTS - 1] = -1;

        // invalidate "Make fireteam private?"
        // prompt response in case we joined a
        // fireteam and left without responding
        g_entities[entityNum].client->pers.autofireteamEndTime = level.time;

        break;
      }
    }

    game.fireteamCountdown->removeCountdown(entityNum);
  } else {
    return;
  }

  if (ft->joinOrder[0] != -1) {
    if (g_entities[(int)ft->joinOrder[0]].r.svFlags & SVF_BOT) {
      G_RemoveClientFromFireteams(ft->joinOrder[0], qfalse, qfalse);
    }
  }

  if (print) {
    for (i = 0; i < MAX_CLIENTS; i++) {
      if (ft->joinOrder[i] == -1) {
        break;
      }

      trap_SendServerCommand(ft->joinOrder[i],
                             va("cpm \"%s ^7has left the Fireteam\"\n",
                                level.clients[entityNum].pers.netname));
    }
  }

  // if the leader leaves it seems the savelimit will be set to
  // 2^32-1 / -2^32-1 so this should fix it.
  if (ft->saveLimit != FT_SAVELIMIT_NOT_SET) {
    gentity_t *ent;
    for (i = 0; i < level.numConnectedClients; i++) {
      if (ft->joinOrder[i] == -1) {
        continue;
      } else {
        ent = g_entities + ft->joinOrder[i];
        ent->client->sess.saveLimitFt = ft->saveLimit;
      }
    }
  }

  if (update) {
    G_UpdateFireteamConfigString(ft);
  }
}

// The only way a client should ever be invitied to join a team
void G_InviteToFireTeam(int entityNum, int otherEntityNum) {
  fireteamData_t *ft;

  if ((entityNum < 0 || entityNum >= MAX_CLIENTS) ||
      !g_entities[entityNum].client) {
    G_Error("G_InviteToFireTeam: invalid client");
  }

  if ((otherEntityNum < 0 || otherEntityNum >= MAX_CLIENTS) ||
      !g_entities[otherEntityNum].client) {
    G_Error("G_InviteToFireTeam: invalid client");
  }

  if (!G_IsFireteamLeader(entityNum, &ft)) {
    G_ClientPrintAndReturn(entityNum, "You are not the leader of a fireteam");
  }

  if (G_IsOnFireteam(otherEntityNum, NULL)) {
    G_ClientPrintAndReturn(entityNum,
                           "The other player is already on a fireteam");
  }

  if (g_entities[otherEntityNum].r.svFlags & SVF_BOT) {
    // Gordon: bots auto join
    G_AddClientToFireteam(otherEntityNum, entityNum);
  } else {
    trap_SendServerCommand(entityNum, va("invitation -1"));
    trap_SendServerCommand(otherEntityNum, va("invitation %i", entityNum));
    g_entities[otherEntityNum].client->pers.invitationClient = entityNum;
    g_entities[otherEntityNum].client->pers.invitationEndTime =
        level.time + 20500;
  }
}

void G_DestroyFireteam(int entityNum) {
  fireteamData_t *ft;

  if ((entityNum < 0 || entityNum >= MAX_CLIENTS) ||
      !g_entities[entityNum].client) {
    G_Error("G_DestroyFireteam: invalid client");
  }

  if (!G_IsFireteamLeader(entityNum, &ft)) {
    G_ClientPrintAndReturn(entityNum, "You are not the leader of a fireteam");
  }

  while (ft->joinOrder[0] != -1) {
    if (ft->joinOrder[0] != entityNum) {
      trap_SendServerCommand(ft->joinOrder[0],
                             "cpm \"The Fireteam you are on has been "
                             "disbanded\"\n");
    }

    G_RemoveClientFromFireteams(ft->joinOrder[0], qfalse, qfalse);
  }

  G_UpdateFireteamConfigString(ft);
}

void G_WarnFireTeamPlayer(int entityNum, int otherEntityNum) {
  fireteamData_t *ft, *ft2;

  if (entityNum == otherEntityNum) {
    return; // ok, stop being silly :p
  }

  if ((entityNum < 0 || entityNum >= MAX_CLIENTS) ||
      !g_entities[entityNum].client) {
    G_Error("G_WarnFireTeamPlayer: invalid client");
  }

  if ((otherEntityNum < 0 || otherEntityNum >= MAX_CLIENTS) ||
      !g_entities[otherEntityNum].client) {
    G_Error("G_WarnFireTeamPlayer: invalid client");
  }

  if (!G_IsFireteamLeader(entityNum, &ft)) {
    G_ClientPrintAndReturn(entityNum, "You are not the leader of a fireteam");
  }

  if ((!G_IsOnFireteam(otherEntityNum, &ft2)) || ft != ft2) {
    G_ClientPrintAndReturn(
        entityNum, "You are not on the same Fireteam as the other player");
  }

  trap_SendServerCommand(
      otherEntityNum,
      "cpm \"You have been warned by your Fireteam Commander\n\"");
}

void G_KickFireTeamPlayer(int entityNum, int otherEntityNum) {
  fireteamData_t *ft, *ft2;
  ;

  if (entityNum == otherEntityNum) {
    return; // ok, stop being silly :p
  }

  if ((entityNum < 0 || entityNum >= MAX_CLIENTS) ||
      !g_entities[entityNum].client) {
    G_Error("G_KickFireTeamPlayer: invalid client");
  }

  if ((otherEntityNum < 0 || otherEntityNum >= MAX_CLIENTS) ||
      !g_entities[otherEntityNum].client) {
    G_Error("G_KickFireTeamPlayer: invalid client");
  }

  if (!G_IsFireteamLeader(entityNum, &ft)) {
    G_ClientPrintAndReturn(entityNum, "You are not the leader of a fireteam");
  }

  if ((!G_IsOnFireteam(otherEntityNum, &ft2)) || ft != ft2) {
    G_ClientPrintAndReturn(
        entityNum, "You are not on the same Fireteam as the other player");
  }

  G_RemoveClientFromFireteams(otherEntityNum, qtrue, qfalse);

  G_ClientPrintAndReturn(otherEntityNum,
                         "You have been kicked from the fireteam");
}

// The only way a client should ever apply to join a team
void G_ApplyToFireTeam(int entityNum, int fireteamNum) {
  gentity_t *leader;
  fireteamData_t *ft;

  if ((entityNum < 0 || entityNum >= MAX_CLIENTS) ||
      !g_entities[entityNum].client) {
    G_Error("G_AddClientToFireteam: invalid client");
  }

  if (G_IsOnFireteam(entityNum, NULL)) {
    G_ClientPrintAndReturn(entityNum, "You are already on a fireteam");
  }

  ft = &level.fireTeams[fireteamNum];
  if (!ft->inuse) {
    G_ClientPrintAndReturn(entityNum,
                           "The Fireteam you requested does not exist");
  }

  if (ft->joinOrder[0] < 0 || ft->joinOrder[0] >= MAX_CLIENTS) {
    G_Error("G_ApplyToFireTeam: Fireteam leader is invalid\n");
  }

  leader = &g_entities[(int)ft->joinOrder[0]];
  if (!leader->client) {
    G_Error("G_ApplyToFireTeam: Fireteam leader client is NULL\n");
  }

  // TEMP
  //	G_AddClientToFireteam( entityNum, ft->joinOrder[0] );

  trap_SendServerCommand(entityNum, va("application -1"));
  trap_SendServerCommand(leader - g_entities, va("application %i", entityNum));
  leader->client->pers.applicationClient = entityNum;
  leader->client->pers.applicationEndTime = level.time + 20000;
}

void G_ProposeFireTeamPlayer(int entityNum, int otherEntityNum) {
  fireteamData_t *ft;
  gentity_t *leader;

  if (entityNum == otherEntityNum) {
    return; // ok, stop being silly :p
  }

  if ((entityNum < 0 || entityNum >= MAX_CLIENTS) ||
      !g_entities[entityNum].client) {
    G_Error("G_ProposeFireTeamPlayer: invalid client");
  }

  if ((otherEntityNum < 0 || otherEntityNum >= MAX_CLIENTS) ||
      !g_entities[otherEntityNum].client) {
    G_Error("G_ProposeFireTeamPlayer: invalid client");
  }

  if (G_IsOnFireteam(otherEntityNum, NULL)) {
    G_ClientPrintAndReturn(entityNum,
                           "The other player is already on a fireteam");
  }

  if (!G_IsOnFireteam(entityNum, &ft)) {
    G_ClientPrintAndReturn(entityNum, "You are not on a fireteam");
  }

  if (ft->joinOrder[0] == entityNum) {
    // you are the leader so just invite them
    G_InviteToFireTeam(entityNum, otherEntityNum);
    return;
  }

  leader = &g_entities[(int)ft->joinOrder[0]];
  if (!leader->client) {
    G_Error("G_ProposeFireTeamPlayer: invalid client");
  }

  trap_SendServerCommand(entityNum, va("proposition -1"));
  trap_SendServerCommand(leader - g_entities,
                         va("proposition %i %i", otherEntityNum, entityNum));
  leader->client->pers.propositionClient = otherEntityNum;
  leader->client->pers.propositionClient2 = entityNum;
  leader->client->pers.propositionEndTime = level.time + 20000;
}

int G_FireteamNumberForString(const char *name, team_t team) {
  int i, fireteam = 0;

  for (i = 0; i < MAX_FIRETEAMS; i++) {
    if (!level.fireTeams[i].inuse) {
      continue;
    }

    if (!Q_stricmp(bg_fireteamNames[level.fireTeams[i].ident - 1], name)) {
      fireteam = i + 1;
    }

    /*		if(!Q_stricmp(level.fireTeams[i].name, name)) {
                fireteam = i+1;
            }*/
  }

  if (fireteam <= 0) {
    fireteam = Q_atoi(name);
  }

  return fireteam;
}

fireteamData_t *G_FindFreePublicFireteam(team_t team) {
  int i, j;

  for (i = 0; i < MAX_FIRETEAMS; i++) {
    if (!level.fireTeams[i].inuse) {
      continue;
    }

    if (level.fireTeams[i].priv) {
      continue;
    }

    for (j = 0; j < MAX_CLIENTS; j++) {
      if (j >= MAX_FIRETEAM_USERS || level.fireTeams[i].joinOrder[j] == -1) {
        break;
      }
    }
    if (j >= MAX_FIRETEAM_USERS) {
      continue;
    }

    return &level.fireTeams[i];
  }

  return NULL;
}

void G_FireteamRace(int clientNum) {
  fireteamData_t *ft;
  if (!G_IsOnFireteam(clientNum, &ft)) {
    G_ClientPrintAndReturn(clientNum, "You are not on a fireteam");
  }

  if (!G_IsFireteamLeader(clientNum, &ft)) {
    G_ClientPrintAndReturn(clientNum, "You are not the leader.");
  }

  if (trap_Argc() < 3) {
    G_ClientPrintAndReturn(clientNum, "usage: fireteam race start");
  }

  G_ClientPrintAndReturn(clientNum, "Fireteam races are not supported yet.");

  //	trap_Argv(2, arg, sizeof(arg));
  //	if (!Q_stricmp(arg, "start"))
  //	{
  //		StartRace(g_entities + clientNum);
  //	}
}

namespace ETJump {
// if the client is not in a fireteam, returns nullptr
gentity_t *getFireteamLeader(const int clientNum) {
  if (!EntityUtilities::isPlayer(&g_entities[clientNum])) {
    G_Error("%s: invalid client", __func__);
  }

  for (auto &ft : level.fireTeams) {
    if (!ft.inuse) {
      continue;
    }

    // this is a char array....
    for (const char &c : ft.joinOrder) {
      if (c == -1) {
        break;
      }

      // we're in this fireteam, get the leader
      if (c == clientNum) {
        return g_entities + static_cast<unsigned char>(ft.joinOrder[0]);
      }
    }
  }

  return nullptr;
}

void setSaveLimitForFTMembers(fireteamData_t *ft, const int limit) {
  ft->saveLimit = limit;

  for (int i = 0; i < level.numConnectedClients; i++) {
    if (ft->joinOrder[i] == -1) {
      continue;
    }

    gentity_t *ent = g_entities + ft->joinOrder[i];
    ent->client->sess.saveLimitFt = limit;
    Printer::popup(
        ent, stringFormat("fireteam: ^3savelimit ^7was set to ^3%i", limit));
  }
}

void setFireTeamGhosting(fireteamData_t *ft, const bool noGhost) {
  const std::string &msg = stringFormat("fireteam: ^3noghost ^7has been ^3%s",
                                        noGhost ? "enabled" : "disabled");

  ft->noGhost = noGhost;

  for (int i = 0; i < level.numConnectedClients; i++) {
    if (ft->joinOrder[i] == -1) {
      continue;
    }

    gentity_t *ent = g_entities + ft->joinOrder[i];

    if (noGhost) {
      ent->client->ftNoGhostThisLife = true;

      if (ent->client->pers.hideMe) {
        ent->client->pers.hideMe = false;
        Printer::popup(ent,
                       "fireteam: disabling ^3etj_hideMe ^7due to ^3noghost\n");
      }
    }

    Printer::popup(ent, msg);
  }
}

void setFireteamTeamjumpMode(fireteamData_t *ft, const bool teamjumpMode) {
  const std::string &msg =
      stringFormat("fireteam: ^3teamjump mode ^7has been ^3%s",
                   teamjumpMode ? "enabled" : "disabled");

  ft->teamJumpMode = teamjumpMode;

  for (int i = 0; i < level.numConnectedClients; i++) {
    if (ft->joinOrder[i] == -1) {
      continue;
    }

    Printer::popup(ft->joinOrder[i], msg);
  }
}

static bool fireTeamMemberIsTimerunning(fireteamData_t *ft) {
  for (int i = 0; i < level.numConnectedClients; i++) {
    if (ft->joinOrder[i] == -1) {
      continue;
    }

    const gentity_t *ent = g_entities + ft->joinOrder[i];

    if (ent->client->sess.timerunActive &&
        !(ent->client->sess.runSpawnflags &
          static_cast<int>(TimerunSpawnflags::AllowFTNoGhost))) {
      return true;
    }
  }

  return false;
}

bool canEnableFtNoGhost(const int clientNum, fireteamData_t *ft,
                        const gentity_t *ent) {
  if (g_cheats.integer) {
    return true;
  }

  // 'target_ft_setrules' bypasses worldspawn key restriction
  // so mappers have control of where in the map ghosting will be toggled
  if (level.noFTNoGhost && ent && ent->client) {
    Printer::popup(clientNum,
                   "fireteam: ^3noghost ^7cannot be set on this map");
    return false;
  }

  // ghosting cannot be enabled if someone is already timerunning unless
  // the run allows it, so we need to only check for enabling here
  if (!ft->noGhost && fireTeamMemberIsTimerunning(ft)) {
    Printer::popup(clientNum, "fireteam: a member of your fireteam is "
                              "timerunning, cannot enable ^3noghost");
    return false;
  }

  return true;
}

bool canSetFtSavelimit(const int clientNum, const gentity_t *ent) {
  if (g_cheats.integer) {
    return true;
  }

  // 'target_ft_setrules' bypasses worldspawn key restriction
  // so mappers have control of where in the map savelimit will be set
  if (level.noFTSaveLimit && ent && ent->client) {
    Printer::popup(clientNum,
                   "fireteam: ^3savelimit ^7cannot be set on this map");
    return false;
  }

  if (level.limitedSaves > 0) {
    Printer::popup(
        clientNum,
        "fireteam: ^7unable to set ^3savelimit ^7- save is limited by the map");
    return false;
  }

  return true;
}

bool canSetFtTeamjumpMode(const int clientNum, const gentity_t *ent) {
  if (g_cheats.integer) {
    return true;
  }

  // 'target_ft_setrules' bypasses worldspawn key restriction
  // so mappers have control of where in the map teamjump mode will be set
  if (level.noFTTeamjumpMode && ent && ent->client) {
    Printer::popup(clientNum,
                   "fireteam: ^3teamjump mode ^7cannot be set on this map");
    return false;
  }

  return true;
}

static bool canSetFireteamRules(const int &clientNum, fireteamData_t **ft) {
  if (!G_IsOnFireteam(clientNum, ft)) {
    Printer::popup(clientNum, "You are not in a fireteam");
    return false;
  }

  if (!G_IsFireteamLeader(clientNum, ft)) {
    Printer::popup(clientNum, "You are not the fireteam leader");
    return false;
  }

  return true;
}

static void setFireTeamRules(const int &clientNum) {
  char arg1[MAX_TOKEN_CHARS];
  char val[MAX_TOKEN_CHARS];
  fireteamData_t *ft;
  const std::string &usageStr =
      "^3usage: ^7fireteam rules <rule> <value>\n\nAvailable rules:\nsavelimit "
      "<value|reset>\nnoghost <on|off>\n";

  if (!canSetFireteamRules(clientNum, &ft)) {
    return;
  }

  if (trap_Argc() < 4) {
    Printer::console(clientNum, usageStr);
    return;
  }

  trap_Argv(2, arg1, sizeof(arg1));
  const gentity_t *ent = &g_entities[clientNum];

  if (!Q_stricmp(arg1, "savelimit")) {
    if (!canSetFtSavelimit(clientNum, ent)) {
      return;
    }

    trap_Argv(3, val, sizeof(val));
    const int limit = std::clamp(Q_atoi(val), -1, 100);

    if (!Q_stricmp(val, "reset")) {
      setSaveLimitForFTMembers(ft, ft->saveLimit);
      // we can return here as we don't need to update the limit in fireteam cs,
      // since we're re-using the existing value and just re-setting
      // the amount of saves available for each member
      // TODO: if we want to support dynamic changes in the fireteam UI for
      //  savelimit amount, we must perform an update here too
      return;
    }

    setSaveLimitForFTMembers(ft, limit);
    G_UpdateFireteamConfigString(ft);
    return;
  }

  if (!Q_stricmp(arg1, "noghost")) {
    if (g_ghostPlayers.integer != 1) {
      Printer::popup(clientNum,
                     stringFormat("fireteam: ^3noghost ^7is disabled by the %s",
                                  level.noGhost ? "map" : "server"));
      return;
    }

    if (!canEnableFtNoGhost(clientNum, ft, ent)) {
      return;
    }

    trap_Argv(3, val, sizeof(val));

    if (!Q_stricmp(val, "on") || !Q_stricmp(val, "1")) {
      if (ft->noGhost) {
        Printer::popup(clientNum, "fireteam: ^3noghost ^7is already enabled");
        return;
      }

      setFireTeamGhosting(ft, true);
    } else if (!Q_stricmp(val, "off") || !Q_stricmp(val, "0")) {
      if (!ft->noGhost) {
        Printer::popup(clientNum, "fireteam: ^3noghost ^7is already disabled");
        return;
      }

      setFireTeamGhosting(ft, false);
    } else {
      Printer::popup(clientNum, "fireteam: invalid ^3noghost ^7value");
      Printer::popup(clientNum, "Valid values are: ^3<on|1> <off|0>");
      return;
    }

    G_UpdateFireteamConfigString(ft);
    return;
  }

  Printer::popup(clientNum,
                 "Failed to set fireteam rules, see console for usage");
  Printer::console(clientNum, usageStr);
}

void setupTeamJumpMode(const int &clientNum) {
  fireteamData_t *ft;

  if (!canSetFireteamRules(clientNum, &ft)) {
    return;
  }

  if (trap_Argc() != 3) {
    Printer::popup(clientNum, "^3usage: ^7fireteam <tj|teamjump> <on|off>");
    return;
  }

  if (!canSetFtTeamjumpMode(clientNum, &g_entities[clientNum])) {
    return;
  }

  char arg[MAX_STRING_TOKENS] = "\0";
  trap_Argv(2, arg, sizeof(arg));

  if (!Q_stricmp(arg, "on") || !Q_stricmp(arg, "1")) {
    if (ft->teamJumpMode) {
      Printer::popup(clientNum,
                     "fireteam: ^3teamjump mode ^7is already enabled");
      return;
    }

    setFireteamTeamjumpMode(ft, true);
  } else if (!Q_stricmp(arg, "off") || !Q_stricmp(arg, "0")) {
    if (!ft->teamJumpMode) {
      Printer::popup(clientNum,
                     "fireteam: ^3teamjump mode ^7is already disabled");
      return;
    }

    setFireteamTeamjumpMode(ft, false);
  } else {
    Printer::popup(clientNum, "fireteam: invalid ^3teamjump ^7value");
    Printer::popup(clientNum, "Valid values are: ^3<on|1> <off|0>");
    return;
  }

  G_UpdateFireteamConfigString(ft);
}

static void startFireteamCountdown(gentity_t *ent) {
  const int argc = trap_Argc();

  // the client should always produce a countdown command with duration,
  // even if it's not manually specified
  if (argc < 3) {
    Printer::console(
        ent, "Malformed 'countdown' command - no duration specified!\n");
    return;
  }

  if (ent->client->sess.muted) {
    Printer::console(ent, "You are muted.\n");
    return;
  }

  const int clientNum = ClientNum(ent);

  // sanity check
  if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
    Printer::console(ent,
                     "Malformed 'countdown' command - invalid clientNum!\n");
    return;
  }

  fireteamData_t *ft = nullptr;

  if (!G_IsOnFireteam(clientNum, &ft)) {
    Printer::console(ent, "You are not in a fireteam.\n");
    return;
  }

  // max 10s countdown, minimum 1s
  // if the client side cvar is <= 0, the default value (3s) will be used
  char buf[8];
  trap_Argv(2, buf, sizeof(buf));
  const auto seconds = static_cast<int8_t>(std::clamp(Q_atoi(buf), 1, 10));
  game.fireteamCountdown->addCountdown(clientNum, seconds);
}
} // namespace ETJump

// Checks if given command buffer matches a valid client on the server
// Returns true if given argument matches a valid client
bool validClientForFireteam(gentity_t *ent, int *targetNum, char *numbuffer) {
  bool validClient = true;

  if ((*targetNum = ClientNumberFromString(ent, numbuffer)) == -1) {
    validClient = false;
  } else {
    gentity_t *other = g_entities + *targetNum;

    if (!other->inuse || !other->client) {
      validClient = false;
    }
  }

  return validClient;
}

// Command handler
void Cmd_FireTeam_MP_f(gentity_t *ent) {
  char command[MAX_NAME_LENGTH]; // more than enough to hold the commands
  const int selfNum = ClientNum(ent);
  int targetNum = -1;

  if (trap_Argc() < 2) {
    G_ClientPrintAndReturn(
        selfNum,
        "usage: fireteam <create|leave|apply|invite|rules|teamjump|countdown>");
  }

  trap_Argv(1, command, sizeof(command));

  if (!Q_stricmp(command, "create")) {
    G_RegisterFireteam(selfNum);
  } else if (!Q_stricmp(command, "disband")) {
    G_DestroyFireteam(selfNum);
  } else if (!Q_stricmp(command, "leave")) {
    G_RemoveClientFromFireteams(selfNum, qtrue, qtrue);
  } else if (!Q_stricmp(command, "apply")) {
    char namebuffer[MAX_NAME_LENGTH];
    int fireteam;

    if (trap_Argc() < 3) {
      G_ClientPrintAndReturn(
          selfNum, "usage: fireteam apply <fireteamname|fireteamnumber>");
    }

    trap_Argv(2, namebuffer, sizeof(namebuffer));
    fireteam =
        G_FireteamNumberForString(namebuffer, ent->client->sess.sessionTeam);

    if (fireteam <= 0) {
      G_ClientPrintAndReturn(
          selfNum, "usage: fireteam apply <fireteamname|fireteamnumber>");
    }

    G_ApplyToFireTeam(selfNum, fireteam - 1);
  } else if (!Q_stricmp(command, "invite")) {
    char numbuffer[MAX_NAME_LENGTH];

    if (trap_Argc() < 3) {
      G_ClientPrintAndReturn(
          selfNum, "usage: fireteam invite <clientname|clientnumber>");
    }
    trap_Argv(2, numbuffer, sizeof(numbuffer));

    targetNum = Q_atoi(numbuffer);
    if (!validClientForFireteam(ent, &targetNum, numbuffer)) {
      G_ClientPrintAndReturn(selfNum, "Invalid client selected");
    }

    G_InviteToFireTeam(selfNum, targetNum);
  } else if (!Q_stricmp(command, "warn")) {
    char numbuffer[MAX_NAME_LENGTH];

    if (trap_Argc() < 3) {
      G_ClientPrintAndReturn(selfNum,
                             "usage: fireteam warn <clientname|clientnumber>");
    }
    trap_Argv(2, numbuffer, sizeof(numbuffer));

    targetNum = Q_atoi(numbuffer);
    if (!validClientForFireteam(ent, &targetNum, numbuffer)) {
      G_ClientPrintAndReturn(selfNum, "Invalid client selected");
    }

    G_WarnFireTeamPlayer(selfNum, targetNum);
  } else if (!Q_stricmp(command, "kick")) {
    char numbuffer[MAX_NAME_LENGTH];

    if (trap_Argc() < 3) {
      G_ClientPrintAndReturn(selfNum,
                             "usage: fireteam kick <clientname|clientnumber>");
    }
    trap_Argv(2, numbuffer, sizeof(numbuffer));

    targetNum = Q_atoi(numbuffer);
    if (!validClientForFireteam(ent, &targetNum, numbuffer)) {
      G_ClientPrintAndReturn(selfNum, "Invalid client selected");
    }

    G_KickFireTeamPlayer(selfNum, targetNum);
  } else if (!Q_stricmp(command, "propose")) {
    char numbuffer[MAX_NAME_LENGTH];

    if (trap_Argc() < 3) {
      G_ClientPrintAndReturn(
          selfNum, "usage: fireteam propose <clientname|clientnumber>");
    }
    trap_Argv(2, numbuffer, sizeof(numbuffer));

    targetNum = Q_atoi(numbuffer);
    if (!validClientForFireteam(ent, &targetNum, numbuffer)) {
      G_ClientPrintAndReturn(selfNum, "Invalid client selected");
    }

    G_ProposeFireTeamPlayer(selfNum, targetNum);
  }
  // Challenge group.
  // Only leader
  else if (!Q_stricmp(command, "rules")) {
    ETJump::setFireTeamRules(selfNum);
  } else if (!Q_stricmp(command, "tj") || !Q_stricmp(command, "teamjump")) {
    ETJump::setupTeamJumpMode(selfNum);
  } else if (!Q_stricmp(command, "race")) {
    G_FireteamRace(selfNum);
  } else if (!Q_stricmp(command, "countdown")) {
    ETJump::startFireteamCountdown(ent);
  }
}
