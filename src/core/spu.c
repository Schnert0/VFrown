#include "spu.h"

static SPU_t this;

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
//   "stat",                   // 0x0f
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

static int16_t adpcmStep[] = {
  16,   17,   19,   21,   23,   25,   28,   31,  34,   37,   41,   45,
  50,   55,   60,   66,   73,   80,   88,   97,  107,  118,  130,  143,
  157,  173,  190,  209,  230,  253,  279,  307, 337,  371,  408,  449,
  494,  544,  598,  658,  724,  796,  876,  963, 1060, 1166, 1282, 1411, 1552
};

static int8_t adpcmStepShift[] = { -1, -1, -1, -1, 2, 4, 6, 8 };

bool SPU_Init() {
  memset(&this, 0, sizeof(SPU_t));

  Backend_InitAudioDevice();

  this.sampleTimer = SPU_SAMPLE_TIMER;

  // for (int32_t i = 0; i < 16; i++) {
  // }

  return true;
}


void SPU_Cleanup() {

}


void SPU_Tick(int32_t cycles) {
  if (this.sampleTimer > 0) {
    this.sampleTimer -= cycles;
    return;
  }
  this.sampleTimer += SPU_SAMPLE_TIMER;

  int32_t leftSample = 0;
  int32_t rightSample = 0;
  int32_t numEnabled = 0;

  for (int32_t i = 0; i < 16; i++) {
    if (this.regs4[0x00] & (1 << i)) { // Channel is enabled
      numEnabled++;

      int32_t sample = SPU_TickChannel(i);

      leftSample  = sample;
      rightSample = sample;

      // uint8_t pan = this.channels[i].panVol.pan;
      // uint8_t vol = this.channels[i].panVol.vol;
      //
      // int32_t panLeft, panRight;
      // if (pan < 0x40) {
      //   panLeft  = 127 * vol;
      //   panRight = pan * 2 * vol;
      // } else {
      //   panLeft  = (127 - pan) * 2 * vol;
      //   panRight =  127 * vol;
      // }
      //
      // leftSample  += ((int16_t)sample * (int16_t)panLeft)  >> 14;
      // rightSample += ((int16_t)sample * (int16_t)panRight) >> 14;
    }
  }

  if (numEnabled > 0) {
    leftSample /= numEnabled;
    rightSample /= numEnabled;
    Backend_PushAudioSample(leftSample, rightSample);
  } else {
    Backend_PushAudioSample(0, 0);
  }
}


int32_t SPU_TickChannel(uint8_t ch) {
  Channel_t* channel = &this.channels[ch];

  channel->accum += channel->rate;
  int32_t sampleTicks = 0;
  while (channel->accum >= 70312.5) {
    channel->accum -= 70312.5;
    sampleTicks++;
  }

  if (sampleTicks < 1)
    return channel->sample;

  uint32_t waveAddr = channel->waveAddr | (channel->mode.waveHi << 16);

  for (int32_t i = 0; i < sampleTicks; i++) {
    switch (channel->mode.pcmMode) {
    case 0: // 8-bit PCM mode
      channel->sample = (int16_t)Bus_Load(waveAddr + channel->sampleOffset) >> channel->pcmShift;
      channel->sample <<= 8;
      channel->pcmShift += 8;
      if (channel->pcmShift > 8) {
        channel->pcmShift = 0;
        channel->sampleOffset++;
      }
      break;

    case 1: // 16-bit PCM mode
      channel->sample = (int16_t)Bus_Load(waveAddr + channel->sampleOffset);
      channel->sampleOffset++;
      break;

    case 2:   // ADPCM mode
    case 3: { // ???
      uint16_t adpcmSample = (Bus_Load(waveAddr + channel->sampleOffset) >> channel->pcmShift) & 0xf;
      channel->sample = SPU_GetADPCMSample(channel, adpcmSample);
      channel->pcmShift += 4;
      if (channel->pcmShift >= 16) {
        channel->pcmShift = 0;
        channel->sampleOffset++;
      }
    } break;

    }
  }

  return channel->sample;
}


int16_t SPU_GetADPCMSample(Channel_t* channel, uint8_t nybble) {
  int16_t step = adpcmStep[channel->adpcmStepIndex];
  int16_t e = step / 8;
  if (nybble & 1) e += step/4;
  if (nybble & 2) e += step/2;
  if (nybble & 4) e += step;
  int16_t diff = nybble & 8 ? -e : e;
  int16_t sample = channel->adpcmLastSample + diff;

  if (sample >  2047) sample =  2047;
  if (sample < -2048) sample = -2048;

  channel->adpcmLastSample = sample;
  channel->adpcmStepIndex += adpcmStepShift[nybble];
  if (channel->adpcmStepIndex <  0) channel->adpcmStepIndex =  0;
  if (channel->adpcmStepIndex > 48) channel->adpcmStepIndex = 48;

  return sample;
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
  else if ((addr & 0x0f00) == 0x0200) {
    uint8_t channel = (addr >> 0x4) & 0xf;
    uint8_t reg     = (addr &  0x7);
    return this.channels[channel].regs2[reg];
  }
  else if ((addr & 0x0f00) == 0x0400) {
    uint8_t reg = (addr & 0x1f);
    return this.regs4[reg];
  }

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

  // switch (addr & 0x0f0f) {
  //   case 0x0000:
  //   case 0x0001: {
  //     uint8_t ch = (addr >> 0x4) & 0xf;
  //     uint32_t waveAddr = this.channels[ch].waveAddr | (this.channels[ch].mode.waveHi << 16);
  //     printf("channel %d wave address %06x\n", ch, waveAddr);
  //   } break;
  // }


  if ((addr & 0x0f00) == 0x0000) {
    Channel_t* channel = &this.channels[(addr >> 0x4) & 0xf];
    uint8_t reg = (addr &  0xf);
    switch (reg) {
    case 0x00:
    case 0x01:
      channel->sampleOffset = 0;
      channel->accum = 0.0f;
    }
    channel->regs0[reg] = data;
  }
  else if ((addr & 0x0f00) == 0x0200) {
    Channel_t* channel = &this.channels[(addr >> 0x4) & 0xf];
    uint8_t reg = (addr &  0x7);
    switch (reg) {
      case 0x00:   // Phase high
      case 0x04: { // Phase
        channel->regs2[reg] = data;
        float phase = (float)(channel->phase | ((channel->phaseHigh & 3) << 16));
        channel->rate = (phase * 281250.0f) / 524288.0f;
        channel->accum = 0.0f;
        // printf("channel %d rate set to %f\n", (addr >> 0x4) & 0xf, channel->rate);
      } break;

    }

    channel->regs2[reg] = data;
  }
  else if ((addr & 0x0f00) == 0x0400) {
    uint8_t reg = (addr & 0x1f);
    switch (reg) {
    // case 0x00: SPU_EnableChannels(data); break;
    case 0x05:
      if (data & 0xc000)
        this.irq = false;
      this.regs4[reg] &= ~0xc000;
      this.regs4[reg] |= (~data & 0xc000);
      return;
    }

    this.regs4[reg] = data;
  }
}


void SPU_EnableChannels(uint16_t data) {
  uint16_t changed = this.regs4[0x00] ^ data;

  if (!changed) return;

  for (int32_t i = 0; i < 16; i++) {
    uint16_t mask = 1 << i;
    if (!(changed & mask))
      continue;

    // Do something
  }
}


uint16_t SPU_GetIRQ() {
  return this.irq;
}


uint16_t SPU_GetChannelIRQ() {
  return false;
}
