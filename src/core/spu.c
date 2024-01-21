#include "spu.h"
#include "cpu.h"

static SPU_t this;

static const float rateCutoff = OUTPUT_FREQUENCY;

// static const char* registerNames0[] = {
//   "wave address",          // 0x0
//   "audio mode",            // 0x1
//   "loop address",          // 0x2
//   "pan volume",            // 0x3
//   "envelope 0",            // 0x4
//   "envelope data",         // 0x5
//   "envelope 1",            // 0x6
//   "envelope high address", // 0x7
//   "envelope address",      // 0x8
//   "prev wave data",        // 0x9
//   "envelope loop ctrl",    // 0xA
//   "wave data",             // 0xB
//   "adpcm select",          // 0xC
//   "unknown 13",            // 0xD
//   "unknown 14",            // 0xE
//   "unknown 15",            // 0xF
// };
//
// static const char* registerNames2[] = {
//   "phase high",             // 0x0
//   "phase accumulator high", // 0x1
//   "target phase high",      // 0x2
//   "ramp down clock",        // 0x3
//   "phase",                  // 0x4
//   "phase accumulator",      // 0x5
//   "target phase",           // 0x6
//   "phase ctrl",             // 0x7
// };
//
// static const char* registerNames4[] = {
//   "enable",                // 0x00
//   "volume",                // 0x01
//   "FIQ enable",            // 0x02
//   "FIQ stat",              // 0x03
//   "beat base count",       // 0x04
//   "beat count",            // 0x05
//   "envelope clock 0",      // 0x06
//   "envelope clock 0 high", // 0x07
//   "envelope clock 1",      // 0x08
//   "envelope clock 1 high", // 0x09
//   "envelope ramp down",    // 0x0a
//   "stop",                  // 0x0b
//   "zero cross",            // 0x0c
//   "ctrl",                  // 0x0d
//   "compression ctrl",      // 0x0e
//   "stat",                  // 0x0f
//   "L wave in",             // 0x10
//   "R wave in",             // 0x11
//   "L wave out",            // 0x12
//   "R wave out",            // 0x13
//   "channel repeat",        // 0x14
//   "channel env mode",      // 0x15
//   "channel tone release",  // 0x16
//   "channel env irq",       // 0x17
//   "channel pitch bend",    // 0x18
//   "channel soft phase",    // 0x19
//   "attack release",        // 0x1a
//   "eq cutoff 10",          // 0x1b
//   "eq cutoff 32",          // 0x1c
//   "eq gain 10",            // 0x1d
//   "eq gain 32",            // 0x1e
//   "unknown 0x1f"           // 0x1f
// };


