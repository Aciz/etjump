/*
 * name:		g_spawn.c
 *
 * desc:
 *
 */
#include "etj_entity_utilities.h"

#include <sstream>
#include <string>
#include <unordered_map>

#include "g_local.h"
#include "etj_save_system.h"
#include "etj_string_utilities.h"
#include "etj_timerun_entities.h"
#include "etj_missilepad.h"
#include "etj_target_init.h"
#include "etj_trigger_teleport_client.h"
#include "etj_target_ft_setrules.h"
#include "etj_target_spawn_relay.h"
#include "etj_portalgun.h"
#include "etj_portalgun_shared.h"

qboolean G_SpawnStringExt(const char *key, const char *defaultString,
                          char **out, const char *file, int line) {
  int i;

  if (!level.spawning) {
    G_Error("G_SpawnString() called while not spawning, file "
            "%s, line %i",
            file, line);
  }

  for (i = 0; i < level.numSpawnVars; i++) {
    if (!strcmp(key, level.spawnVars[i][0])) {
      *out = level.spawnVars[i][1];
      return qtrue;
    }
  }

  // cast is fairly reasonable here, as nobody spawns a string just to overwrite
  // the default arg
  *out = const_cast<char *>(defaultString);
  return qfalse;
}

qboolean G_SpawnFloatExt(const char *key, const char *defaultString, float *out,
                         const char *file, int line) {
  char *s;
  qboolean present;

  present = G_SpawnStringExt(key, defaultString, &s, file, line);
  *out = Q_atof(s);
  return present;
}

qboolean G_SpawnIntExt(const char *key, const char *defaultString, int *out,
                       const char *file, int line) {
  char *s;
  qboolean present;

  present = G_SpawnStringExt(key, defaultString, &s, file, line);
  *out = Q_atoi(s);
  return present;
}

qboolean G_SpawnVectorExt(const char *key, const char *defaultString,
                          float *out, const char *file, int line) {
  char *s;
  qboolean present;

  present = G_SpawnStringExt(key, defaultString, &s, file, line);
  sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]);
  return present;
}

qboolean G_SpawnVector2DExt(const char *key, const char *defaultString,
                            float *out, const char *file, int line) {
  char *s;
  qboolean present;

  present = G_SpawnStringExt(key, defaultString, &s, file, line);
  sscanf(s, "%f %f", &out[0], &out[1]);
  return present;
}

// FIXME: this is woefully out of date, it doesn't contain *ANY* custom keys
//  we've added for ETJump, and is also lacking a lot of stock entity keys
field_t fields[] = {
    {"classname", FOFS(classname), F_LSTRING},
    {"origin", FOFS(s.origin), F_VECTOR},
    {"model", FOFS(model), F_LSTRING},
    {"model2", FOFS(model2), F_LSTRING},
    {"spawnflags", FOFS(spawnflags), F_INT},
    {"speed", FOFS(speed), F_FLOAT},
    {"closespeed", FOFS(closespeed), F_FLOAT}, //----(SA)	added
    {"target", FOFS(target), F_LSTRING},
    {"targetname", FOFS(targetname), F_LSTRING},
    {"message", FOFS(message), F_LSTRING},
    {"popup", FOFS(message),
     F_LSTRING}, // (SA) mutually exclusive from 'message', but makes the ent
                 // more logical for the level designer
    {"book", FOFS(message),
     F_LSTRING}, // (SA) mutually exclusive from 'message', but makes the ent
                 // more logical for the level designer
    {"team", FOFS(team), F_LSTRING},
    {"wait", FOFS(wait), F_FLOAT},
    {"random", FOFS(random), F_FLOAT},
    {"count", FOFS(count), F_INT},
    {"health", FOFS(health), F_INT},
    {"light", 0, F_IGNORE},
    {"dmg", FOFS(damage), F_INT},
    {"angles", FOFS(s.angles), F_VECTOR},
    {"angle", FOFS(s.angles), F_ANGLEHACK},
    // JOSEPH 9-27-99
    {"duration", FOFS(duration), F_FLOAT},
    {"rotate", FOFS(rotate), F_VECTOR},
    // END JOSEPH
    {"degrees", FOFS(angle), F_FLOAT},
    {"time", FOFS(speed), F_FLOAT},

    //----(SA) additional ai field
    {"skin", FOFS(aiSkin), F_LSTRING},

    //----(SA) done

    // (SA) dlight lightstyles (made all these unique variables for testing)
    {"_color", FOFS(dl_color),
     F_VECTOR}, // color of the light	(the underscore is inserted by the color
                // picker in QER)
    {"color", FOFS(dl_color), F_VECTOR}, // color of the light
    {"stylestring", FOFS(dl_stylestring),
     F_LSTRING}, // user defined stylestring "fffndlsfaaaaaa" for example
    // done

    //----(SA)
    {"shader", FOFS(dl_shader),
     F_LSTRING}, // shader to use for a target_effect or dlight

    // (SA) for target_unlock
    {"key", FOFS(key), F_INT},
    // done

    // Rafael - mg42
    {"harc", FOFS(harc), F_FLOAT},
    {"varc", FOFS(varc), F_FLOAT},
    // done.

    // Rafael - sniper
    {"delay", FOFS(delay), F_FLOAT},
    {"radius", FOFS(radius), F_INT},

    // Ridah, for reloading savegames at correct mission spot
    {"missionlevel", FOFS(missionLevel), F_INT},

    // Rafel
    {"start_size", FOFS(start_size), F_INT},
    {"end_size", FOFS(end_size), F_INT},

    {"shard", FOFS(count), F_INT},

    // Rafael
    {"spawnitem", FOFS(spawnitem), F_LSTRING},

    {"track", FOFS(track), F_LSTRING},

    {"scriptName", FOFS(scriptName), F_LSTRING},

    {"shortname", FOFS(message), F_LSTRING},
    {"constages", FOFS(constages), F_LSTRING},
    {"desstages", FOFS(desstages), F_LSTRING},
    {"partofstage", FOFS(partofstage), F_INT},
    {"override", FOFS(spawnitem), F_LSTRING},

    {"damageparent", FOFS(damageparent), F_LSTRING},

    {"targetShaderName", FOFS(targetShaderName), F_LSTRING},
    {"targetShaderNewName", FOFS(targetShaderNewName), F_LSTRING},

    {"cursorhint", FOFS(s.dmgFlags), F_CURSORHINT},

    {"savelimit", FOFS(ftSavelimit), F_LSTRING},
    {"noghost", FOFS(damage), F_INT},
    {"teamjumpmode", FOFS(health), F_INT},
    {"leader_only_message", FOFS(message), F_LSTRING},

    {nullptr}};

