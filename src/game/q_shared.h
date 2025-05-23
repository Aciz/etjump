#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

// #define PRE_RELEASE_DEMO

#ifndef PRE_RELEASE_DEMO
  #define Q3_VERSION "ET 2.60"
#else
  #define Q3_VERSION "ET 2.32"
#endif // PRE_RELEASE_DEMO
// 2.6x: Enemy Territory - ETPro team maintenance release
// 2.5x: Enemy Territory FINAL
// 2.4x: Enemey Territory RC's
// 2.3x: Enemey Territory TEST
// 2.2+: post SP removal
// 2.1+: post Enemy Territory moved standalone
// 2.x: post Enemy Territory
// 1.x: pre Enemy Territory
////
// 1.3-MP : final for release
// 1.1b - TTimo SP linux release (+ MP updates)
// 1.1b5 - Mac update merge in

inline constexpr char CONFIG_NAME[] = "etconfig.cfg";

// #define LOCALIZATION_SUPPORT

#define MOD_CHECK_ETLEGACY(isETLegacy, versionNum, outputValue)                \
  outputValue = ((isETLegacy) ? true : false);                                 \
  if (outputValue) {                                                           \
    (outputValue) = versionNum;                                                \
  }

#if defined _WIN32 && !defined __GNUC__

  #pragma warning(disable : 4018) // signed/unsigned mismatch
  #pragma warning(disable : 4032)
  #pragma warning(disable : 4051)
  #pragma warning(disable : 4057) // slightly different base types
  #pragma warning(disable : 4100) // unreferenced formal parameter
  #pragma warning(disable : 4115)
  #pragma warning(disable : 4125) // decimal digit terminates octal
                                  // escape sequence
  #pragma warning(disable : 4127) // conditional expression is constant
  #pragma warning(disable : 4136)
  #pragma warning(disable : 4152) // nonstandard extension, function/data
                                  // pointer conversion in expression
  #pragma warning(disable : 4201)
  #pragma warning(disable : 4214)
  #pragma warning(disable : 4244)
  // #pragma warning(disable	: 4142)		// benign redefinition
  #pragma warning(disable : 4305) // truncation from const double to float
  // #pragma warning(disable : 4310)		// cast truncates constant
  //  value #pragma warning(disable :	4505)		// unreferenced
  //  local function
  //  has been removed
  #pragma warning(disable : 4514)
  #pragma warning(disable : 4702) // unreachable code
  #pragma warning(disable : 4711) // selected for automatic inline expansion
  #pragma warning(disable : 4220) // varargs matches remaining parameters
#endif

#if defined(ppc) || defined(__ppc) || defined(__ppc__) || defined(__POWERPC__)
  #define idppc 1
#endif

/**********************************************************************
  VM Considerations

  The VM can not use the standard system headers because we aren't really
  using the compiler they were meant for.  We use bg_lib.h which contains
  prototypes for the functions we define for our own use in bg_lib.c.

  When writing mods, please add needed headers HERE, do not start including
  stuff like <stdio.h> in the various .c files that make up each of the VMs
  since you will be including system headers files can will have issues.

  Remember, if you use a C library function that is not defined in bg_lib.c,
  you will have to add your own version for support in the VM.

 **********************************************************************/

#ifdef Q3_VM

  #include "bg_lib.h"

#else

  #include <assert.h>
  #include <cmath>
  #include <stdio.h>
  #include <stdarg.h>
  #include <string.h>
  #include <stdlib.h>
  #include <time.h>
  #include <ctype.h>
  #include <limits.h>
  #include <sys/stat.h> // rain
  #include <float.h>
  #include <cstdlib>
  #include <string>
  #include <cstdint>
  #include <array>

#endif

#include "etj_syscalls.h"

#ifdef _WIN32

  // #pragma intrinsic( memset, memcpy )

#endif

// this is the define for determining if we have an asm version of a C function
#if (defined _M_IX86 || defined __i386__) && !defined __sun__ &&               \
    !defined __LCC__
  #define id386 1
#else
  #define id386 0
#endif

// for windows fastcall option

#define QDECL

// bani
//======================= GNUC DEFINES ==================================
#ifdef __GNUC__
  #define _attribute(x) __attribute__(x)
#else
  #define _attribute(x)
#endif

//======================= WIN32 DEFINES =================================

#ifdef WIN32

  #define MAC_STATIC

  #undef QDECL
  #define QDECL __cdecl

  // buildstring will be incorporated into the version string
  #ifdef _WIN64
    #define CPUSTRING "win-x64"
  #else
    #define CPUSTRING "win-x86"
  #endif

inline constexpr char PATH_SEP_STRING[] = "\\";
inline constexpr char PATH_SEP = '\\';

#endif

//======================= MAC OS X SERVER DEFINES =====================

#if defined(__APPLE__)

  #define MAC_STATIC

  #define CPUSTRING "MacOS"

inline constexpr char PATH_SEP_STRING[] = "/";
inline constexpr char PATH_SEP = '/';

#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

  #define MAC_STATIC

  #define CPUSTRING "MacOS-PPC"

inline constexpr char PATH_SEP_STRING[] = "/";
inline constexpr char PATH_SEP = '/';

void Sys_PumpEvents(void);

#endif

//======================= LINUX DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __linux__

  #define MAC_STATIC

  #ifdef __i386__
    #define CPUSTRING "linux-i386"
  #elif defined __x86_64__
    #define CPUSTRING "linux-x86_64"
  #elif defined __axp__
    #define CPUSTRING "linux-alpha"
  #elif defined ARM
    #define CPUSTRING "linux-arm"
  #else
    #define CPUSTRING "linux-other"
  #endif

inline constexpr char PATH_SEP_STRING[] = "/";
inline constexpr char PATH_SEP = '/';

#endif

//=============================================================

// relative path to a source file (src/foo/bar.cpp)
#define SRC_FILENAME ((__FILE__) + (SOURCE_PATH_SIZE))

// tell compilers that trap_Error syscalls abort execution
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
  #define UNREACHABLE __assume(false);
#else // GCC, Clang
  #define UNREACHABLE __builtin_unreachable();
#endif

typedef unsigned char byte;

typedef enum { qfalse, qtrue } qboolean;

typedef union {
  float f;
  int i;
  unsigned int ui;
} floatint_t;

typedef int qhandle_t;
typedef int sfxHandle_t;
typedef int fileHandle_t;
typedef int clipHandle_t;

// networked fields in entityState_t and playerState_t,
// with their respective networked sizes
typedef int net_uint2_t;
typedef int net_uint4_t;
typedef int net_uint7_t;
typedef int net_uint8_t;
typedef int net_uint9_t;
typedef int net_uint10_t;
typedef int net_uint16_t;
typedef int net_uint24_t;

typedef int net_int8_t;
typedef int net_int16_t;
typedef int net_int32_t;

typedef float net_float;

// allow sound to be cut off by any following sounds on this channel
inline constexpr int SND_OKTOCUT = 0x001;
// allow sound to be cut off by following sounds on this channel
// only for sounds who request cutoff
inline constexpr int SND_REQUESTCUT = 0x002;
// cut off sounds on this channel that are marked 'SND_REQUESTCUT'
inline constexpr int SND_CUTOFF = 0x004;
// cut off all sounds on this channel
inline constexpr int SND_CUTOFF_ALL = 0x008;
// don't cut off, always let finish (overridden by SND_CUTOFF_ALL)
inline constexpr int SND_NOCUT = 0x010;
// don't attenuate (even though the sound is in voice channel, for example)
inline constexpr int SND_NO_ATTENUATION = 0x020;

#ifndef NULL
  #define NULL ((void *)0)
#endif

inline constexpr int32_t MAX_QINT = 0x7fffffff;
inline constexpr int32_t MIN_QINT = -MAX_QINT - 1;

// angle indexes
inline constexpr int PITCH = 0; // up / down
inline constexpr int YAW = 1;   // left / right
inline constexpr int ROLL = 2;  // fall over

// the game guarantees that no string from the network
// will ever exceed MAX_STRING_CHARS
// max length of a string passed to Cmd_TokenizeString
inline constexpr int MAX_STRING_CHARS = 1024;
// max tokens resulting from Cmd_TokenizeString
inline constexpr int MAX_STRING_TOKENS = 256;
// max length of an individual token
inline constexpr int MAX_TOKEN_CHARS = 1024;

inline constexpr int MAX_INFO_STRING = 1024;
inline constexpr int MAX_INFO_KEY = 1024;
inline constexpr int MAX_INFO_VALUE = 1024;