static const int8_t adpcmStepShift[] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static const int16_t adpcmLookup[] = {
    2,     6,    10,    14,    18,    22,    26,    30,    -2,    -6,   -10,   -14,   -18,   -22,   -26,   -30,
    2,     6,    10,    14,    19,    23,    27,    31,    -2,    -6,   -10,   -14,   -19,   -23,   -27,   -31,
    2,     6,    11,    15,    21,    25,    30,    34,    -2,    -6,   -11,   -15,   -21,   -25,   -30,   -34,
    2,     7,    12,    17,    23,    28,    33,    38,    -2,    -7,   -12,   -17,   -23,   -28,   -33,   -38,
    2,     7,    13,    18,    25,    30,    36,    41,    -2,    -7,   -13,   -18,   -25,   -30,   -36,   -41,
    3,     9,    15,    21,    28,    34,    40,    46,    -3,    -9,   -15,   -21,   -28,   -34,   -40,   -46,
    3,    10,    17,    24,    31,    38,    45,    52,    -3,   -10,   -17,   -24,   -31,   -38,   -45,   -52,
    3,    10,    18,    25,    34,    41,    49,    56,    -3,   -10,   -18,   -25,   -34,   -41,   -49,   -56,
    4,    12,    21,    29,    38,    46,    55,    63,    -4,   -12,   -21,   -29,   -38,   -46,   -55,   -63,
    4,    13,    22,    31,    41,    50,    59,    68,    -4,   -13,   -22,   -31,   -41,   -50,   -59,   -68,
    5,    15,    25,    35,    46,    56,    66,    76,    -5,   -15,   -25,   -35,   -46,   -56,   -66,   -76,
    5,    16,    27,    38,    50,    61,    72,    83,    -5,   -16,   -27,   -38,   -50,   -61,   -72,   -83,
    6,    18,    31,    43,    56,    68,    81,    93,    -6,   -18,   -31,   -43,   -56,   -68,   -81,   -93,
    6,    19,    33,    46,    61,    74,    88,   101,    -6,   -19,   -33,   -46,   -61,   -74,   -88,  -101,
    7,    22,    37,    52,    67,    82,    97,   112,    -7,   -22,   -37,   -52,   -67,   -82,   -97,  -112,
    8,    24,    41,    57,    74,    90,   107,   123,    -8,   -24,   -41,   -57,   -74,   -90,  -107,  -123,
    9,    27,    45,    63,    82,   100,   118,   136,    -9,   -27,   -45,   -63,   -82,  -100,  -118,  -136,
   10,    30,    50,    70,    90,   110,   130,   150,   -10,   -30,   -50,   -70,   -90,  -110,  -130,  -150,
   11,    33,    55,    77,    99,   121,   143,   165,   -11,   -33,   -55,   -77,   -99,  -121,  -143,  -165,
   12,    36,    60,    84,   109,   133,   157,   181,   -12,   -36,   -60,   -84,  -109,  -133,  -157,  -181,
   13,    39,    66,    92,   120,   146,   173,   199,   -13,   -39,   -66,   -92,  -120,  -146,  -173,  -199,
   14,    43,    73,   102,   132,   161,   191,   220,   -14,   -43,   -73,  -102,  -132,  -161,  -191,  -220,
   16,    48,    81,   113,   146,   178,   211,   243,   -16,   -48,   -81,  -113,  -146,  -178,  -211,  -243,
   17,    52,    88,   123,   160,   195,   231,   266,   -17,   -52,   -88,  -123,  -160,  -195,  -231,  -266,
   19,    58,    97,   136,   176,   215,   254,   293,   -19,   -58,   -97,  -136,  -176,  -215,  -254,  -293,
   21,    64,   107,   150,   194,   237,   280,   323,   -21,   -64,  -107,  -150,  -194,  -237,  -280,  -323,
   23,    70,   118,   165,   213,   260,   308,   355,   -23,   -70,  -118,  -165,  -213,  -260,  -308,  -355,
   26,    78,   130,   182,   235,   287,   339,   391,   -26,   -78,  -130,  -182,  -235,  -287,  -339,  -391,
   28,    85,   143,   200,   258,   315,   373,   430,   -28,   -85,  -143,  -200,  -258,  -315,  -373,  -430,
   31,    94,   157,   220,   284,   347,   410,   473,   -31,   -94,  -157,  -220,  -284,  -347,  -410,  -473,
   34,   103,   173,   242,   313,   382,   452,   521,   -34,  -103,  -173,  -242,  -313,  -382,  -452,  -521,
   38,   114,   191,   267,   345,   421,   498,   574,   -38,  -114,  -191,  -267,  -345,  -421,  -498,  -574,
   42,   126,   210,   294,   379,   463,   547,   631,   -42,  -126,  -210,  -294,  -379,  -463,  -547,  -631,
   46,   138,   231,   323,   417,   509,   602,   694,   -46,  -138,  -231,  -323,  -417,  -509,  -602,  -694,
   51,   153,   255,   357,   459,   561,   663,   765,   -51,  -153,  -255,  -357,  -459,  -561,  -663,  -765,
   56,   168,   280,   392,   505,   617,   729,   841,   -56,  -168,  -280,  -392,  -505,  -617,  -729,  -841,
   61,   184,   308,   431,   555,   678,   802,   925,   -61,  -184,  -308,  -431,  -555,  -678,  -802,  -925,
   68,   204,   340,   476,   612,   748,   884,  1020,   -68,  -204,  -340,  -476,  -612,  -748,  -884, -1020,
   74,   223,   373,   522,   672,   821,   971,  1120,   -74,  -223,  -373,  -522,  -672,  -821,  -971, -1120,
   82,   246,   411,   575,   740,   904,  1069,  1233,   -82,  -246,  -411,  -575,  -740,  -904, -1069, -1233,
   90,   271,   452,   633,   814,   995,  1176,  1357,   -90,  -271,  -452,  -633,  -814,  -995, -1176, -1357,
   99,   298,   497,   696,   895,  1094,  1293,  1492,   -99,  -298,  -497,  -696,  -895, -1094, -1293, -1492,
  109,   328,   547,   766,   985,  1204,  1423,  1642,  -109,  -328,  -547,  -766,  -985, -1204, -1423, -1642,
  120,   360,   601,   841,  1083,  1323,  1564,  1804,  -120,  -360,  -601,  -841, -1083, -1323, -1564, -1804,
  132,   397,   662,   927,  1192,  1457,  1722,  1987,  -132,  -397,  -662,  -927, -1192, -1457, -1722, -1987,
  145,   436,   728,  1019,  1311,  1602,  1894,  2185,  -145,  -436,  -728, -1019, -1311, -1602, -1894, -2185,
  160,   480,   801,  1121,  1442,  1762,  2083,  2403,  -160,  -480,  -801, -1121, -1442, -1762, -2083, -2403,
  176,   528,   881,  1233,  1587,  1939,  2292,  2644,  -176,  -528,  -881, -1233, -1587, -1939, -2292, -2644,
  194,   582,   970,  1358,  1746,  2134,  2522,  2910,  -194,  -582,  -970, -1358, -1746, -2134, -2522, -2910
};


