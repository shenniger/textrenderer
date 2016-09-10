#include "yatref.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>

#define CHECK(f)                                                               \
  if ((f) != 0) {                                                              \
    printf("ERROR in %s: %s", __STRING(f), SDL_GetError());                    \
    SDL_Quit();                                                                \
    exit(EXIT_FAILURE);                                                        \
  }

yatref_Font *f[3];
SDL_Texture *userchar;

void init(SDL_Renderer *rend) {
  SDL_Surface *surf = SDL_LoadBMP("char.bmp");
  CHECK(surf == NULL);
  userchar = SDL_CreateTextureFromSurface(rend, surf);
  CHECK(userchar == NULL);

  CHECK(TTF_Init());

  TTF_Font *ttf_font = TTF_OpenFont("test.ttf", 16);
  if (ttf_font == NULL) {
    printf("ERROR in TTF_OpenFont: %s", SDL_GetError());
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  TTF_Font *ttf_font2 = TTF_OpenFont("test.ttf", 30);
  if (ttf_font2 == NULL) {
    printf("ERROR in TTF_OpenFont: %s", SDL_GetError());
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  TTF_Font *ttf_font3 = TTF_OpenFont("test.ttf", 10);
  if (ttf_font3 == NULL) {
    printf("ERROR in TTF_OpenFont: %s", SDL_GetError());
    SDL_Quit();
    exit(EXIT_FAILURE);
  }

  char def[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
               ",.-;:_!\"$%&/()=?+*#'ÄÖÜßäöü\\";
  SDL_Color col;
  col.r = 0;
  col.g = 0;
  col.b = 0;
  col.a = 0;

  f[0] = yatref_CreateFont(ttf_font, rend, def, sizeof(def), &col);
  if (f[0] == NULL) {
    printf("ERROR in yatref_CreateFont: %s", SDL_GetError());
    SDL_Quit();
    exit(EXIT_FAILURE);
  }

  col.r = 255;

  f[1] = yatref_CreateFont(ttf_font2, rend, def, sizeof(def), &col);
  if (f[1] == NULL) {
    printf("ERROR in yatref_CreateFont: %s", SDL_GetError());
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  col.r = 0;
  col.g = 255;
  f[2] = yatref_CreateFont(ttf_font3, rend, def, sizeof(def), &col);
  if (f[2] == NULL) {
    printf("ERROR in yatref_CreateFont: %s", SDL_GetError());
    SDL_Quit();
    exit(EXIT_FAILURE);
  }

  TTF_CloseFont(ttf_font);
  TTF_CloseFont(ttf_font2);
  TTF_CloseFont(ttf_font3);

  TTF_Quit();
}

void render(SDL_Renderer *rend) {
  SDL_Rect a;
  a.w = 50;
  a.h = 30;
  const char *text = "Hello, world.\nHello, world.\nHällö Unicode \x11u\x01\n"
                     "That was a control character.";
  CHECK(yatref_Print(rend, text, f, 3, NULL, NULL, 0, &a, 1, NULL, 10, 20, 790,
                     SDL_FALSE, NULL, NULL));

  CHECK(SDL_SetRenderDrawColor(rend, 255, 0, 0, 0));
  CHECK(SDL_RenderDrawRect(rend, &a));

  const char *text2 =
      "This is a test. Lorem ipsum, dolor sit amet. Lorem ipsum, dolor sit "
      "amet. Here is a very long word: "
      "Donaudampfschifffahrtskapitänsmützenhaltershgehilfentextili"
      "enfabrikantenjunge.\nHere are some more (2-byte) Unicode characters: "
      "äöüÄÖÜß";
  CHECK(yatref_Print(rend, text2, f, 3, NULL, NULL, 0, NULL, 0, NULL, 200, 200,
                     200, SDL_TRUE, NULL, NULL));

  yatref_TextureSize size;
  CHECK(SDL_QueryTexture(userchar, NULL, NULL, size, size + 1));

  const char *text3 =
      "This renderer can be used to draw"
      "\x11n\x02 big and red \x11n\x01 as well as"
      "\x11n\x03 small and green\x11n\x01 text.\n\nIn addition to that, it "
      "can draw \x11i\x01 fancy arrows and other user generated characters.";
  CHECK(yatref_Print(rend, text3, f, 3, &userchar, &size, 1, NULL, 0, NULL, 200,
                     10, 200, SDL_TRUE, NULL, NULL));
}

void destroy() {
  yatref_DestroyFont(f[0]);
  yatref_DestroyFont(f[1]);
  yatref_DestroyFont(f[2]);
}

int main() {
  CHECK(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO));
  SDL_Window *wind;
  SDL_Renderer *rend;
  CHECK(SDL_CreateWindowAndRenderer(800, 600, SDL_WINDOW_SHOWN, &wind, &rend));
  SDL_SetWindowTitle(wind, "yatref");

  init(rend);

  while (SDL_GetTicks() < 10000) {
    CHECK(SDL_SetRenderDrawColor(rend, 255, 255, 255, 255));
    CHECK(SDL_RenderClear(rend));

    render(rend);

    SDL_RenderPresent(rend);
  }

  destroy();
  SDL_DestroyRenderer(rend);
  SDL_DestroyWindow(wind);
  SDL_Quit();
  return 0;
}
