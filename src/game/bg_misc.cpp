/*
 * name:		bg_misc.c
 *
 * desc:		both games misc functions, all completely stateless
 *
 */

#include <algorithm>

#include "q_shared.h"
#include "bg_public.h"
#include "../../assets/ui/menudef.h"

#ifdef CGAMEDLL
extern vmCvar_t cg_gameType;
  #define gametypeCvar cg_gameType
#elif GAMEDLL
extern vmCvar_t g_developer;
extern vmCvar_t g_gametype;
  #define gametypeCvar g_gametype
#else
extern vmCvar_t ui_gameType;
  #define gametypeCvar ui_gameType
#endif

const char *skillNames[SK_NUM_SKILLS] = {
    "Battle Sense",  "Engineering",   "First Aid", "Signals",
    "Light Weapons", "Heavy Weapons", "Covert Ops"};

const char *skillNamesLine1[SK_NUM_SKILLS] = {
    "Battle", "Engineering", "First", "Signals", "Light", "Heavy", "Covert"};

const char *skillNamesLine2[SK_NUM_SKILLS] = {"Sense",   "",        "Aid", "",
                                              "Weapons", "Weapons", "Ops"};

const char *medalNames[SK_NUM_SKILLS] = {"Distinguished Service Medal",
                                         "Steel Star",
                                         "Silver Cross",
                                         "Signals Medal",
                                         "Infantry Medal",
                                         "Bombardment Medal",
                                         "Silver Snake"};

const int skillLevels[NUM_SKILL_LEVELS] = {
    0,  // reaching level 0
    20, // reaching level 1
    50, // reaching level 2
    90, // reaching level 3
    140 // reaching level 4
        //	200		// reaching level 5
};

vec3_t playerMins = {-18, -18, -24};
vec3_t playerMaxs = {18, 18, 48};
vec3_t playerlegsProneMins = {-13.5f, -13.5f, -24.f};
vec3_t playerlegsProneMaxs = {13.5f, 13.5f, -14.4f};

int numSplinePaths;
splinePath_t splinePaths[MAX_SPLINE_PATHS];

int numPathCorners;
pathCorner_t pathCorners[MAX_PATH_CORNERS];

// these defines are matched with the character torso animations
inline constexpr int DELAY_LOW = 100;     // machineguns, tesla, spear, flame
inline constexpr int DELAY_HIGH = 100;    // mauser, garand
inline constexpr int DELAY_PISTOL = 100;  // colt, luger, sp5, cross
inline constexpr int DELAY_SHOULDER = 50; // rl
inline constexpr int DELAY_THROW = 250;   // grenades, dynamite