static const int32_t rampdownFrameCounts[] = {
  13*4, 13*16, 13*64, 13*256, 13*1024, 13*4096, 13*8192, 13*8192
};


static const int32_t envelopeFrameCounts[] = {
	4*4, 8*4, 16*4, 32*4, 64*4, 128*4, 256*4, 512*4, 1024*4, 2048*4, 4096*4, 8192*4, 8192*4, 8192*4, 8192*4, 8192*4
};


bool SPU_Init() {
  memset(&this, 0, sizeof(SPU_t));

  this.sampleTimer = SPU_SAMPLE_TIMER;

  for (int32_t i = 0; i < 16; i++) {
    this.channels[i].timer = Timer_Init(0, (TimerFunc_t)SPU_TriggerChannelIRQ, i);
  }

  //SYSCLOCK / (281250.0f/4.0f)
  this.beatTimer = Timer_Init(384, (TimerFunc_t)SPU_TriggerBeatIRQ, 0);
  Timer_Reset(this.beatTimer);

  this.enabledChannels = 0xffff;

  return true;
}


void SPU_Cleanup() {
  for (int32_t i = 0; i < 16; i++) {
    Timer_Cleanup(this.channels[i].timer);
  }

  Timer_Cleanup(this.beatTimer);
}


void SPU_Reset() {
  // for (int32_t i = 0; i < 16; i++) {
  //
  // }

  Timer_Reset(this.beatTimer);
}


void SPU_SaveState() {
  Backend_WriteSave(&this, sizeof(SPU_t));
  Timer_SaveState(this.beatTimer);
  for (int32_t i = 0; i < 16; i++) {
    Timer_SaveState(this.channels[i].timer);
  }
}


void SPU_LoadState() {
  Timer_t* beatTimer = this.beatTimer;
  Timer_t* timer[16];
  for (int32_t i = 0; i < 16; i++)
    timer[i] = this.channels[i].timer;
  Backend_ReadSave(&this, sizeof(SPU_t));
  this.beatTimer = beatTimer;
  Timer_LoadState(this.beatTimer);
  for (int32_t i = 0; i < 16; i++) {
    this.channels[i].timer = timer[i];
    Timer_LoadState(this.channels[i].timer);
  }
}