typedef struct {
  const char *name;
  void (*spawn)(gentity_t *ent);
} spawn_t;

void SP_info_player_start(gentity_t *ent);
void SP_info_player_checkpoint(gentity_t *ent);
void SP_info_player_deathmatch(gentity_t *ent);
void SP_info_player_intermission(gentity_t *ent);
void SP_info_firstplace(gentity_t *ent);
void SP_info_secondplace(gentity_t *ent);
void SP_info_thirdplace(gentity_t *ent);
void SP_info_podium(gentity_t *ent);

void SP_func_plat(gentity_t *ent);
void SP_func_static(gentity_t *ent);
void SP_func_leaky(gentity_t *ent); //----(SA)	added
void SP_func_rotating(gentity_t *ent);
void SP_func_bobbing(gentity_t *ent);
void SP_func_pendulum(gentity_t *ent);
void SP_func_button(gentity_t *ent);
void SP_func_explosive(gentity_t *ent);
void SP_func_door(gentity_t *ent);
void SP_func_train(gentity_t *ent);
void SP_func_timer(gentity_t *self);
// JOSEPH 1-26-00
void SP_func_train_rotating(gentity_t *ent);
void SP_func_secret(gentity_t *ent);
// END JOSEPH
// Rafael
void SP_func_door_rotating(gentity_t *ent);
// RF
void SP_func_constructible(gentity_t *ent);
void SP_func_brushmodel(gentity_t *ent);
void SP_misc_constructiblemarker(gentity_t *ent);
void SP_target_explosion(gentity_t *ent);
void SP_misc_landmine(gentity_t *ent);
void SP_func_portaltarget(gentity_t *ent);

void SP_trigger_always(gentity_t *ent);
void SP_trigger_multiple(gentity_t *ent);
void SP_trigger_multiple_ext(gentity_t *ent);
void SP_trigger_push(gentity_t *ent);
void SP_trigger_teleport(gentity_t *ent);
void SP_trigger_hurt(gentity_t *ent);
void SP_trigger_savereset(gentity_t *ent);

void SP_trigger_heal(gentity_t *ent); // xkan,	9/17/2002
void SP_trigger_ammo(gentity_t *ent); // xkan,	9/17/2002

// Gordon
void SP_misc_cabinet_health(gentity_t *self);
void SP_misc_cabinet_supply(gentity_t *self);

//---- (SA) Wolf triggers
void SP_trigger_concussive_dust(gentity_t *ent); // JPW NERVE
void SP_trigger_once(gentity_t *ent);
//---- done

void SP_target_remove_powerups(gentity_t *ent);
void SP_target_give(gentity_t *ent);
void SP_target_delay(gentity_t *ent);
void SP_target_speaker(gentity_t *ent);
void SP_target_print(gentity_t *ent);
void SP_target_laser(gentity_t *self);
void SP_target_character(gentity_t *ent);
void SP_target_score(gentity_t *ent);
void SP_target_teleporter(gentity_t *ent);
void SP_target_relay(gentity_t *ent);
void SP_target_kill(gentity_t *ent);
void SP_target_position(gentity_t *ent);
void SP_target_location(gentity_t *ent);
void SP_target_push(gentity_t *ent);
void SP_target_script_trigger(gentity_t *ent);
void SP_misc_beam(gentity_t *self);

//---- (SA) Wolf targets
// targets
void SP_target_alarm(gentity_t *ent);
void SP_target_counter(gentity_t *ent);
void SP_target_lock(gentity_t *ent);
void SP_target_effect(gentity_t *ent);
void SP_target_fog(gentity_t *ent);
void SP_target_autosave(gentity_t *ent);

// entity visibility dummy
void SP_misc_vis_dummy(gentity_t *ent);
void SP_misc_vis_dummy_multiple(gentity_t *ent);

//----(SA) done

void SP_light(gentity_t *self);
void SP_info_null(gentity_t *self);
void SP_info_notnull(gentity_t *self);
void SP_info_camp(gentity_t *self);
void SP_path_corner(gentity_t *self);
void SP_path_corner_2(gentity_t *self);
void SP_info_limbo_camera(gentity_t *self);
void SP_info_train_spline_main(gentity_t *self);
// void SP_bazooka (gentity_t *self);

void SP_misc_teleporter_dest(gentity_t *self);
void SP_misc_model(gentity_t *ent);
void SP_misc_gamemodel(gentity_t *ent);
void SP_misc_portal_camera(gentity_t *ent);
void SP_misc_portal_surface(gentity_t *ent);
void SP_misc_light_surface(gentity_t *ent);
void SP_misc_grabber_trap(gentity_t *ent);

void SP_misc_commandmap_marker(gentity_t *ent);

void SP_shooter_rocket(gentity_t *ent);
void SP_shooter_grenade(gentity_t *ent);

void SP_team_CTF_redplayer(gentity_t *ent);
void SP_team_CTF_blueplayer(gentity_t *ent);

void SP_team_CTF_redspawn(gentity_t *ent);
void SP_team_CTF_bluespawn(gentity_t *ent);

// JPW NERVE for multiplayer spawnpoint selection
void SP_team_WOLF_objective(gentity_t *ent);
// jpw

void SP_team_WOLF_checkpoint(gentity_t *ent); // DHM - Nerve

// JOSEPH 1-18-00
void SP_props_box_32(gentity_t *self);
void SP_props_box_48(gentity_t *self);
void SP_props_box_64(gentity_t *self);
// END JOSEPH

