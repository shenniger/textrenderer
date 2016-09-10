#include "yatref.h"

#define MASK_FIRST_BYTE_1 0x80 /* 0b10000000 */
#define MASK_FIRST_BYTE_2 0x40 /* 0b01000000 */
#define MASK_NEXT_BYTE 0x80    /* 0b10000000 */

/* this struct stores a single character */
struct FontCharacter {
  uint32_t character;
  uint16_t x, w; /* y = 0, height is set in the font */
};

typedef struct FontCharacter FontCharacter;

/* this struct stores a font */
struct yatref_Font {
  int height;
  int numChars;
  int spaceWidth;
  FontCharacter *chars;
  SDL_Texture *texture;
};

/* goes to the next character and returns a uint32_t containing the last one  */
static uint32_t nextUtf8(char **ptr) {
  uint8_t **a = (uint8_t **)ptr;
  uint32_t res = **a;
  ++(*a);

  if ((res & MASK_FIRST_BYTE_1) && (res & MASK_FIRST_BYTE_2)) {
    /* we're in a sequence */
    res |= **a << 8;
    ++(*a);
    if (**a & MASK_NEXT_BYTE && !(**a & MASK_FIRST_BYTE_2)) {
      res |= **a << 16;
      ++(*a);
      if (**a & MASK_NEXT_BYTE && !(**a & MASK_FIRST_BYTE_2)) {
        res |= **a << 24;
        ++(*a);
        if (**a & MASK_NEXT_BYTE && !(**a & MASK_FIRST_BYTE_2)) {
          return 0;
        }
      }
    }
  }

  return res;
}

/* looks up the given character in the chars array */
static FontCharacter *lookupCharacter(yatref_Font *f, uint32_t character) {
  int i;
  for (i = 0; i < f->numChars; i++) {
    if (f->chars[i].character == character) {
      return f->chars + i;
    }
  }
  return NULL;
}

static int getSpaceLength(TTF_Font *font) {
  SDL_Color c;
  c.r = 0;
  c.g = 0;
  c.b = 0;
  c.a = 0;
  SDL_Surface *surf = TTF_RenderText_Solid(font, " ", c);
  if (surf == NULL) {
    return 0;
  }
  SDL_FreeSurface(surf);
  return surf->w;
}

/* creates a font from a TTF_Font */
yatref_Font *yatref_CreateFont(TTF_Font *font, SDL_Renderer *renderer,
                               char *def, int defLength, SDL_Color *colFG) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  uint32_t rmask = 0xff000000, gmask = 0x00ff0000, bmask = 0x0000ff00,
           amask = 0x000000ff;
#else
  uint32_t rmask = 0x000000ff, gmask = 0x0000ff00, bmask = 0x00ff0000,
           amask = 0xff000000;
#endif /* from libsdl.org, shorted */
  int w, h;
  if (TTF_SizeUTF8(font, def, &w, &h) != 0) {
    return NULL; /* TODO */
  }
  SDL_Surface *surf = SDL_CreateRGBSurface(0, w + 30 /* a little more */, h, 32,
                                           rmask, gmask, bmask, amask);
  if (surf == NULL) {
    return NULL; /* TODO */
  }

  FontCharacter *chars = SDL_malloc(
      defLength * sizeof(FontCharacter)); /* allocating more than necessary */

  FontCharacter *curChar = chars;
  char *curDef = def;
  char *curDefBegin = curDef;
  uint32_t c = nextUtf8(&curDef);
  uint16_t curX = 0;
  SDL_Rect r;
  r.h = surf->h;
  r.y = 0;
  while (c && c != 0x0a) { /* 0x0a => LF */
    /* juggling with the nullbyte */
    char previousChar = *curDef;
    *curDef = 0;

    /* we now have a clearly defined range between curCharBegin and curChar */
    SDL_Surface *textSurf = TTF_RenderUTF8_Blended(font, curDefBegin, *colFG);
    if (textSurf == NULL) {
      /* reverting character (we just set it to 0) */
      *curDef = previousChar;

      return NULL; /* TODO */
    }

    r.x = curX;
    r.w = textSurf->w;

    curChar->character = c;
    curChar->x = curX;
    curX += textSurf->w;
    curChar->w = textSurf->w;

    if (SDL_BlitSurface(textSurf, NULL, surf, &r) != 0) {
      /* reverting character (we just set it to 0) */
      *curDef = previousChar;

      return NULL; /* TODO */
    }
    SDL_FreeSurface(textSurf);

    /* reverting character (we just set it to 0) */
    *curDef = previousChar;

    curChar++;
    curDefBegin = curDef;
    c = nextUtf8(&curDef);
  }

  yatref_Font *res = SDL_malloc(sizeof(yatref_Font));
  if (res == NULL) {
    return NULL; /* TODO */
  }
  res->chars = chars;
  res->numChars = curChar - chars;
  res->height = surf->h;
  res->spaceWidth = getSpaceLength(font);
  if (res->spaceWidth == 0) {
    return NULL; /* TODO */
  }
  res->texture = SDL_CreateTextureFromSurface(renderer, surf);
  if (res->texture == NULL) {
    return NULL; /* TODO */
  }
  SDL_FreeSurface(surf);
  return res;
}
int yatref_GetFontHeight(yatref_Font *f) { return f->height; }
void yatref_DestroyFont(yatref_Font *f) {
  SDL_free(f->chars);
  SDL_DestroyTexture(f->texture);
  SDL_free(f);
}