void SPU_Tick(int32_t cycles) {
  Timer_Tick(this.beatTimer, cycles);

  if (this.sampleTimer > 0) {
    this.sampleTimer -= cycles;
    this.accumulatedSamples += cycles;
    return;
  }
  this.sampleTimer += SPU_SAMPLE_TIMER;

  while (this.accumulatedSamples > 0) {
    for (int32_t i = 0; i < 16; i++) {
      Timer_Tick(this.channels[i].timer, SPU_SAMPLE_TIMER-this.sampleTimer);
    }
    this.accumulatedSamples -= SPU_SAMPLE_TIMER;
  }

  if (this.channelIrq) {
    CPU_TriggerFIQ(FIQSRC_SPU);
    this.channelIrq = false;
  }

  float leftSample = 0;
  float rightSample = 0;

  int32_t left, right;
  for (int32_t i = 0; i < 16; i++) {
    if (this.chanEnable & (1 << i)) { // Channel is enabled
      SPU_TickChannel(i, &left, &right);

      if (this.enabledChannels & (1 << i)) {
        leftSample  += left  / 16.0f;
        rightSample += right / 16.0f;
      }

    } else {
      Backend_PushOscilloscopeSample(i, 0);
    }

  }

  leftSample  *= (1.0f/32.0f);
  rightSample *= (1.0f/32.0f);

  switch((this.volumeSelect >> 6) & 3) { // Volume select
  case 0:
    leftSample  *= (1.0f/16.0f);
    rightSample *= (1.0f/16.0f);
    break;

  case 1:
    leftSample  *= (1.0f/8.0f);
    rightSample *= (1.0f/8.0f);
    break;

  case 2: break;

  case 3:
    leftSample *= 2.0f;
    leftSample *= 2.0f;
    break;
  }

  // printf("%f, %f\n", leftSample, rightSample);

  // Normalize samples
  if (leftSample >  1.0f) leftSample =  1.0f;
  if (leftSample < -1.0f) leftSample = -1.0f;

  if (rightSample >  1.0f) rightSample =  1.0f;
  if (rightSample < -1.0f) rightSample = -1.0f;

  // Push to audio buffer
  Backend_PushAudioSample(leftSample, rightSample);
  this.leftOut = (int16_t)(leftSample*32767.0f);
  this.rightOut = (int16_t)(rightSample*32767.0f);
}


uint16_t SPU_TickSample(uint8_t ch) {
  Channel_t* channel = &this.channels[ch];

  channel->prevWaveData = channel->waveData;

  channel->accum += channel->rate * VSmile_GetClockScale();
  int32_t sampleTicks = 0;
  while (channel->accum >= rateCutoff) {
    channel->accum -= rateCutoff;
    sampleTicks++;
  }

  if (sampleTicks < 1)
    return channel->waveData;

  uint32_t waveAddr = channel->waveAddr | (channel->mode.waveHi << 16);

  uint16_t nybble;
  for (int32_t i = 0; i < sampleTicks; i++) {
    if ((channel->mode.pcmMode & 2) || channel->adpcmSel.codec) { // ADPCM mode
      if (channel->adpcmSel.codec && channel->adpcm36Remaining == 0 && channel->mode.playMode != 0) {
        channel->adpcm36Header.raw = Bus_Load(waveAddr);
        waveAddr++;
        channel->adpcm36Remaining = 8;
      }
      if(Bus_Load(waveAddr) == 0xffff) {
        waveAddr++;
        if (channel->mode.playMode == 1) { // One shot mode
          SPU_StopChannel(ch);
        } else {
          waveAddr = channel->loopAddr | (channel->mode.loopHi << 16);
          channel->pcmShift = 0;
          channel->adpcmLastSample = 0;
          channel->adpcmStepIndex = 0;
          channel->mode.raw &= ~0x8000;
        }
      } else {
        nybble = (Bus_Load(waveAddr) >> channel->pcmShift) & 0xf;
        if (channel->adpcmSel.codec) {
          channel->waveData = SPU_GetADPCM36Sample(ch, nybble);
        } else {
          channel->waveData = SPU_GetADPCMSample(ch, nybble) ^ 0x8000;
        }

        channel->pcmShift += 4;
        if (channel->pcmShift >= 16) {
          channel->pcmShift = 0;
          waveAddr++;
          channel->adpcm36Remaining--;
        }
      }
      continue;
    }

    if (channel->mode.pcmMode == 1) { // 16-bit PCM mode
      channel->waveData = Bus_Load(waveAddr);
      waveAddr++;

      if(Bus_Load(waveAddr) == 0xffff && channel->mode.playMode != 0) {
        if (channel->mode.playMode == 1) { // One shot mode
          SPU_StopChannel(ch);
        } else {
          waveAddr = channel->loopAddr | (channel->mode.loopHi << 16);
        }
      }
      continue;
    }

    if (channel->mode.pcmMode == 0) { // 8-bit PCM mode
      channel->waveData = (uint8_t)(Bus_Load(waveAddr) >> channel->pcmShift) & 0xff;
      channel->waveData <<= 8;
      channel->pcmShift += 8;
      if (channel->pcmShift > 8) {
        channel->pcmShift = 0;
        waveAddr++;
      }

      uint8_t nextSample = (Bus_Load(waveAddr) >> channel->pcmShift) & 0xff;
      if(nextSample == 0xff) {
        if (channel->mode.playMode == 1) { // One shot mode
          SPU_StopChannel(ch);
        } else {
          waveAddr = channel->loopAddr | (channel->mode.loopHi << 16);
          channel->pcmShift = 0;
        }
      }
      continue;
    }
  }

  channel->waveAddr = waveAddr & 0xffff;
  channel->mode.waveHi = (waveAddr >> 16) & 0x3f;

  return channel->waveData;
}