inline constexpr int BIG_INFO_STRING = 8192; // used for system info key only
inline constexpr int BIG_INFO_KEY = 8192;
inline constexpr int BIG_INFO_VALUE = 8192;

inline constexpr int MAX_QPATH = 64;   // max length of a quake game pathname
inline constexpr int MAX_OSPATH = 256; // max length of a filesystem pathname

// rain - increased to 36 to match MAX_NETNAME, fixes #13 - UI stuff breaks
// with very long names
inline constexpr int MAX_NAME_LENGTH = 36; // max length of a client name

inline constexpr int MAX_SAY_TEXT = 265;
inline constexpr int MAX_CHAT_TEXT = MAX_SAY_TEXT - 64;

inline constexpr int MAX_BINARY_MESSAGE = 32768; // max length of binary message

inline constexpr int MAX_VA_STRING = 32000;

// delimiter characters for command tokenization
constexpr std::array<char, 3> tokenDelimiters = {';', '\n', '\r'};

typedef enum {
  MESSAGE_EMPTY = 0,
  MESSAGE_WAITING,          // rate/packet limited
  MESSAGE_WAITING_OVERFLOW, // packet too large with message
} messageStatus_t;

// paramters for command buffer stuffing
typedef enum {
  EXEC_NOW,    // don't return until completed, a VM should NEVER use this,
               // because some commands might cause the VM to be unloaded...
  EXEC_INSERT, // insert at current position, but don't run yet
  EXEC_APPEND  // add to end of the command buffer (normal case)
} cbufExec_t;

inline constexpr int MAX_MAP_AREA_BYTES = 32; // bit vector of area visibility

// print levels from renderer (FIXME: set up for game / cgame?)
typedef enum {
  PRINT_ALL,
  PRINT_DEVELOPER, // only print when "developer 1"
  PRINT_WARNING,
  PRINT_ERROR
} printParm_t;

#ifdef ERR_FATAL
  #undef ERR_FATAL // this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum {
  ERR_FATAL,            // exit the entire game with a popup window
  ERR_VID_FATAL,        // exit the entire game with a popup window and doesn't
                        // delete profile.pid
  ERR_DROP,             // print to console and disconnect from game
  ERR_SERVERDISCONNECT, // don't kill server
  ERR_DISCONNECT,       // client disconnected from the server
  ERR_NEED_CD,          // pop up the need-cd dialog
  ERR_AUTOUPDATE
} errorParm_t;

// font rendering values used by ui and cgame

inline constexpr int PROP_GAP_WIDTH = 3;
inline constexpr int PROP_SPACE_WIDTH = 8;
inline constexpr int PROP_HEIGHT = 27;
inline constexpr float PROP_SMALL_SIZE_SCALE = 0.75f;

// only used in bitwise checks and should stay int
inline constexpr int BLINK_DIVISOR = 200;
// floating point for higher pulse accuracy
inline constexpr float PULSE_DIVISOR = 75.0f;

inline constexpr int UI_LEFT = 0x00000000; // default
inline constexpr int UI_CENTER = 0x00000001;
inline constexpr int UI_RIGHT = 0x00000002;
inline constexpr int UI_FORMATMASK = 0x00000007;
inline constexpr int UI_SMALLFONT = 0x00000010;
inline constexpr int UI_BIGFONT = 0x00000020; // default
inline constexpr int UI_GIANTFONT = 0x00000040;
inline constexpr int UI_DROPSHADOW = 0x00000800;
inline constexpr int UI_BLINK = 0x00001000;
inline constexpr int UI_INVERSE = 0x00002000;
inline constexpr int UI_PULSE = 0x00004000;
inline constexpr int UI_MENULEFT = 0x00008000;
inline constexpr int UI_MENURIGHT = 0x00010000;
inline constexpr int UI_EXSMALLFONT = 0x00020000;
inline constexpr int UI_MENUFULL = 0x00080000;

inline constexpr int UI_SMALLFONT75 = 0x00100000;

inline constexpr int CIN_system = 1;
inline constexpr int CIN_loop = 2;
inline constexpr int CIN_hold = 4;
inline constexpr int CIN_silent = 8;
inline constexpr int CIN_shader = 16;

/*
==============================================================

MATHLIB

==============================================================
*/

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
  #define M_PI 3.14159265358979323846f // matches value in gcc v2 math.h
#endif

inline constexpr int NUMVERTEXNORMALS = 162;
extern vec3_t bytedirs[NUMVERTEXNORMALS];

inline constexpr int MINICHAR_WIDTH = 8;
inline constexpr int MINICHAR_HEIGHT = 12;

inline constexpr int SMALLCHAR_WIDTH = 8;
inline constexpr int SMALLCHAR_HEIGHT = 16;

inline constexpr int TINYCHAR_WIDTH = SMALLCHAR_WIDTH;
inline constexpr int TINYCHAR_HEIGHT = SMALLCHAR_HEIGHT;

inline constexpr int BIGCHAR_WIDTH = 16;
inline constexpr int BIGCHAR_HEIGHT = 16;

inline constexpr int GIANTCHAR_WIDTH = 32;
inline constexpr int GIANTCHAR_HEIGHT = 48;

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorOrange;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;
extern vec4_t colorMdRed;
extern vec4_t colorMdGreen;
extern vec4_t colorDkGreen;
extern vec4_t colorMdCyan;
extern vec4_t colorMdYellow;
extern vec4_t colorMdOrange;
extern vec4_t colorMdBlue;

extern vec4_t clrBrown;
extern vec4_t clrBrownDk;
extern vec4_t clrBrownLine;
extern vec4_t clrBrownText;
extern vec4_t clrBrownTextDk;
extern vec4_t clrBrownTextDk2;
extern vec4_t clrBrownTextLt;
extern vec4_t clrBrownTextLt2;
extern vec4_t clrBrownLineFull;

inline constexpr int GAME_INIT_FRAMES = 6;
inline constexpr int FRAMETIME = 100; // msec

inline constexpr char Q_COLOR_ESCAPE = '^';
#define Q_IsColorString(p)                                                     \
  (p && *(p) == Q_COLOR_ESCAPE && *((p) + 1) && *((p) + 1) != Q_COLOR_ESCAPE)

inline constexpr char COLOR_BLACK = '0';
inline constexpr char COLOR_RED = '1';
inline constexpr char COLOR_GREEN = '2';
inline constexpr char COLOR_YELLOW = '3';
inline constexpr char COLOR_BLUE = '4';
inline constexpr char COLOR_CYAN = '5';
inline constexpr char COLOR_MAGENTA = '6';
inline constexpr char COLOR_WHITE = '7';
inline constexpr char COLOR_ORANGE = '8';
inline constexpr char COLOR_MDGREY = '9';
inline constexpr char COLOR_LTGREY = ':';
inline constexpr char COLOR_MDGREEN = '<';
inline constexpr char COLOR_MDYELLOW = '=';
inline constexpr char COLOR_MDBLUE = '>';
inline constexpr char COLOR_MDRED = '?';
inline constexpr char COLOR_LTORANGE = 'A';
inline constexpr char COLOR_MDCYAN = 'B';
inline constexpr char COLOR_MDPURPLE = 'C';
inline constexpr char COLOR_NULL = '*';

inline constexpr int COLOR_BITS = 31;
#define ColorIndex(c) (((c) - '0') & COLOR_BITS)

// these should stay as macros due to the way they are used in variadic print
// funcs, otherwise we'd have to go and surround all the usages with quotes

#define S_COLOR_BLACK "^0"
#define S_COLOR_RED "^1"
#define S_COLOR_GREEN "^2"
#define S_COLOR_YELLOW "^3"
#define S_COLOR_BLUE "^4"
#define S_COLOR_CYAN "^5"
#define S_COLOR_MAGENTA "^6"
#define S_COLOR_WHITE "^7"
#define S_COLOR_ORANGE "^8"
#define S_COLOR_MDGREY "^9"
#define S_COLOR_LTGREY "^:"
#define S_COLOR_MDGREEN "^<"
#define S_COLOR_MDYELLOW "^="
#define S_COLOR_MDBLUE "^>"
#define S_COLOR_MDRED "^?"
#define S_COLOR_LTORANGE "^A"
#define S_COLOR_MDCYAN "^B"
#define S_COLOR_MDPURPLE "^C"
#define S_COLOR_NULL "^*"

extern vec4_t g_color_table[32];