// Ridah
/*void SP_ai_soldier( gentity_t *ent );
void SP_ai_american( gentity_t *ent );
void SP_ai_zombie( gentity_t *ent );
void SP_ai_warzombie( gentity_t *ent );
void SP_ai_femzombie( gentity_t *ent );
void SP_ai_undead( gentity_t *ent );
void SP_ai_marker( gentity_t *ent );
void SP_ai_effect( gentity_t *ent );
void SP_ai_trigger( gentity_t *ent );
void SP_ai_venom( gentity_t *ent );
void SP_ai_loper( gentity_t *ent );
void SP_ai_sealoper( gentity_t *ent );
void SP_ai_boss_helga( gentity_t *ent );
void SP_ai_boss_heinrich( gentity_t *ent );	//----(SA)	added
void SP_ai_eliteguard( gentity_t *ent );
void SP_ai_stimsoldier_dual( gentity_t *ent );
void SP_ai_stimsoldier_rocket( gentity_t *ent );
void SP_ai_stimsoldier_tesla( gentity_t *ent );
void SP_ai_supersoldier( gentity_t *ent );
void SP_ai_blackguard( gentity_t *ent );
void SP_ai_protosoldier( gentity_t *ent );
void SP_ai_rejectxcreature( gentity_t *ent );
void SP_ai_frogman( gentity_t *ent );
void SP_ai_partisan( gentity_t *ent );
void SP_ai_civilian( gentity_t *ent );
void SP_ai_chimp( gentity_t *ent );	//----(SA)	added*/
// done.

// Rafael particles
void SP_target_smoke(gentity_t *ent);
// done.

// (SA) dlights
void SP_dlight(gentity_t *ent);
// done
void SP_corona(gentity_t *ent);

void SP_mg42(gentity_t *ent);
void SP_aagun(gentity_t *ent);

//----(SA)
// void SP_shooter_zombiespit (gentity_t *ent);
void SP_shooter_mortar(gentity_t *ent);

// alarm
void SP_alarm_box(gentity_t *ent);
//----(SA)	end

void SP_trigger_flagonly(gentity_t *ent);          // DHM - Nerve
void SP_trigger_flagonly_multiple(gentity_t *ent); // DHM - Nerve
void SP_trigger_objective_info(gentity_t *ent);    // DHM - Nerve

void SP_gas(gentity_t *ent);
void SP_target_rumble(gentity_t *ent);

// Mad Doc - TDF
// put this back in for single player bots
void SP_trigger_aidoor(gentity_t *ent);

// Rafael
// void SP_trigger_aidoor (gentity_t *ent);
void SP_SmokeDust(gentity_t *ent);
void SP_Dust(gentity_t *ent);
void SP_props_sparks(gentity_t *ent);
void SP_props_gunsparks(gentity_t *ent);

// Props
void SP_Props_Bench(gentity_t *ent);
void SP_Props_Radio(gentity_t *ent);
void SP_Props_Chair(gentity_t *ent);
void SP_Props_ChairHiback(gentity_t *ent);
void SP_Props_ChairSide(gentity_t *ent);
void SP_Props_ChairChat(gentity_t *ent);
void SP_Props_ChairChatArm(gentity_t *ent);
void SP_Props_DamageInflictor(gentity_t *ent);
void SP_Props_Locker_Tall(gentity_t *ent);
void SP_Props_Desklamp(gentity_t *ent);
void SP_Props_Flamebarrel(gentity_t *ent);
void SP_crate_64(gentity_t *ent);
void SP_Props_Flipping_Table(gentity_t *ent);
void SP_crate_32(gentity_t *self);
void SP_Props_Crate32x64(gentity_t *ent);
void SP_Props_58x112tablew(gentity_t *ent);
void SP_Props_RadioSEVEN(gentity_t *ent);
// void SP_propsFireColumn (gentity_t *ent);
void SP_props_flamethrower(gentity_t *ent);

void SP_func_invisible_user(gentity_t *ent);

void SP_lightJunior(gentity_t *self);

// void SP_props_me109 (gentity_t *ent);
void SP_misc_flak(gentity_t *ent);

void SP_props_snowGenerator(gentity_t *ent);

void SP_props_decoration(gentity_t *ent);
void SP_props_decorBRUSH(gentity_t *ent);
void SP_props_statue(gentity_t *ent);
void SP_props_statueBRUSH(gentity_t *ent);
void SP_skyportal(gentity_t *ent);

// RF, scripting
void SP_script_model_med(gentity_t *ent);
void SP_script_mover(gentity_t *ent);
void SP_script_multiplayer(gentity_t *ent); // DHM - Nerve

void SP_props_footlocker(gentity_t *self);
void SP_misc_firetrails(gentity_t *ent);
void SP_trigger_deathCheck(gentity_t *ent);
void SP_misc_spawner(gentity_t *ent);
void SP_props_decor_Scale(gentity_t *ent);

// Gordon: debris test
void SP_func_debris(gentity_t *ent);
// ===================
void SP_target_set_ident(gentity_t *ent);
void SP_target_activate(gentity_t *ent);
void SP_target_printname(gentity_t *ent);
void SP_target_fireonce(gentity_t *self);
void SP_func_fakebrush(gentity_t *ent);
void SP_target_savereset(gentity_t *self);
void SP_target_increase_ident(gentity_t *self);
void SP_target_save(gentity_t *self);
// Feen: PGM
void SP_target_remove_portals(gentity_t *self);
void SP_target_portal_relay(gentity_t *self);
void SP_target_ftrelay(gentity_t *self);

// Savelimit
void SP_target_savelimit_set(gentity_t *self);
void SP_target_savelimit_inc(gentity_t *self);
void SP_target_decay(gentity_t *self);
void SP_target_interrupt_timerun(gentity_t *self);
// Check speed and if it's too high/low, fire the target
void SP_target_activate_if_velocity(gentity_t *self);
// Scale velocity
void SP_target_scale_velocity(gentity_t *self);
void SP_target_tracker(gentity_t *self);
void SP_trigger_tracker(gentity_t *self);
void SP_target_set_health(gentity_t *self);
void SP_target_deathrun_start(gentity_t *self);
void SP_target_deathrun_checkpoint(gentity_t *self);

// TJL trigger
void SP_target_tjlclear(gentity_t *self);
void SP_target_tjldisplay(gentity_t *self);