void SPU_TickChannel(uint8_t ch, int32_t* left, int32_t* right) {
  Channel_t* channel = &this.channels[ch];

  SPU_TickSample(ch);

  int32_t sample = (int16_t)(channel->waveData ^ 0x8000);
  if (!(this.ctrl & 0x0200)) { // Audio CTRL
    int32_t prevSample = (int16_t)(channel->waveData ^ 0x8000);
    int32_t lerp = (int32_t)((channel->rate / OUTPUT_FREQUENCY) * 256.0f);
    prevSample = (prevSample * (0x100 - lerp)) >> 8;
    sample = (sample * lerp) >> 8;
    sample += prevSample;
  }

  Backend_PushOscilloscopeSample(ch, (int16_t)sample);

  float fsample = ((sample * (int32_t)channel->envData.envelopeData) >> 7) / 8192.0f;
  if (channel->mode.pcmMode & 2) fsample *= 8.0f; // Hack to make VOX samples louder

  float pan = channel->panVol.pan / 127.0f;
  float vol = channel->panVol.vol;

  float panLeft, panRight;
  if (pan > 1/64.0f) {
    panLeft = 1.0f * vol;
    panRight = pan * 2.0f * vol;
  } else {
    panLeft  = (1.0f - pan) * 2.0f * vol;
    panRight = 1.0f * vol;
  }

  (*left)  = (fsample * panLeft);
  (*right) = (fsample * panRight);

  if (this.envRampDown & (1 << ch)) { // Ramp Down
    if (channel->rampDownFrame > 0)
      channel->rampDownFrame--;

    if (channel->rampDownFrame == 0) {
      uint16_t prevEnvData = channel->envData.envelopeData;
      uint16_t currEnvData = prevEnvData - channel->envLoopCtrl.rampDownOffset;
      if (currEnvData > prevEnvData)
        currEnvData = 0;

      if (currEnvData > 0) {
        channel->envData.envelopeData = currEnvData;
        channel->rampDownFrame = rampdownFrameCounts[channel->rampDownClock & 7];
      } else {
        SPU_StopChannel(ch);
        // this.chanEnable  &= ~(1 << ch);
        // this.chanStat    &= ~(1 << ch);
        // this.chanStop    |=  (1 << ch);
        // this.envRampDown &= ~(1 << ch);
        // this.toneRelease &= ~(1 << ch);
      }
    }
  }
  else if (!(this.envMode & (1 << ch))) { // Envelope mode
    if (channel->envelopeFrame > 0)
      channel->envelopeFrame--;

    if (channel->envelopeFrame == 0) {
      SPU_TickEnvelope(ch);
      channel->envelopeFrame = envelopeFrameCounts[SPU_GetEnvelopeClock(ch)];
    }
  }
}