#define MAKERGB(v, r, g, b)                                                    \
  v[0] = r;                                                                    \
  v[1] = g;                                                                    \
  v[2] = b
#define MAKERGBA(v, r, g, b, a)                                                \
  v[0] = r;                                                                    \
  v[1] = g;                                                                    \
  v[2] = b;                                                                    \
  v[3] = a
#define Q_IsNoneColor(v) v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0

// Hex Color string support
#define gethex(ch)                                                             \
  ((ch) > '9' ? ((ch) >= 'a' ? ((ch) - 'a' + 10) : ((ch) - '7')) : ((ch) - '0'))
#define ishex(ch)                                                              \
  ((ch) && (((ch) >= '0' && (ch) <= '9') || ((ch) >= 'A' && (ch) <= 'F') ||    \
            ((ch) >= 'a' && (ch) <= 'f')))
// check if it's format rrggbb r,g,b e {0..9} U {A...F}
#define Q_IsHexColorString(p)                                                  \
  (ishex(*(p)) && ishex(*((p) + 1)) && ishex(*((p) + 2)) &&                    \
   ishex(*((p) + 3)) && ishex(*((p) + 4)) && ishex(*((p) + 5)))
#define Q_HexColorStringHasAlpha(p) (ishex(*((p) + 6)) && ishex(*((p) + 7)))

#define DEG2RAD(a) (((a) * M_PI) / 180.0F)
#define RAD2DEG(a) (((a) * 180.0f) / M_PI)
#define RAD2SHORT(a) ((a) * (32768.f / (float)M_PI))
#define SHORT2RAD(a) ((a) * ((float)M_PI / 32768.f))
#define SHORT2DEG(a) (((a) / 32768.f) * 180.0f)

struct cplane_s;

extern vec3_t vec3_origin;
extern vec3_t axisDefault[3];

signed char ClampChar(int i);
signed short ClampShort(int i);

// this isn't a real cheap function to call!
int DirToByte(vec3_t dir);
void ByteToDir(int b, vec3_t dir);

#if 1

  #define DotProduct(x, y) ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2])
  #define VectorSubtract(a, b, c)                                              \
    ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1],                       \
     (c)[2] = (a)[2] - (b)[2])
  #define VectorAdd(a, b, c)                                                   \
    ((c)[0] = (a)[0] + (b)[0], (c)[1] = (a)[1] + (b)[1],                       \
     (c)[2] = (a)[2] + (b)[2])
  #define VectorCopy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])
  #define VectorScale(v, s, o)                                                 \
    ((o)[0] = (v)[0] * (s), (o)[1] = (v)[1] * (s), (o)[2] = (v)[2] * (s))
  #define VectorMA(v, s, b, o)                                                 \
    ((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s),           \
     (o)[2] = (v)[2] + (b)[2] * (s))

#else

  #define DotProduct(x, y) _DotProduct(x, y)
  #define VectorSubtract(a, b, c) _VectorSubtract(a, b, c)
  #define VectorAdd(a, b, c) _VectorAdd(a, b, c)
  #define VectorCopy(a, b) _VectorCopy(a, b)
  #define VectorScale(v, s, o) _VectorScale(v, s, o)
  #define VectorMA(v, s, b, o) _VectorMA(v, s, b, o)

#endif

#ifdef __LCC__
  #ifdef VectorCopy
    #undef VectorCopy
// this is a little hack to get more efficient copies in our interpreter
typedef struct {
  float v[3];
} vec3struct_t;
    #define VectorCopy(a, b) *(vec3struct_t *)b = *(vec3struct_t *)a;
  #endif
#endif

#define VectorClear(a) ((a)[0] = (a)[1] = (a)[2] = 0)
#define VectorNegate(a, b)                                                     \
  ((b)[0] = -(a)[0], (b)[1] = -(a)[1], (b)[2] = -(a)[2])
#define VectorSet(v, x, y, z) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))

#define Vector2Set(v, x, y) ((v)[0] = (x), (v)[1] = (y))
#define Vector2Copy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1])
#define Vector2Subtract(a, b, c)                                               \
  ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1])

#define Vector4Set(v, x, y, z, n)                                              \
  ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z), (v)[3] = (n))
#define Vector4Copy(a, b)                                                      \
  ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])
#define Vector4MA(v, s, b, o)                                                  \
  ((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s),             \
   (o)[2] = (v)[2] + (b)[2] * (s), (o)[3] = (v)[3] + (b)[3] * (s))
#define Vector4Average(v, b, s, o)                                             \
  ((o)[0] = ((v)[0] * (1 - (s))) + ((b)[0] * (s)),                             \
   (o)[1] = ((v)[1] * (1 - (s))) + ((b)[1] * (s)),                             \
   (o)[2] = ((v)[2] * (1 - (s))) + ((b)[2] * (s)),                             \
   (o)[3] = ((v)[3] * (1 - (s))) + ((b)[3] * (s)))

#define SnapVector(v)                                                          \
  (v[0] = ((int)(v[0])), v[1] = ((int)(v[1])), v[2] = ((int)(v[2])))

// just in case you don't want to use the macros
vec_t _DotProduct(const vec3_t v1, const vec3_t v2);
void _VectorSubtract(const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorAdd(const vec3_t veca, const vec3_t vecb, vec3_t out);
void _VectorCopy(const vec3_t in, vec3_t out);
void _VectorScale(const vec3_t in, float scale, vec3_t out);
void _VectorMA(const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc);

unsigned ColorBytes3(float r, float g, float b);
unsigned ColorBytes4(float r, float g, float b, float a);

float NormalizeColor(const vec3_t in, vec3_t out);

float RadiusFromBounds(const vec3_t mins, const vec3_t maxs);
void ClearBounds(vec3_t mins, vec3_t maxs);
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs);
qboolean PointInBounds(const vec3_t v, const vec3_t mins, const vec3_t maxs);
int VectorCompare(const vec3_t v1, const vec3_t v2);
vec_t VectorLength(const vec3_t v);
vec_t VectorLengthSquared(const vec3_t v);
vec_t VectorLength2(vec2_t const v);
vec_t VectorLengthSquared2(vec2_t const v);
vec_t Distance(const vec3_t p1, const vec3_t p2);
vec_t DistanceSquared(const vec3_t p1, const vec3_t p2);
void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross);
vec_t VectorNormalize(vec3_t v); // returns vector length
void VectorNormalizeFast(
    vec3_t v); // does NOT return vector length, uses rsqrt approximation
vec_t VectorNormalize2(const vec3_t v, vec3_t out);
void VectorInverse(vec3_t v);
void Vector4Scale(const vec4_t in, vec_t scale, vec4_t out);
void VectorRotate(vec3_t in, vec3_t matrix[3], vec3_t out);
int Q_log2(int val);
float Q_atof(const char *str);
int Q_atoi(const char *str);
float Q_acos(float c);

int Q_rand(int *seed);
float Q_random(int *seed);
float Q_crandom(int *seed);

#define random() ((rand() & 0x7fff) / ((float)0x7fff))
#define crandom() (2.0 * (random() - 0.5))

void vectoangles(const vec3_t value1, vec3_t angles);
float vectoyaw(const vec3_t vec);
void AnglesToAxis(const vec3_t angles, vec3_t axis[3]);
// TTimo: const vec_t ** would require explicit casts for ANSI C conformance
// see unix/const-arg.c
void AxisToAngles(/*const*/ vec3_t axis[3], vec3_t angles);
float VectorDistance(const vec3_t v1, const vec3_t v2);
float VectorDistanceSquared(const vec3_t v1, const vec3_t v2);

void AxisClear(vec3_t axis[3]);
void AxisCopy(vec3_t in[3], vec3_t out[3]);

void SetPlaneSignbits(struct cplane_s *out);
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *plane);

float AngleMod(float a);
float LerpAngle(float from, float to, float frac);
void LerpPosition(vec3_t start, vec3_t end, float frac, vec3_t out);
float AngleSubtract(float a1, float a2);
void AnglesSubtract(vec3_t v1, vec3_t v2, vec3_t v3);

float AngleNormalizePI(float angle);
float AngleNormalize2Pi(float angle);
float AngleNormalize360(float angle);
float AngleNormalize180(float angle);
int AngleNormalize65536(int angle);
float AngleDelta(float angle1, float angle2);

qboolean PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b,
                         const vec3_t c);
void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point,
                             float degrees);
void RotateAroundDirection(vec3_t axis[3], float yaw);
void MakeNormalVectors(const vec3_t forward, vec3_t right, vec3_t up);
// perpendicular vector could be replaced by this