spawn_t spawns[] = {
    // info entities don't do anything at all, but provide positional
    // information for things controlled by other processes
    {"info_player_start", SP_info_player_start},
    {"info_player_checkpoint", SP_info_player_checkpoint},
    {"info_player_deathmatch", SP_info_player_deathmatch},
    {"info_player_intermission", SP_info_player_intermission},
    {"info_null", SP_info_null},
    {"info_notnull", SP_info_notnull},     // use target_position instead
    {"info_notnull_big", SP_info_notnull}, // use target_position instead
    {"info_camp", SP_info_camp},

    // Gordon: debris test
    {"func_debris", SP_func_debris},
    // ===================

    {"func_plat", SP_func_plat},
    {"func_button", SP_func_button},
    {"func_explosive", SP_func_explosive},
    {"func_door", SP_func_door},
    {"func_static", SP_func_static},
    {"func_leaky", SP_func_leaky},
    {"func_rotating", SP_func_rotating},
    {"func_bobbing", SP_func_bobbing},
    {"func_pendulum", SP_func_pendulum},
    {"func_train", SP_func_train},
    {"func_group", SP_info_null},
    // JOSEPH 1-26-00
    {"func_train_rotating", SP_func_train_rotating},
    {"func_secret", SP_func_secret},
    // END JOSEPH
    // Rafael
    {"func_door_rotating", SP_func_door_rotating},

    {"func_timer", SP_func_timer}, // rename trigger_timer?

    {"func_invisible_user", SP_func_invisible_user},

    {"func_portaltarget", SP_func_portaltarget},

    // Triggers are brush objects that cause an effect when contacted
    // by a living player, usually involving firing targets.
    // While almost everything could be done with
    // a single trigger class and different targets, triggered effects
    // could not be client side predicted (push and teleport).
    {"trigger_always", SP_trigger_always},
    {"trigger_multiple", SP_trigger_multiple},
    {"trigger_multiple_ext", SP_trigger_multiple_ext},
    {"trigger_push", SP_trigger_push},
    {"trigger_teleport", SP_trigger_teleport},
    {"trigger_hurt", SP_trigger_hurt},
    {"trigger_savereset", SP_trigger_savereset},

    //---- (SA) Wolf triggers
    {"trigger_concussive_dust", SP_trigger_concussive_dust}, // JPW NERVE
    {"trigger_once", SP_trigger_once},
    //---- done

    // Mad Doc - TDf
    // I'm going to put trigger_aidoors back in. I'll make sure they only work
    // in single player
    {"trigger_aidoor", SP_trigger_aidoor},
    // START	xkan,	9/17/2002
    {"trigger_heal", SP_trigger_heal},
    {"trigger_ammo", SP_trigger_ammo},
    // END		xkan,	9/17/2002

    // Gordon: 16/12/02: adding the model things to go with the triggers
    {"misc_cabinet_health", SP_misc_cabinet_health},
    {"misc_cabinet_supply", SP_misc_cabinet_supply},
    // end

    // Rafael
    //	{"trigger_aidoor", SP_trigger_aidoor},
    //	{"trigger_deathCheck",SP_trigger_deathCheck},

    // targets perform no action by themselves, but must be triggered
    // by another entity
    {"target_give", SP_target_give},
    {"target_remove_powerups", SP_target_remove_powerups},
    {"target_delay", SP_target_delay},
    {"target_speaker", SP_target_speaker},
    {"target_print", SP_target_print},
    {"target_laser", SP_target_laser},
    {"target_score", SP_target_score},
    {"target_teleporter", SP_target_teleporter},
    {"target_relay", SP_target_relay},
    {"target_kill", SP_target_kill},
    {"target_position", SP_target_position},
    {"target_location", SP_target_location},
    {"target_push", SP_target_push},
    {"target_script_trigger", SP_target_script_trigger},

    //---- (SA) Wolf targets
    {"target_alarm", SP_target_alarm},
    {"target_counter", SP_target_counter},
    {"target_lock", SP_target_lock},
    {"target_effect", SP_target_effect},
    {"target_fog", SP_target_fog},
    {"target_autosave", SP_target_autosave}, //----(SA)	added
                                             //---- done

    {"target_rumble", SP_target_rumble},

    {"light", SP_light},

    {"lightJunior", SP_lightJunior},

    {"path_corner", SP_path_corner},
    {"path_corner_2", SP_path_corner_2},

    {"info_train_spline_main", SP_info_train_spline_main},
    {"info_train_spline_control", SP_path_corner_2},
    {"info_limbo_camera", SP_info_limbo_camera},

    {"misc_teleporter_dest", SP_misc_teleporter_dest},
    {"misc_model", SP_misc_model},
    {"misc_gamemodel", SP_misc_gamemodel},
    {"misc_portal_surface", SP_misc_portal_surface},
    {"misc_portal_camera", SP_misc_portal_camera},

    {"misc_commandmap_marker", SP_misc_commandmap_marker},

    {"misc_vis_dummy", SP_misc_vis_dummy},
    {"misc_vis_dummy_multiple", SP_misc_vis_dummy_multiple},
    {"misc_light_surface", SP_misc_light_surface},
    {"misc_grabber_trap", SP_misc_grabber_trap},

    {"misc_mg42", SP_mg42},
    {"misc_aagun", SP_aagun},

    {"misc_flak", SP_misc_flak},
    {"misc_firetrails", SP_misc_firetrails},

    {"shooter_rocket", SP_shooter_rocket},
    {"shooter_grenade", SP_shooter_grenade},

    {"shooter_mortar", SP_shooter_mortar},
    {"alarm_box", SP_alarm_box},

    // Gordon: FIXME remove
    {"team_CTF_redplayer", SP_team_CTF_redplayer},
    {"team_CTF_blueplayer", SP_team_CTF_blueplayer},

    {"team_CTF_redspawn", SP_team_CTF_redspawn},
    {"team_CTF_bluespawn", SP_team_CTF_bluespawn},

    {"team_WOLF_objective", SP_team_WOLF_objective},

    {"team_WOLF_checkpoint", SP_team_WOLF_checkpoint},

    {"target_smoke", SP_target_smoke},

    {"misc_spawner", SP_misc_spawner},

    {"props_box_32", SP_props_box_32},
    {"props_box_48", SP_props_box_48},
    {"props_box_64", SP_props_box_64},

    {"props_smokedust", SP_SmokeDust},
    {"props_dust", SP_Dust},
    {"props_sparks", SP_props_sparks},
    {"props_gunsparks", SP_props_gunsparks},

    {"props_bench", SP_Props_Bench},
    {"props_radio", SP_Props_Radio},
    {"props_chair", SP_Props_Chair},
    {"props_chair_hiback", SP_Props_ChairHiback},
    {"props_chair_side", SP_Props_ChairSide},
    {"props_chair_chat", SP_Props_ChairChat},
    {"props_chair_chatarm", SP_Props_ChairChatArm},
    {"props_damageinflictor", SP_Props_DamageInflictor},
    {"props_locker_tall", SP_Props_Locker_Tall},
    {"props_desklamp", SP_Props_Desklamp},
    {"props_flamebarrel", SP_Props_Flamebarrel},
    {"props_crate_64", SP_crate_64},
    {"props_flippy_table", SP_Props_Flipping_Table},
    {"props_crate_32", SP_crate_32},
    {"props_crate_32x64", SP_Props_Crate32x64},
    {"props_58x112tablew", SP_Props_58x112tablew},
    {"props_radioSEVEN", SP_Props_RadioSEVEN},
    {"props_snowGenerator", SP_props_snowGenerator},
    //	{"props_FireColumn", SP_propsFireColumn},
    {"props_decoration", SP_props_decoration},
    {"props_decorBRUSH", SP_props_decorBRUSH},
    {"props_statue", SP_props_statue},
    {"props_statueBRUSH", SP_props_statueBRUSH},
    {"props_skyportal", SP_skyportal},
    {"props_footlocker", SP_props_footlocker},
    {"props_flamethrower", SP_props_flamethrower},
    {"props_decoration_scale", SP_props_decor_Scale},

    {"dlight", SP_dlight},

    {"corona", SP_corona},

    {"trigger_flagonly", SP_trigger_flagonly},
    {"trigger_flagonly_multiple", SP_trigger_flagonly_multiple},

    {"test_gas", SP_gas},
    {"trigger_objective_info", SP_trigger_objective_info},

    // RF, scripting
    {"script_model_med", SP_script_model_med},
    {"script_mover", SP_script_mover},
    {"script_multiplayer", SP_script_multiplayer},

    {"func_constructible", SP_func_constructible},
    {"func_brushmodel", SP_func_brushmodel},
    {"misc_beam", SP_misc_beam},
    {"misc_constructiblemarker", SP_misc_constructiblemarker},
    {"target_explosion", SP_target_explosion},
    {"misc_landmine", SP_misc_landmine},
    {"target_setident", SP_target_set_ident},
    {"target_activate", SP_target_activate},

    // deprecated, kept for compatibility, uses target_print code
    {"target_printname", SP_target_printname},
    /*{"etjump_target_relay",	SP_target_fireonce },*/ // Changed for RC1 -
                                                        // Feen
    {"etjump2_target_relay", SP_target_fireonce},
    {"etjump203_target_relay", SP_target_fireonce},
    {"func_fakebrush", SP_func_fakebrush},
    {"target_savereset", SP_target_savereset},
    {"weapon_portalgun", ETJump::Portalgun::spawn}, // Feen: PGM
    {"target_increase_ident", SP_target_increase_ident},
    {"target_save", SP_target_save},
    {"target_remove_portals", SP_target_remove_portals},
    {"target_portal_relay", SP_target_portal_relay},
    {"target_ftrelay", SP_target_ftrelay},
    {"target_savelimit_set", SP_target_savelimit_set},
    {"target_savelimit_inc", SP_target_savelimit_inc},
    {"target_decay", SP_target_decay},
    {"target_starttimer", ETJump::TargetStartTimer::spawn},
    {"trigger_starttimer", ETJump::TriggerStartTimer::spawn},
    {"trigger_starttimer_ext", ETJump::TriggerStartTimerExt::spawn},
    {"target_stoptimer", ETJump::TargetStopTimer::spawn},
    {"trigger_stoptimer", ETJump::TriggerStopTimer::spawn},
    {"trigger_stoptimer_ext", ETJump::TriggerStopTimerExt::spawn},
    {"target_checkpoint", ETJump::TargetCheckpoint::spawn},
    {"trigger_checkpoint", ETJump::TriggerCheckpoint::spawn},
    {"trigger_checkpoint_ext", ETJump::TriggerCheckpointExt::spawn},
    {"target_interrupt_timerun", SP_target_interrupt_timerun},
    {"target_activate_if_velocity", SP_target_activate_if_velocity},
    {"target_scale_velocity", SP_target_scale_velocity},
    {"trigger_tracker", SP_trigger_tracker},
    {"target_tracker", SP_target_tracker},
    {"target_set_health", SP_target_set_health},
    {"target_deathrun_start", SP_target_deathrun_start},
    {"target_deathrun_checkpoint", SP_target_deathrun_checkpoint},
    {"target_displaytjl", SP_target_tjldisplay},
    {"target_cleartjl", SP_target_tjlclear},
    {"target_init", ETJump::TargetInit::spawn},
    {"func_missilepad", ETJump::Missilepad::spawn},
    {"trigger_teleport_client", ETJump::TriggerTeleportClient::spawn},
    {"target_ft_setrules", ETJump::TargetFtSetRules::spawn},
    {"target_spawn_relay", ETJump::TargetSpawnRelay::spawn},
    {nullptr, nullptr},
};

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn(gentity_t *ent) {
  spawn_t *s;
  gitem_t *item;

  if (!ent->classname) {
    G_Printf("G_CallSpawn: NULL classname\n");
    return qfalse;
  }

  // check item spawn functions
  for (item = bg_itemlist + 1; item->classname; item++) {
    if (!Q_stricmp(item->classname, ent->classname)) {
      // found it
      G_SpawnItem(ent, item);

      G_Script_ScriptParse(ent);
      G_Script_ScriptEvent(ent, "spawn", "");
      return qtrue;
    }
  }

  // check normal spawn functions
  for (s = spawns; s->name; s++) {
    if (!Q_stricmp(s->name, ent->classname)) {
      // found it
      s->spawn(ent);

      // RF, entity scripting
      if (/*ent->s.number >= MAX_CLIENTS &&*/ ent->scriptName) {
        G_Script_ScriptParse(ent);
        G_Script_ScriptEvent(ent, "spawn", "");
      }

      return qtrue;
    }
  }
  G_Printf("%s doesn't have a spawn function\n", ent->classname);
  return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString(const char *string) {
  char *newb, *new_p;
  int i, l;

  l = strlen(string) + 1;

  newb = static_cast<char *>(G_Alloc(l));

  new_p = newb;

  // turn \n into a real linefeed
  for (i = 0; i < l; i++) {
    if (string[i] == '\\' && i < l - 1) {
      i++;
      if (string[i] == 'n') {
        *new_p++ = '\n';
      } else {
        *new_p++ = '\\';
      }
    } else {
      *new_p++ = string[i];
    }
  }

  return newb;
}

/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField(const char *key, const char *value, gentity_t *ent) {
  float v;
  vec3_t vec;

  for (const field_t *f = fields; f->name; f++) {
    if (!Q_stricmp(f->name, key)) {
      // found it
      const auto b = reinterpret_cast<byte *>(ent);

      switch (f->type) {
        case F_LSTRING:
          *reinterpret_cast<char **>(b + f->ofs) = G_NewString(value);
          break;
        case F_VECTOR:
          sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
          reinterpret_cast<float *>(b + f->ofs)[0] = vec[0];
          reinterpret_cast<float *>(b + f->ofs)[1] = vec[1];
          reinterpret_cast<float *>(b + f->ofs)[2] = vec[2];
          break;
        case F_INT:
          *reinterpret_cast<int *>(b + f->ofs) = Q_atoi(value);
          break;
        case F_FLOAT:
          *reinterpret_cast<float *>(b + f->ofs) = Q_atof(value);
          break;
        case F_ANGLEHACK:
          v = Q_atof(value);
          reinterpret_cast<float *>(b + f->ofs)[0] = 0;
          reinterpret_cast<float *>(b + f->ofs)[1] = v;
          reinterpret_cast<float *>(b + f->ofs)[2] = 0;
          break;
        case F_CURSORHINT:
          ETJump::EntityUtilities::setCursorhintFromString(
              *reinterpret_cast<int *>(b + f->ofs), value);
          break;
        case F_IGNORE:
        default:
          break;
      }

      return;
    }
  }
}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars(void) {
  int i;
  gentity_t *ent;
  char *str;

  // get the next free entity
  ent = G_Spawn();

  for (i = 0; i < level.numSpawnVars; i++) {
    G_ParseField(level.spawnVars[i][0], level.spawnVars[i][1], ent);
  }

  // check for "notteam" / "notfree" flags
  G_SpawnInt("notteam", "0", &i);
  if (i) {
    G_FreeEntity(ent);
    return;
  }

  // allowteams handling
  G_SpawnString("allowteams", "", &str);
  if (str[0]) {
    str = Q_strlwr(str);
    if (strstr(str, "axis")) {
      ent->allowteams |= ALLOW_AXIS_TEAM;
    }
    if (strstr(str, "allies")) {
      ent->allowteams |= ALLOW_ALLIED_TEAM;
    }
    if (strstr(str, "cvops")) {
      ent->allowteams |= ALLOW_DISGUISED_CVOPS;
    }
  }

  if (ent->targetname && *ent->targetname) {
    ent->targetnamehash = BG_StringHashValue(ent->targetname);
  } else {
    ent->targetnamehash = -1;
  }

  // move editor origin to pos
  VectorCopy(ent->s.origin, ent->s.pos.trBase);
  VectorCopy(ent->s.origin, ent->r.currentOrigin);

  // if we didn't get a classname, don't bother spawning anything
  if (!G_CallSpawn(ent)) {
    G_FreeEntity(ent);
  }

  // RF, try and move it into the bot entities if possible
  //	BotCheckBotGameEntity( ent );
}
/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken(const char *string) {
  int l;
  char *dest;

  l = static_cast<int>(strlen(string));
  if (level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS) {
    G_Error("G_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS");
  }

  dest = level.spawnVarChars + level.numSpawnVarChars;
  memcpy(dest, string, l + 1);

  level.numSpawnVarChars += l + 1;

  return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars(void) {
  char keyname[MAX_TOKEN_CHARS];
  char com_token[MAX_TOKEN_CHARS];

  level.numSpawnVars = 0;
  level.numSpawnVarChars = 0;

  // parse the opening brace
  if (!trap_GetEntityToken(com_token, sizeof(com_token))) {
    // end of spawn string
    return qfalse;
  }
  if (com_token[0] != '{') {
    G_Error("G_ParseSpawnVars: found %s when expecting {", com_token);
  }

  // go through all the key / value pairs
  while (1) {
    // parse key
    if (!trap_GetEntityToken(keyname, sizeof(keyname))) {
      G_Error("G_ParseSpawnVars: EOF without closing "
              "brace");
    }

    if (keyname[0] == '}') {
      break;
    }

    // parse value
    if (!trap_GetEntityToken(com_token, sizeof(com_token))) {
      G_Error("G_ParseSpawnVars: EOF without closing "
              "brace");
    }

    if (com_token[0] == '}') {
      G_Error("G_ParseSpawnVars: closing brace without "
              "data");
    }
    if (level.numSpawnVars == MAX_SPAWN_VARS) {
      G_Error("G_ParseSpawnVars: MAX_SPAWN_VARS");
    }
    level.spawnVars[level.numSpawnVars][0] = G_AddSpawnVarToken(keyname);
    level.spawnVars[level.numSpawnVars][1] = G_AddSpawnVarToken(com_token);
    level.numSpawnVars++;
  }

  return qtrue;
}

namespace ETJump {
static void initNoOverbounce() {
  auto value = 0;
  G_SpawnInt("nooverbounce", "0", &value);
  level.noOverbounce = value > 0;
  level.noOverbounce ? shared.integer |= BG_LEVEL_NO_OVERBOUNCE
                     : shared.integer &= ~BG_LEVEL_NO_OVERBOUNCE;

  trap_Cvar_Set("shared", va("%d", shared.integer));
  G_Printf("No overbounce %s.\n", level.noOverbounce ? "enabled" : "disabled");
}

static void initNoJumpDelay() {
  auto value = 0;
  G_SpawnInt("nojumpdelay", "0", &value);
  level.noJumpDelay = value > 0;
  level.noJumpDelay ? shared.integer |= BG_LEVEL_NO_JUMPDELAY
                    : shared.integer &= ~BG_LEVEL_NO_JUMPDELAY;

  trap_Cvar_Set("shared", va("%d", shared.integer));
  G_Printf("No jump delay %s.\n", level.noJumpDelay ? "enabled" : "disabled");
}

static void initNoSave() {
  auto value = 0;
  G_SpawnInt("nosave", "0", &value);
  level.noSave = value > 0 ? qtrue : qfalse;
  level.noSave ? shared.integer |= BG_LEVEL_NO_SAVE
               : shared.integer &= ~BG_LEVEL_NO_SAVE;

  trap_Cvar_Set("shared", va("%d", shared.integer));
  G_Printf("Save is %s.\n", level.noSave ? "disabled" : "enabled");
}

std::unordered_map<std::string, SaveSystem::SaveLoadRestrictions>
    allowedStrictValues{
        {"default", SaveSystem::SaveLoadRestrictions::Default},
        {"move", SaveSystem::SaveLoadRestrictions::Move},
        {"dead", SaveSystem::SaveLoadRestrictions::Dead},
    };

static void initStrictSaveLoad() {
  char *buff = nullptr;
  G_SpawnString("strictsaveload", "0", &buff);
  std::istringstream str{buff};
  auto value = 0;
  if (isdigit(buff[0])) {
    str >> value;
  } else {
    std::string token;
    while (str >> token) {
      token = ETJump::StringUtil::toLowerCase(token);
      value |= static_cast<int>(allowedStrictValues[token]); // else 0
    }
  }
  level.saveLoadRestrictions = value;
  G_Printf("Save restrictions are %s.\n", value ? "enabled" : "disabled");
}

static void initNoFallDamage() {
  auto value = 0;

  G_SpawnInt("nofalldamage", "0", &value);
  level.noFallDamage = value > 0;

  // reset flags
  shared.integer &= ~BG_LEVEL_NO_FALLDAMAGE;
  shared.integer &= ~BG_LEVEL_NO_FALLDAMAGE_FORCE;

  if (value == 1) {
    shared.integer |= BG_LEVEL_NO_FALLDAMAGE;
  } else if (value == 2) {
    shared.integer |= BG_LEVEL_NO_FALLDAMAGE_FORCE;
  }

  trap_Cvar_Set("shared", va("%d", shared.integer));
  G_Printf("No fall damage %s.\n", level.noFallDamage ? "enabled" : "disabled");
}

static void initNoProne() {
  auto value = 0;
  G_SpawnInt("noprone", "0", &value);
  level.noProne = value > 0 ? true : false;
  level.noProne ? shared.integer |= BG_LEVEL_NO_PRONE
                : shared.integer &= ~BG_LEVEL_NO_PRONE;

  trap_Cvar_Set("shared", va("%d", shared.integer));
  G_Printf("Prone is %s.\n", level.noProne ? "disabled" : "enabled");
}

static void initNoDrop() {
  auto value = 0;
  G_SpawnInt("nodrop", "0", &value);
  level.noDrop = value > 0;
  level.noDrop ? shared.integer |= BG_LEVEL_NO_DROP
               : shared.integer &= ~BG_LEVEL_NO_DROP;

  trap_Cvar_Set("shared", va("%d", shared.integer));
  G_Printf("Nodrop is %s.\n", level.noDrop ? "enabled" : "disabled");
}

static void initNoWallbug() {
  int value = 0;
  G_SpawnInt("nowallbug", "0", &value);
  level.noWallbug = value;
  level.noWallbug ? shared.integer |= BG_LEVEL_NO_WALLBUG
                  : shared.integer &= ~BG_LEVEL_NO_WALLBUG;

  trap_Cvar_Set("shared", va("%d", shared.integer));
  G_Printf("Wallbugging is %s.\n", level.noWallbug ? "disabled" : "enabled");
}

static void initNoNoclip() {
  int value;
  G_SpawnInt("nonoclip", "0", &value);

  level.noNoclip = value;
  level.noNoclip ? shared.integer |= BG_LEVEL_NO_NOCLIP
                 : shared.integer &= ~BG_LEVEL_NO_NOCLIP;

  trap_Cvar_Set("shared", va("%d", shared.integer));
  G_Printf("Noclip is %s.\n", level.noNoclip ? "disabled" : "enabled");
}

static void initNoFTNoGhost() {
  int value;
  G_SpawnInt("noftnoghost", "0", &value);

  level.noFTNoGhost = value;
  G_Printf("Fireteam noghost %s be toggled by players.\n",
           level.noFTNoGhost ? "cannot" : "can");
}

static void initNoFTSaveLimit() {
  int value;
  G_SpawnInt("noftsavelimit", "0", &value);

  level.noFTSaveLimit = value;
  G_Printf("Fireteam savelimit %s be set by players.\n",
           level.noFTSaveLimit ? "cannot" : "can");
}

static void initNoFTTeamjumpMode() {
  int value;
  G_SpawnInt("noftteamjumpmode", "0", &value);

  level.noFTTeamjumpMode = value;
  G_Printf("Fireteam teamjump mode %s be toggled by players.\n",
           level.noFTTeamjumpMode ? "cannot" : "can");
}

static void initPortalPredict() {
  int value = 0;
  G_SpawnInt("portalpredict", "0", &value);

  level.portalPredict = value;
  level.portalPredict ? shared.integer |= BG_LEVEL_PORTAL_PREDICT
                      : shared.integer &= ~BG_LEVEL_PORTAL_PREDICT;

  trap_Cvar_Set("shared", va("%d", shared.integer));
  G_Printf("Predicted portal teleports are %sforced.\n",
           level.portalPredict ? "" : "not ");
}
} // namespace ETJump

/*QUAKED worldspawn (0 0 0) ? NO_GT_WOLF NO_GT_STOPWATCH NO_GT_CHECKPOINT NO_LMS

Every map should have exactly one worldspawn.
"music"     Music wav file
"gravity"   800 is default gravity
"message" Text to print during connection process
"ambient"  Ambient light value (must use '_color')
"_color"    Ambient light color (must be used with 'ambient')
"sun"        Shader to use for 'sun' image
*/
void SP_worldspawn(void) {
  char *s;
  int val = 0;

  G_SpawnString("classname", "", &s);
  if (Q_stricmp(s, "worldspawn")) {
    G_Error("SP_worldspawn: The first entity isn't 'worldspawn'");
  }

  // make some data visible to connecting client
  trap_SetConfigstring(CS_GAME_VERSION, GAME_NAME);

  trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime));

  G_SpawnString("music", "", &s);
  trap_SetConfigstring(CS_MUSIC, s);

  G_SpawnString("message", "", &s);
  trap_SetConfigstring(CS_MESSAGE, s); // map specific message

  G_SpawnString("cclayers", "0", &s);
  if (Q_atoi(s)) {
    level.ccLayers = qtrue;
  }

  G_Printf("ETJump: checking worldspawn key values:\n");

  G_SpawnString("noexplosives", "0", &s);
  if (Q_atoi(s)) {
    int noExplosives = Q_atoi(s);
    if (noExplosives > 2) {
      noExplosives = 2;
    } else if (noExplosives < 0) {
      noExplosives = 0;
    }

    level.noExplosives = noExplosives;
  }
  G_Printf("Explosives are %s.\n", level.noExplosives ? "disabled" : "enabled");

  G_SpawnString("nogod", "0", &s);
  if (Q_atoi(s)) {
    level.noGod = qtrue;
  } else {
    level.noGod = qfalse;
  }
  G_Printf("God mode is %s.\n", level.noGod ? "disabled" : "enabled");

  G_SpawnString("nogoto", "0", &s);
  if (Q_atoi(s)) {
    level.noGoto = qtrue;
  } else {
    level.noGoto = qfalse;
  }
  G_Printf("Goto is %s.\n", level.noGoto ? "disabled" : "enabled");

  // Feen: PGM - Enable/Disable frivolous use
  //			  of portal gun....
  G_SpawnString("portalgun_spawn", "1", &s);
  if (Q_atoi(s)) {
    level.portalEnabled = qtrue;
    G_Printf("Players spawn with portal gun by default.\n");
  } else {
    level.portalEnabled = qfalse;
    G_Printf("Players do not spawn with portal gun by default.\n");
  }

  G_SpawnString("portalsurfaces", "1", &s);
  if (Q_atoi(s)) {
    level.portalSurfaces = qtrue;
  } else {
    level.portalSurfaces = qfalse;
  }
  G_Printf("Surfaces are %s by default for portals.\n",
           level.portalSurfaces ? "enabled" : "disabled");

  G_SpawnString("noghost", "0", &s);
  if (Q_atoi(s)) {
    char buf[32] = "\0";
    int currentValue = g_ghostPlayers.integer;
    currentValue |= 2;

    Com_sprintf(buf, sizeof(buf), "%d", currentValue);

    trap_Cvar_Set("g_ghostPlayers", buf);
    trap_Cvar_Update(&g_ghostPlayers);
    G_Printf("Ghosting is disabled.\n");
    level.noGhost = true;
  } else {
    char buf[128] = "\0";
    int currentValue = g_ghostPlayers.integer;
    currentValue &= ~(2);

    Com_sprintf(buf, sizeof(buf), "%d", currentValue);

    trap_Cvar_Set("g_ghostPlayers", buf);
    trap_Cvar_Update(&g_ghostPlayers);
    G_Printf("Ghosting is enabled.\n");
    level.noGhost = false;
  }

  G_SpawnString("limitedsaves", "0", &s);
  if (Q_atoi(s)) {
    int limit = Q_atoi(s);
    level.limitedSaves = limit;
    G_Printf("Save is limited to %s.\n",
             ETJump::getPluralizedString(limit, "save").c_str());
  } else {
    level.limitedSaves = 0;
    G_Printf("Save is not limited.\n");
  }

  G_SpawnString("portalteam", "0", &s);
  val = Q_atoi(s);
  if (val) {
    if (val == 1) {
      level.portalTeam = ETJump::PORTAL_TEAM_FT;
    } else {
      level.portalTeam = ETJump::PORTAL_TEAM_ALL;
    }
  }

  trap_Cvar_Set("g_portaLTeam", s);
  G_Printf("Portal team is set to %d.\n", val);

  ETJump::initNoSave();
  ETJump::initNoOverbounce();
  ETJump::initNoJumpDelay();
  ETJump::initStrictSaveLoad();
  ETJump::initNoFallDamage();
  ETJump::initNoProne();
  ETJump::initNoDrop();
  ETJump::initNoWallbug();
  ETJump::initNoNoclip();
  ETJump::initNoFTNoGhost();
  ETJump::initNoFTSaveLimit();
  ETJump::initNoFTTeamjumpMode();
  ETJump::initPortalPredict();

  level.mapcoordsValid = qfalse;
  if (G_SpawnVector2D("mapcoordsmins", "-128 128",
                      level.mapcoordsMins) && // top left
      G_SpawnVector2D("mapcoordsmaxs", "128 -128",
                      level.mapcoordsMaxs)) // bottom right
  {
    level.mapcoordsValid = qtrue;
  }

  trap_SetConfigstring(CS_MOTD, g_motd.string); // message of the day

  G_SpawnString("spawnflags", "0", &s);
  g_entities[ENTITYNUM_WORLD].spawnflags = Q_atoi(s);
  g_entities[ENTITYNUM_WORLD].r.worldflags =
      g_entities[ENTITYNUM_WORLD].spawnflags;

  g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
  g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

  // see if we want a warmup time
  trap_SetConfigstring(CS_WARMUP, "");
  if (g_restarted.integer) {
    trap_Cvar_Set("g_restarted", "0");
    level.warmupTime = 0;
  }

  if (g_gamestate.integer == GS_PLAYING) {
    G_initMatch();
  }
}

/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString(void) {
  // allow calls to G_Spawn*()
  G_Printf("Enable spawning!\n");
  level.spawning = qtrue;
  level.numSpawnVars = 0;

  // the worldspawn is not an actual entity, but it still
  // has a "spawn" function to perform any global setup
  // needed by a level (setting configstrings or cvars, etc)
  if (!G_ParseSpawnVars()) {
    G_Error("SpawnEntities: no entities");
  }
  SP_worldspawn();

  // parse ents
  while (G_ParseSpawnVars()) {
    G_SpawnGEntityFromSpawnVars();
  }

  if (!level.gameManager) {
    G_Printf("^3WARNING: ^7No ^3'script_multiplayer' ^7found in the map, "
             "checking for other entities with scriptname... ");

    if (!ETJump::checkEntsForScriptname()) {
      G_Printf("^7No scriptable entities found, spawning "
               "^3'etjump_game_manager'^7... ");

      ETJump::spawnGameManager();

      G_Printf("^2DONE\n");
    }
  }

  G_Printf("Disable spawning!\n");
  level.spawning = qfalse; // any future calls to G_Spawn*() will be errors
}
