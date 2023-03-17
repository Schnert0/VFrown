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


typedef union {
  uint16_t regs[5];
  struct {
    uint16_t xPos, yPos;
    TileAttr_t attr;
    TileCtrl_t ctrl;
    uint16_t tilemapAddr;
    uint16_t attribAddr;
  };
} Layer_t;


typedef union {
  uint16_t regs[4];
  struct {
    uint16_t   tileID;
    uint16_t   xPos, yPos;
    TileAttr_t attr;
  };
} Sprite_t;


/*
 * Picture Processing Unit
 * Renders Tile data, bitmaps and sprites to the screen
 */
typedef struct PPU_t {
  uint32_t* scanlineBuffer;

  bool spriteOutlinesEnabled;
  bool flipVisualEnabled;
  bool isFirstLayer;
  bool layerEnabled[3];

  Layer_t  layers[2];     // 0x2810 - 0x281b
  uint16_t vertScale;     // 0x281c
  uint16_t vertMovement;  // 0x281d
  uint16_t segmentPtr[2]; // 0x2820 - 0x2821
  uint16_t spriteSegment; // 0x2822
  uint16_t blendLevel;    // 0x282a
  uint16_t fadeLevel;     // 0x2830
  uint16_t vCompare;      // 0x2836
  uint16_t hCompare;      // 0x2837
  uint16_t currLine;      // 0x2838
  uint16_t hueSatAdjust;  // 0x283c
  uint16_t LFPInterlace;  // 0x283d
  uint16_t lightpenX;     // 0x283e
  uint16_t lightpenY;     // 0x283f
  uint16_t spriteEnable;  // 0x2842
  uint16_t lcdCtrl;       // 0x2854
  uint16_t irqCtrl;       // 0x2862
  uint16_t irqStat;       // 0x2863
  uint16_t dmaSrc;        // 0x2870
  uint16_t dmaDst;        // 0x2871
  uint16_t dmaSize;       // 0x2872

  uint16_t scroll[0x100];  // 0x2900 - 0x29ff
  uint16_t hScale[0x100];  // 0x2a00 - 0x2aff
  uint16_t palette[0x100]; // 0x2b00 - 0x2bff
  union {
    uint16_t sprData[0x400]; // 0x2c00 - 0x2fff
    Sprite_t sprites[0x100];
  };
} PPU_t;

bool PPU_Init();
void PPU_Cleanup();

bool PPU_Reset();
void PPU_UpdateScreen();

void PPU_SaveState();
void PPU_LoadState();

uint16_t PPU_Read(uint16_t addr);
void PPU_Write(uint16_t addr, uint16_t data);

bool PPU_RenderLine();
void PPU_RenderTileStrip(int16_t xPos, int16_t tileWidth, uint16_t nc, uint16_t palOffset, uint32_t tileData, bool hFlip, bool vFlip);
void PPU_RenderLayerStrip(int32_t layer, int32_t depth, int32_t line);
void PPU_RenderSpriteStrips(int32_t depth, int32_t line);

uint16_t PPU_GetCurrLine();
void PPU_IncrementCurrLine();

void PPU_DoDMA(uint16_t data);
void PPU_SetIRQFlags(uint16_t data);

void PPU_ToggleSpriteOutlines();
void PPU_ToggleFlipVisual();
void PPU_ToggleLayer(uint8_t layer);

#endif // PPU_H