int PlaneTypeForNormal(vec3_t normal);

void MatrixMultiply(const float in1[3][3], const float in2[3][3],
                    float out[3][3]);
void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void PerpendicularVector(vec3_t dst, const vec3_t src);

// Ridah
void GetPerpendicularViewVector(const vec3_t point, const vec3_t p1,
                                const vec3_t p2, vec3_t up);
void ProjectPointOntoVector(vec3_t point, vec3_t vStart, vec3_t vEnd,
                            vec3_t vProj);
void ProjectPointOntoVectorBounded(vec3_t point, vec3_t vStart, vec3_t vEnd,
                                   vec3_t vProj);
float DistanceFromLineSquared(vec3_t p, vec3_t lp1, vec3_t lp2);
float DistanceFromVectorSquared(vec3_t p, vec3_t lp1, vec3_t lp2);
// done.

//=============================================

float Com_Clamp(float min, float max, float value);

char *COM_SkipPath(char *pathname);
void COM_FixPath(char *pathname);
void COM_StripExtension(const char *in, char *out);
void COM_StripFilename(const char *in, char *out);
void COM_DefaultExtension(char *path, int maxSize, const char *extension);

void COM_BeginParseSession(const char *name);
void COM_RestoreParseSession(const char **data_p);
void COM_SetCurrentParseLine(int line);
int COM_GetCurrentParseLine(void);
char *COM_Parse(const char **data_p);
char *COM_ParseExt(const char **data_p, qboolean allowLineBreak);
int COM_Compress(char *data_p);
void COM_ParseError(const char *format, ...);
void COM_ParseWarning(const char *format, ...);
int Com_ParseInfos(char *buf, int max, char infos[][MAX_INFO_STRING]);

qboolean COM_BitCheck(const int array[], int bitNum);
void COM_BitSet(int array[], int bitNum);
void COM_BitClear(int array[], int bitNum);

inline constexpr int MAX_TOKENLENGTH = 1024;

#ifndef TT_STRING
// token types
inline constexpr int TT_STRING = 1;      // string
inline constexpr int TT_LITERAL = 2;     // literal
inline constexpr int TT_NUMBER = 3;      // number
inline constexpr int TT_NAME = 4;        // name
inline constexpr int TT_PUNCTUATION = 5; // punctuation
#endif

typedef struct pc_token_s {
  int type;
  int subtype;
  int intvalue;
  float floatvalue;
  char string[MAX_TOKENLENGTH];
  int line;
  int linescrossed;
} pc_token_t;

// data is an in/out parm, returns a parsed out token

void COM_MatchToken(const char **buf_p, const char *match);

void SkipBracedSection(const char **program);
// start at given depth if already
void SkipBracedSection_Depth(const char **program, int depth);
void SkipRestOfLine(const char **data);

void Parse1DMatrix(const char **buf_p, int x, float *m);
void Parse2DMatrix(const char **buf_p, int y, int x, float *m);
void Parse3DMatrix(const char **buf_p, int z, int y, int x, float *m);

void QDECL Com_sprintf(char *dest, int size, const char *fmt, ...);

// mode parm for FS_FOpenFile
typedef enum { FS_READ, FS_WRITE, FS_APPEND, FS_APPEND_SYNC } fsMode_t;

typedef enum { FS_SEEK_CUR, FS_SEEK_END, FS_SEEK_SET } fsOrigin_t;

//=============================================

int Q_isprint(int c);
int Q_islower(int c);
int Q_isupper(int c);
int Q_isalpha(int c);
int Q_isnumeric(int c);
int Q_isalphanumeric(int c);
int Q_isforfilename(int c);

// portable case insensitive compare
int Q_stricmp(const char *s1, const char *s2);
int Q_strncmp(const char *s1, const char *s2, int n);
int Q_stricmpn(const char *s1, const char *s2, int n);
char *Q_strlwr(char *s1);
char *Q_strupr(char *s1);

// buffer size safe library replacements
#define Q_strncpyz(dest, src, destsize)                                        \
  Q_strncpyz_fn(dest, src, destsize, __func__, SRC_FILENAME, __LINE__)
void Q_strncpyz_fn(char *dest, const char *src, int destsize, const char *func,
                   const char *file, int line);

#define Q_strcat(dest, size, src)                                              \
  Q_strcat_fn(dest, size, src, __func__, SRC_FILENAME, __LINE__)
void Q_strcat_fn(char *dest, int size, const char *src, const char *func,
                 const char *file, int line);

// strlen that discounts Quake color sequences
int Q_PrintStrlen(const char *string);
// removes color sequences from string
char *Q_CleanStr(char *string);
// removes whitespaces and other bad directory characters
char *Q_CleanDirName(char *dirname);
// safe strlen up to N chars
#define Q_strnlen(str, maxlen)                                                 \
  Q_strnlen_fn(str, maxlen, __func__, SRC_FILENAME, __LINE__)
size_t Q_strnlen_fn(const char *str, size_t maxlen, const char *func,
                    const char *file, int line);

// #define _vsnprintf use_Q_vsnprintf
// #define vsnprintf use_Q_vsnprintf
int Q_vsnprintf(char *dest, int size, const char *fmt, va_list argptr);

//=============================================

// 64-bit integers for global rankings interface
// implemented as a struct for qvm compatibility
typedef struct {
  byte b0;
  byte b1;
  byte b2;
  byte b3;
  byte b4;
  byte b5;
  byte b6;
  byte b7;
} qint64;

//=============================================

short LittleShort(short l);
int LittleLong(int l);
qint64 LittleLong64(qint64 l);
float LittleFloat(float l);

short BigShort(short l);
int BigLong(int l);
qint64 BigLong64(qint64 l);
float BigFloat(float l);

void Swap_Init(void);
char *QDECL va(const char *format, ...);
float *tv(float x, float y, float z);
char *vtos(const vec3_t v);
char *vtosf(const vec3_t v);

//=============================================

//
// key / value info strings
//
char *Info_ValueForKey(const char *s, const char *key);
void Info_RemoveKey(char *s, const char *key);
void Info_RemoveKey_big(char *s, const char *key);
void Info_SetValueForKey(char *s, const char *key, const char *value);
void Info_SetValueForKey_Big(char *s, const char *key, const char *value);
qboolean Info_Validate(const char *s);
void Info_NextPair(const char **s, char *key, char *value);
/* String manipulation */
qboolean charErase(int pos, char *str);
void RemoveAllChars(char c, char *str);
void RemoveDuplicates(char *str);
void SortString(char *src);

// this is only here so the functions in q_shared.c and bg_*.c can link
[[noreturn]] void QDECL Com_Error(int level, const char *error, ...);
void QDECL Com_Printf(const char *msg, ...);
void QDECL Com_LocalPrintf(const char *msg, ...);

/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

// saved to etconfig.cfg
inline constexpr int CVAR_ARCHIVE = 1;
// sent to server on connect or change
inline constexpr int CVAR_USERINFO = 2;
// sent in response to front end requests
inline constexpr int CVAR_SERVERINFO = 4;
// these cvars will be duplicated on all clients
inline constexpr int CVAR_SYSTEMINFO = 8;
// don't allow change from console at all, but can be set from the command line
inline constexpr int CVAR_INIT = 16;
// will only change when C code next does a Cvar_Get(),
// so it can't be changed without proper initialization
// modified will be set, even though the value hasn't changed yet
inline constexpr int CVAR_LATCH = 32;
// display only, cannot be set by user at all
inline constexpr int CVAR_ROM = 64;
// created by a set command
inline constexpr int CVAR_USER_CREATED = 128;
// can be set even when cheats are disabled, but is not archived
inline constexpr int CVAR_TEMP = 256;
// can not be changed if cheats are disabled
inline constexpr int CVAR_CHEAT = 512;
// do not clear when a cvar_restart is issued
inline constexpr int CVAR_NORESTART = 1024;
// DHM - NERVE :: Like userinfo, but for wolf multiplayer info
inline constexpr int CVAR_WOLFINFO = 2048;
// ydnar: unsafe system cvars
// (renderer, sound settings, anything that might cause a crash)
inline constexpr int CVAR_UNSAFE = 4096;
// gordon: WON'T automatically send this to clients,
// but server browsers will see it
inline constexpr int CVAR_SERVERINFO_NOUPDATE = 8192;

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
  char *name;
  char *string;
  char *resetString;   // cvar_restart will reset to this value
  char *latchedString; // for CVAR_LATCH vars
  int flags;
  qboolean modified;     // set each time the cvar is changed
  int modificationCount; // incremented each time the cvar is changed
  float value;           // Q_atof( string )
  int integer;           // Q_atoi( string )
  struct cvar_s *next;
  struct cvar_s *hashNext;
} cvar_t;

