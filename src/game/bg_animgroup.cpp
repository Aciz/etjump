/*
**  Animation group code
*/

#include "q_shared.h"
#include "bg_public.h"

inline constexpr int MAX_ANIMPOOL_SIZE = 5 * MAX_MODEL_ANIMATIONS;

static animation_t animationPool[MAX_ANIMPOOL_SIZE];

void BG_ClearAnimationPool(void) {
  memset(animationPool, 0, sizeof(animationPool));
}

#ifdef CGAMEDLL
static animation_t *BG_RAG_FindFreeAnimation(qhandle_t mdxFile,
                                             const char *name)
#else
static animation_t *BG_RAG_FindFreeAnimation(const char *mdxFileName,
                                             const char *name)
#endif // CGAMEDLL
{
  int i;

  for (i = 0; i < MAX_ANIMPOOL_SIZE; i++) {
#ifdef CGAMEDLL
    if (animationPool[i].mdxFile == mdxFile &&
        !Q_stricmp(animationPool[i].name, name)) {
#else
    if (*animationPool[i].mdxFileName &&
        !Q_stricmp(animationPool[i].mdxFileName, mdxFileName) &&
        !Q_stricmp(animationPool[i].name, name)) {
#endif // CGAMEDLL
      return (&animationPool[i]);
    }
  }

  for (i = 0; i < MAX_ANIMPOOL_SIZE; i++) {
#ifdef CGAMEDLL
    if (!animationPool[i].mdxFile) {
      animationPool[i].mdxFile = mdxFile;
#else
    if (!animationPool[i].mdxFileName[0]) {
      Q_strncpyz(animationPool[i].mdxFileName, mdxFileName,
                 sizeof(animationPool[i].mdxFileName));
#endif // CGAMEDLL
      Q_strncpyz(animationPool[i].name, name, sizeof(animationPool[i].name));
      return (&animationPool[i]);
    }
  }

  return NULL;
}

static qboolean BG_RAG_ParseError(int handle, const char *format, ...) {
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

  Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);

  trap_PC_FreeSource(handle);

  return qfalse;
}

static qboolean BG_RAG_ParseAnimation(int handle, animation_t *animation) {
  int i;

  animation->flags = 0;

  if (!PC_Int_Parse(handle, &animation->firstFrame)) {
    return BG_RAG_ParseError(handle, "expected first frame integer");
  }

  if (!PC_Int_Parse(handle, &animation->numFrames)) {
    return BG_RAG_ParseError(handle, "expected length integer");
  }

  if (!PC_Int_Parse(handle, &animation->loopFrames)) {
    return BG_RAG_ParseError(handle, "expected looping integer");
  }

  if (!PC_Int_Parse(handle, &i)) {
    return BG_RAG_ParseError(handle, "expected fps integer");
  }

  if (i == 0) {
    i = 1;
  }

  animation->frameLerp = 1000 / (float)i;
  animation->initialLerp = 1000 / (float)i;

  if (!PC_Int_Parse(handle, &animation->moveSpeed)) {
    return BG_RAG_ParseError(handle, "expected move speed integer");
  }

  if (!PC_Int_Parse(handle, &animation->animBlend)) {
    return BG_RAG_ParseError(handle, "expected transition integer");
  }

  if (!PC_Int_Parse(handle, &i)) {
    return BG_RAG_ParseError(handle, "expected reversed integer");
  }

  if (i == 1) {
    animation->flags |= ANIMFL_REVERSED;
  }

  // calculate the duration
  animation->duration = animation->initialLerp +
                        animation->frameLerp * animation->numFrames +
                        animation->animBlend;

  // get the nameHash
  animation->nameHash = BG_StringHashValue(animation->name);

  // hacky-ish stuff
  if (!Q_strncmp(animation->name, "climb", 5)) {
    animation->flags |= ANIMFL_LADDERANIM;
  }

  if (strstr(animation->name, "firing")) {
    animation->flags |= ANIMFL_FIRINGANIM;
    animation->initialLerp = 40;
  }

  return qtrue;
}

#ifdef CGAMEDLL
extern qhandle_t trap_R_RegisterModel(const char *name);
#endif // CGAMEDLL;

static qboolean BG_RAG_ParseAnimFile(int handle,
                                     animModelInfo_t *animModelInfo) {
  pc_token_t token;
#ifdef CGAMEDLL
  qhandle_t mdxFile;
#else
  char mdxFileName[MAX_QPATH];
#endif // CGAMEDLL

  animation_t *animation;

  if (!trap_PC_ReadToken(handle, &token)) {
    return BG_RAG_ParseError(handle, "expected mdx filename");
  }

#ifdef CGAMEDLL
  mdxFile = trap_R_RegisterModel(token.string);
  if (!mdxFile) {
    return BG_RAG_ParseError(handle, "failed to load %s", token.string);
  }
#else
  Q_strncpyz(mdxFileName, token.string, sizeof(mdxFileName));
#endif // CGAMEDLL

  if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{")) {
    return BG_RAG_ParseError(handle, "expected '{'");
  }

  while (1) {
    if (!trap_PC_ReadToken(handle, &token)) {
      return BG_RAG_ParseError(handle, "unexpected EOF");
    }

    if (token.string[0] == '}') {
      break;
    }

#ifdef CGAMEDLL
    animation = BG_RAG_FindFreeAnimation(mdxFile, token.string);
    if (!animation) {
#else
    animation = BG_RAG_FindFreeAnimation(mdxFileName, token.string);
    if (!animation) {
#endif // CGAMEDLL
      return BG_RAG_ParseError(handle, "out of animation storage space");
    }

    Q_strncpyz(animation->name, token.string, sizeof(animation->name));
    Q_strlwr(animation->name);

    if (!BG_RAG_ParseAnimation(handle, animation)) {
      return qfalse;
    }

    animModelInfo->animations[animModelInfo->numAnimations] = animation;
    animModelInfo->numAnimations++;
  }

  return qtrue;
}

qboolean BG_R_RegisterAnimationGroup(const char *filename,
                                     animModelInfo_t *animModelInfo) {
  pc_token_t token;
  int handle;

  animModelInfo->numAnimations = 0;
  animModelInfo->footsteps = FOOTSTEP_NORMAL;
  animModelInfo->gender = GENDER_MALE;
  animModelInfo->isSkeletal = qtrue;
  animModelInfo->version = 3;
  animModelInfo->numHeadAnims = 0;

  handle = trap_PC_LoadSource(filename);

  if (!handle) {
    return qfalse;
  }

  if (!trap_PC_ReadToken(handle, &token) ||
      Q_stricmp(token.string, "animgroup")) {
    return BG_RAG_ParseError(handle, "expected 'animgroup'");
  }

  if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{")) {
    return BG_RAG_ParseError(handle, "expected '{'");
  }

  while (1) {
    if (!trap_PC_ReadToken(handle, &token)) {
      break;
    }

    if (token.string[0] == '}') {
      break;
    }

    if (!Q_stricmp(token.string, "animfile")) {
      if (!BG_RAG_ParseAnimFile(handle, animModelInfo)) {
        return qfalse;
      }
    } else {
      return BG_RAG_ParseError(handle, "unknown token '%s'", token.string);
    }
  }

  trap_PC_FreeSource(handle);

  return qtrue;
}
