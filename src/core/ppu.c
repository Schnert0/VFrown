#include "ppu.h"

static PPU_t this;

bool PPU_Init() {
  memset(&this, 0, sizeof(PPU_t));

  this.layerEnabled[0] = true; // Layer 0
  this.layerEnabled[1] = true; // Layer 1
  this.layerEnabled[2] = true; // Sprites

  return true;
}


void PPU_Cleanup() {
}


bool PPU_Reset() {
  this.irqCtrl = 0xffff;
  this.irqStat = 0xffff;

  return true;
}


void PPU_UpdateScreen() {
  Backend_UpdateWindow();
}


bool PPU_RenderLine() {
  if (this.currLine < 240) { // If out of frame, don't render line
    if (Backend_RenderScanline()) {
      this.scanlineBuffer = Backend_GetScanlinePointer(this.currLine);

      this.isFirstLayer = true;
      for (int32_t i = 0; i < 4; i++) {
        PPU_RenderLayerStrip(0, i, this.currLine);
        PPU_RenderLayerStrip(1, i, this.currLine);
        PPU_RenderSpriteStrips( i, this.currLine);
      }
    }
  }

  if (this.currLine == this.vCompare) {
    PPU_SetIRQFlags(0x0002);
  }

  if (this.currLine == 240) {
    PPU_SetIRQFlags(0x0001);
  }

  this.currLine++;
  if (this.currLine >= LINES_PER_FIELD) {
    PPU_UpdateScreen();
    this.currLine = 0;
    return true;
  }

  return false;
}


uint16_t PPU_Read(uint16_t addr) {
  // printf("PPU read %04x\n", addr);
  if (addr < 0x2900) {
    switch (addr) {
    case 0x2810: return this.layers[0].xPos;
    case 0x2811: return this.layers[0].yPos;
    case 0x2812: return this.layers[0].attr.raw;
    case 0x2813: return this.layers[0].ctrl.raw;
    case 0x2814: return this.layers[0].tilemapAddr;
    case 0x2815: return this.layers[0].attribAddr;
    case 0x2816: return this.layers[1].xPos;
    case 0x2817: return this.layers[1].yPos;
    case 0x2818: return this.layers[1].attr.raw;
    case 0x2819: return this.layers[1].ctrl.raw;
    case 0x281a: return this.layers[1].tilemapAddr;
    case 0x281b: return this.layers[1].attribAddr;
    case 0x281c: return this.vertScale;
    case 0x281d: return this.vertMovement;
    case 0x2820: return this.segmentPtr[0];
    case 0x2821: return this.segmentPtr[1];
    case 0x2822: return this.spriteSegment;
    case 0x282a: return this.blendLevel;
    case 0x2830: return this.fadeLevel;
    case 0x2836: return this.vCompare;
    case 0x2837: return this.hCompare;
    case 0x2838: return this.currLine;
    case 0x283c: return this.hueSatAdjust;
    case 0x283d: return this.LFPInterlace;
    case 0x283e: return this.lightpenX;
    case 0x283f: return this.lightpenY;
    case 0x2842: return this.spriteEnable;
    case 0x2854: return this.lcdCtrl;
    case 0x2862: return this.irqCtrl;
    case 0x2863: return this.irqStat;
    case 0x2870: return this.dmaSrc;
    case 0x2871: return this.dmaDst;
    case 0x2872: return this.dmaSize;
    }

    VSmile_Warning("unknown read from PPU register %04x at %06x", addr, CPU_GetCSPC());
    return 0x0000;
  }
  else if (addr < 0x2a00) { // 0x2900 - 0x29ff
    return this.scroll[addr - 0x2900];
  }
  else if (addr < 0x2b00) { // 0x2a00 - 0x2aff
    return this.hScale[addr - 0x2a00];
  }
  else if (addr < 0x2c00) { // 0x2b00 - 0x2bff
    return this.palette[addr - 0x2b00];
  }
  else if (addr < 0x3000) { // 0x2c00 - 0x2fff
    return this.sprData[addr - 0x2c00];
  }
  else {
    VSmile_Warning("unknown read from PPU register %04x at %06x", addr, CPU_GetCSPC());
  }

  return 0x0000;
}