inline constexpr int MAX_CVAR_VALUE_STRING = 256;

typedef int cvarHandle_t;

// the modules that run in the virtual machine can't access the cvar_t directly,
// so they must ask for structured updates
typedef struct {
  cvarHandle_t handle;
  int modificationCount;
  float value;
  int integer;
  char string[MAX_CVAR_VALUE_STRING];
} vmCvar_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

#include "surfaceflags.h" // shared with the q3map utility

// plane types are used to speed some tests
// 0-2 are axial planes
inline constexpr int PLANE_X = 0;
inline constexpr int PLANE_Y = 1;
inline constexpr int PLANE_Z = 2;
inline constexpr int PLANE_NON_AXIAL = 3;
inline constexpr int PLANE_NON_PLANAR = 4;

/*
=================
PlaneTypeForNormal
=================
*/

// #define PlaneTypeForNormal(x) (x[0] == 1.0 ? PLANE_X : (x[1] == 1.0 ? PLANE_Y
//: (x[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL) ) )
#define PlaneTypeForNormal(x)                                                  \
  (x[0] == 1.0                                                                 \
       ? PLANE_X                                                               \
       : (x[1] == 1.0                                                          \
              ? PLANE_Y                                                        \
              : (x[2] == 1.0 ? PLANE_Z                                         \
                             : (x[0] == 0.f && x[1] == 0.f && x[2] == 0.f      \
                                    ? PLANE_NON_PLANAR                         \
                                    : PLANE_NON_AXIAL))))

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s {
  vec3_t normal;
  float dist;
  byte type;     // for fast side tests: 0,1,2 = axial, 3 = nonaxial
  byte signbits; // signx + (signy<<1) + (signz<<2), used as lookup
                 // during collision
  byte pad[2];
} cplane_t;

#define CPLANE

// a trace is returned when a box is swept through the world
typedef struct {
  qboolean allsolid;   // if true, plane is not valid
  qboolean startsolid; // if true, the initial point was in a solid area
  float fraction;      // time completed, 1.0 = didn't hit anything
  vec3_t endpos;       // final position
  cplane_t plane;      // surface normal at impact, transformed to world space
  int surfaceFlags;    // surface hit
  int contents;        // contents on other side of surface hit
  int entityNum;       // entity the contacted sirface is a part of
} trace_t;

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD

// markfragments are returned by CM_MarkFragments()
typedef struct {
  int firstPoint;
  int numPoints;
} markFragment_t;

typedef struct {
  vec3_t origin;
  vec3_t axis[3];
} orientation_t;

//=====================================================================

// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
inline constexpr int KEYCATCH_CONSOLE = 0x0001;
inline constexpr int KEYCATCH_UI = 0x0002;
inline constexpr int KEYCATCH_MESSAGE = 0x0004;
inline constexpr int KEYCATCH_CGAME = 0x0008;

// sound channels
// channel 0 never willingly overrides
// other channels will always override a playing sound on that channel
typedef enum {
  CHAN_AUTO,
  CHAN_LOCAL, // menu sounds, etc
  CHAN_WEAPON,
  CHAN_VOICE,
  CHAN_ITEM,
  CHAN_BODY,
  CHAN_LOCAL_SOUND, // chat messages, etc
  CHAN_ANNOUNCER,   // announcer voices, etc
  CHAN_VOICE_BG,    // xkan - background sound for voice (radio static, etc.)
} soundChannel_t;

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/
inline constexpr int ANIM_BITS = 10;

#define ANGLE2SHORT(x) ((int)((x) * 65536 / 360) & 65535)
#define SHORT2ANGLE(x) ((x) * (360.0 / 65536))

inline constexpr int SNAPFLAG_RATE_DELAYED = 1;
// snapshot used during connection and for zombies
inline constexpr int SNAPFLAG_NOT_ACTIVE = 2;
// toggled every map_restart so transitions can be detected
inline constexpr int SNAPFLAG_SERVERCOUNT = 4;

//
// per-level limits
//
// JPW NERVE back to q3ta default was 128
// absolute limit
inline constexpr int MAX_CLIENTS = 64;

// JPW NERVE put q3ta default back for testing
// don't need to send any more
inline constexpr int GENTITYNUM_BITS = 10;
inline constexpr int MAX_GENTITIES = 1 << GENTITYNUM_BITS;

// entitynums are communicated with GENTITYNUM_BITS,
// so any reserved values that are going to be communicated
// over the net need to also be in this range
inline constexpr int ENTITYNUM_NONE = MAX_GENTITIES - 1;
inline constexpr int ENTITYNUM_WORLD = MAX_GENTITIES - 2;
inline constexpr int ENTITYNUM_MAX_NORMAL = MAX_GENTITIES - 2;

// these are sent over the net as 8 bits (Gordon: upped to 9 bits, erm actually
// it was already at 9 bits, wtf? NEVAR TRUST GAMECODE COMMENTS, comments are
// evil :E, lets hope it doesn't horribly break anything....)
inline constexpr int MAX_MODELS = 256;
inline constexpr int MAX_SOUNDS = 256; // so they cannot be blindly increased
inline constexpr int MAX_CS_SKINS = 64;
inline constexpr int MAX_CSSTRINGS = 32;

inline constexpr int MAX_CS_SHADERS = 32;
inline constexpr int MAX_SERVER_TAGS = 256;
inline constexpr int MAX_TAG_FILES = 64;

// cm_local.h in engine
inline constexpr int MAX_SUBMODELS = 512;

inline constexpr int MAX_MULTI_SPAWNTARGETS = 16; // JPW NERVE

inline constexpr int MAX_CONFIGSTRINGS = 1024;

inline constexpr size_t MAX_DLIGHT_STYLESTRING = 64;
inline constexpr int MAX_DLIGHT_CONFIGSTRINGS = 16;
inline constexpr int MAX_SPLINE_CONFIGSTRINGS = 8;

inline constexpr int PARTICLE_SNOW128 = 1;
inline constexpr int PARTICLE_SNOW64 = 2;
inline constexpr int PARTICLE_SNOW32 = 3;
inline constexpr int PARTICLE_SNOW256 = 0;

inline constexpr int PARTICLE_BUBBLE8 = 4;
inline constexpr int PARTICLE_BUBBLE16 = 5;
inline constexpr int PARTICLE_BUBBLE32 = 6;
inline constexpr int PARTICLE_BUBBLE64 = 7;

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication

// an info string with all the serverinfo cvars
inline constexpr int CS_SERVERINFO = 0;
// an info string for server system to client system configuration
// (timescale, etc.)
inline constexpr int CS_SYSTEMINFO = 1;
// game can't modify below this, only the system can
inline constexpr int RESERVED_CONFIGSTRINGS = 2;

inline constexpr int MAX_GAMESTATE_CHARS = 16000;

typedef struct {
  int stringOffsets[MAX_CONFIGSTRINGS];
  char stringData[MAX_GAMESTATE_CHARS];
  int dataCount;
} gameState_t;

// xkan, 1/10/2003 - adapted from original SP
typedef enum : net_uint2_t {
  AISTATE_RELAXED,
  AISTATE_QUERY,
  AISTATE_ALERT,
  AISTATE_COMBAT,

  MAX_AISTATES
} aistateEnum_t;

// RF, passed in through overdraw parameter,
// force this dlight under all conditions
inline constexpr int REF_FORCE_DLIGHT = 1 << 31;
// (SA) this dlight does not light surfaces, it only affects dynamic light grid
inline constexpr int REF_JUNIOR_DLIGHT = 1 << 30;
// ydnar: global directional light,
// origin should be interpreted as a normal vector
inline constexpr int REF_DIRECTED_DLIGHT = 1 << 29;

// bit field limits
inline constexpr int MAX_STATS = 16;
inline constexpr int MAX_PERSISTANT = 16;
inline constexpr int MAX_POWERUPS = 16;
inline constexpr int MAX_WEAPONS = 64; // (SA) and yet more!

inline constexpr int MAX_EVENTS =
    4; // max events per frame before we drop events

inline constexpr int PS_PMOVEFRAMECOUNTBITS = 6;

