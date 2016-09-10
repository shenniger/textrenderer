#ifndef YATREF_H
#define YATREF_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifdef __cplusplus
extern "C" {
#endif

struct yatref_Font;
typedef struct yatref_Font yatref_Font;
typedef int yatref_TextureSize[2];
typedef SDL_Texture *(*yatref_TextureLoaderFunction)(uint8_t num);

yatref_Font *yatref_CreateFont(TTF_Font *font, SDL_Renderer *renderer,
                               char *def, int defLength, SDL_Color *colFG);

int yatref_GetFontHeight(yatref_Font *f);

int yatref_Print(SDL_Renderer *r, const char *str, yatref_Font **fonts,
                 int numFonts, SDL_Texture **textures,
                 yatref_TextureSize *textureSizes, int numTextures,
                 SDL_Rect *controls, int numControls,
                 yatref_TextureLoaderFunction loader, int x, int y,
                 int widthLimit, SDL_bool wordWrap, int *sizeX, int *sizeY);

void yatref_DestroyFont(yatref_Font *f);

#ifdef __cplusplus
}
#endif

#endif