void PPU_Write(uint16_t addr, uint16_t data) {
  // printf("PPU write %04x with %04x\n", addr, data);
  if (addr < 0x2900) {
    switch (addr) {
    case 0x2810: this.layers[0].xPos = data & 0x1ff; return;
    case 0x2811: this.layers[0].yPos = data & 0xff;  return;
    case 0x2812: this.layers[0].attr.raw = data;     return;
    case 0x2813: this.layers[0].ctrl.raw = data;     return;
    case 0x2814: this.layers[0].tilemapAddr = data;  return;
    case 0x2815: this.layers[0].attribAddr = data;   return;
    case 0x2816: this.layers[1].xPos = data & 0x1ff; return;
    case 0x2817: this.layers[1].yPos = data & 0xff;  return;
    case 0x2818: this.layers[1].attr.raw = data;     return;
    case 0x2819: this.layers[1].ctrl.raw = data;     return;
    case 0x281a: this.layers[1].tilemapAddr = data;  return;
    case 0x281b: this.layers[1].attribAddr = data;   return;
    case 0x281c: this.vertScale = data & 0xff;       return;
    case 0x281d: this.vertMovement = data & 0x1ff;   return;
    case 0x2820: this.segmentPtr[0] = data;          return;
    case 0x2821: this.segmentPtr[1] = data;          return;
    case 0x2822: this.spriteSegment = data;          return;
    case 0x282a: this.blendLevel = data & 3;         return;
    case 0x2830: this.fadeLevel = data & 0xff;       return;
    case 0x2836: this.vCompare = data & 0x1ff;       return;
    case 0x2837: this.hCompare = data & 0x1ff;       return;
    case 0x283c: this.hueSatAdjust = data & 0xff;    return;
    case 0x283d: this.LFPInterlace = data & 0x5;     return;
    case 0x283e: this.lightpenX = data & 0x1ff;      return;
    case 0x283f: this.lightpenY = data & 0x1ff;      return;
    case 0x2842: this.spriteEnable = data & 1;       return;
    case 0x2854: this.lcdCtrl = data & 0x3f;         return;
    case 0x2862: this.irqCtrl = data;                return;
    case 0x2863: this.irqStat &= ~data;              return;
    case 0x2870: this.dmaSrc  = data & 0x3fff;       return;
    case 0x2871: this.dmaDst  = data & 0x3ff;        return;
    case 0x2872: PPU_DoDMA(data);                    return;
    }
    VSmile_Warning("unknown write to PPU register %04x with %04x at %06x", addr, data, CPU_GetCSPC());
    return;
  }
  else if (addr < 0x2a00) { // 0x2900 - 0x29ff
    this.scroll[addr - 0x2900] = data;
    return;
  }
  else if (addr < 0x2b00) { // 0x2a00 - 0x2aff
    this.hScale[addr - 0x2a00] = data;
    return;
  }
  else if (addr < 0x2c00) { // 0x2b00 - 0x2bff
    // printf("%04x = %04x\n", addr, data);
    this.palette[addr - 0x2b00] = data;
    return;
  }
  else if (addr < 0x3000) { // 0x2c00 - 0x2fff
    this.sprData[addr - 0x2c00] = data;
    return;
  }
  else {
    VSmile_Warning("unknown write to PPU register %04x with %04x at %06x", addr, data, CPU_GetCSPC());
  }
}