/*
 * playerState_t is the information needed by both the client and server
 * to predict player motion and actions. Nothing outside Pmove should
 * modify these, or some degree of prediction error will occur.
 *
 * You can't add anything to this without modifying the code in msg.c
 * (Gordon: unless it doesn't need transmitted over the network,
 * in which case it should proxy go in the new text struct anyway)
 *
 * playerState_t is a full superset of entityState_t as it is used by players,
 * so if a playerState_t is transmitted, the entityState_t can be fully derived
 * from it.
 *
 * NOTE: all fields in here must be 32 bits (or those within sub-structures)
 *
 * The actual communicated field sizes are represented by the typedefs.
 * If a field is not declared as a net_* type, it is likely not communicated
 * (unless it's an enum or another typedef such as vec3_t).
 *
 * Fields that are NOT networked:
 *
 * oldEventSequence
 * externalEvent
 * externalEventParm
 * externalEventTime
 * item
 * holding
 * serverCursorHintTrace
 * ping
 * pmove_framecount
 * entityEventSequence
 * sprintExertTime
 * jumpTime
 * aimSpreadScaleFloat
 * lastFireTime
 * quickGrenTime
 * leanStopDebounceTime
 * weapHeat
 */

typedef struct playerState_s {
  net_int32_t commandTime; // cmd->serverTime of last executed command
  net_uint8_t pm_type;
  net_uint8_t bobCycle;  // for view bobbing and footstep generation
  net_uint16_t pm_flags; // ducked, jump_held, etc
  net_int16_t pm_time;

  vec3_t origin;
  vec3_t velocity;
  net_int16_t weaponTime;

  // for weapons that don't fire immediately when 'fire' is hit
  // (grenades, panzer, satchel...)
  net_int16_t weaponDelay;

  // for delayed grenade throwing. This is set to 5s when
  // the attack button goes down, then when attack is released,
  // this is the amount of time left before the grenade goes off
  // (or if it gets to 0 while in players hand, it explodes)
  net_int16_t grenadeTimeLeft;

  net_uint16_t gravity;
  net_float leanf; // amount of 'lean' when player is looking around corner

  net_uint16_t speed;

  // add to command angles to get view direction changed by spawns,
  // rotating objects, and teleporters
  net_uint16_t delta_angles[3];

  net_uint10_t groundEntityNum; // ENTITYNUM_NONE = in air

  // don't change low priority animations until this runs out
  net_uint16_t legsTimer;
  net_uint10_t legsAnim; // mask off ANIM_TOGGLEBIT

  // don't change low priority animations until this runs out
  net_uint16_t torsoTimer;
  net_uint10_t torsoAnim; // mask off ANIM_TOGGLEBIT

  // a number 0 to 7 that represents the reletive angle of movement
  // to the view angle (axial and diagonals)
  // when at rest, the value will remain unchanged
  // used to twist the legs during strafing
  net_uint8_t movementDir;

  // copied to entityState_t->eFlags
  net_uint24_t eFlags;

  net_uint8_t eventSequence; // pmove generated events
  net_uint8_t events[MAX_EVENTS];
  net_uint8_t eventParms[MAX_EVENTS];

  // so we can see which events have been added
  // since we last converted to entityState_t
  int oldEventSequence;

  int externalEvent; // events set on player from another source
  int externalEventParm;
  int externalEventTime;

  net_uint8_t clientNum; // ranges from 0 to MAX_CLIENTS-1

  // weapon info
  net_uint7_t weapon; // copied to entityState_t->weapon
  net_uint4_t weaponstate;

  // item info
  int item;

  vec3_t viewangles; // for fixed views
  net_int8_t viewheight;

  // damage feedback
  net_uint8_t damageEvent; // when it changes, latch the other parms
  net_uint8_t damageYaw;
  net_uint8_t damagePitch;
  net_uint8_t damageCount;

  net_uint16_t stats[MAX_STATS];
  net_uint16_t persistant[MAX_PERSISTANT]; // stats that aren't cleared on death
  net_int32_t powerups[MAX_POWERUPS]; // level.time that the powerup runs out
  net_uint16_t ammo[MAX_WEAPONS];     // total amount of ammo
  net_uint16_t ammoclip[MAX_WEAPONS]; // ammo in clip
  net_uint16_t holdable[16];
  int holding; // the current item in holdable[] that is selected (held)
  // 64 bits for weapons held
  net_int32_t weapons[MAX_WEAPONS / (sizeof(int) * 8)];

  // Ridah, allow for individual bounding boxes
  vec3_t mins, maxs;
  net_float crouchMaxZ;
  net_float crouchViewHeight, standViewHeight, deadViewHeight;
  // variable movement speed
  net_float runSpeedScale, sprintSpeedScale, crouchSpeedScale;
  // done.

  // Ridah, view locking for mg42
  net_uint8_t viewlocked;
  net_uint16_t viewlocked_entNum;

  net_float friction;

  net_uint8_t nextWeapon;
  net_uint8_t teamNum;

  // RF, burning effect is required for view blending effect
  net_int32_t onFireStart;

  // what type of cursor hint the server is dictating
  net_uint8_t serverCursorHint;
  // a value (0-255) associated with the above
  net_uint8_t serverCursorHintVal;

  // not communicated over net, but used to store
  // the current server-side cursorhint trace
  trace_t serverCursorHintTrace;

  // ----------------------------------------------------------------------
  // So to use persistent variables here, which don't need to come from
  // the server, we could use a marker variable, and use that to store
  // everything after it before we read in the new values for the
  // predictedPlayerState, then restore them after copying the structure
  // recieved from the server.

  // Arnout: use the pmoveExt_t structure in bg_public.h to store this
  // kind of data now (presistant on client, not network transmitted)

  int ping; // server to game info for scoreboard
  int pmove_framecount;
  int entityEventSequence;

  int sprintExertTime;

  // JPW NERVE -- value for all multiplayer classes with regenerating
  // "class weapons" -- ie LT artillery, medic medpack, engineer build
  // points, etc
  int classWeaponTime; // Arnout : DOES get send over the network
  int jumpTime;        // used in MP to prevent jump accel
  // jpw

  net_uint10_t weapAnim; // mask off ANIM_TOGGLEBIT

  qboolean releasedFire;

  // (SA) the server-side aimspreadscale that lets it track finer changes,
  // but still only transmit the 8bit int to the client
  float aimSpreadScaleFloat;
  // 0 - 255 increases with angular movement
  // // Arnout : DOES get send over the network
  net_uint8_t aimSpreadScale;

  // used by server to hold last firing frame briefly
  // when randomly releasing trigger (AI)
  int lastFireTime;

  int quickGrenTime;

  int leanStopDebounceTime;

  //----(SA)	added

  // seems like heat and aimspread could be tied together somehow,
  // however, they (appear to) change at different rates and I can't
  // currently see how to optimize this to one server->client
  // transmission "weapstatus" value.

  // some weapons can overheat, this tracks (server-side) how hot
  // each weapon currently is.
  // ETJump: unused, we track this in pmext instead
  // this doesn't get communicated though, weapHeat is communicated
  // by storing currently held weapon's heat to curWeapHeat
  int weapHeat[MAX_WEAPONS];
  // value for the currently selected weapon (for transmission to client)
  net_uint8_t curWeapHeat;
  net_uint8_t identifyClient; // NERVE - SMF
  net_uint8_t identifyClientHealth;

  aistateEnum_t aiState; // xkan, 1/10/2003
} playerState_t;

//====================================================================

//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//

// +attack
inline constexpr uint8_t BUTTON_ATTACK = 1;
// displays talk balloon and disables actions
inline constexpr uint8_t BUTTON_TALK = 2;
// +useitem
// //Feen: this cmd and +salute are not currently used for anything...
inline constexpr uint8_t BUTTON_USE_HOLDABLE = 4;
// +salute
// //Feen: normally used for bot taunts, nothing now....
inline constexpr uint8_t BUTTON_GESTURE = 8;
// walking can't just be inferred from MOVE_RUN because a key pressed
// late in the frame will only generate a small move value for that frame
// walking will use different animations and won't generate footsteps
inline constexpr uint8_t BUTTON_WALKING = 16;
// +sprint
inline constexpr uint8_t BUTTON_SPRINT = 32;
// +activate
inline constexpr uint8_t BUTTON_ACTIVATE = 64;
// any key whatsoever
inline constexpr uint8_t BUTTON_ANY = 128;

