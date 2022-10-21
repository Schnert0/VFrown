#ifndef SPU_H
#define SPU_H

#include "../common.h"
#include "vsmile.h"
#include "timer.h"

#define SPU_SAMPLE_TIMER (SYSCLOCK / 44100)

struct Timer_t;

typedef union {
  uint16_t raw;
  struct {
    uint16_t waveHi   : 6;
    uint16_t loopHi   : 6;
    uint16_t playMode : 2;
    uint16_t pcmMode  : 2;
  };
} SPUMode_t;

typedef union {
  uint16_t raw;
  struct {
    uint16_t vol : 7;
    uint16_t     : 1;
    uint16_t pan : 7;
  };
} SPUPanVol_t;

typedef union {
  uint16_t raw;
  struct {
    uint16_t increment      : 8;
    uint16_t target         : 7;
    uint16_t loopEnable     : 1;
  };
} SPUEnv0_t;

typedef union {
  uint16_t raw;
  struct {
    uint16_t envelopeData  : 7;
    uint16_t               : 1;
    uint16_t envelopeCount : 7;
  };
} SPUEnvData_t;

typedef union {
  uint16_t raw;
  struct {
    uint16_t loadVal      : 8;
    uint16_t repeatEnable : 1;
    uint16_t repeatCount  : 7;
  };
} SPUEnv1_t;

typedef union {
  uint16_t raw;
  struct {
    uint16_t envAddrHi   : 6;
    uint16_t irqEnable   : 1;
    uint16_t irqAddr     : 9;
  };
} SPUEnvAddrHigh_t;

typedef union {
  uint16_t raw;
  struct {
    uint16_t envAddrOffset  : 9;
    uint16_t rampDownOffset : 7;
  };
} SPUEnvLoopCtrl_t;

typedef union {
  uint16_t raw;
  struct {
    uint16_t pointNum : 15;
    uint16_t codec    : 1;
  };
} SPUADPCMSel_t;

typedef union {
  uint16_t raw;
  struct {
    uint16_t phaseOffset : 13;
    uint16_t timeStep    : 3;
  };
} SPUPhaseCtrl_t;

typedef union {
  uint16_t raw;
  struct {
    uint16_t count : 14;
    uint16_t irq   : 1;
    uint16_t irqEn : 1;
  };
} SPUBeatCount_t;


typedef union {
  uint16_t raw;
  struct {
    uint16_t shift  : 4;
    uint16_t filter : 6;
  };
} ADPCM36Header_t;


typedef struct {
  union {
    uint16_t regs0[16];
    struct {
      uint16_t         waveAddr;
      SPUMode_t        mode;
      uint16_t         loopAddr;
      SPUPanVol_t      panVol;
      SPUEnv0_t        env0;
      SPUEnvData_t     envData;
      SPUEnv1_t        env1;
      SPUEnvAddrHigh_t envAddrHigh;
      uint16_t         envAddr;
      uint16_t         prevWaveData;
      SPUEnvLoopCtrl_t envLoopCtrl;
      uint16_t         waveData;
      SPUADPCMSel_t    adpcmSel;
    };
  };

  union {
    uint16_t regs2[8];
    struct {
      uint16_t       phaseHigh;
      uint16_t       phaseAccumHigh;
      uint16_t       targetPhaseHigh;
      uint16_t       rampDownClock;
      uint16_t       phase;
      uint16_t       phaseAccum;
      uint16_t       targetPhase;
      SPUPhaseCtrl_t phaseCtrl;
    };
  };

  float    accum;
  float    rate;
  int32_t  sample;
  uint8_t  pcmShift;
  bool     isPlaying;

  int16_t adpcmStepIndex;
  int16_t  adpcmLastSample;

  uint16_t adpcm36Remaining;
  ADPCM36Header_t adpcm36Header;
  int16_t adpcm36Prev[2];

  uint16_t rampDownFrame, envelopeFrame;

  struct Timer_t* timer;
} Channel_t;

/*
 * Sound Processing Unit
 * Generates soundwaves from sample data
 */

typedef struct SPU_t {
  Channel_t channels[16];
  union {
    uint16_t  regs4[32];
    struct {
      uint16_t chanEnable;
      uint16_t volumeSelect;
      uint16_t fiqEnable;
      uint16_t fiqStat;
      uint16_t beatBaseCount;
      SPUBeatCount_t beatCount;
      uint16_t envClock0;
      uint16_t envClock0Hi;
      uint16_t envClock1;
      uint16_t envClock1Hi;
      uint16_t envRampDown;
      uint16_t chanStop;
      uint16_t zeroCross;
      uint16_t ctrl;
      uint16_t compressionCtrl;
      uint16_t chanStat;
      uint16_t leftIn;
      uint16_t rightIn;
      uint16_t leftOut;
      uint16_t rightOut;
      uint16_t chanRepeat;
      uint16_t envMode;
      uint16_t toneRelease;
      uint16_t chanEnvIrq;
      uint16_t chanPitchBend;
      uint16_t chanSoftRelease;
      uint16_t attackRelease;
      uint16_t eqCutoff10;
      uint16_t eqCutoff32;
      uint16_t eqGain10;
      uint16_t eqGain32;
      uint16_t unknown1f;
    };
  };

  struct Timer_t* beatTimer;
  bool irq, channelIrq;

  uint16_t currBeatBase;
  uint32_t bufferLen;
  int32_t  sampleTimer, accumulatedSamples;
} SPU_t;


bool SPU_Init();
void SPU_Cleanup();

void SPU_Tick(int32_t cycles);

uint16_t SPU_TickSample(uint8_t ch);
void SPU_TickChannel(uint8_t ch, int32_t* left, int32_t* right);
int16_t SPU_GetADPCMSample(uint8_t ch, uint8_t nybble);
int16_t SPU_GetADPCM36Sample(uint8_t ch, uint8_t nybble);
void SPU_TickEnvelope(uint8_t ch);
uint16_t SPU_GetEnvelopeClock(uint8_t ch);
void SPU_TriggerChannelIRQ(uint8_t ch);
void SPU_StartChannel(uint8_t ch);
void SPU_StopChannel(uint8_t ch);


uint16_t SPU_Read(uint16_t addr);
void SPU_Write(uint16_t addr, uint16_t data);

void SPU_EnableChannels(uint16_t data);
void SPU_WriteBeatCount(uint16_t data);

void SPU_TriggerBeatIRQ(uint8_t index);
uint16_t SPU_GetIRQ();
uint16_t SPU_GetChannelIRQ();

#endif // SPU_H