void PPU_RenderTileStrip(int16_t xPos, int16_t tileWidth, uint16_t nc, uint16_t palOffset, uint32_t tileData, bool hFlip, bool vFlip) {
  uint32_t m = tileData;
  uint32_t bits = 0;
  uint32_t nbits = 0;

  int32_t start, end, step;
  if (hFlip) {
    start = xPos + (tileWidth - 1);
    end   = xPos - 1;
    step  = -1;
  } else {
    start = xPos;
    end   = xPos + tileWidth;
    step  = 1;
  }

  for (int32_t x = start; x != end; x += step) {
    bits <<= nc;
    if (nbits < nc) {
      uint16_t b = Bus_Load(m++ & 0x3fffff);
      b = (b << 8) | (b >> 8);
      bits |= b << (nc-nbits);
      nbits += 16;
    }
    nbits -= nc;
    uint16_t color = Bus_Load(0x2b00 + palOffset + (bits >> 16));
    if (x >= 0 && x < 320) {
      if (!(color & 0x8000))
        this.scanlineBuffer[x] = color & 0x7fff;

      if (this.flipVisualEnabled) {
        uint16_t color = 0x0000;
        if (hFlip)
          color |= 0xf800;
        if (vFlip)
          color |= 0x001f;
        if ((x & 1) == (this.currLine & 1) && (hFlip || vFlip))
          this.scanlineBuffer[x] = color;
      }
    }

    bits &= 0xffff;
  }
}


void PPU_RenderLayerStrip(int32_t layer, int32_t depth, int32_t line) {
  TileAttr_t attr;
  TileCtrl_t ctrl;
  uint16_t xOffset = Bus_Load(0x2810+layer*6);
  uint16_t yOffset = Bus_Load(0x2811+layer*6);

  ctrl.raw = Bus_Load(0x2813+layer*6);

  attr.raw = Bus_Load(0x2812+layer*6);

  int16_t y = (line + yOffset) & 0xff;

  uint16_t tileWidth  = 8 << (attr.width);
  uint16_t tileHeight = 8 << (attr.height);
  uint16_t nc = (attr.bpp + 1) << 1;
  uint16_t palOffset = attr.palBank << 4;
  palOffset >>= nc;
  palOffset <<= nc;

  if (this.isFirstLayer) {
    memset(this.scanlineBuffer, 0, 320 * sizeof(uint16_t));
    this.isFirstLayer = false;
  }

  if (!this.layerEnabled[layer])
    return;

  if (!ctrl.pageEnable)
    return;

  if (attr.depth != depth)
    return;

  uint16_t hOffset = Bus_Load(0x2900 + ((line+yOffset) & 0xff)) * ctrl.rowScroll;

  int32_t screenTilesW = (320 / tileWidth) + 1;
  int32_t numTilesW = 512 / tileWidth;

  uint16_t tileRow = Bus_Load(0x2814+layer*6);
  uint16_t attributeRow = Bus_Load(0x2815+layer*6);
  uint16_t t = 0;
  if (!ctrl.wallPaper) {
    tileRow += (numTilesW * (y / tileHeight));
    attributeRow += ((numTilesW/2) * (y / tileHeight));
    t = (xOffset / tileWidth) - 1;
    t += (hOffset / tileWidth);
    t &= (numTilesW - 1);
  }

  for (int16_t x = -1; x < screenTilesW+1; x++) {
    uint16_t tile = Bus_Load(tileRow+t);

    int32_t tileHeightOffset = (y & (tileHeight-1));
    if (attr.vFlip && ctrl.regSet)
      tileHeightOffset = tileHeight - tileHeightOffset - 1;

    if (!ctrl.regSet) { // Use attributes from RAM
      // Each word has two attribute entries
      uint16_t attrData = Bus_Load(attributeRow + (t>>1));
      if (t & 1) attrData >>= 8; else attrData &= 0xff;

      // Update horizontal and vertical flip
      attr.raw &= ~0x000c;
      attr.raw |= ((attrData >> 2) & 0x000c);

      // Update palette
      attr.raw &= ~0x0f00;
			attr.raw |= ((attrData << 8) & 0x0f00);

      // Update blending mode
      ctrl.raw &= ~0x0100;
			ctrl.raw |= ((attrData << 2) & 0x0100);

      if (attr.vFlip)
        tileHeightOffset = tileHeight - tileHeightOffset - 1;

      palOffset = attr.palBank << 4;
      palOffset >>= nc;
      palOffset <<= nc;
    }

    int32_t tileSize = tileWidth*tileHeight*nc/16*tile;

    if (tile) {
      uint32_t tileData = (Bus_Load(0x2820+layer) << 6) + tileSize + (tileHeightOffset*(tileWidth*nc/16));
      PPU_RenderTileStrip((x*tileWidth)-(xOffset & (tileWidth-1)) - (hOffset & (tileWidth-1)), tileWidth, nc, palOffset, tileData, attr.hFlip, attr.vFlip);
    }

    if (!ctrl.wallPaper) {
      t++;
      t &= (numTilesW - 1);
    }
  }
}