typedef struct ChunkRenderResult {
  char stop;
  uint8_t arg;
} ChunkRenderResult;
static int printChunk(SDL_Renderer *r, char **str, yatref_Font *f,
                      ChunkRenderResult *res, int *x, int y, int limitX) {
  SDL_Rect rectSrc;
  SDL_Rect rectDest;
  rectSrc.y = 0;
  rectSrc.h = f->height;

  rectDest.x = *x;
  rectDest.y = y;
  rectDest.h = f->height;

  uint32_t c;
  char *lastChar = *str;
  while ((c = nextUtf8(str))) {
    if (c == 17) {
      res->stop = **str;
      res->arg = *(++(*str));
      ++(*str);
      *x = rectDest.x;
      return 0;
    } else if (c == 10) {
      res->stop = '\n';
      *x = rectDest.x;
      return 0;
    } else if (c == ' ') {
      res->stop = ' ';
      *x = rectDest.x;
      return 0;
    } else {
      FontCharacter *fontChar = lookupCharacter(f, c);
      if (fontChar == NULL) {
        return 2;
      }
      if (rectDest.x + fontChar->w > limitX) {
        res->stop = '\n';
        *x = rectDest.x;
        *str = lastChar;
        return 0;
      }
      if (r != NULL) {
        rectSrc.x = fontChar->x;
        rectSrc.w = fontChar->w;
        rectDest.w = fontChar->w;
        if (SDL_RenderCopy(r, f->texture, &rectSrc, &rectDest) != 0) {
          return 1;
        }
      }
      rectDest.x += fontChar->w;
    }
    lastChar = *str;
  }

  res->stop = 0;
  *x = rectDest.x;

  return 0;
}