int16_t SPU_GetADPCMSample(uint8_t ch, uint8_t nybble) {
  Channel_t* channel = &this.channels[ch];

  channel->adpcmLastSample += adpcmLookup[(channel->adpcmStepIndex << 4) + (nybble & 0xf)];
  if (channel->adpcmLastSample >  2047) channel->adpcmLastSample =  2047;
  if (channel->adpcmLastSample < -2048) channel->adpcmLastSample = -2048;

  channel->adpcmStepIndex += adpcmStepShift[(nybble & 0x7)];
  if (channel->adpcmStepIndex > 48) channel->adpcmStepIndex = 48;
  if (channel->adpcmStepIndex <  0) channel->adpcmStepIndex =  0;

  return channel->adpcmLastSample;
}


int16_t SPU_GetADPCM36Sample(uint8_t ch, uint8_t nybble) {
  Channel_t* channel = &this.channels[ch];

  int32_t shift = channel->adpcm36Header.shift;
  int16_t f0 = channel->adpcm36Header.filter;
  if (f0 & 0x20) f0 |= ~0x3f;
  int16_t f1 = 1;
  int16_t sample = nybble << 12;
	sample = (sample >> shift) + (((channel->adpcm36Prev[0] * f0) + (channel->adpcm36Prev[1] * f1) + 32) >> 12);
	channel->adpcm36Prev[1] = channel->adpcm36Prev[0];
	channel->adpcm36Prev[0] = sample;
  // printf("%04x\n", sample ^ 0x8000);

  return sample ^ 0x8000;
}


void SPU_TickEnvelope(uint8_t ch) {
  Channel_t* channel = &this.channels[ch];

  uint16_t prevEnvData = channel->envData.envelopeData;

  if (channel->envData.envelopeCount > 0)
    channel->envData.envelopeCount--;

  if (channel->envData.envelopeCount == 0) {
    if (channel->envData.envelopeData != channel->env0.target) {
      if (channel->env0.increment & 0x80) { // Decrement envelope data
        channel->envData.envelopeData -= (channel->env0.increment & 0x7f);

        // Clamp to 0 or target value
        if (channel->envData.envelopeData > prevEnvData)
          channel->envData.envelopeData = 0;
        else if (channel->envData.envelopeData < channel->env0.target)
          channel->envData.envelopeData = channel->env0.target;

        // Stop Channel
        if (channel->envData.envelopeData == 0) {
          SPU_StopChannel(ch);
          return;
        }
      } else { // Increment envelope data
        channel->envData.envelopeData += (channel->env0.increment & 0x7f);

        // Clamp to target value
        if (channel->envData.envelopeData >= channel->env0.target)
          channel->envData.envelopeData = channel->env0.target;
      }
    }

    if (channel->envData.envelopeData == channel->env0.target) {
      uint32_t envAddr = channel->envAddr | (channel->envAddrHigh.envAddrHi << 16);

      if (channel->env1.repeatEnable) {
        channel->env1.repeatCount--;
        if (channel->env1.repeatCount == 0) {
          channel->env0.raw = Bus_Load(envAddr);
          channel->env1.raw = Bus_Load(envAddr+1);
          channel->envLoopCtrl.raw = Bus_Load(envAddr+2);
           envAddr = (channel->envAddr | (channel->envAddrHigh.envAddrHi << 16)) + channel->envLoopCtrl.envAddrOffset;
        }
      } else {
        channel->env0.raw = Bus_Load(envAddr);
        channel->env1.raw = Bus_Load(envAddr+1);
        envAddr += 2;
      }

      channel->envAddr = envAddr & 0xffff;
      channel->envAddrHigh.envAddrHi = (envAddr >> 16) & 0x3f;
      channel->envData.envelopeCount = channel->env1.loadVal;
    }
  }
}


uint16_t SPU_GetEnvelopeClock(uint8_t ch) {
  switch (ch) {
  case 0x0 ... 0x3: return (this.envClock0   >> ((ch)    << 2)) & 0xf;
  case 0x4 ... 0x7: return (this.envClock0Hi >> ((ch-4)  << 2)) & 0xf;
  case 0x8 ... 0xb: return (this.envClock1   >> ((ch-8)  << 2)) & 0xf;
  case 0xc ... 0xf: return (this.envClock1Hi >> ((ch-12) << 2)) & 0xf;
  }

  return 0x0000;
}


