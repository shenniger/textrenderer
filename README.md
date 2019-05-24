This is a text rendering system which is designed less for rigorous typographic
correctness and rather for speed and flexibility.

# Features
When I say flexibility, I mean:
* Multi-line text/Word-wrap
* Mutiple fonts of different sizes in the same text block
* Support for rendering other textures and elements into text blocks
* UTF-8 support in simple cases like German umlaute, might also work for longer Unicode sequences

# Control character syntax
All control sequences consist of three characters, of which the first one is always `0x11`.
The second character specifies the control command (see below table) and the third is an
argument to that command (referred to as `a`).

| Character | Description                                                                              |
| --------- | ---------------------------------------------------------------------------------------- |
| 'n'       | Changes the font to `fonts[a-1]`                                                         |
| 'i'       | Renders the texture `textures[a-1]` to the current location                              |
| 'u'       | Saves the current position to `controls[a-1].{x,y}`, then jumps by `controls[a-1].{w,h}` |
| 'l'       | Evaluates `loader(a-1)` and renders the result (useful for lazy evaluation of textures)  |

# Dependencies
* SDL2
* SDL2_ttf

# To-Do
* Eliminate SDL2_ttf dependency and instead use Freetype directly.
* Improve Unicode support.
* Implement basic syllable division features for German and English.
* Render characters on demand.