int yatref_Print(SDL_Renderer *r, const char *str, yatref_Font **fonts,
                 int numFonts, SDL_Texture **textures,
                 yatref_TextureSize *textureSizes, int numTextures,
                 SDL_Rect *controls, int numControls,
                 yatref_TextureLoaderFunction loader, int x, int y,
                 int widthLimit, SDL_bool wordWrap, int *sizeX, int *sizeY) {
  yatref_Font *f = *fonts;
  char *curStr = (char *)str;

  int posX = x;
  int posY = y;

  int greatestLineWidth = 0;
  int lineHeight = f->height;
  int limitX = widthLimit + x;

  ChunkRenderResult chRes;
  for (;;) {
    if (wordWrap == SDL_TRUE) {
      /* calculate length of next word */
      char *countStr = curStr;
      int countPos = posX;
      if (printChunk(NULL, &countStr, f, &chRes, &countPos, 0, INT32_MAX) !=
          0) {
        return 1;
      }

      if (countPos > limitX && posX != x) {
        /* add a line break */
        posY += lineHeight;
        lineHeight = f->height;

        if (greatestLineWidth < posX - x) {
          greatestLineWidth = posX - x;
        }
        posX = x;
      }
    }

    if (printChunk(r, &curStr, f, &chRes, &posX, posY, limitX) != 0) {
      return 1;
    }
    if (chRes.stop == 0) {
      break;
    } else if (chRes.stop == ' ') {
      posX += f->spaceWidth;
    } else if (chRes.stop == '\n') {
      posY += lineHeight;
      lineHeight = f->height;

      if (greatestLineWidth < posX - x) {
        greatestLineWidth = posX - x;
      }
      posX = x;
    } else if (chRes.stop == 'n') { /* change font */
      uint8_t chosenFont = ((uint8_t)chRes.arg) - 1;
      if (chosenFont >= numFonts) {
        return 5;
      }

      f = fonts[chosenFont];
      if (f->height > lineHeight) {
        lineHeight = f->height;
      }
    } else if (chRes.stop == 'i') { /* display texture */
      uint8_t chosenTexture = ((uint8_t)chRes.arg) - 1;
      if (chosenTexture >= numTextures) {
        return 6;
      }

      if (textureSizes[chosenTexture][1] > lineHeight) {
        lineHeight = textureSizes[chosenTexture][1];
      }

      if (r != NULL) {
        SDL_Rect src;
        SDL_Rect dest;
        src.x = 0;
        src.y = 0;
        dest.x = posX;
        dest.y = posY;
        src.h = textureSizes[chosenTexture][1];
        dest.h = textureSizes[chosenTexture][1];
        if (textureSizes[chosenTexture][0] + posX > limitX) {
          src.w = limitX - posX;
          dest.w = limitX - posX;
        } else {
          src.w = textureSizes[chosenTexture][0];
          dest.w = textureSizes[chosenTexture][0];
        }

        if (SDL_RenderCopy(r, textures[chosenTexture], &src, &dest) != 0) {
          return 7;
        }
      }

      if (textureSizes[chosenTexture][0] + posX > limitX) {
        posX += limitX - posX;

        posY += lineHeight;
        lineHeight = f->height;

        if (greatestLineWidth < posX - x) {
          greatestLineWidth = posX - x;
        }
        posX = x;
      } else {
        posX += textureSizes[chosenTexture][0];
      }
    } else if (chRes.stop == 'u') {
      uint8_t chosenControl = ((uint8_t)chRes.arg) - 1;
      if (chosenControl >= numControls) {
        return 9;
      }
      controls[chosenControl].x = posX;
      controls[chosenControl].y = posY;
      posX += controls[chosenControl].w;
      if (controls[chosenControl].h > lineHeight) {
        lineHeight = controls[chosenControl].h;
      }
    } else if (chRes.stop == 'l') {
      if (loader == NULL) {
        return 10;
      }
      SDL_Texture *t = loader(((uint8_t)chRes.arg) - 1);
      if (t == NULL) {
        return 11;
      }

      yatref_TextureSize size;
      if (SDL_QueryTexture(t, NULL, NULL, size, size + 1)) {
        return 12;
      }

      if (size[1] > lineHeight) {
        lineHeight = size[1];
      }

      if (r != NULL) {
        SDL_Rect src;
        SDL_Rect dest;
        src.x = 0;
        src.y = 0;
        dest.x = posX;
        dest.y = posY;
        src.h = size[1];
        dest.h = size[1];
        if (size[0] + posX > limitX) {
          src.w = limitX - posX;
          dest.w = limitX - posX;
        } else {
          src.w = size[0];
          dest.w = size[0];
        }

        if (SDL_RenderCopy(r, t, &src, &dest) != 0) {
          return 7;
        }
      }

      if (size[0] + posX > limitX) {
        posX += limitX - posX;

        posY += lineHeight;
        lineHeight = f->height;

        if (greatestLineWidth < posX - x) {
          greatestLineWidth = posX - x;
        }
        posX = x;
      } else {
        posX += size[0];
      }
    } else {
      return 4;
    }
  }

  if (sizeX != NULL) {
    *sizeX = greatestLineWidth;
  }
  if (sizeY != NULL) {
    *sizeY = posY - y;
  }
  return 0;
}