//----(SA) wolf buttons
inline constexpr uint8_t WBUTTON_ATTACK2 = 1; // +attack2
inline constexpr uint8_t WBUTTON_ZOOM = 2;    // +zoom
inline constexpr uint8_t WBUTTON_RELOAD = 8;
inline constexpr uint8_t WBUTTON_LEANLEFT = 16;
inline constexpr uint8_t WBUTTON_LEANRIGHT = 32;
inline constexpr uint8_t WBUTTON_DROP = 64; // Arnout: wbutton now
inline constexpr uint8_t WBUTTON_PRONE = 128;
//----(SA) end

// Arnout: doubleTap buttons - DT_NUM can be max 8
typedef enum {
  DT_NONE,
  DT_MOVELEFT,
  DT_MOVERIGHT,
  DT_FORWARD,
  DT_BACK,
  DT_LEANLEFT,
  DT_LEANRIGHT,
  DT_UP,
  DT_NUM
} dtType_t;

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
  int serverTime;
  byte buttons;
  byte wbuttons;
  byte weapon;
  byte flags;
  int angles[3];

  signed char forwardmove, rightmove, upmove;
  byte doubleTap; // Arnout: only 3 bits used

  // rain - in ET, this can be any entity, and it's used as an array
  // index, so make sure it's unsigned
  byte identClient; // NERVE - SMF
} usercmd_t;

//===================================================================

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
inline constexpr int SOLID_BMODEL = 0xffffff;

typedef enum : net_uint8_t {
  TR_STATIONARY,
  TR_INTERPOLATE, // non-parametric, but interpolate between snapshots
  TR_LINEAR,
  TR_LINEAR_STOP,
  TR_LINEAR_STOP_BACK, //----(SA)	added.  so reverse movement can
                       // be
                       // different than forward
  TR_SINE,             // value = base + sin( time / duration ) * delta
  TR_GRAVITY,
  // Ridah
  TR_GRAVITY_LOW,
  TR_GRAVITY_FLOAT,  // super low grav with no gravity acceleration
                     // (floating feathers/fabric/leaves/...)
  TR_GRAVITY_PAUSED, //----(SA)	has stopped, but will still do a short
                     // trace to
                     // see if it should be switched back to TR_GRAVITY
  TR_ACCELERATE,
  TR_DECCELERATE,
  // Gordon
  TR_SPLINE,
  TR_LINEAR_PATH
} trType_t;

typedef struct {
  trType_t trType;
  int trTime;
  int trDuration; // if non 0, trTime + trDuration = stop time
                  //----(SA)	removed
  vec3_t trBase;
  vec3_t trDelta; // velocity, etc
                  //----(SA)	removed
} trajectory_t;

typedef enum {
  ET_GENERAL,
  ET_PLAYER,
  ET_ITEM,
  ET_MISSILE,
  ET_MOVER,
  ET_BEAM,
  ET_PORTAL,
  ET_SPEAKER,
  ET_PUSH_TRIGGER,
  ET_TELEPORT_TRIGGER,
  ET_INVISIBLE,
  ET_CONCUSSIVE_TRIGGER,  // JPW NERVE trigger for concussive dust
                          // particles
  ET_OID_TRIGGER,         // DHM - Nerve :: Objective Info Display
  ET_EXPLOSIVE_INDICATOR, // NERVE - SMF

  //---- (SA) Wolf
  ET_EXPLOSIVE,    // brush that will break into smaller bits when damaged
  ET_EF_SPOTLIGHT, // ETJump: unused
  ET_ALARMBOX,
  ET_CORONA,
  ET_TRAP,

  ET_GAMEMODEL,  // misc_gamemodel.  similar to misc_model, but it's a
                 // dynamic model so we have LOD
  ET_FOOTLOCKER, //----(SA)	added
  //---- end

  ET_FLAMEBARREL,
  ET_FP_PARTS,

  // FIRE PROPS
  ET_FIRE_COLUMN,
  ET_FIRE_COLUMN_SMOKE,
  ET_RAMJET,

  ET_FLAMETHROWER_CHUNK, // DHM - NERVE :: Used in server side collision
                         // detection for flamethrower

  ET_EXPLO_PART,

  ET_PROP,

  ET_AI_EFFECT,

  ET_CAMERA,
  ET_MOVERSCALED,

  ET_CONSTRUCTIBLE_INDICATOR,
  ET_CONSTRUCTIBLE,
  ET_CONSTRUCTIBLE_MARKER,
  ET_BOMB,
  ET_WAYPOINT,
  ET_BEAM_2,
  ET_TANK_INDICATOR,
  ET_TANK_INDICATOR_DEAD,
  // Start - TAT - 8/29/2002
  // An indicator object created by the bot code to show where the bots
  // are moving to
  ET_BOTGOAL_INDICATOR,
  // End - TA - 8/29/2002
  ET_CORPSE, // Arnout: dead player
  ET_SMOKER, // Arnout: target_smoke entity

  ET_TEMPHEAD,    // Gordon: temporary head for clients for bullet traces
  ET_MG42_BARREL, // Arnout: MG42 barrel
  ET_TEMPLEGS,    // Arnout: temporary leg for clients for bullet traces
  ET_TRIGGER_MULTIPLE,
  ET_TRIGGER_FLAGONLY,
  ET_TRIGGER_FLAGONLY_MULTIPLE,
  ET_GAMEMANAGER,
  ET_AAGUN,
  ET_CABINET_H,
  ET_CABINET_A,
  ET_HEALER,
  ET_SUPPLIER,

  // Feen: PGM - Portal Entity Types
  ET_PORTAL_BLUE, // Portal 1
  ET_PORTAL_RED,  // Portal 2
  // Feen: PGM - END Entity Types

  // Tokens
  ET_TOKEN_EASY,
  ET_TOKEN_MEDIUM,
  ET_TOKEN_HARD,

  ET_LANDMINE_HINT,     // Gordon: landmine hint for botsetgoalstate filter
  ET_ATTRACTOR_HINT,    // Gordon: attractor hint for botsetgoalstate
                        // filter
  ET_SNIPER_HINT,       // Gordon: sniper hint for botsetgoalstate filter
  ET_LANDMINESPOT_HINT, // Gordon: landminespot hint for botsetgoalstate
                        // filter

  ET_COMMANDMAP_MARKER,

  ET_WOLF_OBJECTIVE,

  ET_VELOCITY_PUSH_TRIGGER, // ETJump

  ET_TELEPORT_TRIGGER_CLIENT, // client side predicted teleport

  ET_FAKEBRUSH, // func_fakebrush

  ET_EVENTS, // any of the EV_* events can be added freestanding
             // by setting eType to ET_EVENTS + eventNum
             // this avoids having to set eFlags and eventNum
} entityType_t;

static constexpr std::array<const char *, ET_EVENTS + 1> entityTypeNames = {
    "ET_GENERAL",
    "ET_PLAYER",
    "ET_ITEM",
    "ET_MISSILE",
    "ET_MOVER",
    "ET_BEAM",
    "ET_PORTAL",
    "ET_SPEAKER",
    "ET_PUSH_TRIGGER",
    "ET_TELEPORT_TRIGGER",
    "ET_INVISIBLE",
    "ET_CONCUSSIVE_TRIGGER",
    "ET_OID_TRIGGER",
    "ET_EXPLOSIVE_INDICATOR",
    "ET_EXPLOSIVE",
    "ET_EF_SPOTLIGHT",
    "ET_ALARMBOX",
    "ET_CORONA",
    "ET_TRAP",
    "ET_GAMEMODEL",
    "ET_FOOTLOCKER",
    "ET_FLAMEBARREL",
    "ET_FP_PARTS",
    "ET_FIRE_COLUMN",
    "ET_FIRE_COLUMN_SMOKE",
    "ET_RAMJET",
    "ET_FLAMETHROWER_CHUNK",
    "ET_EXPLO_PART",
    "ET_PROP",
    "ET_AI_EFFECT",
    "ET_CAMERA",
    "ET_MOVERSCALED",
    "ET_CONSTRUCTIBLE_INDICATOR",
    "ET_CONSTRUCTIBLE",
    "ET_CONSTRUCTIBLE_MARKER",
    "ET_BOMB",
    "ET_WAYPOINT",
    "ET_BEAM_2",
    "ET_TANK_INDICATOR",
    "ET_TANK_INDICATOR_DEAD",
    "ET_BOTGOAL_INDICATOR",
    "ET_CORPSE",
    "ET_SMOKER",
    "ET_TEMPHEAD",
    "ET_MG42_BARREL",
    "ET_TEMPLEGS",
    "ET_TRIGGER_MULTIPLE",
    "ET_TRIGGER_FLAGONLY",
    "ET_TRIGGER_FLAGONLY_MULTIPLE",
    "ET_GAMEMANAGER",
    "ET_AAGUN",
    "ET_CABINET_H",
    "ET_CABINET_A",
    "ET_HEALER",
    "ET_SUPPLIER",
    "ET_PORTAL_BLUE",
    "ET_PORTAL_RED",
    "ET_TOKEN_EASY",
    "ET_TOKEN_MEDIUM",
    "ET_TOKEN_HARD",
    "ET_LANDMINE_HINT",
    "ET_ATTRACTOR_HINT",
    "ET_SNIPER_HINT",
    "ET_LANDMINESPOT_HINT",
    "ET_COMMANDMAP_MARKER",
    "ET_WOLF_OBJECTIVE",
    "ET_VELOCITY_PUSH_TRIGGER",
    "ET_TELEPORT_TRIGGER_CLIENT",
    "ET_FAKEBRUSH",
    "ET_EVENTS",
};

