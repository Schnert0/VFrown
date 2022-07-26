#ifndef PPU_H
#define PPU_H

#include "../common.h"
#include "vsmile.h"

// 0-1 	Bits Per Pixel 	0 = 2bpp, 1 = 4bpp, 2 = 6bpp, 3 = 8bpp
// 2 	X-Flip 	0 = Not Mirrored, 1 = Mirrored Horizontally
// 3 	Y-Flip 	0 = Not Mirrored, 1 = Mirror Vertically
// 4-5 	Width 	0 = 0, 1 = 256, 2 = 512, 3 = 768
// 6-7 	Height 	0 = 0, 1 = 256, 2 = 512, 3 = 768
// 8-11 	Palette Bank 	Offset into Palette Ram. 0 = 0, 1 = 0x100
// 12-13 	Depth?
// 14-15 	Unused?

typedef union {
  uint16_t raw;
  struct {
    uint16_t bpp     : 2;
    uint16_t hFlip   : 1;
    uint16_t vFlip   : 1;
    uint16_t width   : 2;
    uint16_t height  : 2;
    uint16_t palBank : 4;
    uint16_t depth   : 2;
    uint16_t         : 2;
  };
} TileAttr_t;


// 0 	Bitmap Mode 	0 = Tiled, 1 = Bitmap
// 1 	RegSet 	?
// 2 	Wallpaper 	?
// 3 	Page Enable 	0 = Disabled, 1 = Enabled
// 4 	Row Scroll 	0 = Disabled, 1 = Enabled
// 5 	Horizontal Compression 	?
// 6 	Vertical Compression 	?
// 7 	Hi-Color 	0 = Paletted, 1 = Direct Colour
// 8 	Blend Enable 	0 = Not Blended, 1 = Blended
// 9-15 	Unused? 	?

typedef union {
  uint16_t raw;
  struct {
    uint16_t bitMap       : 1;
    uint16_t regSet       : 1;
    uint16_t wallPaper    : 1;
    uint16_t pageEnable   : 1;
    uint16_t rowScroll    : 1;
    uint16_t hCompression : 1;
    uint16_t vCompression : 1;
    uint16_t directColor  : 1;
    uint16_t blendEnable  : 1;
    uint16_t              : 7;
  };
} TileCtrl_t;

/*
 * Picture Processing Unit
 * Renders Tile data, bitmaps and sprites to the screen
 */
typedef struct PPU_t {
  uint16_t scroll[0x100];
  uint16_t hComp[0x100];
  uint16_t palette[0x100];
  uint16_t sprites[0x400];

  uint16_t currLine;

  bool spriteOutlinesEnabled;
  bool flipVisualEnabled;
  bool isFirstLayer;
  bool layerEnabled[3];

  uint16_t* scanlineBuffer;
} PPU_t;

bool PPU_Init();
void PPU_Cleanup();

bool PPU_Reset();
void PPU_UpdateScreen();

void PPU_RenderLine();
void PPU_RenderTileStrip(int16_t xPos, int16_t tileWidth, uint16_t nc, uint16_t palOffset, uint32_t tileData, bool hFlip, bool vFlip);
void PPU_RenderLayerStrip(int32_t layer, int32_t depth, int32_t line);
void PPU_RenderSpriteStrips(int32_t depth, int32_t line);

uint16_t PPU_GetCurrLine();
void PPU_IncrementCurrLine();

void PPU_ToggleSpriteOutlines();
void PPU_ToggleFlipVisual();
void PPU_ToggleLayer(uint8_t layer);

#endif // PPU_H