void SPU_TriggerChannelIRQ(uint8_t ch) {
  // printf("channel IRQ triggered for channel %d\n", ch);

  if (!(this.fiqEnable & (1 << ch))) {
    this.fiqStat |= (1 << ch);
    this.channelIrq = true;
    CPU_ActivatePendingIRQs();
  }

  Timer_Adjust(this.channels[ch].timer, SYSCLOCK / this.channels[ch].rate);
}


uint16_t SPU_Read(uint16_t addr) {
  // switch (addr & 0x0f00) {
  // case 0x0000: {
  //   uint8_t channel = (addr >> 0x4) & 0xf;
  //   uint8_t reg     = (addr &  0xf);
  //   printf("read from channel %d %s (%04x) at %06x\n", channel, registerNames0[reg], addr, CPU_GetCSPC());
  // } break;
  //
  // case 0x0200: {
  //   uint8_t channel = (addr >> 0x4) & 0xf;
  //   uint8_t reg     = (addr &  0x7);
  //   printf("read from channel %d %s (%04x) at %06x\n", channel, registerNames2[reg], addr, CPU_GetCSPC());
  // } break;
  //
  // case 0x0400: {
  //   uint8_t reg = (addr & 0x1f);
  //   printf("read from channel %s (%04x) at %06x\n", registerNames4[reg], addr, CPU_GetCSPC());
  // } break;
  //
  // default:
  //   printf("read from unknown register %04x at %06x\n", addr, CPU_GetCSPC());
  // }

  if ((addr & 0x0f00) == 0x0000) {
    uint8_t channel = (addr >> 0x4) & 0xf;
    uint8_t reg     = (addr &  0xf);
    return this.channels[channel].regs0[reg];
  }

  if ((addr & 0x0f00) == 0x0200) {
    uint8_t channel = (addr >> 0x4) & 0xf;
    uint8_t reg     = (addr &  0x7);
    return this.channels[channel].regs2[reg];
  }

  if ((addr & 0x0f00) == 0x0400) {
    uint8_t reg = (addr & 0x1f);
    return this.regs4[reg];
  }

  VSmile_Warning("Read from unknown SPU register %04x", addr);

  return 0x0000;
}


void SPU_Write(uint16_t addr, uint16_t data) {
  // switch (addr & 0x0f00) {
  // case 0x0000: {
  //   uint8_t channel = (addr >> 0x4) & 0xf;
  //   uint8_t reg     = (addr &  0xf);
  //   printf("write to channel %d %s (%04x) with %04x at %06x\n", channel, registerNames0[reg], addr, data, CPU_GetCSPC());
  // } break;
  //
  // case 0x0200: {
  //   uint8_t channel = (addr >> 0x4) & 0xf;
  //   uint8_t reg     = (addr &  0x7);
  //   printf("write to channel %d %s (%04x) with %04x at %06x\n", channel, registerNames2[reg], addr, data, CPU_GetCSPC());
  // } break;
  //
  // case 0x0400: {
  //   uint8_t reg = (addr & 0x1f);
  //   printf("write to %s (%04x) with %04x at %06x\n", registerNames4[reg], addr, data, CPU_GetCSPC());
  // } break;
  //
  // default:
  //   printf("write to unknown register %04x with %04x at %06x\n", addr, data, CPU_GetCSPC());
  // }


  if ((addr & 0x0f00) == 0x0000) {
    Channel_t* channel = &this.channels[(addr >> 0x4) & 0xf];

    uint8_t reg = (addr &  0xf);

    switch (reg) {
    case 0x00:
    case 0x01:
      channel->accum = 0.0f;
      channel->regs0[reg] = data;
      return;

    default: channel->regs0[reg] = data; return;
    }
  }

  if ((addr & 0x0f00) == 0x0200) {
    Channel_t* channel = &this.channels[(addr >> 0x4) & 0xf];

    uint8_t reg = (addr &  0x7);

    switch (reg) {
      case 0x00:   // Phase high
      case 0x04: { // Phase
        channel->regs2[reg] = data;
        int32_t intPhase = channel->phase | ((channel->phaseHigh & 7) << 16);
        float phase = (float)intPhase;
        channel->rate = (phase * 281250.0f) / 524288.0f;
        channel->accum = 0.0f;
        if (channel->isPlaying)
          Timer_Adjust(channel->timer, channel->rate);
        // printf("channel %d rate set to %f\n", (addr >> 0x4) & 0xf, channel->rate);
      } return;

      default: channel->regs2[reg] = data; return;
    }

    return;
  }

  if ((addr & 0x0f00) == 0x0400) {
    uint8_t reg = (addr & 0x1f);

    switch (reg) {
    case 0x00: SPU_EnableChannels(data); return;
    case 0x03:
      this.regs4[reg] &= ~data;
      if (!this.regs4[reg])
        this.channelIrq = false;
      return;
    case 0x05: SPU_WriteBeatCount(data); return;
    case 0x0b: this.regs4[reg] &= ~data; return;
    // case 0x1b: case 0x1c: printf("0x34%02x = %04x\n", reg, data);
    default:   this.regs4[reg] = data;   return;
    }

    return;
  }

  VSmile_Warning("write to unknown SPU register %04x with %04x", addr, data);

}