static_assert(sizeof(entityTypeNames) / sizeof(entityTypeNames[0]) ==
                  ET_EVENTS + 1,
              "Entity types array size does not match enum list");

/*
 * entityState_t is the information conveyed from the server in an update
 * message about entities that the client will need to render in some way.
 * Different eTypes may use the information in different ways.
 * The messages are delta compressed, so it doesn't really matter
 * if the structure size is fairly large.
 *
 * Adding new fields to this and making them transmit over network
 * would require changes in msg.c, so we cannot add anything new here.
 * Use the fields sparingly when repurposing these.
 *
 * NOTE: all fields in here must be 32 bits (or those within sub-structures)
 */

typedef struct entityState_s {
  net_uint8_t number; // entity index
  // changed from entityType_t to int to allow ET_EVENTS + eventNum
  net_uint8_t eType;
  net_uint24_t eFlags;

  trajectory_t pos;  // for calculating position
  trajectory_t apos; // for calculating angles

  net_int32_t time;
  net_int32_t time2;

  vec3_t origin;
  vec3_t origin2;

  vec3_t angles;
  vec3_t angles2;

  net_uint10_t otherEntityNum; // shotgun sources, etc
  net_uint10_t otherEntityNum2;

  net_uint10_t groundEntityNum; // -1 = in air

  net_int32_t constantLight; // r + (g<<8) + (b<<16) + (intensity<<24)
  net_int32_t dl_intensity;  // used for coronas
  net_uint8_t loopSound;     // constantly loop this sound

  net_uint9_t modelindex;
  net_uint9_t modelindex2;
  net_uint8_t clientNum; // 0 to (MAX_CLIENTS - 1), for players and corpses
  net_uint16_t frame;

  // for client side prediction
  // trap_LinkEntity sets this properly if the entity has an inline model
  net_uint24_t solid;

  // old style events, in for compatibility only
  net_uint8_t event;
  net_uint8_t eventParm;

  net_uint8_t eventSequence; // pmove generated events
  net_uint8_t events[MAX_EVENTS];
  net_uint8_t eventParms[MAX_EVENTS];

  // for players
  // bit flags
  // Arnout: used to store entState_t for non-player entities
  // (so we know to draw them translucent clientsided)
  net_uint16_t powerups;
  net_uint8_t weapon;     // determines weapon and flash model, etc
  net_uint10_t legsAnim;  // mask off ANIM_TOGGLEBIT
  net_uint10_t torsoAnim; // mask off ANIM_TOGGLEBIT

  // for particle effects, PlayerDensityFlags for players in entityState
  net_uint10_t density;

  // to pass along additional information for damage effects for players
  // Also used for cursorhints for non-player entities
  net_int32_t dmgFlags;

  // Ridah
  net_int32_t onFireStart, onFireEnd;

  net_uint8_t nextWeapon;
  net_uint8_t teamNum;

  net_int32_t effect1Time, effect2Time, effect3Time;

  aistateEnum_t aiState; // xkan, 1/10/2003
  // clients can't derive movetype of other clients for anim scripting system
  net_uint4_t animMovetype;
} entityState_t;

typedef enum {
  CA_UNINITIALIZED,
  CA_DISCONNECTED, // not talking to a server
  CA_AUTHORIZING,  // not used any more, was checking cd key
  CA_CONNECTING,   // sending request packets to the server
  CA_CHALLENGING,  // sending challenge packets to the server
  CA_CONNECTED,    // netchan_t established, getting gamestate
  CA_LOADING,      // only during cgame initialization, never during main
                   // loop
  CA_PRIMED,       // got gamestate, waiting for first frame
  CA_ACTIVE,       // game views should be displayed
  CA_CINEMATIC     // playing a cinematic or a static pic, not connected to
                   // a server
} connstate_t;

// font support

inline constexpr int GLYPH_START = 0;
inline constexpr int GLYPH_END = 255;
inline constexpr int GLYPH_CHARSTART = 32;
inline constexpr int GLYPH_CHAREND = 127;
inline constexpr int GLYPHS_PER_FONT = GLYPH_END - GLYPH_START + 1;

typedef struct {
  int height;      // number of scan lines
  int top;         // top of glyph in buffer
  int bottom;      // bottom of glyph in buffer
  int pitch;       // width for copying
  int xSkip;       // x adjustment
  int imageWidth;  // width of actual image
  int imageHeight; // height of actual image
  float s;         // x offset in image where glyph starts
  float t;         // y offset in image where glyph starts
  float s2;
  float t2;
  qhandle_t glyph; // handle to the shader with the glyph
  char shaderName[32];
} glyphInfo_t;

typedef struct {
  glyphInfo_t glyphs[GLYPHS_PER_FONT];
  float glyphScale;
  char name[MAX_QPATH];
} fontInfo_t;

#define Square(x) ((x) * (x))

// real time
//=============================================

typedef struct qtime_s {
  int tm_sec;   /* seconds after the minute - [0,59] */
  int tm_min;   /* minutes after the hour - [0,59] */
  int tm_hour;  /* hours since midnight - [0,23] */
  int tm_mday;  /* day of the month - [1,31] */
  int tm_mon;   /* months since January - [0,11] */
  int tm_year;  /* years since 1900 */
  int tm_wday;  /* days since Sunday - [0,6] */
  int tm_yday;  /* days since January 1 - [0,365] */
  int tm_isdst; /* daylight savings time flag */
} qtime_t;

// server browser sources
inline constexpr int AS_LOCAL = 0;
inline constexpr int AS_GLOBAL = 1; // NERVE - SMF - modified
inline constexpr int AS_FAVORITES = 2;

// cinematic states
typedef enum {
  FMV_IDLE,
  FMV_PLAY, // play
  FMV_EOF,  // all other conditions, i.e. stop/EOF/abort
  FMV_ID_BLT,
  FMV_ID_IDLE,
  FMV_LOOPED,
  FMV_ID_WAIT
} e_status;

typedef enum _flag_status {
  FLAG_ATBASE = 0,
  FLAG_TAKEN,      // CTF
  FLAG_TAKEN_RED,  // One Flag CTF
  FLAG_TAKEN_BLUE, // One Flag CTF
  FLAG_DROPPED
} flagStatus_t;

inline constexpr int MAX_GLOBAL_SERVERS = 4096;
inline constexpr int MAX_OTHER_SERVERS = 128;
inline constexpr int MAX_PINGREQUESTS = 16;
inline constexpr int MAX_SERVERSTATUSREQUESTS = 16;

// NERVE - SMF - localization
typedef enum {
#ifndef __MACOS__ // DAJ USA
  LANGUAGE_FRENCH = 0,
  LANGUAGE_GERMAN,
  LANGUAGE_ITALIAN,
  LANGUAGE_SPANISH,
#endif
  MAX_LANGUAGES
} languages_t;

// NERVE - SMF - wolf server/game states
typedef enum {
  GS_INITIALIZE = -1,
  GS_PLAYING,
  GS_WARMUP_COUNTDOWN,
  GS_WARMUP,
  GS_INTERMISSION,
  GS_WAITING_FOR_PLAYERS,
  GS_RESET
} gamestate_t;

#define SQR(a) ((a) * (a))

inline constexpr int MAX_TIMERUN_CHECKPOINTS = 16;
inline constexpr int TIMERUN_CHECKPOINT_NOT_SET = -1;

#endif // __Q_SHARED_H