void PPU_RenderSpriteStrips(int32_t depth, int32_t line) {
  if (!this.layerEnabled[2])
    return;

  if (!(Bus_Load(0x2842) & 1))
    return;

  for (int32_t i = 0; i < 256; i++) {
    uint16_t tile = Bus_Load(0x2c00+(i<<2));
    if (tile == 0)
      continue;

    TileAttr_t attr;
    attr.raw = Bus_Load(0x2c03+(i<<2));

   if (attr.depth != depth)
     continue;

    uint8_t spriteWidth = 8 << attr.width;
    uint8_t spriteHeight = 8 << attr.height;
    uint8_t nc = (attr.bpp + 1) << 1;
    uint16_t palOffset = attr.palBank << 4;

    uint16_t x = Bus_Load(0x2c01+(i<<2));
    uint16_t y = Bus_Load(0x2c02+(i<<2));

    int16_t xPos = 160 + x - (spriteWidth  / 2);
    int16_t yPos = 120 - y - (spriteHeight / 2) + 8;

    if (line < yPos || line >= yPos + spriteHeight)
      continue;

    int32_t spriteSize = spriteWidth*spriteHeight*nc/16*tile;
    int32_t spriteHeightOffset = ((line - yPos) % spriteHeight);

    if (attr.vFlip)
      spriteHeightOffset = spriteHeight - spriteHeightOffset - 1;

    uint32_t tileData = (Bus_Load(0x2822) << 6) + spriteSize + (spriteHeightOffset*(spriteWidth*nc/16));
    PPU_RenderTileStrip(xPos, spriteWidth, nc, palOffset, tileData, attr.hFlip, attr.vFlip);

    if (this.spriteOutlinesEnabled) {
      if (line < 240) {
        if (line == yPos || line == yPos + spriteHeight-1) {
            for (int32_t i = xPos; i < xPos+spriteWidth; i++) {
              if (i >= 0 && i < 320) {
                this.scanlineBuffer[i] = 0xf800;
              }
            }
        } else {
          if (xPos >= 0 && xPos < 320) {
            this.scanlineBuffer[xPos] = 0xf800;
          }

          if (xPos+spriteWidth-1 >= 0 && xPos+spriteWidth-1 < 320) {
            this.scanlineBuffer[xPos+spriteWidth-1] = 0xf800;
          }
        }
      }
    }

  }
}


uint16_t PPU_GetCurrLine() {
  return this.currLine;
}


void PPU_DoDMA(uint16_t data) {
  // printf("PPU DMA\n");
  data &= 0x03ff;
  if (data == 0)
    data = 0x0400;

  uint32_t src = this.dmaSrc;
  uint32_t dst = this.dmaDst;

  for (uint32_t i = 0; i < data; i++) {
    uint16_t transfer = Bus_Load(src+i);
    this.sprData[(dst+i) & 0x3ff] = transfer;
  }

  this.dmaSize = 0;
  PPU_SetIRQFlags(0x0004);
  // printf("PPU DMA\n");
}


void PPU_SetIRQFlags(uint16_t data) {
  this.irqStat |= data;
  CPU_ActivatePendingIRQs();
  // printf("PPU IRQs set (%04x)\n", data);
}


void PPU_ToggleSpriteOutlines() {
  this.spriteOutlinesEnabled = !this.spriteOutlinesEnabled;
}


void PPU_ToggleFlipVisual() {
  this.flipVisualEnabled = !this.flipVisualEnabled;
}


void PPU_ToggleLayer(uint8_t layer) {
  this.layerEnabled[layer] = !this.layerEnabled[layer];
}