// Arnout: the new loadout for WolfXP
int weapBanksMultiPlayer[MAX_WEAP_BANKS_MP][MAX_WEAPS_IN_BANK_MP] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // empty bank '0'
    {WP_KNIFE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {WP_LUGER, WP_COLT, WP_AKIMBO_COLT, WP_AKIMBO_LUGER, WP_AKIMBO_SILENCEDCOLT,
     WP_AKIMBO_SILENCEDLUGER, 0, 0, 0, 0, 0, 0},
    {WP_MP40, WP_THOMPSON, WP_STEN, WP_GARAND, WP_PANZERFAUST, WP_FLAMETHROWER,
     WP_KAR98, WP_CARBINE, WP_FG42, WP_K43, WP_MOBILE_MG42, WP_MORTAR},
    {WP_GRENADE_LAUNCHER, WP_GRENADE_PINEAPPLE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {
        WP_MEDIC_SYRINGE,
        WP_PLIERS,
        WP_SMOKE_MARKER,
        WP_SMOKE_BOMB,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    },
    {WP_DYNAMITE, WP_MEDKIT, WP_AMMO, WP_SATCHEL, WP_SATCHEL_DET, 0, 0, 0, 0, 0,
     0, 0},
    {WP_LANDMINE, WP_MEDIC_ADRENALINE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {WP_BINOCULARS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // Feen: PGM - Weapon bank 8 in most cases...
    {WP_PORTAL_GUN, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

// TAT 10/4/2002
//		Using one unified list for which weapons can received ammo
//		This is used both by the ammo pack code and by the bot code to
// determine if reloads are needed
int reloadableWeapons[] = {WP_MP40,
                           WP_THOMPSON,
                           WP_STEN,
                           WP_GARAND,
                           WP_PANZERFAUST,
                           WP_FLAMETHROWER,
                           WP_KAR98,
                           WP_CARBINE,
                           WP_FG42,
                           WP_K43,
                           WP_MOBILE_MG42,
                           WP_COLT,
                           WP_LUGER,
                           WP_MORTAR,
                           WP_AKIMBO_COLT,
                           WP_AKIMBO_LUGER,
                           WP_M7,
                           WP_GPG40,
                           WP_AKIMBO_SILENCEDCOLT,
                           WP_AKIMBO_SILENCEDLUGER,
                           -1};

// [0] = maxammo		-	max player ammo carrying capacity.
// [1] = uses			-	how many 'rounds' it takes/costs to fire
// one cycle. [2] = maxclip		-	max 'rounds' in a clip. [3] =
// reloadTime -	time from start of reload until ready to fire. [4] =
// fireDelayTime	-	time from pressing 'fire' until first shot is
// fired. (used for delaying fire while weapon is 'readied' in animation) [5] =
// nextShotTime	-	when firing continuously, this is the time between shots
// [6] = maxHeat		-	max active firing time before weapon
// 'overheats' (at which point the weapon will fail for a moment) [7] = coolRate
// -	how fast the weapon cools down. [8] = mod			-
// means of death

// potential inclusions in the table:
// damage			-
// splashDamage		-
// soundRange		-	distance which ai can hear the weapon
// ammoWarning		-	amount we give the player a 'low on ammo'
// warning (just a HUD color change or something) clipWarning		-
// amount we give the player a 'low in clip' warning (just a HUD color change or
// something)
// maxclip2			-	allow the player to (mod/powerup)
// upgrade clip size when aplicable (luger has 8 round standard clip and 32
// round snail magazine, for ex.)
//
//
//

// Separate table for SP and MP allow us to make the ammo and med packs function
// differently and may allow use to balance weapons separately for each game.
// Gordon: changed to actually use the maxammo values
ammotable_t ammoTableMP[WP_NUM_WEAPONS] = {
    //	MAX				USES	MAX		START	START
    // RELOAD
    // FIRE
    // NEXT
    // HEAT,
    // COOL,	MOD,	...
    //	AMMO			AMT.	CLIP	AMMO	CLIP	TIME	DELAY
    // SHOT
    {0, 0, 0, 0, 0, 0, 50, 0, 0, 0,
     0}, // WP_NONE					// 0
    {999, 0, 999, 0, 0, 0, 50, 200, 0, 0,
     MOD_KNIFE}, // WP_KNIFE					// 1
    {24, 1, 8, 24, 8, 1500, DELAY_PISTOL, 400, 0, 0,
     MOD_LUGER}, // WP_LUGER					// 2	// NOTE:
                 // also
                 // 32
                 // round
                 // 'snail' magazine
    {90, 1, 30, 30, 30, 2400, DELAY_LOW, 150, 0, 0,
     MOD_MP40}, // WP_MP40					// 3
    {45, 1, 15, 0, 4, 1000, DELAY_THROW, 1600, 0, 0,
     MOD_GRENADE_LAUNCHER}, // WP_GRENADE_LAUNCHER		// 4
    {4, 1, 1, 0, 4, 1000, 750, 2000, 0, 0,
     MOD_PANZERFAUST}, // WP_PANZERFAUST			// 5	// DHM -
                       // Nerve :: updated delay so prediction is correct
    {200, 1, 200, 0, 200, 1000, DELAY_LOW, 50, 0, 0,
     MOD_FLAMETHROWER}, // WP_FLAMETHROWER			// 6
    {24, 1, 8, 24, 8, 1500, DELAY_PISTOL, 400, 0, 0,
     MOD_COLT}, // WP_COLT					// 7
    {90, 1, 30, 30, 30, 2400, DELAY_LOW, 150, 0, 0,
     MOD_THOMPSON}, // WP_THOMPSON				// 8
    {45, 1, 15, 0, 4, 1000, DELAY_THROW, 1600, 0, 0,
     MOD_GRENADE_PINEAPPLE}, // WP_GRENADE_PINEAPPLE		// 9

    {96, 1, 32, 32, 32, 3100, DELAY_LOW, 150, 1200, 450,
     MOD_STEN}, // WP_STEN					// 10
    {10, 0, 1, 0, 1, 1500, 50, 1000, 0, 0,
     MOD_SYRINGE}, // WP_MEDIC_SYRINGE			// 11
    {
        1,
        0,
        1,
        0,
        0,
        3000,
        50,
        1000,
        0,
        0,
        MOD_AMMO,
    }, // WP_AMMO					// 12
    {
        1,
        0,
        1,
        0,
        1,
        3000,
        50,
        1000,
        0,
        0,
        MOD_ARTY,
    }, // WP_ARTY					// 13
    {24, 1, 8, 24, 8, 1500, DELAY_PISTOL, 400, 0, 0,
     MOD_SILENCER}, // WP_SILENCER				// 14
    {1, 0, 10, 0, 0, 1000, DELAY_THROW, 1600, 0, 0,
     MOD_DYNAMITE},                         // WP_DYNAMITE				// 15
    {999, 0, 999, 0, 0, 0, 50, 0, 0, 0, 0}, // WP_SMOKETRAIL // 16
    {999, 0, 999, 0, 0, 0, 50, 0, 0, 0,
     0},                                    // WP_MAPMORTAR				// 17
    {999, 0, 999, 0, 0, 0, 50, 0, 0, 0, 0}, // VERYBIGEXPLOSION // 18
    {999, 0, 999, 1, 1, 0, 50, 0, 0, 0,
     0}, // WP_MEDKIT				// 19

    {999, 0, 999, 0, 0, 0, 50, 0, 0, 0, 0}, // WP_BINOCULARS // 20
    {999, 0, 999, 0, 0, 0, 50, 0, 0, 0,
     0}, // WP_PLIERS				// 21
    {999, 0, 999, 0, 1, 0, 50, 0, 0, 0,
     MOD_AIRSTRIKE}, // WP_SMOKE_MARKER			// 22
    {30, 1, 10, 20, 10, 2500, DELAY_LOW, 400, 0, 0,
     MOD_KAR98}, // WP_KAR98					// 23
                 // K43
    {24, 1, 8, 16, 8, 1500, DELAY_LOW, 400, 0, 0,
     MOD_CARBINE}, // WP_CARBINE				// 24
                   // GARAND
    {24, 1, 8, 16, 8, 1500, DELAY_LOW, 400, 0, 0,
     MOD_GARAND}, // WP_GARAND				// 25		GARAND
    {1, 0, 1, 0, 1, 100, DELAY_LOW, 100, 0, 0,
     MOD_LANDMINE}, // WP_LANDMINE				// 26
    {1, 0, 1, 0, 0, 3000, DELAY_LOW, 2000, 0, 0,
     MOD_SATCHEL}, // WP_SATCHEL				// 27
    {
        1,
        0,
        1,
        0,
        0,
        3000,
        722,
        2000,
        0,
        0,
        0,
    }, // WP_SATCHEL_DET			// 28
    {6, 1, 1, 0, 0, 2000, DELAY_HIGH, 2000, 0, 0,
     MOD_TRIPMINE}, // WP_TRIPMINE				// 29

    {1, 0, 10, 0, 1, 1000, DELAY_THROW, 1600, 0, 0,
     MOD_SMOKEBOMB}, // WP_SMOKE_BOMB			// 30
    {450, 1, 150, 0, 150, 3000, DELAY_LOW, 66, 1500, 300,
     MOD_MOBILE_MG42}, // WP_MOBILE_MG42			// 31
    {30, 1, 10, 20, 10, 2500, DELAY_LOW, 400, 0, 0,
     MOD_K43}, // WP_K43					// 32
               // K43
    {60, 1, 20, 40, 20, 2000, DELAY_LOW, 100, 0, 0,
     MOD_FG42},                             // WP_FG42					// 33
    {0, 0, 0, 0, 0, 0, 0, 0, 1500, 300, 0}, // WP_DUMMY_MG42 // 34
    {15, 1, 1, 0, 0, 0, 750, 1600, 0, 0,
     MOD_MORTAR}, // WP_MORTAR				// 35
    {999, 0, 1, 0, 0, 1000, 750, 1600, 0, 0,
     0}, // WP_LOCKPICK				// 36
    {48, 1, 8, 48, 8, 2700, DELAY_PISTOL, 200, 0, 0,
     MOD_AKIMBO_COLT}, // WP_AKIMBO_COLT			// 37
    {48, 1, 8, 48, 8, 2700, DELAY_PISTOL, 200, 0, 0,
     MOD_AKIMBO_LUGER}, // WP_AKIMBO_LUGER			// 38
    {999, 0, 999, 0, 0, 0, 50, 200, 0, 0,
     MOD_PORTAL_GUN}, // WP_PORTAL_GUN			// 39 //Feen: Portal Gun
                      // Mod

    {4, 1, 1, 4, 1, 3000, DELAY_LOW, 400, 0, 0,
     MOD_GPG40}, // WP_GPG40					// 40

    {4, 1, 1, 4, 1, 3000, DELAY_LOW, 400, 0, 0,
     MOD_M7}, // WP_M7					// 41
    {24, 1, 8, 24, 8, 1500, DELAY_PISTOL, 400, 0, 0,
     MOD_SILENCED_COLT}, // WP_SILENCED_COLT			// 42
    {24, 1, 8, 16, 8, 1500, 0, 400, 0, 0,
     MOD_GARAND_SCOPE}, // WP_GARAND_SCOPE			// 43
                        // GARAND
    {30, 1, 10, 20, 10, 2500, 0, 400, 0, 0,
     MOD_K43_SCOPE}, // WP_K43_SCOPE				// 44
                     // K43
    {60, 1, 20, 40, 20, 2000, DELAY_LOW, 400, 0, 0,
     MOD_FG42SCOPE}, // WP_FG42SCOPE				// 45
    {16, 1, 1, 12, 0, 0, 750, 1400, 0, 0,
     MOD_MORTAR}, // WP_MORTAR_SET			// 44
    {10, 0, 1, 0, 1, 1500, 50, 1000, 0, 0,
     MOD_SYRINGE}, // WP_MEDIC_ADRENALINE		// 47
    {48, 1, 8, 48, 8, 2700, DELAY_PISTOL, 200, 0, 0,
     MOD_AKIMBO_SILENCEDCOLT}, // WP_AKIMBO_SILENCEDCOLT	// 48
    {48, 1, 8, 48, 8, 2700, DELAY_PISTOL, 200, 0, 0,
     MOD_AKIMBO_SILENCEDLUGER}, // WP_AKIMBO_SILENCEDLUGER	// 49
    {450, 1, 150, 0, 150, 3000, DELAY_LOW, 66, 1500, 300,
     MOD_MOBILE_MG42}, // WP_MOBILE_MG42_SET		// 50
};

//----(SA)	moved in here so both games can get to it
int weapAlts[] = {
    WP_NONE,     // 0 WP_NONE
    WP_NONE,     // 1 WP_KNIFE
    WP_SILENCER, // 2 WP_LUGER
    WP_NONE,     // 3 WP_MP40
    WP_NONE,     // 4 WP_GRENADE_LAUNCHER
    WP_NONE,     // 5 WP_PANZERFAUST
    WP_NONE,     // 6 WP_FLAMETHROWER

    WP_SILENCED_COLT, // 7 WP_COLT
    WP_NONE,          // 8 WP_THOMPSON
    WP_NONE,          // 9 WP_GRENADE_PINEAPPLE
    WP_NONE,          // 10 WP_STEN
    WP_NONE,          // 11 WP_MEDIC_SYRINGE	// JPW NERVE
    WP_NONE,          // 12 WP_AMMO		// JPW NERVE
    WP_NONE,          // 13 WP_ARTY		// JPW NERVE

    WP_LUGER, // 14 WP_SILENCER	//----(SA)	was sp5
    WP_NONE,  // 15 WP_DYNAMITE	//----(SA)	modified (not in rotation yet)
    WP_NONE,  // 16 WP_SMOKETRAIL
    WP_NONE,  // 17 WP_MAPMORTAR
    WP_NONE,  // 18 VERYBIGEXPLOSION
    WP_NONE,  // 19 WP_MEDKIT
    WP_NONE,  // 20 WP_BINOCULARS

    WP_NONE,         // 21 WP_PLIERS
    WP_NONE,         // 22 WP_SMOKE_MARKER
    WP_GPG40,        // 23 WP_KAR98
    WP_M7,           // 24 WP_CARBINE (GARAND really)
    WP_GARAND_SCOPE, // 25 WP_GARAND
    WP_NONE,         // 26 WP_LANDMINE
    WP_NONE,         // 27 WP_SATCHEL
    WP_NONE,         // 28 WP_SATCHEL_DET
    WP_NONE,         // 29 WP_TRIPMINE

    WP_NONE,            // 30 WP_SMOKE_BOMB
    WP_MOBILE_MG42_SET, // 31 WP_MOBILE_MG42
    WP_K43_SCOPE,       // 32 WP_K43
    WP_FG42SCOPE,       // 33 WP_FG42
    WP_NONE,            // 34 WP_DUMMY_MG42
    WP_MORTAR_SET,      // 35 WP_MORTAR
    WP_NONE,            // 36 WP_LOCKPICK Mad Doc - TDF
    WP_NONE,            // 37 WP_AKIMBO_COLT
    WP_NONE,            // 38 WP_AKIMBO_LUGER

    // Feen: PGM
    WP_NONE, // 39 WP_PORTAL_GUN
    // END PGM

    WP_KAR98,       // 39 WP_GPG40 //Feen: +1 from this point on...
    WP_CARBINE,     // 40 WP_M7
    WP_COLT,        // 41 WP_SILENCED_COLT
    WP_GARAND,      // 42 WP_GARAND_SCOPE
    WP_K43,         // 43 WP_K43_SCOPE
    WP_FG42,        // 44 WP_FG42SCOPE
    WP_MORTAR,      // 45 WP_MORTAR_SET
    WP_NONE,        // 46 WP_MEDIC_ADRENALINE
    WP_NONE,        // 47 WP_AKIMBO_SILENCEDCOLT
    WP_NONE,        // 48 WP_AKIMBO_SILENCEDLUGER
    WP_MOBILE_MG42, // 49 WP_MOBILE_MG42_SET
};

// new (10/18/00)
const char *animStrings[] = {
    "BOTH_DEATH1",
    "BOTH_DEAD1",
    "BOTH_DEAD1_WATER",
    "BOTH_DEATH2",
    "BOTH_DEAD2",
    "BOTH_DEAD2_WATER",
    "BOTH_DEATH3",
    "BOTH_DEAD3",
    "BOTH_DEAD3_WATER",

    "BOTH_CLIMB",
    "BOTH_CLIMB_DOWN",
    "BOTH_CLIMB_DISMOUNT",

    "BOTH_SALUTE",

    "BOTH_PAIN1",
    "BOTH_PAIN2",
    "BOTH_PAIN3",
    "BOTH_PAIN4",
    "BOTH_PAIN5",
    "BOTH_PAIN6",
    "BOTH_PAIN7",
    "BOTH_PAIN8",

    "BOTH_GRAB_GRENADE",

    "BOTH_ATTACK1",
    "BOTH_ATTACK2",
    "BOTH_ATTACK3",
    "BOTH_ATTACK4",
    "BOTH_ATTACK5",

    "BOTH_EXTRA1",
    "BOTH_EXTRA2",
    "BOTH_EXTRA3",
    "BOTH_EXTRA4",
    "BOTH_EXTRA5",
    "BOTH_EXTRA6",
    "BOTH_EXTRA7",
    "BOTH_EXTRA8",
    "BOTH_EXTRA9",
    "BOTH_EXTRA10",
    "BOTH_EXTRA11",
    "BOTH_EXTRA12",
    "BOTH_EXTRA13",
    "BOTH_EXTRA14",
    "BOTH_EXTRA15",
    "BOTH_EXTRA16",
    "BOTH_EXTRA17",
    "BOTH_EXTRA18",
    "BOTH_EXTRA19",
    "BOTH_EXTRA20",

    "TORSO_GESTURE",
    "TORSO_GESTURE2",
    "TORSO_GESTURE3",
    "TORSO_GESTURE4",

    "TORSO_DROP",

    "TORSO_RAISE", // (low)
    "TORSO_ATTACK",
    "TORSO_STAND",
    "TORSO_STAND_ALT1",
    "TORSO_STAND_ALT2",
    "TORSO_READY",
    "TORSO_RELAX",

    "TORSO_RAISE2", // (high)
    "TORSO_ATTACK2",
    "TORSO_STAND2",
    "TORSO_STAND2_ALT1",
    "TORSO_STAND2_ALT2",
    "TORSO_READY2",
    "TORSO_RELAX2",

    "TORSO_RAISE3", // (pistol)
    "TORSO_ATTACK3",
    "TORSO_STAND3",
    "TORSO_STAND3_ALT1",
    "TORSO_STAND3_ALT2",
    "TORSO_READY3",
    "TORSO_RELAX3",

    "TORSO_RAISE4", // (shoulder)
    "TORSO_ATTACK4",
    "TORSO_STAND4",
    "TORSO_STAND4_ALT1",
    "TORSO_STAND4_ALT2",
    "TORSO_READY4",
    "TORSO_RELAX4",

    "TORSO_RAISE5", // (throw)
    "TORSO_ATTACK5",
    "TORSO_ATTACK5B",
    "TORSO_STAND5",
    "TORSO_STAND5_ALT1",
    "TORSO_STAND5_ALT2",
    "TORSO_READY5",
    "TORSO_RELAX5",

    "TORSO_RELOAD1", // (low)
    "TORSO_RELOAD2", // (high)
    "TORSO_RELOAD3", // (pistol)
    "TORSO_RELOAD4", // (shoulder)

    "TORSO_MG42", // firing tripod mounted weapon animation

    "TORSO_MOVE", // torso anim to play while moving and not firing (swinging
                  // arms type thing)
    "TORSO_MOVE_ALT", // torso anim to play while moving and not firing
                      // (swinging arms type thing)

    "TORSO_EXTRA",
    "TORSO_EXTRA2",
    "TORSO_EXTRA3",
    "TORSO_EXTRA4",
    "TORSO_EXTRA5",
    "TORSO_EXTRA6",
    "TORSO_EXTRA7",
    "TORSO_EXTRA8",
    "TORSO_EXTRA9",
    "TORSO_EXTRA10",

    "LEGS_WALKCR",
    "LEGS_WALKCR_BACK",
    "LEGS_WALK",
    "LEGS_RUN",
    "LEGS_BACK",
    "LEGS_SWIM",
    "LEGS_SWIM_IDLE",

    "LEGS_JUMP",
    "LEGS_JUMPB",
    "LEGS_LAND",

    "LEGS_IDLE",
    "LEGS_IDLE_ALT", //	"LEGS_IDLE2"
    "LEGS_IDLECR",

    "LEGS_TURN",

    "LEGS_BOOT", // kicking animation

    "LEGS_EXTRA1",
    "LEGS_EXTRA2",
    "LEGS_EXTRA3",
    "LEGS_EXTRA4",
    "LEGS_EXTRA5",
    "LEGS_EXTRA6",
    "LEGS_EXTRA7",
    "LEGS_EXTRA8",
    "LEGS_EXTRA9",
    "LEGS_EXTRA10",
};

// old
const char *animStringsOld[] = {
    "BOTH_DEATH1",
    "BOTH_DEAD1",
    "BOTH_DEATH2",
    "BOTH_DEAD2",
    "BOTH_DEATH3",
    "BOTH_DEAD3",

    "BOTH_CLIMB",
    "BOTH_CLIMB_DOWN",
    "BOTH_CLIMB_DISMOUNT",

    "BOTH_SALUTE",

    "BOTH_PAIN1",
    "BOTH_PAIN2",
    "BOTH_PAIN3",
    "BOTH_PAIN4",
    "BOTH_PAIN5",
    "BOTH_PAIN6",
    "BOTH_PAIN7",
    "BOTH_PAIN8",

    "BOTH_EXTRA1",
    "BOTH_EXTRA2",
    "BOTH_EXTRA3",
    "BOTH_EXTRA4",
    "BOTH_EXTRA5",

    "TORSO_GESTURE",
    "TORSO_GESTURE2",
    "TORSO_GESTURE3",
    "TORSO_GESTURE4",

    "TORSO_DROP",

    "TORSO_RAISE", // (low)
    "TORSO_ATTACK",
    "TORSO_STAND",
    "TORSO_READY",
    "TORSO_RELAX",

    "TORSO_RAISE2", // (high)
    "TORSO_ATTACK2",
    "TORSO_STAND2",
    "TORSO_READY2",
    "TORSO_RELAX2",

    "TORSO_RAISE3", // (pistol)
    "TORSO_ATTACK3",
    "TORSO_STAND3",
    "TORSO_READY3",
    "TORSO_RELAX3",

    "TORSO_RAISE4", // (shoulder)
    "TORSO_ATTACK4",
    "TORSO_STAND4",
    "TORSO_READY4",
    "TORSO_RELAX4",

    "TORSO_RAISE5", // (throw)
    "TORSO_ATTACK5",
    "TORSO_ATTACK5B",
    "TORSO_STAND5",
    "TORSO_READY5",
    "TORSO_RELAX5",

    "TORSO_RELOAD1", // (low)
    "TORSO_RELOAD2", // (high)
    "TORSO_RELOAD3", // (pistol)
    "TORSO_RELOAD4", // (shoulder)

    "TORSO_MG42", // firing tripod mounted weapon animation

    "TORSO_MOVE", // torso anim to play while moving and not firing (swinging
                  // arms type thing)

    "TORSO_EXTRA2",
    "TORSO_EXTRA3",
    "TORSO_EXTRA4",
    "TORSO_EXTRA5",

    "LEGS_WALKCR",
    "LEGS_WALKCR_BACK",
    "LEGS_WALK",
    "LEGS_RUN",
    "LEGS_BACK",
    "LEGS_SWIM",

    "LEGS_JUMP",
    "LEGS_LAND",

    "LEGS_IDLE",
    "LEGS_IDLE2",
    "LEGS_IDLECR",

    "LEGS_TURN",

    "LEGS_BOOT", // kicking animation

    "LEGS_EXTRA1",
    "LEGS_EXTRA2",
    "LEGS_EXTRA3",
    "LEGS_EXTRA4",
    "LEGS_EXTRA5",
};

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) SUSPENDED SPIN PERSISTANT
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
SUSPENDED - will allow items to hang in the air, otherwise they are dropped to
the next surface. SPIN - will allow items to spin in place. PERSISTANT - some
items (ex. clipboards) can be picked up, but don't disappear

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't
carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn
automatically, which can be used with targeted spawning. "random" random number
of plus or minus seconds varied from the respawn time "count" override quantity
or duration on most items. "stand" if the item has a stand (ex: mp40_stand.md3)
this specifies which stand tag to attach the weapon to ("stand":"4" would mean
"tag_stand4" for example)  only weapons support stands currently
*/

// JOSEPH 5-2-00
//----(SA) the addition of the 'ammotype' field was added by me, not removed by
// id (SA)
gitem_t bg_itemlist[] = {
    {
        NULL,
        NULL,
        {0, 0, 0},
        NULL, // icon
        NULL, // ammo icon
        NULL, // pickup
        0,
        IT_BAD,
        0,
        0,  // ammotype
        0,  // cliptype
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },      // leave index 0 alone

    /*QUAKED item_treasure (1 1 0) (-8 -8 -8) (8 8 8) suspended
    Items the player picks up that are just used to tally a score at end-level
    "model" defaults to 'models/powerups/treasure/goldbar.md3'
    "noise" sound to play on pickup.  defaults to
    'sound/pickup/treasure/gold.wav' "message" what to call the item when it's
    picked up.  defaults to "Treasure Item" (SA: temp)
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/treasure/goldbar.md3"
    */
    /*
    "scriptName"
    */
    {
        "item_treasure",
        "sound/pickup/treasure/gold.wav",
        {"models/powerups/treasure/goldbar.md3", 0, 0},
        NULL,            // (SA) placeholder
        NULL,            // ammo icon
        "Treasure Item", // (SA) placeholder
        5,
        IT_TREASURE,
        0,
        0,
        0,
        "",
        "",
        //		{0,0,0,0,0}
    },

    //
    // ARMOR/HEALTH/STAMINA
    //

    /*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/health/health_s.md3"
    */
    {
        "item_health_small",
        "sound/items/n_health.wav",
        {"models/powerups/health/health_s.md3", 0, 0},
        NULL,
        NULL, // ammo icon
        "Small Health",
        5,
        IT_HEALTH,
        0,
        0,
        0,
        "",
        "",
        //		{10,5,5,5,5}
    },

    /*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/health/health_m.md3"
    */
    {
        "item_health",
        "sound/misc/health_pickup.wav",
        //		"sound/multiplayer/health_pickup.wav",
        {"models/multiplayer/medpack/medpack_pickup.md3", // JPW NERVE was
                                                          // "models/powerups/health/health_m.md3",
         0, 0},
        NULL,
        NULL, // ammo icon
        "Med Health",
        20,
        IT_HEALTH,
        0,
        0,
        0,
        "",
        "",
        //		{50,25,20,15,15}
    },

    /*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/health/health_m.md3"
    */
    {
        "item_health_large",
        "sound/misc/health_pickup.wav",
        //		"sound/multiplayer/health_pickup.wav",
        {"models/multiplayer/medpack/medpack_pickup.md3", // JPW NERVE was
                                                          // "models/powerups/health/health_m.md3",
         0, 0},
        NULL,
        NULL, // ammo icon
        "Med Health",
        50, // xkan, 12/20/2002 - increased to 50 from 30 and used it for SP.
        IT_HEALTH,
        0,
        0,
        0,
        "",
        "",
        //		{50,25,20,15,15}
    },

    {
        "item_health_cabinet",
        "sound/misc/health_pickup.wav",
        //		"sound/multiplayer/health_pickup.wav",
        {0, 0, 0},
        NULL,
        NULL, // ammo icon
        "Health",
        0,
        IT_WEAPON,
        0,
        0,
        0,
        "",
        "",
    },

    /*QUAKED item_health_turkey (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    multi-stage health item.
    gives 40 on first use, then gives 20 on "finishing up"

    player will only eat what he needs.  health at 90, turkey fills up and
    leaves remains (leaving 15).  health at 5 you eat the whole thing.
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/health/health_t1.md3"
    */
    {
        "item_health_turkey",
        "sound/items/hot_pickup.wav",
        {
            "models/powerups/health/health_t3.md3", // just plate (should now be
                                                    // destructable)
            "models/powerups/health/health_t2.md3", // half eaten
            "models/powerups/health/health_t1.md3"  // whole turkey
        },
        NULL,
        NULL, // ammo icon
        "Hot Meal",
        20, // amount given in last stage
        IT_HEALTH,
        0,
        0,
        0,
        "",
        "",
        //		{50,50,50,40,30}	// amount given in first stage
        // based on gameskill level
    },

    // xkan, 1/6/2002 - updated
    /*QUAKED item_health_breadandmeat (.3 .3 1) (-16 -16 -16) (16 16 16)
    SUSPENDED SPIN - RESPAWN multi-stage health item. gives 30 on first use,
    then gives 15 on "finishing up"
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/health/health_b1.md3"
    */
    {
        "item_health_breadandmeat",
        "sound/items/cold_pickup.wav",
        {
            "models/powerups/health/health_b3.md3", // just plate (should now be
                                                    // destructable)
            "models/powerups/health/health_b2.md3", // half eaten
            "models/powerups/health/health_b1.md3"  // whole turkey
        },
        NULL,
        NULL, // ammo icon
        "Cold Meal",
        15, // amount given in last stage
        IT_HEALTH,
        0,
        0,
        0,
        "",
        "",
        //{30,30,20,15}	// amount given in first stage based on gameskill level
    },

    // xkan, 1/6/2002 - updated
    /*QUAKED item_health_wall (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    defaults to 50 pts health
    you will probably want to check the 'suspended' box to keep it from falling
    to the ground
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/health/health_w.md3"
    */
    {
        "item_health_wall",
        "sound/items/n_health.wav",
        {"models/powerups/health/health_w.md3", 0, 0},
        NULL,
        NULL, // ammo icon
        "Health",
        25,
        IT_HEALTH,
        0,
        0,
        0,
        "",
        "",
        //		{25,25,25,25,25}
    },

    //
    // STAMINA
    //

    //
    // WEAPONS
    //
    // wolf weapons (SA)

    /*QUAKED weapon_knife (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/knife/knife.md3"
    */
    {
        "weapon_knife",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/knife/knife.md3",
         "models/multiplayer/knife/v_knife.md3", 0},

        "icons/iconw_knife_1", // icon
        "icons/ammo2",         // ammo icon
        "Knife",               // pickup
        50,
        IT_WEAPON,
        WP_KNIFE,
        WP_KNIFE,
        WP_KNIFE,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_luger (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/luger/luger.md3"
    */
    {
        "weapon_luger",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/luger/luger.md3", "models/weapons2/luger/v_luger.md3",
         0},

        "",            // icon
        "icons/ammo2", // ammo icon
        "Luger",       // pickup
        50,
        IT_WEAPON,
        WP_LUGER,
        WP_LUGER,
        WP_LUGER,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_akimboluger (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/akimbo_luger/luger.md3"
    */
    {
        "weapon_akimboluger",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/luger/luger.md3",
         "models/weapons2/akimbo_luger/v_akimbo_luger.md3", 0},

        "icons/iconw_colt_1", // icon
                              // // FIXME: need new icon
        "icons/ammo2",        // ammo icon
        "Akimbo Luger",       // pickup
        50,
        IT_WEAPON,
        WP_AKIMBO_LUGER,
        WP_LUGER,
        WP_AKIMBO_LUGER,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_akimbosilencedluger (.3 .3 1) (-16 -16 -16) (16 16 16)
    suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/akimbo_luger/luger.md3"
    */
    {
        "weapon_akimbosilencedluger",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/luger/luger.md3",
         "models/weapons2/akimbo_luger/v_akimbo_luger.md3", 0},

        "icons/iconw_colt_1",    // icon
                                 // // FIXME: need new icon
        "icons/ammo2",           // ammo icon
        "Silenced Akimbo Luger", // pickup
        50,
        IT_WEAPON,
        WP_AKIMBO_SILENCEDLUGER,
        WP_LUGER,
        WP_AKIMBO_LUGER,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_thompson (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/thompson/thompson.md3"
    */
    {
        "weapon_thompson",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/thompson/thompson.md3",
         //			"models/multiplayer/mg42/v_mg42.md3",
         "models/weapons2/thompson/v_thompson.md3", 0},

        "icons/iconw_thompson_1", // icon
        "icons/ammo2",            // ammo icon
        "Thompson",               // pickup
        30,
        IT_WEAPON,
        WP_THOMPSON,
        WP_THOMPSON,
        WP_THOMPSON,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    {
        "weapon_dummy",
        "",
        {0, 0, 0},

        "",            // icon
        "",            // ammo icon
        "BLANK",       // pickup
        0,             // quantity
        IT_WEAPON,     // item type
        WP_DUMMY_MG42, // giTag
        WP_DUMMY_MG42, // giAmmoIndex
        WP_DUMMY_MG42, // giClipIndex
        "",            // precache
        "",            // sounds
    },

    /*QUAKED weapon_sten (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/sten/sten.md3"
    */
    {
        "weapon_sten",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/sten/sten.md3", "models/weapons2/sten/v_sten.md3", 0},
        "icons/iconw_sten_1", // icon
        "icons/ammo2",        // ammo icon
        "Sten",               // pickup
        30,
        IT_WEAPON,
        WP_STEN,
        WP_STEN,
        WP_STEN,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_colt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/colt/colt.md3"
    */
    {
        "weapon_colt",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/colt/colt.md3", "models/weapons2/colt/v_colt.md3", 0},

        "icons/iconw_colt_1", // icon
        "icons/ammo2",        // ammo icon
        "Colt",               // pickup
        50,
        IT_WEAPON,
        WP_COLT,
        WP_COLT,
        WP_COLT,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_akimbocolt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/akimbo_colt/colt.md3"
    */
    {
        "weapon_akimbocolt",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/colt/colt.md3",
         "models/weapons2/akimbo_colt/v_akimbo_colt.md3", 0},

        "icons/iconw_colt_1", // icon
                              // // FIXME: need new icon
        "icons/ammo2",        // ammo icon
        "Akimbo Colt",        // pickup
        50,
        IT_WEAPON,
        WP_AKIMBO_COLT,
        WP_COLT,
        WP_AKIMBO_COLT,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_akimbosilencedcolt (.3 .3 1) (-16 -16 -16) (16 16 16)
    suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/akimbo_colt/colt.md3"
    */
    {
        "weapon_akimbosilencedcolt",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/colt/colt.md3",
         "models/weapons2/akimbo_colt/v_akimbo_colt.md3", 0},

        "icons/iconw_colt_1",   // icon
                                // // FIXME: need new icon
        "icons/ammo2",          // ammo icon
        "Silenced Akimbo Colt", // pickup
        50,
        IT_WEAPON,
        WP_AKIMBO_SILENCEDCOLT,
        WP_COLT,
        WP_AKIMBO_COLT,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_mp40 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    "stand" values:
        no value:	laying in a default position on it's side (default)
        2:			upright, barrel pointing up, slightly angled
    (rack mount)
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models\weapons2\mp40\mp40.md3"
    */
    {
        "weapon_mp40",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/mp40/mp40.md3", "models/weapons2/mp40/v_mp40.md3", 0},

        "icons/iconw_mp40_1", // icon
        "icons/ammo2",        // ammo icon
        "MP40",               // pickup
        30,
        IT_WEAPON,
        WP_MP40,
        WP_MP40,
        WP_MP40,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_panzerfaust (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/panzerfaust/pf.md3"
    */
    {
        "weapon_panzerfaust",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/panzerfaust/pf.md3",
         "models/weapons2/panzerfaust/v_pf.md3", 0},

        "icons/iconw_panzerfaust_1", // icon
        "icons/ammo6",               // ammo icon
        "Panzerfaust",               // pickup
        1,
        IT_WEAPON,
        WP_PANZERFAUST,
        WP_PANZERFAUST,
        WP_PANZERFAUST,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    //----(SA)	removed the quaked for this.  we don't actually have a grenade
    // launcher as such.  It's given implicitly 			by
    // virtue of getting grenade ammo.  So we don't need to have them in maps
    /*
    weapon_grenadelauncher
    */
    {
        "weapon_grenadelauncher",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/grenade/grenade.md3",
         "models/weapons2/grenade/v_grenade.md3", 0},

        "icons/iconw_grenade_1", // icon
        "icons/icona_grenade",   // ammo icon
        "Grenade",               // pickup
        6,
        IT_WEAPON,
        WP_GRENADE_LAUNCHER,
        WP_GRENADE_LAUNCHER,
        WP_GRENADE_LAUNCHER,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*
    weapon_grenadePineapple
    */
    {
        "weapon_grenadepineapple",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/grenade/pineapple.md3",
         "models/weapons2/grenade/v_pineapple.md3", 0},

        "icons/iconw_pineapple_1", // icon
        "icons/icona_pineapple",   // ammo icon
        "Pineapple",               // pickup
        6,
        IT_WEAPON,
        WP_GRENADE_PINEAPPLE,
        WP_GRENADE_PINEAPPLE,
        WP_GRENADE_PINEAPPLE,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /* JPW NERVE
    weapon_grenadesmoke
    */
    {
        "weapon_grenadesmoke",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/smokegrenade/smokegrenade.md3",
         "models/multiplayer/smokegrenade/v_smokegrenade.md3", 0},

        "icons/iconw_smokegrenade_1", // icon
        "icons/ammo2",                // ammo icon
        "smokeGrenade",               // pickup
        50,
        IT_WEAPON,
        WP_SMOKE_MARKER,
        WP_SMOKE_MARKER,
        WP_SMOKE_MARKER,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },
    // jpw

    /* JPW NERVE
    weapon_smoketrail -- only used as a special effects emitter for smoke trails
    (artillery spotter etc)
    */
    {
        "weapon_smoketrail",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/smokegrenade/smokegrenade.md3",
         "models/multiplayer/smokegrenade/v_smokegrenade.md3", 0},

        "icons/iconw_smokegrenade_1", // icon
        "icons/ammo2",                // ammo icon
        "smokeTrail",                 // pickup
        50,
        IT_WEAPON,
        WP_SMOKETRAIL,
        WP_SMOKETRAIL,
        WP_SMOKETRAIL,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },
    // jpw

    // DHM - Nerve
    /*
    weapon_medic_heal
    */
    {
        "weapon_medic_heal",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/medpack/medpack.md3",
         "models/multiplayer/medpack/v_medpack.md3", 0},

        "icons/iconw_medheal_1", // icon
        "icons/ammo2",           // ammo icon
        "medicheal",             // pickup
        50,
        IT_WEAPON,
        WP_MEDKIT,
        WP_MEDKIT,
        WP_MEDKIT,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },
    // dhm

    /*
    weapon_dynamite
    */
    {
        "weapon_dynamite",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/dynamite/dynamite_3rd.md3", // JPW NERVE
         "models/weapons2/dynamite/v_dynamite.md3",      // JPW NERVE
         0},

        "icons/iconw_dynamite_1", // icon
        "icons/ammo9",            // ammo icon
        "Dynamite Weapon",        // pickup
        7,
        IT_WEAPON,
        WP_DYNAMITE,
        WP_DYNAMITE,
        WP_DYNAMITE,
        "models/multiplayer/dynamite/dynamite.md3 "
        "models/multiplayer/dynamite/dynamite_3rd.md3", // precache // JPW NERVE
        "",                                             // sounds
                                                        //		{0,0,0,0,0}
    },

    /*QUAKED weapon_flamethrower (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/flamethrower/flamethrower.md3"
    */
    {
        "weapon_flamethrower",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/flamethrower/flamethrower.md3",
         "models/weapons2/flamethrower/v_flamethrower.md3",
         "models/weapons2/flamethrower/pu_flamethrower.md3"},

        "icons/iconw_flamethrower_1", // icon
        "icons/ammo10",               // ammo icon
        "Flamethrower",               // pickup
        200,
        IT_WEAPON,
        WP_FLAMETHROWER,
        WP_FLAMETHROWER,
        WP_FLAMETHROWER,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*
    weapon_mortar (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    */
    {
        "weapon_mapmortar",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/grenade/grenade.md3",
         "models/weapons2/grenade/v_grenade.md3", 0},
        "icons/iconw_grenade_1",  // icon
        "icons/icona_grenade",    // ammo icon
        "nopickup(WP_MAPMORTAR)", // pickup
        6,
        IT_WEAPON,
        WP_MAPMORTAR,
        WP_MAPMORTAR,
        WP_MAPMORTAR,
        "",                                  // precache
        "sound/weapons/mortar/mortarf1.wav", // sounds
                                             //		{0,0,0,0,0}
    },

    // JPW NERVE -- class-specific multiplayer weapon, can't be picked up,
    // dropped, or placed in map
    /*
    weapon_class_special (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    */
    {
        "weapon_class_special",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/pliers/pliers.md3",
         "models/multiplayer/pliers/v_pliers.md3", 0},

        "icons/iconw_pliers_1", // icon
        "icons/ammo2",          // ammo icon
        "Special",              // pickup
        50,                     // this should never be picked up
        IT_WEAPON,
        WP_PLIERS,
        WP_PLIERS,
        WP_PLIERS,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*
    weapon_arty (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    */
    {
        "weapon_arty",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/syringe/syringe.md3",
         "models/multiplayer/syringe/v_syringe.md3", 0},

        "icons/iconw_syringe_1", // icon
        "icons/ammo2",           // ammo icon
        "Artillery",             // pickup
        50,                      // this should never be picked up
        IT_WEAPON,
        WP_ARTY,
        WP_ARTY,
        WP_ARTY,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*
    weapon_medic_syringe (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    */
    {
        "weapon_medic_syringe",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/syringe/syringe.md3",
         "models/multiplayer/syringe/v_syringe.md3", 0},

        "icons/iconw_syringe_1", // icon
        "icons/ammo2",           // ammo icon
        "Syringe",               // pickup
        50,                      // this should never be picked up
        IT_WEAPON,
        WP_MEDIC_SYRINGE,
        WP_MEDIC_SYRINGE,
        WP_MEDIC_SYRINGE,
        "",                         // precache
        "sound/misc/vo_revive.wav", // sounds
                                    //		{0,0,0,0,0}
    },

    /*
    weapon_medic_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    */
    {
        "weapon_medic_adrenaline",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/syringe/syringe.md3",
         "models/multiplayer/syringe/v_syringe.md3", 0},

        "icons/iconw_syringe_1", // icon
        "icons/ammo2",           // ammo icon
        "Adrenaline Syringe",    // pickup
        50,                      // this should never be picked up
        IT_WEAPON,
        WP_MEDIC_ADRENALINE,
        WP_MEDIC_SYRINGE,
        WP_MEDIC_SYRINGE,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },
    /*
    weapon_magicammo (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    */
    {
        "weapon_magicammo",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/ammopack/ammopack.md3",
         "models/multiplayer/ammopack/v_ammopack.md3",
         "models/multiplayer/ammopack/ammopack_pickup.md3"},

        "icons/iconw_ammopack_1", // icon
        "icons/ammo2",            // ammo icon
        "Ammo Pack",              // pickup
        50,                       // this should never be picked up
        IT_WEAPON,
        WP_AMMO,
        WP_AMMO,
        WP_AMMO,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    {
        "weapon_magicammo2",
        "sound/misc/w_pkup.wav",
        {
            "models/multiplayer/binocs/v_binocs.md3",
            "models/multiplayer/binocs/v_binocs.md3",
            "models/multiplayer/binocs/v_binocs.md3",
            //			"models/multiplayer/ammopack/ammopack.md3",
            //			"models/multiplayer/ammopack/v_ammopack.md3",
            //			"models/multiplayer/ammopack/ammopack_pickup_s.md3"
        },

        "icons/iconw_ammopack_1", // icon
        "icons/ammo2",            // ammo icon
        "Mega Ammo Pack",         // pickup
        50,                       // this should never be picked up
        IT_WEAPON,
        WP_AMMO,
        WP_AMMO,
        WP_AMMO,
        "", // precache
        "", // sounds
    },

    /*
    weapon_binoculars (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    */
    {
        "weapon_binoculars",
        "sound/misc/w_pkup.wav",
        {"", "models/multiplayer/binocs/v_binocs.md3", 0},

        "",           // icon
        "",           // ammo icon
        "Binoculars", // pickup
        50,           // this should never be picked up
        IT_WEAPON,
        WP_BINOCULARS,
        WP_BINOCULARS,
        WP_BINOCULARS,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_k43 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model=""
    */
    {
        "weapon_kar43",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/kar98/kar98_3rd.md3",
         "models/multiplayer/kar98/v_kar98.md3",
         "models/multiplayer/mauser/mauser_pickup.md3"},

        "icons/iconw_mauser_1", // icon
        "icons/ammo3",          // ammo icon
        "K43 Rifle",            // pickup
        50,
        IT_WEAPON,
        WP_K43,
        WP_K43,
        WP_K43,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_kar43_scope (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model=""
    */
    {
        "weapon_kar43_scope",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/kar98/kar98_3rd.md3",
         "models/multiplayer/kar98/v_kar98.md3",
         "models/multiplayer/mauser/mauser_pickup.md3"},

        "icons/iconw_mauser_1", // icon
        "icons/ammo3",          // ammo icon
        "K43 Rifle Scope",      // pickup
        50,
        IT_WEAPON,
        WP_K43_SCOPE,
        WP_K43,
        WP_K43,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_kar98Rifle (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/mauser/mauser.md3"
    */
    {
        "weapon_kar98Rifle",
        "sound/misc/w_pkup.wav",
        /*        {
                    "models/weapons2/mauser/kar98.md3",
                    "models/multiplayer/kar98/v_kar98.md3",
                    "models/multiplayer/mauser/kar98_pickup.md3"
                },

                "icons/iconw_kar98_1",	// icon
                "icons/ammo3",			// ammo icon*/
        {"models/multiplayer/kar98/kar98_3rd.md3",
         "models/multiplayer/kar98/v_kar98.md3",
         "models/multiplayer/mauser/mauser_pickup.md3"},

        "icons/iconw_kar98_1", // icon
        "icons/ammo3",         // ammo icon
        "K43",                 // pickup
        50,
        IT_WEAPON,
        WP_KAR98,
        WP_KAR98,
        WP_KAR98,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_gpg40 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/mauser/mauser.md3"
    */
    {
        "weapon_gpg40",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/kar98/kar98_3rd.md3",
         "models/multiplayer/kar98/v_kar98.md3",
         "models/multiplayer/mauser/mauser_pickup.md3"},

        "icons/iconw_kar98_1", // icon
        "icons/ammo10",        // ammo icon
        "GPG40",               // pickup
        200,
        IT_WEAPON,
        WP_GPG40,
        WP_GPG40,
        WP_GPG40,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_gpg40_allied (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/mauser/mauser.md3"
    */
    {
        "weapon_gpg40_allied",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/m1_garand/m1_garand_3rd.md3",
         "models/multiplayer/m1_garand/v_m1_garand.md3",
         "models/multiplayer/mauser/mauser_pickup.md3"},

        "icons/iconw_m1_garand_1", // icon
        "icons/ammo10",            // ammo icon
        "GPG40A",                  // pickup
        200,
        IT_WEAPON,
        WP_M7,
        WP_M7,
        WP_M7,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_M1CarbineRifle (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/mauser/mauser.md3"
    */
    {
        "weapon_M1CarbineRifle",
        "sound/misc/w_pkup.wav",
        /*        {
                    "models/weapons2/mauser/mauser.md3",
                    "models/weapons2/mauser/v_mauser.md3",
                    "models/multiplayer/mauser/mauser_pickup.md3"
                },*/
        {"models/multiplayer/m1_garand/m1_garand_3rd.md3",
         "models/multiplayer/m1_garand/v_m1_garand.md3",
         "models/multiplayer/mauser/mauser_pickup.md3"},

        "icons/iconw_m1_garand_1", // icon
        "icons/ammo3",             // ammo icon
        "M1 Garand",               // pickup
        50,
        IT_WEAPON,
        WP_CARBINE,
        WP_CARBINE,
        WP_CARBINE,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*
    weapon_garandRifle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/garand/garand.md3"
    */
    {
        "weapon_garandRifle",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/m1_garand/m1_garand_3rd.md3",
         "models/multiplayer/m1_garand/v_m1_garand.md3",
         "models/multiplayer/mauser/mauser_pickup.md3"},

        "icons/iconw_mauser_1", // icon
        "icons/ammo3",          // ammo icon
        "Garand",               // pickup
        50,
        IT_WEAPON,
        WP_GARAND,
        WP_GARAND,
        WP_GARAND,
        "", // precache
        "", // sounds
            //		{0,0,0,0}
    },

    /*
    weapon_garandRifleScope (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/garand/garand.md3"
    */
    {
        "weapon_garandRifleScope",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/m1_garand/m1_garand_3rd.md3",
         "models/multiplayer/m1_garand/v_m1_garand.md3",
         "models/multiplayer/mauser/mauser_pickup.md3"},

        "icons/iconw_mauser_1", // icon
        "icons/ammo3",          // ammo icon
        "M1 Garand Scope",      // pickup
        50,
        IT_WEAPON,
        WP_GARAND_SCOPE,
        WP_GARAND,
        WP_GARAND,
        "", // precache
        "", // sounds
            //		{0,0,0,0}
    },

    /*QUAKED weapon_fg42 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/fg42/fg42.md3"
    */
    {
        "weapon_fg42",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/fg42/fg42.md3", "models/weapons2/fg42/v_fg42.md3",
         "models/weapons2/fg42/pu_fg42.md3"},

        "icons/iconw_fg42_1",   // icon
        "icons/ammo5",          // ammo icon
        "FG42 Paratroop Rifle", // pickup
        10,
        IT_WEAPON,
        WP_FG42,
        WP_FG42,
        WP_FG42,
        "", // precache
        "", // sounds
            //		{0,0,0,0}
    },

    /*QUAKED weapon_fg42scope (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/fg42/fg42.md3"
    */
    {
        "weapon_fg42scope", //----(SA)	modified
        "sound/misc/w_pkup.wav",
        {"models/weapons2/fg42/fg42.md3", "models/weapons2/fg42/v_fg42.md3",
         "models/weapons2/fg42/pu_fg42.md3"},

        "icons/iconw_fg42_1", // icon
        "icons/ammo5",        // ammo icon
        "FG42 Scope",         // pickup		//----(SA)	modified
        0,
        IT_WEAPON,
        WP_FG42SCOPE, // this weap
        WP_FG42,      // shares ammo w/
        WP_FG42,      // shares clip w/
        "",           // precache
        "",           // sounds
                      //		{0,0,0,0}
    },

    /*
    weapon_mortar (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/bla?bla?/bla!.md3"
    */
    {
        "weapon_mortar",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/mortar/mortar_3rd.md3",
         "models/multiplayer/mortar/v_mortar.md3", 0},

        "icons/iconw_mortar_1", // icon
        "icons/ammo5",          // ammo icon
        "Mortar",               // pickup		//----(SA)	modified
        0,
        IT_WEAPON,
        WP_MORTAR, // this weap
        WP_MORTAR, // shares ammo w/
        WP_MORTAR, // shares clip w/
        "",        // precache
        "",        // sounds
                   //		{0,0,0,0}
    },

    {
        "weapon_mortar_set",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/mortar/mortar_3rd.md3",
         "models/multiplayer/mortar/v_mortar.md3", 0},

        "icons/iconw_mortar_1", // icon
        "icons/ammo5",          // ammo icon
        "Mounted Mortar",       // pickup		//----(SA)	modified
        0,
        IT_WEAPON,
        WP_MORTAR_SET, // this weap
        WP_MORTAR,     // shares ammo w/
        WP_MORTAR,     // shares clip w/
        "",            // precache
        "",            // sounds
                       //		{0,0,0,0}
    },

    /*
    weapon_landmine
    */
    {
        "weapon_landmine",
        "",
        {"models/multiplayer/landmine/landmine.md3",
         "models/multiplayer/landmine/v_landmine.md3", 0},

        "icons/iconw_landmine_1", // icon
        "icons/ammo9",            // ammo icon
        "Landmine",               // pickup
        7,
        IT_WEAPON,
        WP_LANDMINE,
        WP_LANDMINE,
        WP_LANDMINE,
        "models/multiplayer/landmine/landmine.md3",
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*
    weapon_satchel (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    */
    {
        "weapon_satchel",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/satchel/satchel.md3",
         "models/multiplayer/satchel/v_satchel.md3", 0},

        "icons/iconw_satchel_1", // icon
        "icons/ammo2",           // ammo icon
        "Satchel Charge",        // pickup
        0,
        IT_WEAPON,
        WP_SATCHEL,
        WP_SATCHEL,
        WP_SATCHEL,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    {
        "weapon_satchelDetonator",
        "",
        {"models/multiplayer/satchel/radio.md3",
         "models/multiplayer/satchel/v_satchel.md3", 0},

        "icons/iconw_radio_1",      // icon
        "icons/ammo2",              // ammo icon
        "Satchel Charge Detonator", // pickup
        0,
        IT_WEAPON,
        WP_SATCHEL_DET,
        WP_SATCHEL_DET,
        WP_SATCHEL_DET,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    {
        "weapon_smokebomb",
        "",
        {"models/multiplayer/smokebomb/smokebomb.md3",
         "models/multiplayer/smokebomb/v_smokebomb.md3", 0},

        "icons/iconw_dynamite_1", // icon
        "icons/ammo9",            // ammo icon
        "Smoke Bomb",             // pickup
        0,
        IT_WEAPON,
        WP_SMOKE_BOMB,
        WP_SMOKE_BOMB,
        WP_SMOKE_BOMB,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    {
        "weapon_tripmine",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/dynamite/dynamite_3rd.md3",
         "models/weapons2/dynamite/v_dynamite.md3", 0},

        "icons/iconw_dynamite_1", // icon
        "icons/ammo9",            // ammo icon
        "Tripmine",               // pickup
        7,
        IT_WEAPON,
        WP_TRIPMINE,
        WP_TRIPMINE,
        WP_TRIPMINE,
        "models/multiplayer/dynamite/dynamite.md3 "
        "models/multiplayer/dynamite/dynamite_3rd.md3",
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED weapon_mobile_mg42 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    spin - respawn
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/multiplayer/mg42/v_mg42.md3"
    */
    {
        "weapon_mobile_mg42",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/mg42/mg42_3rd.md3",
         "models/multiplayer/mg42/v_mg42.md3", 0},

        "icons/iconw_mg42_1", // icon
        "icons/ammo2",        // ammo icon
        "Mobile MG42",        // pickup
        30,
        IT_WEAPON,
        WP_MOBILE_MG42,
        WP_MOBILE_MG42,
        WP_MOBILE_MG42,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    {
        "weapon_mobile_mg42_set",
        "sound/misc/w_pkup.wav",
        {"models/multiplayer/mg42/mg42_3rd.md3",
         "models/multiplayer/mg42/v_mg42.md3", 0},

        "icons/iconw_mg42_1", // icon
        "icons/ammo2",        // ammo icon
        "Mobile MG42 Bipod",  // pickup
        30,
        IT_WEAPON,
        WP_MOBILE_MG42_SET,
        WP_MOBILE_MG42,
        WP_MOBILE_MG42,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    {
        "weapon_silencer",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/silencer/silencer.md3", //----(SA)	changed 10/25
         "models/weapons2/silencer/v_silencer.md3",
         "models/weapons2/silencer/pu_silencer.md3"},

        "icons/iconw_silencer_1", // icon
        "icons/ammo5",            // ammo icon
                                  //		"Silencer",		// pickup
        "sp5 pistol",
        10,
        IT_WEAPON,
        WP_SILENCER,
        WP_LUGER,
        WP_LUGER,
        "", // precache
        "", // sounds
            //		{0,0,0,0}
    },

    /*QUAKED weapon_colt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/colt/colt.md3"
    */
    {
        "weapon_silencedcolt",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/colt/colt.md3",
         "models/multiplayer/silencedcolt/v_silencedcolt.md3", 0},

        "icons/iconw_colt_1", // icon
        "icons/ammo2",        // ammo icon
        "Silenced Colt",      // pickup
        50,
        IT_WEAPON,
        WP_SILENCED_COLT,
        WP_COLT,
        WP_COLT,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    // Feen: PGM Test 2 //NOTE: Overridden by portalgun.weap
    /*QUAKED weapon_portalgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/weapons2/tesla/pu_tesla.md3"
    */
    {
        "weapon_portal_gun",
        "sound/misc/w_pkup.wav",
        {"models/weapons2/flamethrower/flamethrower.md3",
         "models/weapons2/flamethrower/v_flamethrower.md3",
         "models/weapons2/flamethrower/pu_flamethrower.md3"},

        "icons/iconw_flamethrower_1", // icon
        "icons/ammo10",               // ammo icon
        "Portalgun",                  // pickup
        50,
        IT_WEAPON,
        WP_PORTAL_GUN,
        WP_PORTAL_GUN,
        WP_PORTAL_GUN,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED ammo_syringe (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN -
    RESPAWN used by: medic

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/ammo/syringe/syringe.md3
    */
    {
        "ammo_syringe",
        "sound/misc/am_pkup.wav",
        {"models/ammo/syringe/syringe.md3", 0, 0},
        "",        // icon
        NULL,      // ammo icon
        "syringe", // pickup			//----(SA)	changed
        1,
        IT_AMMO,
        WP_MEDIC_SYRINGE,
        WP_MEDIC_SYRINGE,
        WP_MEDIC_SYRINGE,
        "", // precache
        "", // sounds
    },

    /*QUAKED ammo_smoke_grenade (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED
    SPIN - RESPAWN used by: engineer

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/ammo/smoke_grenade/smoke_grenade.md3"
    */
    {
        "ammo_smoke_grenade",
        "sound/misc/am_pkup.wav",
        {"models/ammo/smoke_grenade/smoke_grenade.md3", 0, 0},
        "",              // icon
        NULL,            // ammo icon
        "smoke grenade", // pickup			//----(SA)	changed
        1,
        IT_AMMO,
        WP_SMOKE_BOMB,
        WP_SMOKE_BOMB,
        WP_SMOKE_BOMB,
        "", // precache
        "", // sounds
    },

    /*QUAKED ammo_dynamite (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN -
    RESPAWN used by: engineer

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/ammo/dynamite/dynamite.md3"
    */
    {
        "ammo_dynamite",
        "sound/misc/am_pkup.wav",
        {"models/ammo/dynamite/dynamite.md3", 0, 0},
        "",         // icon
        NULL,       // ammo icon
        "dynamite", // pickup			//----(SA)	changed
        1,
        IT_AMMO,
        WP_DYNAMITE,
        WP_DYNAMITE,
        WP_DYNAMITE,
        "", // precache
        "", // sounds
    },

    /*QUAKED ammo_disguise (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN -
    RESPAWN used by: covertops

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/ammo/disguise/disguise.md3"
    */
    {
        "ammo_disguise",
        "sound/misc/am_pkup.wav",
        {"models/ammo/disguise/disguise.md3", 0, 0},
        "",         // icon
        NULL,       // ammo icon
        "disguise", // pickup			//----(SA)	changed
        1,
        IT_AMMO,
        -1, // ignored
        -1, // ignored
        -1, // ignored
        "", // precache
        "", // sounds
    },

    /*QUAKED ammo_airstrike (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN -
    RESPAWN used by: LT

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/ammo/airstrike/airstrike.md3"
    */
    {
        "ammo_airstrike",
        "sound/misc/am_pkup.wav",
        {"models/ammo/disguise/disguise.md3", 0, 0},
        "",                   // icon
        NULL,                 // ammo icon
        "airstrike canister", // pickup			//----(SA)	changed
        1,
        IT_AMMO,
        WP_SMOKE_MARKER,
        WP_SMOKE_MARKER,
        WP_SMOKE_MARKER,
        "", // precache
        "", // sounds
    },

    /*QUAKED ammo_landmine (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN -
    RESPAWN used by: LT

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/ammo/landmine/landmine.md3"
    */
    {
        "ammo_landmine",
        "sound/misc/am_pkup.wav",
        {"models/ammo/landmine/landmine.md3", 0, 0},
        "",         // icon
        NULL,       // ammo icon
        "landmine", // pickup			//----(SA)	changed
        1,
        IT_AMMO,
        WP_LANDMINE,
        WP_LANDMINE,
        WP_LANDMINE,
        "", // precache
        "", // sounds
    },

    /*QUAKED ammo_satchel_charge (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED
    SPIN - RESPAWN used by: LT

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/ammo/satchel/satchel.md3"
    */
    {
        "ammo_satchel_charge",
        "sound/misc/am_pkup.wav",
        {"models/ammo/satchel/satchel.md3", 0, 0},
        "",               // icon
        NULL,             // ammo icon
        "satchel charge", // pickup			//----(SA)	changed
        1,
        IT_AMMO,
        WP_SATCHEL,
        WP_SATCHEL,
        WP_SATCHEL,
        "", // precache
        "", // sounds
    },

    //
    // AMMO ITEMS
    //

    /*QUAKED ammo_9mm_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN -
    RESPAWN used by: Luger pistol, MP40 machinegun

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/ammo/am9mm_s.md3"
    */
    {
        "ammo_9mm_small",
        "sound/misc/am_pkup.wav",
        {"models/powerups/ammo/am9mm_s.md3", 0, 0},
        "",           // icon
        NULL,         // ammo icon
        "9mm Rounds", // pickup
        8,
        IT_AMMO,
        WP_LUGER,
        WP_LUGER,
        WP_LUGER,
        "", // precache
        "", // sounds
            //		{32,24,16,16}
    },
    /*QUAKED ammo_9mm (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN -
    RESPAWN used by: Luger pistol, MP40 machinegun

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/ammo/am9mm_m.md3"
    */
    {
        "ammo_9mm",
        "sound/misc/am_pkup.wav",
        {"models/powerups/ammo/am9mm_m.md3", 0, 0},
        "",    // icon
        NULL,  // ammo icon
        "9mm", // pickup			//----(SA)	changed
        16,
        IT_AMMO,
        WP_LUGER,
        WP_LUGER,
        WP_LUGER,
        "", // precache
        "", // sounds
            //		{64,48,32,32}
    },
    /*QUAKED ammo_9mm_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN -
    RESPAWN used by: Luger pistol, MP40 machinegun

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/ammo/am9mm_l.md3"
    */
    {
        "ammo_9mm_large",
        "sound/misc/am_pkup.wav",
        {"models/powerups/ammo/am9mm_l.md3", 0, 0},
        "",        // icon
        NULL,      // ammo icon
        "9mm Box", // pickup
        24,
        IT_AMMO,
        WP_LUGER,
        WP_LUGER,
        WP_LUGER,
        "", // precache
        "", // sounds
            //		{96,64,48,48}
    },

    /*QUAKED ammo_45cal_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
    - RESPAWN used by: Thompson, Colt

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/ammo/am45cal_s.md3"
    */
    {
        "ammo_45cal_small",
        "sound/misc/am_pkup.wav",
        {"models/powerups/ammo/am45cal_s.md3", 0, 0},
        "",              // icon
        NULL,            // ammo icon
        ".45cal Rounds", // pickup
        8,
        IT_AMMO,
        WP_COLT,
        WP_COLT,
        WP_COLT,
        "", // precache
        "", // sounds
            //		{30,20,15,15}
    },
    /*QUAKED ammo_45cal (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN -
    RESPAWN used by: Thompson, Colt

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/ammo/am45cal_m.md3"
    */
    {
        "ammo_45cal",
        "sound/misc/am_pkup.wav",
        {"models/powerups/ammo/am45cal_m.md3", 0, 0},
        "",       // icon
        NULL,     // ammo icon
        ".45cal", // pickup			//----(SA)	changed
        16,
        IT_AMMO,
        WP_COLT,
        WP_COLT,
        WP_COLT,
        "", // precache
        "", // sounds
            //		{60,45,30,30}
    },
    /*QUAKED ammo_45cal_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
    - RESPAWN used by: Thompson, Colt

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/ammo/am45cal_l.md3"
    */
    {
        "ammo_45cal_large",
        "sound/misc/am_pkup.wav",
        {"models/powerups/ammo/am45cal_l.md3", 0, 0},
        "",           // icon
        NULL,         // ammo icon
        ".45cal Box", // pickup
        24,
        IT_AMMO,
        WP_COLT,
        WP_COLT,
        WP_COLT,
        "", // precache
        "", // sounds
            //		{90,60,45,45}
    },

    /*QUAKED ammo_30cal_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
    - RESPAWN used by: Garand rifle

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/ammo/am30cal_s.md3"
    */
    {
        "ammo_30cal_small",
        "sound/misc/am_pkup.wav",
        {"models/powerups/ammo/am30cal_s.md3", 0, 0},
        "",              // icon
        NULL,            // ammo icon
        ".30cal Rounds", // pickup
        8,
        IT_AMMO,
        WP_GARAND,
        WP_GARAND,
        WP_GARAND,
        "", // precache
        "", // sounds
            //		{5,2,2,2}
    },
    /*QUAKED ammo_30cal (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN -
    RESPAWN used by: Garand rifle

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/ammo/am30cal_m.md3"
    */
    {
        "ammo_30cal",
        "sound/misc/am_pkup.wav",
        {"models/powerups/ammo/am30cal_m.md3", 0, 0},
        "",       // icon
        NULL,     // ammo icon
        ".30cal", // pickup			//----(SA)	changed
        16,
        IT_AMMO,
        WP_GARAND,
        WP_GARAND,
        WP_GARAND,
        "", // precache
        "", // sounds
            //		{5,5,5,5	}
    },
    /*QUAKED ammo_30cal_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
    - RESPAWN used by: Garand rifle

    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/ammo/am30cal_l.md3"
    */
    {
        "ammo_30cal_large",
        "sound/misc/am_pkup.wav",
        {"models/powerups/ammo/am30cal_l.md3", 0, 0},
        "",           // icon
        NULL,         // ammo icon
        ".30cal Box", // pickup
        24,
        IT_AMMO,
        WP_GARAND,
        WP_GARAND,
        WP_GARAND,
        "", // precache
        "", // sounds
            //		{10,10,10,5}
    },

    //
    // POWERUP ITEMS
    //

    /*QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
    Only in CTF games
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/flags/r_flag.md3"
    */
    {
        "team_CTF_redflag",
        "",
        {0, 0, 0},
        "",          // icon
        NULL,        // ammo icon
        "Objective", // pickup
        0,
        IT_TEAM,
        PW_REDFLAG,
        0,
        0,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    /*QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
    Only in CTF games
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/flags/b_flag.md3"
    */
    {
        "team_CTF_blueflag",
        "",
        {0, 0, 0},
        "",          // icon
        NULL,        // ammo icon
        "Blue Flag", // pickup
        0,
        IT_TEAM,
        PW_BLUEFLAG,
        0,
        0,
        "", // precache
        "", // sounds
            //		{0,0,0,0,0}
    },

    //---- (SA) Wolf keys

    /* QUAKED key_1 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
    key 1

    pickup sound : "sound/misc/w_pkup.wav"
    -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
    model="models/powerups/xp_key/key.md3"
    */
    /*
        {
            "key_key1",
            "sound/misc/w_pkup.wav",	//"sound/pickup/keys/skull.wav",
            {
                "models/powerups/xp_key/key.md3",
                0, 0
            },
            "", //"icons/iconk_skull",	// icon
            NULL,					// ammo icon
            "Key 1",		// pickup
            0,
            IT_KEY,
            KEY_1,
            0,
            0,
            "",						// precache
            "models/keys/key.wav",	// sounds
            //{0,0,0,0}
        },

    */

    // end of list marker
    {NULL}};
// END JOSEPH

int bg_numItems = sizeof(bg_itemlist) / sizeof(bg_itemlist[0]) - 1;

/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t *BG_FindItemForHoldable(holdable_t pw) {
  int i;

  for (i = 0; i < bg_numItems; i++) {
    if (bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw) {
      return &bg_itemlist[i];
    }
  }

  //	Com_Error( ERR_DROP, "HoldableItem not found" );

  return NULL;
}

/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t *BG_FindItemForWeapon(weapon_t weapon) {
  gitem_t *it;

  for (it = bg_itemlist + 1; it->classname; it++) {
    if (it->giType == IT_WEAPON && it->giTag == weapon) {
      return it;
    }
  }

  Com_Error(ERR_DROP, "Couldn't find item for weapon %i", weapon);
  return NULL;
}

/*
==============
BG_FindClipForWeapon
==============
*/
weapon_t BG_FindClipForWeapon(weapon_t weapon) {
  gitem_t *it;

  for (it = bg_itemlist + 1; it->classname; it++) {
    if (it->giType == IT_WEAPON && it->giTag == weapon) {
      return static_cast<weapon_t>(it->giClipIndex);
    }
  }

  return WP_NONE;
}

/*
==============
BG_FindAmmoForWeapon
==============
*/
weapon_t BG_FindAmmoForWeapon(weapon_t weapon) {
  gitem_t *it;

  for (it = bg_itemlist + 1; it->classname; it++) {
    if (it->giType == IT_WEAPON && it->giTag == weapon) {
      return static_cast<weapon_t>(it->giAmmoIndex);
    }
  }
  return WP_NONE;
}

/*
==============
BG_AkimboFireSequence
    returns 'true' if it's the left hand's turn to fire, 'false' if it's the
right hand's turn
==============
*/
qboolean BG_AkimboFireSequence(int weapon, int akimboClip, int mainClip) {
  if (!BG_IsAkimboWeapon(weapon)) {
    return qfalse;
  }

  if (!akimboClip) {
    return qfalse;
  }

  // no ammo in main weapon, must be akimbo turn
  if (!mainClip) {
    return qtrue;
  }

  // at this point, both have ammo

  // now check 'cycle'   // (removed old method 11/5/2001)
  if ((akimboClip + mainClip) & 1) {
    return qfalse;
  }

  return qtrue;
}

/*
==============
BG_IsAkimboWeapon
==============
*/
qboolean BG_IsAkimboWeapon(int weaponNum) {
  if (weaponNum == WP_AKIMBO_COLT || weaponNum == WP_AKIMBO_SILENCEDCOLT ||
      weaponNum == WP_AKIMBO_LUGER || weaponNum == WP_AKIMBO_SILENCEDLUGER) {
    return qtrue;
  } else {
    return qfalse;
  }
}

/*
==============
BG_IsAkimboSideArm
==============
*/
qboolean BG_IsAkimboSideArm(int weaponNum, playerState_t *ps) {
  switch (weaponNum) {
    case WP_COLT:
      if (ps->weapon == WP_AKIMBO_COLT ||
          ps->weapon == WP_AKIMBO_SILENCEDCOLT) {
        return qtrue;
      }
      break;
    case WP_LUGER:
      if (ps->weapon == WP_AKIMBO_LUGER ||
          ps->weapon == WP_AKIMBO_SILENCEDLUGER) {
        return qtrue;
      }
      break;
  }
  return qfalse;
}

/*
==============
BG_AkimboSidearm
==============
*/
int BG_AkimboSidearm(int weaponNum) {
  switch (weaponNum) {
    case WP_AKIMBO_COLT:
      return WP_COLT;
      break;
    case WP_AKIMBO_SILENCEDCOLT:
      return WP_COLT;
      break;
    case WP_AKIMBO_LUGER:
      return WP_LUGER;
      break;
    case WP_AKIMBO_SILENCEDLUGER:
      return WP_LUGER;
      break;
    default:
      return WP_NONE;
      break;
  }
}

//----(SA) added
/*
==============
BG_FindItemForAmmo
==============
*/
gitem_t *BG_FindItemForAmmo(int ammo) {
  int i = 0;

  for (; i < bg_numItems; i++) {
    if (bg_itemlist[i].giType == IT_AMMO &&
        bg_itemlist[i].giAmmoIndex == ammo) {
      return &bg_itemlist[i];
    }
  }
  Com_Error(ERR_DROP, "Item not found for ammo: %d", ammo);
  return NULL;
}
//----(SA) end

/*
===============
BG_FindItem
===============
*/
gitem_t *BG_FindItem(const char *pickupName) {
  gitem_t *it;

  for (it = bg_itemlist + 1; it->classname; it++) {
    if (!Q_stricmp(it->pickup_name, pickupName)) {
      return it;
    }
  }

  return NULL;
}

gitem_t *BG_FindItemForClassName(const char *className) {
  gitem_t *it;

  for (it = bg_itemlist + 1; it->classname; it++) {
    if (!Q_stricmp(it->classname, className)) {
      return it;
    }
  }

  return NULL;
}

// DHM - Nerve :: returns qtrue if a weapon is indeed used in multiplayer
// Gordon: FIXME: er, we shouldnt really need this, just remove all the weapons
// we dont actually want :)
qboolean BG_WeaponInWolfMP(int weapon) {
  switch (weapon) {
    case WP_KNIFE:
    case WP_LUGER:
    case WP_COLT:
    case WP_MP40:
    case WP_THOMPSON:
    case WP_STEN:
    case WP_GRENADE_LAUNCHER:
    case WP_GRENADE_PINEAPPLE:
    case WP_PANZERFAUST:
    case WP_FLAMETHROWER:
    case WP_AMMO:
    case WP_ARTY:
    case WP_SMOKETRAIL:
    case WP_MEDKIT:
    case WP_PLIERS:
    case WP_SMOKE_MARKER:
    case WP_DYNAMITE:
    case WP_MEDIC_SYRINGE:
    case WP_MEDIC_ADRENALINE:
    case WP_BINOCULARS:
    case WP_KAR98:
    case WP_GPG40:
    case WP_CARBINE:
    case WP_M7:
    case WP_GARAND:
    case WP_GARAND_SCOPE:
    case WP_FG42:
    case WP_FG42SCOPE:
    case WP_LANDMINE:
    case WP_SATCHEL:
    case WP_SATCHEL_DET:
      //	case WP_TRIPMINE:    // bye bye
      // tripmines ;(
    case WP_SMOKE_BOMB:
    case WP_MOBILE_MG42:
    case WP_MOBILE_MG42_SET:
    case WP_SILENCER:
    case WP_SILENCED_COLT:
    case WP_K43:
    case WP_K43_SCOPE:
    case WP_MORTAR:
    case WP_MORTAR_SET:
    // case WP_LOCKPICK:
    case WP_AKIMBO_LUGER:
    case WP_AKIMBO_SILENCEDLUGER:
    case WP_AKIMBO_COLT:
    case WP_AKIMBO_SILENCEDCOLT:
    case WP_PORTAL_GUN: // Feen: PGM
      return qtrue;
    default:
      return qfalse;
  }
}

bool BG_WeaponDisallowedInTimeruns(const int weap) {
  switch (weap) {
    case WP_DYNAMITE:
    case WP_GRENADE_LAUNCHER:
    case WP_GRENADE_PINEAPPLE:
    case WP_M7:
    case WP_SATCHEL_DET:
    case WP_SATCHEL:
    case WP_MORTAR:
    case WP_MORTAR_SET:
    case WP_GPG40:
    case WP_LANDMINE:
    case WP_FLAMETHROWER:
    case WP_PANZERFAUST:
    case WP_PORTAL_GUN:
      return true;
    default:
      return false;
  }
}

bool BG_WeaponHasAmmo(playerState_t *ps, int weap) {
  if (ps->ammo[BG_FindAmmoForWeapon(static_cast<weapon_t>(weap))] ||
      ps->ammoclip[BG_FindClipForWeapon(static_cast<weapon_t>(weap))]) {
    return true;
  }
  return false;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
bool BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime) {
  vec3_t origin;

  BG_EvaluateTrajectory(&item->pos, atTime, origin, qfalse, item->effect2Time);
  static constexpr int boxSide = 36;

  // we are ignoring ducked differences here
  if (ps->origin[0] - origin[0] > boxSide ||
      ps->origin[0] - origin[0] < -boxSide ||
      ps->origin[1] - origin[1] > boxSide ||
      ps->origin[1] - origin[1] < -boxSide ||
      ps->origin[2] - origin[2] > boxSide ||
      ps->origin[2] - origin[2] < -boxSide) {
    return false;
  }

  return true;
}

/*
=================================
BG_AddMagicAmmo:
    if numOfClips is 0, no ammo is added, it just return whether any ammo CAN be
added; otherwise return whether any ammo was ACTUALLY added.

WARNING: when numOfClips is 0, DO NOT CHANGE ANYTHING under ps.
=================================
*/
int BG_GrenadesForClass(int cls, int *skills) {
  switch (cls) {
    case PC_MEDIC:
      if (skills[SK_FIRST_AID] >= 1) {
        return 2;
      }
      return 1;
    case PC_SOLDIER:
      return 4;
    case PC_ENGINEER:
      return 8;
    case PC_FIELDOPS:
      if (skills[SK_SIGNALS] >= 1) {
        return 2;
      }
      return 1;
    case PC_COVERTOPS:
      return 2;
  }

  return 0;
}

weapon_t BG_GrenadeTypeForTeam(team_t team) {
  switch (team) {
    case TEAM_AXIS:
      return WP_GRENADE_LAUNCHER;
    case TEAM_ALLIES:
      return WP_GRENADE_PINEAPPLE;
    default:
      return WP_NONE;
  }
}

qboolean BG_CheckMagicAmmo(const playerState_t *ps, int *skill, int teamNum) {
  int i, weapon;
  int maxammo;
  int clip;

  // Gordon: handle grenades first

  i = BG_GrenadesForClass(ps->stats[STAT_PLAYER_CLASS], skill);
  weapon = BG_GrenadeTypeForTeam(static_cast<team_t>(teamNum));

  clip = BG_FindClipForWeapon(static_cast<weapon_t>(weapon));
  if (ps->ammoclip[clip] < i) {
    return qtrue;
  }

  if (COM_BitCheck(ps->weapons, WP_MEDIC_SYRINGE)) {
    i = skill[SK_FIRST_AID] >= 2 ? 12 : 10;

    clip = BG_FindClipForWeapon(WP_MEDIC_SYRINGE);

    if (ps->ammoclip[clip] < i) {
      return qtrue;
    }
  }

  // Gordon: now other weapons
  for (i = 0; reloadableWeapons[i] >= 0; i++) {
    weapon = reloadableWeapons[i];
    if (COM_BitCheck(ps->weapons, weapon)) {
      maxammo = BG_MaxAmmoForWeapon(static_cast<weapon_t>(weapon), skill);

      // Handle weapons that just use clip, and not ammo
      if (weapon == WP_FLAMETHROWER) {
        clip = BG_FindAmmoForWeapon(static_cast<weapon_t>(weapon));
        if (ps->ammoclip[clip] < maxammo) {
          return qtrue;
        }
      } else if (weapon == WP_PANZERFAUST) {
        clip = BG_FindAmmoForWeapon(static_cast<weapon_t>(weapon));
        if (ps->ammoclip[clip] < maxammo) {
          return qtrue;
        }
      } else {
        clip = BG_FindAmmoForWeapon(static_cast<weapon_t>(weapon));
        if (ps->ammo[clip] < maxammo) {
          return qtrue;
        }
      }
    }
  }

  return qfalse;
}

// Gordon: setting numOfClips = 0 allows you to check if the client needs ammo,
// but doesnt give any
qboolean BG_AddMagicAmmo(playerState_t *ps, int *skill, int teamNum,
                         int numOfClips) {
  int i, weapon;
  int ammoAdded = qfalse;
  int maxammo;
  int clip;
  int weapNumOfClips;

  if (!numOfClips) {
    return BG_CheckMagicAmmo(ps, skill, teamNum);
  }

  // Gordon: handle grenades first

  i = BG_GrenadesForClass(ps->stats[STAT_PLAYER_CLASS], skill);
  weapon = BG_GrenadeTypeForTeam(static_cast<team_t>(teamNum));

  clip = BG_FindClipForWeapon(static_cast<weapon_t>(weapon));
  if (ps->ammoclip[clip] < i) {
    ps->ammoclip[clip] += numOfClips;

    ammoAdded = qtrue;

    COM_BitSet(ps->weapons, weapon);

    if (ps->ammoclip[clip] > i) {
      ps->ammoclip[clip] = i;
    }
  }

  if (COM_BitCheck(ps->weapons, WP_MEDIC_SYRINGE)) {
    i = skill[SK_FIRST_AID] >= 2 ? 12 : 10;

    clip = BG_FindClipForWeapon(WP_MEDIC_SYRINGE);

    if (ps->ammoclip[clip] < i) {
      ps->ammoclip[clip] += numOfClips;

      ammoAdded = qtrue;

      if (ps->ammoclip[clip] > i) {
        ps->ammoclip[clip] = i;
      }
    }
  }

  // Gordon: now other weapons
  for (i = 0; reloadableWeapons[i] >= 0; i++) {
    weapon = reloadableWeapons[i];
    if (COM_BitCheck(ps->weapons, weapon)) {
      maxammo = BG_MaxAmmoForWeapon(static_cast<weapon_t>(weapon), skill);

      // Handle weapons that just use clip, and not ammo
      if (weapon == WP_FLAMETHROWER) {
        clip = BG_FindAmmoForWeapon(static_cast<weapon_t>(weapon));
        if (ps->ammoclip[clip] < maxammo) {
          ammoAdded = qtrue;
          ps->ammoclip[clip] = maxammo;
        }
      } else if (weapon == WP_PANZERFAUST) {
        clip = BG_FindAmmoForWeapon(static_cast<weapon_t>(weapon));
        if (ps->ammoclip[clip] < maxammo) {
          ammoAdded = qtrue;
          ps->ammoclip[clip] += numOfClips;
          if (ps->ammoclip[clip] >= maxammo) {
            ps->ammoclip[clip] = maxammo;
          }
        }
      } else {
        clip = BG_FindAmmoForWeapon(static_cast<weapon_t>(weapon));
        if (ps->ammo[clip] < maxammo) {
          ammoAdded = qtrue;

          if (BG_IsAkimboWeapon(weapon)) {
            // double clips babeh!
            weapNumOfClips = numOfClips * 2;
          } else {
            weapNumOfClips = numOfClips;
          }

          // add and limit check
          ps->ammo[clip] += weapNumOfClips * GetAmmoTableData(weapon)->maxclip;
          if (ps->ammo[clip] > maxammo) {
            ps->ammo[clip] = maxammo;
          }
        }
      }
    }
  }

  return ammoAdded ? qtrue : qfalse;
}

#define AMMOFORWEAP BG_FindAmmoForWeapon(item->giTag)
/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed(const entityState_t *ent, const playerState_t *ps,
                             int *skill, int teamNum) {
  gitem_t *item;

  if (ent->modelindex < 1 || ent->modelindex >= bg_numItems) {
    Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");
  }

  item = &bg_itemlist[ent->modelindex];

  switch (item->giType) {
    case IT_WEAPON:
      if (item->giTag == WP_AMMO) {
        return BG_CheckMagicAmmo(ps, skill, teamNum);
      }

      return qtrue;

    case IT_AMMO:
      return qfalse;

    case IT_ARMOR:
      return qfalse;

    case IT_HEALTH:
      // Gordon: ps->teamNum is really class.... thx
      // whoever decided on that...
      if (ps->teamNum == PC_MEDIC) {
        // Gordon: medics can go up to 12% extra
        // on max health as they have perm.
        // regen
        if (ps->stats[STAT_HEALTH] >=
            (int)(ps->stats[STAT_MAX_HEALTH] * 1.12)) {
          return qfalse;
        }
      } else {
        if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH]) {
          return qfalse;
        }
      }
      return qtrue;

    case IT_TEAM: // team items, such as flags

      // density tracks how many uses left
      if ((ent->density < 1) || (((ps->persistant[PERS_TEAM] == TEAM_AXIS)
                                      ? ps->powerups[PW_BLUEFLAG]
                                      : ps->powerups[PW_REDFLAG]) != 0)) {
        return qfalse;
      }

      // DHM - Nerve :: otherEntity2 is now used instead
      // of modelindex2 ent->modelindex2 is non-zero on
      // items if they are dropped we need to know this
      // because we can pick up our dropped flag (and
      // return it) but we can't pick up our flag at
      // base
      if (ps->persistant[PERS_TEAM] == TEAM_AXIS) {
        if (item->giTag == PW_BLUEFLAG ||
            (item->giTag == PW_REDFLAG &&
             ent->otherEntityNum2 /*ent->modelindex2*/) ||
            (item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG])) {
          return qtrue;
        }
      } else if (ps->persistant[PERS_TEAM] == TEAM_ALLIES) {
        if (item->giTag == PW_REDFLAG ||
            (item->giTag == PW_BLUEFLAG &&
             ent->otherEntityNum2 /*ent->modelindex2*/) ||
            (item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG])) {
          return qtrue;
        }
      }

      return qfalse;

    case IT_HOLDABLE:
      return qtrue;

    case IT_TREASURE: // treasure always picked up
      return qtrue;

    case IT_KEY:
      return qtrue; // keys are always picked up

    case IT_BAD:
      Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD");
  }
  return qfalse;
}

//======================================================================

void BG_CalculateSpline_r(splinePath_t *spline, vec3_t out1, vec3_t out2,
                          float tension) {
  vec3_t points[18];
  int i;
  int count = spline->numControls + 2;
  vec3_t dist;

  VectorCopy(spline->point.origin, points[0]);
  for (i = 0; i < spline->numControls; i++) {
    VectorCopy(spline->controls[i].origin, points[i + 1]);
  }
  if (!spline->next) {
    return;
    //		Com_Error( ERR_DROP, "Spline (%s) with no target
    // referenced",
    // spline->point.name );
  }
  VectorCopy(spline->next->point.origin, points[i + 1]);

  while (count > 2) {
    for (i = 0; i < count - 1; i++) {
      VectorSubtract(points[i + 1], points[i], dist);
      VectorMA(points[i], tension, dist, points[i]);
    }
    count--;
  }

  VectorCopy(points[0], out1);
  VectorCopy(points[1], out2);
}

qboolean BG_TraverseSpline(float *deltaTime, splinePath_t **pSpline) {
  float dist;

  while ((*deltaTime) > 1) {
    (*deltaTime) -= 1;
    dist = (*pSpline)->length * (*deltaTime);

    if (!(*pSpline)->next || !(*pSpline)->next->length) {
      return qfalse;
      //			Com_Error( ERR_DROP,
      //"Spline path end passed
      //(%s)", (*pSpline)->point.name );
    }

    (*pSpline) = (*pSpline)->next;
    *deltaTime = dist / (*pSpline)->length;
  }

  while ((*deltaTime) < 0) {
    dist = -((*pSpline)->length * (*deltaTime));

    if (!(*pSpline)->prev || !(*pSpline)->prev->length) {
      return qfalse;
      //			Com_Error( ERR_DROP,
      //"Spline path end passed
      //(%s)", (*pSpline)->point.name );
    }

    (*pSpline) = (*pSpline)->prev;
    (*deltaTime) = 1 - (dist / (*pSpline)->length);
  }

  return qtrue;
}

/*
================
BG_RaySphereIntersection

================
*/

qboolean BG_RaySphereIntersection(float radius, vec3_t origin,
                                  splineSegment_t *path, float *t0, float *t1) {
  vec3_t v;
  float b, c, d;

  VectorSubtract(path->start, origin, v);

  b = 2 * DotProduct(v, path->v_norm);
  c = DotProduct(v, v) - (radius * radius);

  d = (b * b) - (4 * c);
  if (d < 0) {
    return qfalse;
  }
  d = sqrt(d);

  *t0 = (-b + d) * 0.5f;
  *t1 = (-b - d) * 0.5f;

  return qtrue;
}

void BG_LinearPathOrigin2(float radius, splinePath_t **pSpline,
                          float *deltaTime, vec3_t result, qboolean backwards) {
  qboolean first = qtrue;
  float t = 0.f;
  int i = floor((*deltaTime) * (MAX_SPLINE_SEGMENTS));
  float frac;
  //	int x = 0;
  //	splinePath_t* start = *pSpline;

  if (i >= MAX_SPLINE_SEGMENTS) {
    i = MAX_SPLINE_SEGMENTS - 1;
    frac = 1.f;
  } else {
    frac = (((*deltaTime) * (MAX_SPLINE_SEGMENTS)) - i);
  }

  while (qtrue) {
    float t0, t1;

    while (qtrue) {
      if (BG_RaySphereIntersection(radius, result, &(*pSpline)->segments[i],
                                   &t0, &t1)) {
        qboolean found = qfalse;

        t0 /= (*pSpline)->segments[i].length;
        t1 /= (*pSpline)->segments[i].length;

        if (first) {
          if (radius < 0) {
            if (t0 < frac && (t0 >= 0.f && t0 <= 1.f)) {
              t = t0;
              found = qtrue;
            } else if (t1 < frac) {
              t = t1;
              found = qtrue;
            }
          } else {
            if (t0 > frac && (t0 >= 0.f && t0 <= 1.f)) {
              t = t0;
              found = qtrue;
            } else if (t1 > frac) {
              t = t1;
              found = qtrue;
            }
          }
        } else {
          if (radius < 0) {
            if (t0 < t1 && (t0 >= 0.f && t0 <= 1.f)) {
              t = t0;
              found = qtrue;
            } else {
              t = t1;
              found = qtrue;
            }
          } else {
            if (t0 > t1 && (t0 >= 0.f && t0 <= 1.f)) {
              t = t0;
              found = qtrue;
            } else {
              t = t1;
              found = qtrue;
            }
          }
        }

        if (found) {
          if (t >= 0.f && t <= 1.f) {
            *deltaTime = (i / (float)(MAX_SPLINE_SEGMENTS)) +
                         (t / (float)(MAX_SPLINE_SEGMENTS));
            VectorMA((*pSpline)->segments[i].start,
                     t * (*pSpline)->segments[i].length,
                     (*pSpline)->segments[i].v_norm, result);
            return;
          }
        }
        found = qfalse;
      }

      first = qfalse;
      if (radius < 0) {
        i--;
        if (i < 0) {
          i = MAX_SPLINE_SEGMENTS - 1;
          break;
        }
      } else {
        i++;
        if (i >= MAX_SPLINE_SEGMENTS) {
          i = 0;
          break;
        }
      }
    }

    if (radius < 0) {
      if (!(*pSpline)->prev) {
        return;
        //				Com_Error(
        // ERR_DROP, "End of spline
        // reached
        //(%s)\n", start->point.name );
      }
      *pSpline = (*pSpline)->prev;
    } else {
      if (!(*pSpline)->next) {
        return;
        //				Com_Error(
        // ERR_DROP, "End of spline
        // reached
        //(%s)\n", start->point.name );
      }
      *pSpline = (*pSpline)->next;
    }
  }
}

void BG_ComputeSegments(splinePath_t *pSpline) {
  int i;
  float granularity = 1 / ((float)(MAX_SPLINE_SEGMENTS));
  vec3_t vec[4];

  for (i = 0; i < MAX_SPLINE_SEGMENTS; i++) {
    BG_CalculateSpline_r(pSpline, vec[0], vec[1], i * granularity);
    VectorSubtract(vec[1], vec[0], pSpline->segments[i].start);
    VectorMA(vec[0], i * granularity, pSpline->segments[i].start,
             pSpline->segments[i].start);

    BG_CalculateSpline_r(pSpline, vec[2], vec[3], (i + 1) * granularity);
    VectorSubtract(vec[3], vec[2], vec[0]);
    VectorMA(vec[2], (i + 1) * granularity, vec[0], vec[0]);

    VectorSubtract(vec[0], pSpline->segments[i].start,
                   pSpline->segments[i].v_norm);
    pSpline->segments[i].length = VectorLength(pSpline->segments[i].v_norm);
    VectorNormalize(pSpline->segments[i].v_norm);
  }
}

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec3_t result,
                           qboolean isAngle, int splinePath) {
  float deltaTime;
  float phase;
  vec3_t v;

  splinePath_t *pSpline;
  vec3_t vec[2];
  qboolean backwards = qfalse;
  float deltaTime2;

  switch (tr->trType) {
    case TR_STATIONARY:
    case TR_INTERPOLATE:
    case TR_GRAVITY_PAUSED: //----(SA)
      VectorCopy(tr->trBase, result);
      break;
    case TR_LINEAR:
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
      break;
    case TR_SINE:
      deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
      phase = sin(deltaTime * M_PI * 2);
      VectorMA(tr->trBase, phase, tr->trDelta, result);
      break;
      //----(SA)	removed
    case TR_LINEAR_STOP:
      if (atTime > tr->trTime + tr->trDuration) {
        atTime = tr->trTime + tr->trDuration;
      }
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      if (deltaTime < 0) {
        deltaTime = 0;
      }
      VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
      break;
    case TR_GRAVITY:
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
      result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime *
                   deltaTime; // FIXME: local gravity...
      break;
    // Ridah
    case TR_GRAVITY_LOW:
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
      result[2] -= 0.5 * (DEFAULT_GRAVITY * 0.3) * deltaTime *
                   deltaTime; // FIXME: local gravity...
      break;
      // done.
      //----(SA)
    case TR_GRAVITY_FLOAT:
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
      result[2] -= 0.5 * (DEFAULT_GRAVITY * 0.2) * deltaTime;
      break;
      //----(SA)	end
      // RF, acceleration
    case TR_ACCELERATE: // trDelta is the ultimate speed
      if (atTime > tr->trTime + tr->trDuration) {
        atTime = tr->trTime + tr->trDuration;
      }
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      // phase is the acceleration constant
      phase = VectorLength(tr->trDelta) / (tr->trDuration * 0.001);
      // trDelta at least gives us the acceleration
      // direction
      VectorNormalize2(tr->trDelta, result);
      // get distance travelled at current time
      VectorMA(tr->trBase, phase * 0.5 * deltaTime * deltaTime, result, result);
      break;
    case TR_DECCELERATE: // trDelta is the starting speed
      if (atTime > tr->trTime + tr->trDuration) {
        atTime = tr->trTime + tr->trDuration;
      }
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      // phase is the breaking constant
      phase = VectorLength(tr->trDelta) / (tr->trDuration * 0.001);
      // trDelta at least gives us the acceleration
      // direction
      VectorNormalize2(tr->trDelta, result);
      // get distance travelled at current time (without
      // breaking)
      VectorMA(tr->trBase, deltaTime, tr->trDelta, v);
      // subtract breaking force
      VectorMA(v, -phase * 0.5 * deltaTime * deltaTime, result, result);
      break;
    case TR_SPLINE:
      pSpline = BG_GetSplineData(splinePath, &backwards);
      if (!pSpline) {
        return;
      }

      deltaTime =
          tr->trDuration ? (atTime - tr->trTime) / ((float)tr->trDuration) : 0;

      if (deltaTime < 0.f) {
        deltaTime = 0.f;
      } else if (deltaTime > 1.f) {
        deltaTime = 1.f;
      }

      if (backwards) {
        deltaTime = 1 - deltaTime;
      }

      /*		if(pSpline->isStart) {
                  deltaTime = 1 - sin((1 - deltaTime) *
         M_PI * 0.5f); } else if(pSpline->isEnd) {
                  deltaTime = sin(deltaTime * M_PI *
         0.5f);
              }*/

      deltaTime2 = deltaTime;

      BG_CalculateSpline_r(pSpline, vec[0], vec[1], deltaTime);

      if (isAngle) {
        qboolean dampin = qfalse;
        qboolean dampout = qfalse;
        float base1;

        if (tr->trBase[0]) {
          //				int pos =
          // 0;
          vec3_t result2;
          splinePath_t *pSp2 = pSpline;

          deltaTime2 += tr->trBase[0] / pSpline->length;

          if (BG_TraverseSpline(&deltaTime2, &pSp2)) {

            VectorSubtract(vec[1], vec[0], result);
            VectorMA(vec[0], deltaTime, result, result);

            BG_CalculateSpline_r(pSp2, vec[0], vec[1], deltaTime2);

            VectorSubtract(vec[1], vec[0], result2);
            VectorMA(vec[0], deltaTime2, result2, result2);

            if (tr->trBase[0] < 0) {
              VectorSubtract(result, result2, result);
            } else {
              VectorSubtract(result2, result, result);
            }
          } else {
            VectorSubtract(vec[1], vec[0], result);
          }
        } else {
          VectorSubtract(vec[1], vec[0], result);
        }

        vectoangles(result, result);

        base1 = tr->trBase[1];
        if (base1 >= 10000 || base1 < -10000) {
          dampin = qtrue;
          if (base1 < 0) {
            base1 += 10000;
          } else {
            base1 -= 10000;
          }
        }

        if (base1 >= 1000 || base1 < -1000) {
          dampout = qtrue;
          if (base1 < 0) {
            base1 += 1000;
          } else {
            base1 -= 1000;
          }
        }

        if (dampin && dampout) {
          result[ROLL] =
              base1 + ((sin(((deltaTime * 2) - 1) * M_PI * 0.5f) + 1) * 0.5f *
                       tr->trBase[2]);
        } else if (dampin) {
          result[ROLL] = base1 + (sin(deltaTime * M_PI * 0.5f) * tr->trBase[2]);
        } else if (dampout) {
          result[ROLL] = base1 + ((1 - sin((1 - deltaTime) * M_PI * 0.5f)) *
                                  tr->trBase[2]);
        } else {
          result[ROLL] = base1 + (deltaTime * tr->trBase[2]);
        }
      } else {
        VectorSubtract(vec[1], vec[0], result);
        VectorMA(vec[0], deltaTime, result, result);
      }

      break;
    case TR_LINEAR_PATH:
      pSpline = BG_GetSplineData(splinePath, &backwards);
      if (!pSpline) {
        return;
      }

      deltaTime =
          tr->trDuration ? (atTime - tr->trTime) / ((float)tr->trDuration) : 0;

      if (deltaTime < 0.f) {
        deltaTime = 0.f;
      } else if (deltaTime > 1.f) {
        deltaTime = 1.f;
      }

      if (backwards) {
        deltaTime = 1 - deltaTime;
      }

      if (isAngle) {
        int pos = floor(deltaTime * (MAX_SPLINE_SEGMENTS));
        float frac;

        if (pos >= MAX_SPLINE_SEGMENTS) {
          pos = MAX_SPLINE_SEGMENTS - 1;
          frac = pSpline->segments[pos].length;
        } else {
          frac = ((deltaTime * (MAX_SPLINE_SEGMENTS)) - pos) *
                 pSpline->segments[pos].length;
        }

        if (tr->trBase[0]) {
          VectorMA(pSpline->segments[pos].start, frac,
                   pSpline->segments[pos].v_norm, result);
          VectorCopy(result, v);

          BG_LinearPathOrigin2(tr->trBase[0], &pSpline, &deltaTime, v,
                               backwards);
          if (tr->trBase[0] < 0) {
            VectorSubtract(v, result, result);
          } else {
            VectorSubtract(result, v, result);
          }

          vectoangles(result, result);
        } else {
          vectoangles(pSpline->segments[pos].v_norm, result);
        }

      } else {
        int pos = floor(deltaTime * (MAX_SPLINE_SEGMENTS));
        float frac;

        if (pos >= MAX_SPLINE_SEGMENTS) {
          pos = MAX_SPLINE_SEGMENTS - 1;
          frac = pSpline->segments[pos].length;
        } else {
          frac = ((deltaTime * (MAX_SPLINE_SEGMENTS)) - pos) *
                 pSpline->segments[pos].length;
        }

        VectorMA(pSpline->segments[pos].start, frac,
                 pSpline->segments[pos].v_norm, result);
      }

      break;
    default:
      Com_Error(ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i",
                tr->trTime);
      break;
  }
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime,
                                vec3_t result, qboolean isAngle,
                                int splineData) {
  float deltaTime;
  float phase;

  switch (tr->trType) {
    case TR_STATIONARY:
    case TR_INTERPOLATE:
      VectorClear(result);
      break;
    case TR_LINEAR:
      VectorCopy(tr->trDelta, result);
      break;
    case TR_SINE:
      deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
      phase = cos(deltaTime * M_PI * 2); // derivative of sin = cos
      phase *= 0.5;
      VectorScale(tr->trDelta, phase, result);
      break;
      //----(SA)	removed
    case TR_LINEAR_STOP:
      if (atTime > tr->trTime + tr->trDuration) {
        VectorClear(result);
        return;
      }
      VectorCopy(tr->trDelta, result);
      break;
    case TR_GRAVITY:
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      VectorCopy(tr->trDelta, result);
      result[2] -= DEFAULT_GRAVITY * deltaTime; // FIXME: local gravity...
      break;
    // Ridah
    case TR_GRAVITY_LOW:
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      VectorCopy(tr->trDelta, result);
      result[2] -=
          (DEFAULT_GRAVITY * 0.3) * deltaTime; // FIXME: local gravity...
      break;
      // done.
      //----(SA)
    case TR_GRAVITY_FLOAT:
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      VectorCopy(tr->trDelta, result);
      result[2] -= (DEFAULT_GRAVITY * 0.2) * deltaTime;
      break;
      //----(SA)	end
      // RF, acceleration
    case TR_ACCELERATE: // trDelta is eventual speed
      if (atTime > tr->trTime + tr->trDuration) {
        VectorClear(result);
        return;
      }
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      phase = deltaTime / (float)tr->trDuration;
      VectorScale(tr->trDelta, deltaTime * deltaTime, result);
      break;
    case TR_DECCELERATE: // trDelta is breaking force
      if (atTime > tr->trTime + tr->trDuration) {
        VectorClear(result);
        return;
      }
      deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
      VectorScale(tr->trDelta, deltaTime, result);
      break;
    case TR_SPLINE:
    case TR_LINEAR_PATH:
      VectorClear(result);
      break;
    default:
      Com_Error(ERR_DROP,
                "BG_EvaluateTrajectoryDelta: unknown "
                "trType: %i",
                tr->trTime);
      break;
  }
}

/*
============
BG_GetMarkDir

  used to find a good directional vector for a mark projection, which will be
more likely to wrap around adjacent surfaces

  dir is the direction of the projectile or trace that has resulted in a surface
being hit
============
*/
void BG_GetMarkDir(const vec3_t dir, const vec3_t normal, vec3_t out) {
  vec3_t ndir, lnormal;
  float minDot = 0.3;
  int x = 0;

  if (dir[0] < 0.001 && dir[1] < 0.001) {
    VectorCopy(dir, out);
    return;
  }

  if (VectorLengthSquared(normal) <
      SQR(1.f)) // this is needed to get rid of (0,0,0) normals (happens
                // with entities?)
  {
    VectorSet(lnormal, 0.f, 0.f, 1.f);
  } else {
    // VectorCopy( normal, lnormal );
    // VectorNormalizeFast( lnormal );
    VectorNormalize2(normal, lnormal);
  }

  VectorNegate(dir, ndir);
  VectorNormalize(ndir);
  if (normal[2] > .8f) {
    minDot = .7f;
  }

  // make sure it makrs the impact surface
  while (DotProduct(ndir, lnormal) < minDot && x < 10) {
    VectorMA(ndir, .5, lnormal, ndir);
    VectorNormalize(ndir);

    x++;
  }

#ifdef GAMEDLL
  if (x >= 10) {
    if (g_developer.integer) {
      Com_Printf("BG_GetMarkDir loops: %i\n", x);
    }
  }
#endif // GAMEDLL

  VectorCopy(ndir, out);
}

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer,
                                    int bufsize);

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm,
                                         playerState_t *ps) {

#ifdef _DEBUG
  {
    char buf[256];
    trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
    if (Q_atof(buf) != 0) {
  #ifdef QAGAME
      Com_Printf(" game event svt %5d -> %5d: num = %20s parm "
                 "%d\n",
                 ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence,
                 eventnames[newEvent], eventParm);
  #else
      Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm "
                 "%d\n",
                 ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence,
                 eventnames[newEvent], eventParm);
  #endif
    }
  }
#endif
  ps->events[ps->eventSequence & (MAX_EVENTS - 1)] = newEvent;
  ps->eventParms[ps->eventSequence & (MAX_EVENTS - 1)] = eventParm;
  ps->eventSequence++;
}

void BG_SetupMountedGunStatus(playerState_t *ps) {
  switch (
      static_cast<ETJump::HeavyWeaponState>(ps->persistant[PERS_HWEAPON_USE])) {
    case ETJump::HeavyWeaponState::MountedMG:
      ps->eFlags |= EF_MG42_ACTIVE;
      ps->eFlags &= ~EF_AAGUN_ACTIVE;
      ps->powerups[PW_OPS_DISGUISED] = 0;
      break;
    case ETJump::HeavyWeaponState::AAGun:
      ps->eFlags |= EF_AAGUN_ACTIVE;
      ps->eFlags &= ~EF_MG42_ACTIVE;
      ps->powerups[PW_OPS_DISGUISED] = 0;
      break;
    default:
      ps->eFlags &= ~EF_MG42_ACTIVE;
      ps->eFlags &= ~EF_AAGUN_ACTIVE;
      break;
  }
}

namespace ETJump {
void setDensityBits(playerState_t *ps, entityState_t *s) {
  s->density = 0;

  if (ps->pm_type == PM_NOCLIP) {
    s->density |= static_cast<int>(PlayerDensityFlags::Noclip);
  }
}
} // namespace ETJump

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s,
                                 qboolean snap) {
  if (ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ||
      ps->stats[STAT_HEALTH] <= GIB_HEALTH ||
      (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR &&
       ps->pm_type == PM_NOCLIP)) {
    s->eType = ET_INVISIBLE;
  } else {
    s->eType = ET_PLAYER;
  }

  s->number = ps->clientNum;
  s->pos.trType = TR_INTERPOLATE;
  VectorCopy(ps->origin, s->pos.trBase);

  if (snap) {
    SnapVector(s->pos.trBase);
  }

  s->apos.trType = TR_INTERPOLATE;
  VectorCopy(ps->viewangles, s->apos.trBase);

  if (snap) {
    SnapVector(s->apos.trBase);
  }

  if (ps->movementDir > 128) {
    s->angles2[YAW] = static_cast<float>(ps->movementDir) - 256;
  } else {
    s->angles2[YAW] = static_cast<float>(ps->movementDir);
  }

  s->legsAnim = ps->legsAnim;
  s->torsoAnim = ps->torsoAnim;

  // ET_PLAYER looks here instead of at number
  // so corpses can also reference the proper config
  s->clientNum = ps->clientNum;

  // Ridah, let clients know if this person is using a mounted weapon,
  // so they don't show any client muzzle flashes
  if (ps->eFlags & EF_MOUNTEDTANK) {
    ps->eFlags &= ~EF_MG42_ACTIVE;
    ps->eFlags &= ~EF_AAGUN_ACTIVE;
  } else {
    BG_SetupMountedGunStatus(ps);
  }

  s->eFlags = ps->eFlags;

  if (ps->stats[STAT_HEALTH] <= 0) {
    s->eFlags |= EF_DEAD;
  } else {
    s->eFlags &= ~EF_DEAD;
  }

  // from MP
  if (ps->externalEvent) {
    s->event = ps->externalEvent;
    s->eventParm = ps->externalEventParm;
  } else if (ps->entityEventSequence < ps->eventSequence) {
    if (ps->entityEventSequence < ps->eventSequence - MAX_EVENTS) {
      ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
    }

    const int seq = ps->entityEventSequence & (MAX_EVENTS - 1);
    s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
    s->eventParm = ps->eventParms[seq];
    ps->entityEventSequence++;
  }
  // end

  // Ridah, now using a circular list of events for all entities
  // add any new events that have been added to the playerState_t
  // (possibly overwriting entityState_t events)
  for (int i = ps->oldEventSequence; i != ps->eventSequence; i++) {
    s->events[s->eventSequence & (MAX_EVENTS - 1)] =
        ps->events[i & (MAX_EVENTS - 1)];
    s->eventParms[s->eventSequence & (MAX_EVENTS - 1)] =
        ps->eventParms[i & (MAX_EVENTS - 1)];
    s->eventSequence++;
  }

  ps->oldEventSequence = ps->eventSequence;
  s->weapon = ps->weapon;
  s->groundEntityNum = ps->groundEntityNum;
  s->powerups = 0;

  for (int i = 0; i < MAX_POWERUPS; i++) {
    if (ps->powerups[i]) {
      s->powerups |= 1 << i;
    }
  }

  s->nextWeapon = ps->nextWeapon; // Ridah, s->loopSound = ps->loopSound;
  s->teamNum = ps->teamNum;
  s->aiState = ps->aiState; // xkan, 1/10/2003

  ETJump::setDensityBits(ps, s);
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate(playerState_t *ps, entityState_t *s,
                                            int time, qboolean snap) {
  int i;

  if (ps->pm_type == PM_INTERMISSION ||
      ps->pm_type == PM_SPECTATOR) // || ps->pm_flags & PMF_LIMBO ) { //
                                   // JPW NERVE limbo
  {
    s->eType = ET_INVISIBLE;
  } else if (ps->stats[STAT_HEALTH] <= GIB_HEALTH) {
    s->eType = ET_INVISIBLE;
  } else {
    s->eType = ET_PLAYER;
  }

  s->number = ps->clientNum;

  s->pos.trType = TR_LINEAR_STOP;
  VectorCopy(ps->origin, s->pos.trBase);
  if (snap) {
    SnapVector(s->pos.trBase);
  }
  // set the trDelta for flag direction and linear prediction
  VectorCopy(ps->velocity, s->pos.trDelta);
  // set the time for linear prediction
  s->pos.trTime = time;
  // set maximum extra polation time
  s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

  s->apos.trType = TR_INTERPOLATE;
  VectorCopy(ps->viewangles, s->apos.trBase);
  if (snap) {
    SnapVector(s->apos.trBase);
  }

  s->angles2[YAW] = ps->movementDir;
  s->legsAnim = ps->legsAnim;
  s->torsoAnim = ps->torsoAnim;
  s->clientNum =
      ps->clientNum; // ET_PLAYER looks here instead of at number
                     // so corpses can also reference the proper config

  if (ps->eFlags & EF_MOUNTEDTANK) {
    ps->eFlags &= ~EF_MG42_ACTIVE;
    ps->eFlags &= ~EF_AAGUN_ACTIVE;
  } else {
    BG_SetupMountedGunStatus(ps);
  }

  s->eFlags = ps->eFlags;
  if (ps->stats[STAT_HEALTH] <= 0) {
    s->eFlags |= EF_DEAD;
  } else {
    s->eFlags &= ~EF_DEAD;
  }

  if (ps->externalEvent) {
    s->event = ps->externalEvent;
    s->eventParm = ps->externalEventParm;
  } else if (ps->entityEventSequence < ps->eventSequence) {
    int seq;

    if (ps->entityEventSequence < ps->eventSequence - MAX_EVENTS) {
      ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
    }
    seq = ps->entityEventSequence & (MAX_EVENTS - 1);
    s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
    s->eventParm = ps->eventParms[seq];
    ps->entityEventSequence++;
  }

  // Ridah, now using a circular list of events for all entities
  // add any new events that have been added to the playerState_t
  // (possibly overwriting entityState_t events)
  for (i = ps->oldEventSequence; i != ps->eventSequence; i++) {
    s->events[s->eventSequence & (MAX_EVENTS - 1)] =
        ps->events[i & (MAX_EVENTS - 1)];
    s->eventParms[s->eventSequence & (MAX_EVENTS - 1)] =
        ps->eventParms[i & (MAX_EVENTS - 1)];
    s->eventSequence++;
  }
  ps->oldEventSequence = ps->eventSequence;

  s->weapon = ps->weapon;
  s->groundEntityNum = ps->groundEntityNum;

  s->powerups = 0;
  for (i = 0; i < MAX_POWERUPS; i++) {
    if (ps->powerups[i]) {
      s->powerups |= 1 << i;
    }
  }

  s->nextWeapon = ps->nextWeapon; // Ridah
  s->teamNum = ps->teamNum;
  s->aiState = ps->aiState; // xkan, 1/10/2003

  ETJump::setDensityBits(ps, s);
}

// Gordon: some weapons are duplicated for code puposes.... just want to treat
// them as a single
weapon_t BG_DuplicateWeapon(weapon_t weap) {
  switch (weap) {
    case WP_M7:
      return WP_GPG40;
    case WP_GARAND_SCOPE:
      return WP_GARAND;
    case WP_K43_SCOPE:
      return WP_K43;
    case WP_GRENADE_PINEAPPLE:
      return WP_GRENADE_LAUNCHER;
    default:
      return weap;
  }
}

gitem_t *BG_ValidStatWeapon(weapon_t weap) {
  weapon_t weap2;

  switch (weap) {
    case WP_MEDKIT:
    case WP_PLIERS:
    case WP_SMOKETRAIL:
    case WP_MEDIC_SYRINGE:
    case WP_SMOKE_BOMB:
    case WP_AMMO:
      return NULL;
    default:
      break;
  }

  if (!BG_WeaponInWolfMP(weap)) {
    return NULL;
  }

  weap2 = BG_DuplicateWeapon(weap);
  if (weap != weap2) {
    return NULL;
  }

  return BG_FindItemForWeapon(weap);
}

weapon_t BG_WeaponForMOD(int MOD) {
  int i;

  for (i = 0; i < WP_NUM_WEAPONS; i++) {
    if (GetAmmoTableData(i)->mod == MOD) {
      return static_cast<weapon_t>(i);
    }
  }

  return static_cast<weapon_t>(0);
}

const char *rankSoundNames_Allies[NUM_EXPERIENCE_LEVELS] = {
    "",
    "allies_hq_promo_private",
    "allies_hq_promo_corporal",
    "allies_hq_promo_sergeant",
    "allies_hq_promo_lieutenant",
    "allies_hq_promo_captain",
    "allies_hq_promo_major",
    "allies_hq_promo_colonel",
    "allies_hq_promo_general_brigadier",
    "allies_hq_promo_general_lieutenant",
    "allies_hq_promo_general",
};

const char *rankSoundNames_Axis[NUM_EXPERIENCE_LEVELS] = {
    "",
    "axis_hq_promo_private",
    "axis_hq_promo_corporal",
    "axis_hq_promo_sergeant",
    "axis_hq_promo_lieutenant",
    "axis_hq_promo_captain",
    "axis_hq_promo_major",
    "axis_hq_promo_colonel",
    "axis_hq_promo_general_major",
    "axis_hq_promo_general_lieutenant",
    "axis_hq_promo_general",
};

const char *rankNames_Axis[NUM_EXPERIENCE_LEVELS] = {
    "Schutze",      "Oberschutze",     "Gefreiter", "Feldwebel",
    "Leutnant",     "Hauptmann",       "Major",     "Oberst",
    "Generalmajor", "Generalleutnant", "General",
};

const char *rankNames_Allies[NUM_EXPERIENCE_LEVELS] = {
    "Private",
    "Private 1st Class",
    "Corporal",
    "Sergeant",
    "Lieutenant",
    "Captain",
    "Major",
    "Colonel",
    "Brigadier General",
    "Lieutenant General",
    "General",
};

const char *miniRankNames_Axis[NUM_EXPERIENCE_LEVELS] = {
    "Stz", "Otz", "Gfr", "Fwb", "Ltn", "Hpt", "Mjr", "Obs", "BGn", "LtG", "Gen",
};

const char *miniRankNames_Allies[NUM_EXPERIENCE_LEVELS] = {
    "Pvt", "PFC", "Cpl", "Sgt", "Lt", "Cpt", "Maj", "Cnl", "GMj", "GLt", "Gen",
};

/*
=============
BG_Find_PathCorner
=============
*/
pathCorner_t *BG_Find_PathCorner(const char *match) {
  int i;

  for (i = 0; i < numPathCorners; i++) {
    if (!Q_stricmp(pathCorners[i].name, match)) {
      return &pathCorners[i];
    }
  }

  return NULL;
}

/*
=============
BG_AddPathCorner
=============
*/
void BG_AddPathCorner(const char *name, vec3_t origin) {
  if (numPathCorners >= MAX_PATH_CORNERS) {
    Com_Error(ERR_DROP, "MAX PATH CORNERS (%i) hit", MAX_PATH_CORNERS);
  }

  VectorCopy(origin, pathCorners[numPathCorners].origin);
  Q_strncpyz(pathCorners[numPathCorners].name, name, 64);
  numPathCorners++;
}

/*
=============
BG_Find_Spline
=============
*/
splinePath_t *BG_Find_Spline(const char *match) {
  int i;

  for (i = 0; i < numSplinePaths; i++) {
    if (!Q_stricmp(splinePaths[i].point.name, match)) {
      return &splinePaths[i];
    }
  }

  return NULL;
}

splinePath_t *BG_AddSplinePath(const char *name, const char *target,
                               vec3_t origin) {
  splinePath_t *spline;
  if (numSplinePaths >= MAX_SPLINE_PATHS) {
    Com_Error(ERR_DROP, "MAX SPLINES (%i) hit", MAX_SPLINE_PATHS);
  }

  spline = &splinePaths[numSplinePaths];

  memset(spline, 0, sizeof(splinePath_t));

  VectorCopy(origin, spline->point.origin);

  Q_strncpyz(spline->point.name, name, 64);
  Q_strncpyz(spline->strTarget, target ? target : "", 64);

  spline->numControls = 0;

  numSplinePaths++;

  return spline;
}

void BG_AddSplineControl(splinePath_t *spline, const char *name) {
  if (spline->numControls >= MAX_SPLINE_CONTROLS) {
    Com_Error(ERR_DROP, "MAX SPLINE CONTROLS (%i) hit", MAX_SPLINE_CONTROLS);
  }

  Q_strncpyz(spline->controls[spline->numControls].name, name, 64);

  spline->numControls++;
}

float BG_SplineLength(splinePath_t *pSpline) {
  float i;
  float granularity = 0.01f;
  float dist = 0;
  //	float tension;
  vec3_t vec[2];
  vec3_t lastPoint = {0, 0, 0};
  vec3_t result;

  for (i = 0; i <= 1.f; i += granularity) {
    /*		if(pSpline->isStart) {
                tension = 1 - sin((1 - i) * M_PI * 0.5f);
            } else if(pSpline->isEnd) {
                tension = sin(i * M_PI * 0.5f);
            } else {
                tension = i;
            }*/

    BG_CalculateSpline_r(pSpline, vec[0], vec[1], i);
    VectorSubtract(vec[1], vec[0], result);
    VectorMA(vec[0], i, result, result);

    if (i != 0) {
      VectorSubtract(result, lastPoint, vec[0]);
      dist += VectorLength(vec[0]);
    }

    VectorCopy(result, lastPoint);
  }

  return dist;
}

void BG_BuildSplinePaths() {
  int i, j;
  pathCorner_t *pnt;
  splinePath_t *spline, *st;

  for (i = 0; i < numSplinePaths; i++) {
    spline = &splinePaths[i];

    if (*spline->strTarget) {
      for (j = 0; j < spline->numControls; j++) {
        pnt = BG_Find_PathCorner(spline->controls[j].name);

        if (!pnt) {
          Com_Printf("^1Cant find control "
                     "point (%s) for spline "
                     "(%s)\n",
                     spline->controls[j].name, spline->point.name);
          // Gordon: Just changing to a
          // warning for now, easier for
          // region compiles...
          continue;

        } else {
          VectorCopy(pnt->origin, spline->controls[j].origin);
        }
      }

      st = BG_Find_Spline(spline->strTarget);
      if (!st) {
        Com_Printf("^1Cant find target point "
                   "(%s) for spline (%s)\n",
                   spline->strTarget, spline->point.name);
        // Gordon: Just changing to a warning
        // for now, easier for region
        // compiles...
        continue;
      }

      spline->next = st;

      spline->length = BG_SplineLength(spline);
      BG_ComputeSegments(spline);
    }
  }

  for (i = 0; i < numSplinePaths; i++) {
    spline = &splinePaths[i];

    if (spline->next) {
      spline->next->prev = spline;
    }
  }
}

splinePath_t *BG_GetSplineData(int number, qboolean *backwards) {
  if (number < 0) {
    *backwards = qtrue;
    number = -number;
  } else {
    *backwards = qfalse;
  }
  number--;

  if (number < 0 || number >= numSplinePaths) {
    return NULL;
  }

  return &splinePaths[number];
}

int BG_MaxAmmoForWeapon(weapon_t weaponNum, int *skill) {
  switch (weaponNum) {
    // case WP_KNIFE:
    case WP_LUGER:
    case WP_COLT:
    case WP_STEN:
    case WP_SILENCER:
    case WP_CARBINE:
    case WP_KAR98:
    case WP_SILENCED_COLT:
      if (skill[SK_LIGHT_WEAPONS] >= 1) {
        return (GetAmmoTableData(weaponNum)->maxammo +
                GetAmmoTableData(weaponNum)->maxclip);
      } else {
        return (GetAmmoTableData(weaponNum)->maxammo);
      }
      break;
    case WP_MP40:
    case WP_THOMPSON:
      if (skill[SK_FIRST_AID] >= 1 || skill[SK_LIGHT_WEAPONS] >= 1) {
        return (GetAmmoTableData(weaponNum)->maxammo +
                GetAmmoTableData(weaponNum)->maxclip);
      } else {
        return (GetAmmoTableData(weaponNum)->maxammo);
      }
      break;
    case WP_M7:
    case WP_GPG40:
      if (skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1) {
        return (GetAmmoTableData(weaponNum)->maxammo + 4);
      } else {
        return (GetAmmoTableData(weaponNum)->maxammo);
      }
      break;
    case WP_GRENADE_PINEAPPLE:
    case WP_GRENADE_LAUNCHER:
      // FIXME: this is class dependant, not ammo table
      if (skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1) {
        return (GetAmmoTableData(weaponNum)->maxammo + 4);
      } else if (skill[SK_FIRST_AID] >= 1) {
        return (GetAmmoTableData(weaponNum)->maxammo + 1);
      } else {
        return (GetAmmoTableData(weaponNum)->maxammo);
      }
      break;
    /*case WP_MOBILE_MG42:
    case WP_PANZERFAUST:
    case WP_FLAMETHROWER:
        if( skill[SK_HEAVY_WEAPONS] >= 1 )
            return( GetAmmoTableData(weaponNum)->maxammo +
    GetAmmoTableData(weaponNum)->maxclip ); else return(
    GetAmmoTableData(weaponNum)->maxammo ); break; case
    WP_MORTAR: case WP_MORTAR_SET: if( skill[SK_HEAVY_WEAPONS]
    >= 1 ) return( GetAmmoTableData(weaponNum)->maxammo + 2 );
    else return( GetAmmoTableData(weaponNum)->maxammo );
    break;*/
    case WP_MEDIC_SYRINGE:
      if (skill[SK_FIRST_AID] >= 2) {
        return (GetAmmoTableData(weaponNum)->maxammo + 2);
      } else {
        return (GetAmmoTableData(weaponNum)->maxammo);
      }
      break;
    case WP_GARAND:
    case WP_K43:
    case WP_FG42:
      if (skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 1 ||
          skill[SK_LIGHT_WEAPONS] >= 1) {
        return (GetAmmoTableData(weaponNum)->maxammo +
                GetAmmoTableData(weaponNum)->maxclip);
      } else {
        return (GetAmmoTableData(weaponNum)->maxammo);
      }
      break;
    case WP_GARAND_SCOPE:
    case WP_K43_SCOPE:
    case WP_FG42SCOPE:
      if (skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 1) {
        return (GetAmmoTableData(weaponNum)->maxammo +
                GetAmmoTableData(weaponNum)->maxclip);
      } else {
        return (GetAmmoTableData(weaponNum)->maxammo);
      }
      break;
    default:
      return (GetAmmoTableData(weaponNum)->maxammo);
      break;
  }
}

/*
================
BG_CreateRotationMatrix
================
*/
void BG_CreateRotationMatrix(const vec3_t angles, vec3_t matrix[3]) {
  AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
  VectorInverse(matrix[1]);
}

/*
================
BG_TransposeMatrix
================
*/
void BG_TransposeMatrix(const vec3_t matrix[3], vec3_t transpose[3]) {
  int i, j;
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      transpose[i][j] = matrix[j][i];
    }
  }
}

/*
================
BG_RotatePoint
================
*/
void BG_RotatePoint(vec3_t point, const vec3_t matrix[3]) {
  vec3_t tvec;

  VectorCopy(point, tvec);
  point[0] = DotProduct(matrix[0], tvec);
  point[1] = DotProduct(matrix[1], tvec);
  point[2] = DotProduct(matrix[2], tvec);
}

/*
================
BG_AdjustAAGunMuzzleForBarrel
================
*/
void BG_AdjustAAGunMuzzleForBarrel(vec_t *origin, vec_t *forward, vec_t *right,
                                   vec_t *up, int barrel) {
  switch (barrel) {
    case 0:
      VectorMA(origin, 64, forward, origin);
      VectorMA(origin, 20, right, origin);
      VectorMA(origin, 40, up, origin);
      break;
    case 1:
      VectorMA(origin, 64, forward, origin);
      VectorMA(origin, 20, right, origin);
      VectorMA(origin, 20, up, origin);
      break;
    case 2:
      VectorMA(origin, 64, forward, origin);
      VectorMA(origin, -20, right, origin);
      VectorMA(origin, 40, up, origin);
      break;
    case 3:
      VectorMA(origin, 64, forward, origin);
      VectorMA(origin, -20, right, origin);
      VectorMA(origin, 20, up, origin);
      break;
  }
}

/*
=================
PC_SourceWarning
=================
*/
void PC_SourceWarning(int handle, const char *format, ...) {
  int line;
  char filename[128];
  va_list argptr;
  static char string[4096];

  va_start(argptr, format);
  Q_vsnprintf(string, sizeof(string), format, argptr);
  va_end(argptr);

  filename[0] = '\0';
  line = 0;
  trap_PC_SourceFileAndLine(handle, filename, &line);

  Com_Printf(S_COLOR_YELLOW "WARNING: %s, line %d: %s\n", filename, line,
             string);
}

/*
=================
PC_SourceError
=================
*/
void PC_SourceError(int handle, const char *format, ...) {
  int line;
  char filename[128];
  va_list argptr;
  static char string[4096];

  va_start(argptr, format);
  Q_vsnprintf(string, sizeof(string), format, argptr);
  va_end(argptr);

  filename[0] = '\0';
  line = 0;
  trap_PC_SourceFileAndLine(handle, filename, &line);

#ifdef GAMEDLL
  Com_Error(ERR_DROP, S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line,
            string);
#else
  Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);
#endif
}

namespace ETJump {
bool PC_hasFloat(int handle) {
  pc_token_t token;
  if (trap_PC_ReadToken(handle, &token)) {
    trap_PC_UnReadToken(handle);
    if (token.type == TT_NUMBER) {
      return true;
    }
  }
  return false;
}
} // namespace ETJump

/*
=================
PC_Float_Parse
=================
*/
qboolean PC_Float_Parse(int handle, float *f) {
  pc_token_t token;
  int negative = qfalse;

  if (!trap_PC_ReadToken(handle, &token)) {
    return qfalse;
  }
  if (token.string[0] == '-') {
    if (!trap_PC_ReadToken(handle, &token)) {
      return qfalse;
    }
    negative = qtrue;
  }
  if (token.type != TT_NUMBER) {
    PC_SourceError(handle, "expected float but found %s\n", token.string);
    return qfalse;
  }
  if (negative) {
    *f = -token.floatvalue;
  } else {
    *f = token.floatvalue;
  }
  return qtrue;
}

/*
=================
PC_Color_Parse
=================
*/
qboolean PC_Color_Parse(int handle, vec4_t *c) {
  int i;
  float f;

  for (i = 0; i < 4; i++) {
    if (!PC_Float_Parse(handle, &f)) {
      return qfalse;
    }
    (*c)[i] = f;
  }
  return qtrue;
}

/*
=================
PC_Vec_Parse
=================
*/
qboolean PC_Vec_Parse(int handle, vec3_t *c) {
  int i;
  float f;

  for (i = 0; i < 3; i++) {
    if (!PC_Float_Parse(handle, &f)) {
      return qfalse;
    }
    (*c)[i] = f;
  }
  return qtrue;
}

/*
=================
PC_Int_Parse
=================
*/
qboolean PC_Int_Parse(int handle, int *i) {
  pc_token_t token;
  int negative = qfalse;

  if (!trap_PC_ReadToken(handle, &token)) {
    return qfalse;
  }
  if (token.string[0] == '-') {
    if (!trap_PC_ReadToken(handle, &token)) {
      return qfalse;
    }
    negative = qtrue;
  }
  if (token.type != TT_NUMBER) {
    PC_SourceError(handle, "expected integer but found %s\n", token.string);
    return qfalse;
  }
  *i = token.intvalue;

  if (negative) {
    *i = -*i;
  }
  return qtrue;
}

#ifdef GAMEDLL
/*
=================
PC_String_Parse
=================
*/
const char *PC_String_Parse(int handle) {
  static char buf[MAX_TOKEN_CHARS];
  pc_token_t token;
  if (!trap_PC_ReadToken(handle, &token)) {
    return NULL;
  }

  Q_strncpyz(buf, token.string, MAX_TOKEN_CHARS);
  return buf;
}
#else
/*
=================
PC_String_Parse
=================
*/
qboolean PC_String_Parse(int handle, const char **out) {
  pc_token_t token;
  if (!trap_PC_ReadToken(handle, &token)) {
    return qfalse;
  }

  *(out) = String_Alloc(token.string);
  return qtrue;
}

// same as above but converts to lower before allocating in the string pool
qboolean PC_String_ParseLower(int handle, const char **out) {
  char buf[MAX_TOKEN_CHARS];
  pc_token_t token;
  if (!trap_PC_ReadToken(handle, &token)) {
    return qfalse;
  }
  Q_strncpyz(buf, token.string, sizeof(buf));
  Q_strlwr(buf);

  *(out) = String_Alloc(buf);
  return qtrue;
}
#endif

/*
=================
PC_String_ParseNoAlloc

Same as one above, but uses a static buff and not the string memory pool
=================
*/
qboolean PC_String_ParseNoAlloc(int handle, char *out, size_t size) {
  pc_token_t token;

  if (!trap_PC_ReadToken(handle, &token)) {
    return qfalse;
  }

  Q_strncpyz(out, token.string, size);
  return qtrue;
}

const char *bg_fireteamNames[MAX_FIRETEAMS] = {
    "Alpha", "Bravo", "Charlie", "Delta",  "Echo", "Foxtrot",
    "Golf",  "Hotel", "India",   "Juliet", "Kilo", "Lima"};

const voteType_t voteToggles[] = {
    {"vote_allow_map", CV_SVF_MAP},
    {"vote_allow_matchreset", CV_SVF_MATCHRESET},
    {"vote_allow_randommap", CV_SVF_RANDOMMAP},
    {"vote_allow_rtv", CV_SVF_RTV},
    {"vote_allow_autoRtv", CV_SVF_AUTORTV},
    {"vote_allow_portalPredict", CV_SVF_PORTALPREDICT},
};

const char *BG_TeamnameForNumber(team_t teamNum) {
  switch (teamNum) {
    case TEAM_AXIS:
      return "axis";
    case TEAM_ALLIES:
      return "allies";
    case TEAM_SPECTATOR:
      return "spectator";
    default:
      Com_Error(ERR_DROP, va("BG_TeamnameForNumber: unknown team %d", teamNum));
      return "unknown";
  }
}

int numVotesAvailable = sizeof(voteToggles) / sizeof(voteType_t);

// consts to offset random reinforcement seeds
const unsigned int aReinfSeeds[MAX_REINFSEEDS] = {11, 3, 13, 7, 2, 5, 1, 17};

// Weapon full names + headshot capability
const weap_ws_t aWeaponInfo[WS_MAX] = {
    {qfalse, "KNIF", "Knife"},     // 0
    {qtrue, "LUGR", "Luger"},      // 1
    {qtrue, "COLT", "Colt"},       // 2
    {qtrue, "MP40", "MP-40"},      // 3
    {qtrue, "TMPS", "Thompson"},   // 4
    {qtrue, "STEN", "Sten"},       // 5
    {qtrue, "FG42", "FG-42"},      // 6
    {qtrue, "PNZR", "Panzer"},     // 7
    {qtrue, "FLAM", "F.Thrower"},  // 8
    {qfalse, "GRND", "Grenade"},   // 9
    {qfalse, "MRTR", "Mortar"},    // 10
    {qfalse, "DYNA", "Dynamite"},  // 11
    {qfalse, "ARST", "Airstrike"}, // 12
    {qfalse, "ARTY", "Artillery"}, // 13
    {qfalse, "SRNG", "Syringe"},   // 14
    {qfalse, "SMOK", "SmokeScrn"}, // 15
    {qfalse, "STCH", "Satchel"},   // 16
    {qfalse, "GRLN", "G.Launchr"}, // 17
    {qfalse, "LNMN", "Landmine"},  // 18
    {qtrue, "MG42", "MG-42 Gun"},  // 19
    {qtrue, "GARN", "Garand"},     // 20
    {qtrue, "K-43", "K43 Rifle"}   // 21
};

// Multiview: Convert weaponstate to simpler format
int BG_simpleWeaponState(int ws) {
  switch (ws) {
    case WEAPON_READY:
    case WEAPON_READYING:
    case WEAPON_RELAXING:
      return (WSTATE_IDLE);
    case WEAPON_RAISING:
    case WEAPON_DROPPING:
    case WEAPON_DROPPING_TORELOAD:
      return (WSTATE_SWITCH);
    case WEAPON_FIRING:
    case WEAPON_FIRINGALT:
      return (WSTATE_FIRE);
    case WEAPON_RELOADING:
      return (WSTATE_RELOAD);
  }

  return (WSTATE_IDLE);
}

// Multiview: Reduce hint info to 2 bits.  However, we can really
// have up to 8 values, as some hints will have a 0 value for
// cursorHintVal
int BG_simpleHintsCollapse(int hint, int val) {
  switch (hint) {
    case HINT_DISARM:
      if (val > 0) {
        return (0);
      }
      break;
    case HINT_BUILD:
      if (val > 0) {
        return (1);
      }
      break;
    case HINT_BREAKABLE:
      if (val == 0) {
        return (1);
      }
      break;
    case HINT_DOOR_ROTATING:
    case HINT_BUTTON:
    case HINT_MG42:
      if (val == 0) {
        return (2);
      }
      break;
    case HINT_BREAKABLE_DYNAMITE:
      if (val == 0) {
        return (3);
      }
      break;
    default:
      break;
  }

  return (0);
}

// Multiview: Expand the hints.  Because we map a couple hints
// into a single value, we can't replicate the proper hint back
// in all cases.
int BG_simpleHintsExpand(int hint, int val) {
  switch (hint) {
    case 0:
      return ((val >= 0) ? HINT_DISARM : 0);
    case 1:
      return ((val >= 0) ? HINT_BUILD : HINT_BREAKABLE);
    case 2:
      return ((val >= 0) ? HINT_BUILD : HINT_MG42);
    case 3:
      return ((val >= 0) ? HINT_BUILD : HINT_BREAKABLE_DYNAMITE);
  }

  return (0);
}

// Real printable charater count
int BG_drawStrlen(const char *str) {
  int cnt = 0;

  while (*str) {
    if (Q_IsColorString(str)) {
      str += 2;
    } else {
      cnt++;
      str++;
    }
  }
  return (cnt);
}

// Copies a color string, with limit of real chars to print
//		in = reference buffer w/color
//		out = target buffer
//		str_max = max size of printable string
//		out_max = max size of target buffer
//
// Returns size of printable string
int BG_colorstrncpyz(char *in, char *out, int str_max, int out_max) {
  int str_len = 0; // current printable string size
  int out_len = 0; // current true string size
  const int in_len = strlen(in);

  out_max--;
  while (*in && out_len < out_max && str_len < str_max) {
    if (*in == '^') {
      if (out_len + 2 >= in_len && out_len + 2 >= out_max) {
        break;
      }

      *out++ = *in++;
      *out++ = *in++;
      out_len += 2;
      continue;
    }

    *out++ = *in++;
    str_len++;
    out_len++;
  }

  *out = 0;

  return (str_len);
}

int BG_strRelPos(char *in, int index) {
  int cPrintable = 0;
  const char *ref = in;

  while (*ref && cPrintable < index) {
    if (Q_IsColorString(ref)) {
      ref += 2;
    } else {
      ref++;
      cPrintable++;
    }
  }

  return (ref - in);
}

// strip colors and control codes, copying up to dwMaxLength-1 "good" chars and
// nul-terminating returns the length of the cleaned string
int BG_cleanName(const char *pszIn, char *pszOut, unsigned int dwMaxLength,
                 qboolean fCRLF) {
  const char *pInCopy = pszIn;
  const char *pszOutStart = pszOut;

  while (*pInCopy &&
         (static_cast<unsigned int>(pszOut - pszOutStart) < dwMaxLength - 1)) {
    if (*pInCopy == '^') {
      pInCopy += ((pInCopy[1] == 0) ? 1 : 2);
    } else if ((*pInCopy < 32 && (!fCRLF || *pInCopy != '\n')) ||
               (*pInCopy > 126)) {
      pInCopy++;
    } else {
      *pszOut++ = *pInCopy++;
    }
  }

  *pszOut = 0;
  return (pszOut - pszOutStart);
}

// Only used locally
typedef struct {
  const char *colorname;
  vec4_t *color;
} colorTable_t;

// Colors for crosshairs
colorTable_t OSP_Colortable[] = {
    {"white", &colorWhite},       {"red", &colorRed},
    {"green", &colorGreen},       {"blue", &colorBlue},
    {"yellow", &colorYellow},     {"magenta", &colorMagenta},
    {"cyan", &colorCyan},         {"orange", &colorOrange},
    {"mdred", &colorMdRed},       {"mdgreen", &colorMdGreen},
    {"dkgreen", &colorDkGreen},   {"mdcyan", &colorMdCyan},
    {"mdyellow", &colorMdYellow}, {"mdorange", &colorMdOrange},
    {"mdblue", &colorMdBlue},     {"ltgrey", &colorLtGrey},
    {"mdgrey", &colorMdGrey},     {"dkgrey", &colorDkGrey},
    {"black", &colorBlack},       {NULL, NULL}};

extern void trap_Cvar_Set(const char *var_name, const char *value);

void BG_setColor(char *colString, vec4_t col, float alpha,
                 const char *cvarName) {
  char *s = colString;

  col[0] = 1.0f;
  col[1] = 1.0f;
  col[2] = 1.0f;
  col[3] = (alpha > 1.0f) ? 1.0f : (alpha < 0.0f) ? 0.0f : alpha;

  if (*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X')) {
    s += 2;
    // parse rrggbb
    if (Q_IsHexColorString(s)) {
      col[0] = ((float)(gethex(*(s)) * 16 + gethex(*(s + 1)))) / 255.00;
      col[1] = ((float)(gethex(*(s + 2)) * 16 + gethex(*(s + 3)))) / 255.00;
      col[2] = ((float)(gethex(*(s + 4)) * 16 + gethex(*(s + 5)))) / 255.00;
      return;
    }
  } else {
    int i = 0;
    while (OSP_Colortable[i].colorname != NULL) {
      if (Q_stricmp(s, OSP_Colortable[i].colorname) == 0) {
        col[0] = (*OSP_Colortable[i].color)[0];
        col[1] = (*OSP_Colortable[i].color)[1];
        col[2] = (*OSP_Colortable[i].color)[2];
        return;
      }
      i++;
    }
  }

  trap_Cvar_Set(cvarName, "White");
}

qboolean BG_isLightWeaponSupportingFastReload(int weapon) {
  if (weapon == WP_LUGER || weapon == WP_COLT || weapon == WP_MP40 ||
      weapon == WP_THOMPSON || weapon == WP_STEN || weapon == WP_SILENCER ||
      weapon == WP_FG42 || weapon == WP_SILENCED_COLT) {
    return qtrue;
  }
  return qfalse;
}

qboolean BG_IsScopedWeapon(int weapon) {
  switch (weapon) {
    case WP_GARAND_SCOPE:
    case WP_K43_SCOPE:
    case WP_FG42SCOPE:
      return qtrue;
  }
  return qfalse;
}

qboolean BG_BBoxCollision(vec3_t min1, vec3_t max1, vec3_t min2, vec3_t max2) {
  int i;

  for (i = 0; i < 3; i++) {
    if (min1[i] > max2[i]) {
      return qfalse;
    }
    if (min2[i] > max1[i]) {
      return qfalse;
    }
  }

  return qtrue;
}

weapon_t bg_heavyWeapons[NUM_HEAVY_WEAPONS] = {
    WP_FLAMETHROWER, WP_MOBILE_MG42, WP_MOBILE_MG42_SET,
    WP_PANZERFAUST,  WP_MORTAR,      WP_MORTAR_SET};

/////////////////////////

int BG_FootstepForSurface(int surfaceFlags) {
  if (surfaceFlags & SURF_NOSTEPS) {
    return FOOTSTEP_TOTAL;
  }

  if (surfaceFlags & SURF_METAL) {
    return FOOTSTEP_METAL;
  }

  if (surfaceFlags & SURF_WOOD) {
    return FOOTSTEP_WOOD;
  }

  if (surfaceFlags & SURF_GRASS) {
    return FOOTSTEP_GRASS;
  }

  if (surfaceFlags & SURF_GRAVEL) {
    return FOOTSTEP_GRAVEL;
  }

  if (surfaceFlags & SURF_ROOF) {
    return FOOTSTEP_ROOF;
  }

  if (surfaceFlags & SURF_SNOW) {
    return FOOTSTEP_SNOW;
  }

  if (surfaceFlags & SURF_CARPET) {
    return FOOTSTEP_CARPET;
  }

  if (surfaceFlags & SURF_SPLASH) {
    return FOOTSTEP_SPLASH;
  }

  return FOOTSTEP_NORMAL;
}

/*
============
Q_vsnprintf

vsnprintf portability:

C99 standard: vsnprintf returns the number of characters (excluding the trailing
'\0') which would have been written to the final string if enough space had been
available snprintf and vsnprintf do not write more than size bytes (including
the trailing '\0')

win32: _vsnprintf returns the number of characters written, not including the
terminating null character, or a negative value if an output error occurs. If
the number of characters to write exceeds count, then count characters are
written and -1 is returned and no trailing '\0' is added.

Q_vsnPrintf: always append a trailing '\0', returns number of characters written
or returns -1 on failure or if the buffer would be overflowed.

copied over from common.c implementation
============
*/
int Q_vsnprintf(char *dest, int size, const char *fmt, va_list argptr) {
  int ret;

#ifdef _WIN32
  #undef _vsnprintf
  ret = _vsnprintf(dest, size - 1, fmt, argptr);
  #define _vsnprintf use_idStr_vsnPrintf
#else
  #undef vsnprintf
  ret = vsnprintf(dest, size, fmt, argptr);
  #define vsnprintf use_idStr_vsnPrintf
#endif
  dest[size - 1] = '\0';
  if (ret < 0 || ret >= size) {
    return -1;
  }
  return ret;
}

// Feen: Misc color functions intended for use with PGM

void RGBtoHSL(const vec4_t rgb, vec4_t *hsl) {

  float R, G, B;
  float d_R, d_G, d_B; // delta vars

  float rgb_min, rgb_max, del_max; // min/max RGB values and delta

  // NOTE: Probably don't need to do this since values
  //		SHOULD be passed in as floats anyway...
  R = rgb[0]; // / 255.f;
  G = rgb[1]; // / 255.f;
  B = rgb[2]; // / 255.f;

  *hsl[3] = rgb[3]; // Alpha is not changing...

  rgb_min = std::min(R, G);
  rgb_min = std::min(rgb_min, B);

  rgb_max = std::max(R, G);
  rgb_max = std::max(rgb_max, B);

  del_max = rgb_max - rgb_min;

  *hsl[2] = (rgb_max + rgb_min) / 2.f; // L

  if (del_max == 0) // This is a gray, no chroma...
  {
    *hsl[0] = 0; // H
    *hsl[1] = 0; // S
  } else         // Chromatic data...
  {
    if (*hsl[2] < 0.5) {
      *hsl[1] = del_max / (rgb_max + rgb_min);
    } else {
      *hsl[1] = del_max / (2 - (rgb_max + rgb_min));
    }

    d_R = (((rgb_max - R) / 6) + (del_max / 2)) / del_max;
    d_G = (((rgb_max - G) / 6) + (del_max / 2)) / del_max;
    d_B = (((rgb_max - B) / 6) + (del_max / 2)) / del_max;

    if (R == rgb_max) {
      *hsl[0] = d_B - d_G;
    } else if (G == rgb_max) {
      *hsl[0] = (1 / 3) + d_R - d_B;
    } else if (G == rgb_max) {
      *hsl[0] = (1 / 3) + d_R - d_B;
    }

    if (*hsl[0] < 0) {
      *hsl[0] += 1;
    }
    if (*hsl[0] > 1) {
      *hsl[0] -= 1;
    }
  }
}

// NOTE: Helper method to HSLtoRGB
float Hue_2_RGB(float v1, float v2, float vH) {
  if ((6 * vH) < 1) {
    return (v1 + (v2 - v1) * 6 * vH);
  }
  if ((2 * vH) < 1) {
    return (v2);
  }
  if ((3 * vH) < 2) {
    return (v1 + (v2 - v1) * ((2 / 3) - vH) * 6);
  }

  return (v1);
}

void HSLtoRGB(const vec4_t HSL, vec4_t *RGB) {

  if (HSL[1] == 0) // HSL from 0 to 1
  {
    *RGB[0] = HSL[2];
    *RGB[1] = HSL[2];
    *RGB[2] = HSL[2];

    *RGB[3] = HSL[3]; // Alpha - forever alone......

  } else {

    float t1, t2; // Temporary vars

    if (HSL[2] < 0.5f) {
      t2 = HSL[2] * (1 + HSL[1]);
    } else {
      t2 = (HSL[2] + HSL[1]) - (HSL[1] - HSL[2]);
    }

    t1 = 2 * HSL[2] - t2;

    *RGB[0] = Hue_2_RGB(t1, t2, HSL[0] + (1 / 3));
    *RGB[1] = Hue_2_RGB(t1, t2, HSL[0]);
    *RGB[2] = Hue_2_RGB(t1, t2, HSL[0] - (1 / 3));

    *RGB[3] = HSL[3];
  }
}

void BG_ColorComplement(const vec4_t in_RGB, vec4_t *out_RGB) {

  vec4_t temp_RGB;

  RGBtoHSL(in_RGB, &temp_RGB);

  // Now temp_RGB is HSL, add 180deg to H

  temp_RGB[0] += 0.5f; // 180/360 (deg)

  // Keep it within range...
  if (temp_RGB[0] > 1.0f) {
    temp_RGB[0] -= 1.0f;
  }

  // Change it back
  HSLtoRGB(temp_RGB, out_RGB);

  return;
}

/*
================
BG_DropItems
Checks shared for nodrop worldspawn key value and current content flags,
and returns whether we should drop items to floor or delete them
================
*/
bool BG_DropItems(const int contents, const int shared) {
  if (!(shared & BG_LEVEL_NO_DROP) && !(contents & CONTENTS_NODROP)) {
    return true;
  }

  if ((shared & BG_LEVEL_NO_DROP) && (contents & CONTENTS_NODROP)) {
    return true;
  }

  return false;
}