void SPU_EnableChannels(uint16_t data) {
  const uint16_t changed = (this.chanEnable ^ data);

  if (changed) {
    for (int32_t i = 0; i < 16; i++) {
      uint16_t mask = (1 << i);

      if (!(changed & mask))
        continue;

      if (data & mask) {
        SPU_StartChannel(i);
      } else {
        SPU_StopChannel(i);
      }

    }

    this.chanEnable = data;
  }
}


void SPU_StartChannel(uint8_t ch) {
  Channel_t* channel = &this.channels[ch];

  this.chanEnable |= (1 << ch); // Channel Enable

  if (this.fiqEnable & (1 << ch)) { // Channel FIQ Status
    Timer_Adjust(channel->timer, SYSCLOCK / channel->rate);
  } else {
    Timer_Stop(channel->timer);
  }

  if (!(this.chanStop & (1 << ch))) { // Channel Stop
    this.chanStat |= (1 << ch); // Channel Status

    channel->adpcmLastSample = 0;
    channel->adpcmStepIndex = 0;

    channel->adpcm36Remaining = 0;
    channel->adpcm36Header.raw = 0;
    channel->adpcm36Prev[0] = 0;
    channel->adpcm36Prev[1] = 0;
  }

  channel->isPlaying = true;

}


void SPU_StopChannel(uint8_t ch) {
  Channel_t* channel = &this.channels[ch];

  this.chanEnable  &= ~(1 << ch);
  this.envRampDown &= ~(1 << ch);
  this.chanStop    |=  (1 << ch);
  this.chanStat    &= ~(1 << ch);
  this.toneRelease &= ~(1 << ch);

  channel->mode.pcmMode = 0;
  channel->pcmShift = 0;
  channel->adpcmLastSample = 0;
  channel->adpcmStepIndex = 0;
  channel->waveData = 0;
  channel->prevWaveData = 0;

  Timer_Stop(channel->timer);

  channel->isPlaying = false;
}


void SPU_WriteBeatCount(uint16_t data) {
  // printf("write to beat count with %04x\n", data);

  this.beatCount.raw &= ~(data & 0x4000);
  this.beatCount.raw &= 0x4000;
  this.beatCount.raw |= (data & ~0x4000);

  if ((this.beatCount.raw & 0xc000) == 0xc000) {
    this.irq = true;
    CPU_ActivatePendingIRQs();
  } else {
    this.irq = false;
  }

}


void SPU_TriggerBeatIRQ(uint8_t index) {
  if (this.currBeatBase > 0)
    this.currBeatBase--;

  if (this.currBeatBase == 0) {
    this.currBeatBase = this.beatBaseCount;

    if (this.beatCount.count > 0)
      this.beatCount.count--;

    if (this.beatCount.count == 0) {
      if (this.beatCount.irqEn)
        this.beatCount.irq = true;

      if (this.beatCount.irqEn && this.beatCount.irq) {
        this.irq = true;
        CPU_ActivatePendingIRQs();
      } else {
        this.irq = false;
      }
    }
  }

  Timer_Reset(this.beatTimer);
}


uint16_t SPU_GetIRQ() {
  return this.irq;
}


uint16_t SPU_GetChannelIRQ() {
  return this.channelIrq;
}


void SPU_SetEnabledChannels(uint16_t enabled) {
  this.enabledChannels = enabled;
}
