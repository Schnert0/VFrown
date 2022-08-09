#include "spu.h"

static SPU_t this;

static const char* registerNames0[] = {
  "wave address",          // 0x0
  "audio mode",            // 0x1
  "loop address",          // 0x2
  "pan volume",            // 0x3
  "envelope 0",            // 0x4
  "envelope data",         // 0x5
  "envelope 1",            // 0x6
  "envelope high address", // 0x7
  "envelope address",      // 0x8
  "prev wave data",        // 0x9
  "envelope loop ctrl",    // 0xA
  "wave data",             // 0xB
  "adpcm select",          // 0xC
  "unknown 13",            // 0xD
  "unknown 14",            // 0xE
  "unknown 15",            // 0xF
};

static const char* registerNames2[] = {
  "phase high",             // 0x0
  "phase accumulator high", // 0x1
  "target phase high",      // 0x2
  "ramp down clock",        // 0x3
  "phase",                  // 0x4
  "phase accumulator",      // 0x5
  "target phase",           // 0x6
  "phase ctrl",             // 0x7
};

static const char* registerNames4[] = {
  "enable",                // 0x00
  "volume",                // 0x01
  "FIQ enable",            // 0x02
  "FIQ stat",              // 0x03
  "beat base count",       // 0x04
  "beat count",            // 0x05
  "envelope clock 0",      // 0x06
  "envelope clock 0 high", // 0x07
  "envelope clock 1",      // 0x08
  "envelope clock 1 high", // 0x09
  "envelope ramp down",    // 0x0a
  "stop",                  // 0x0b
  "zero cross",            // 0x0c
  "ctrl",                  // 0x0d
  "compression ctrl",      // 0x0e
  "stat",                   // 0x0f
  "L wave in",             // 0x10
  "R wave in",             // 0x11
  "L wave out",            // 0x12
  "R wave out",            // 0x13
  "channel repeat",        // 0x14
  "channel env mode",      // 0x15
  "channel tone release",  // 0x16
  "channel env irq",       // 0x17
  "channel pitch bend",    // 0x18
  "channel soft phase",    // 0x19
  "attack release",        // 0x1a
  "eq cutoff 10",          // 0x1b
  "eq cutoff 32",          // 0x1c
  "eq gain 10",            // 0x1d
  "eq gain 32",            // 0x1e
  "unknown 0x1f"           // 0x1f
};

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

  for (int32_t i = 0; i < 16; i++) {
    this.channels[i].timer = Timer_Init(0, (TimerFunc_t)SPU_TriggerChannelIRQ, i);
  }

  this.beatTimer = Timer_Init(SYSCLOCK / 281250, (TimerFunc_t)SPU_TriggerBeatIRQ, 0);
  Timer_Reset(this.beatTimer);

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


void SPU_Tick(int32_t cycles) {
  if (this.channelIrq) {
    CPU_TriggerFIQ(FIQSRC_SPU);
    this.channelIrq = false;
  }

  Timer_Tick(this.beatTimer, cycles);
  for (int32_t i = 0; i < 16; i++) {
    Timer_Tick(this.channels[i].timer, cycles);
  }

  if (this.sampleTimer > 0) {
    this.sampleTimer -= cycles;
    return;
  }
  this.sampleTimer += SPU_SAMPLE_TIMER;

  int32_t leftSample = 0;
  int32_t rightSample = 0;
  int32_t numEnabled = 0;

  this.prevSample = this.sample;

  for (int32_t i = 0; i < 16; i++) {
    if ((this.regs4[0x00] & (1 << i)) && !(this.regs4[0x0b] & (1 << i))) { // Channel is enabled
      numEnabled++;

      this.sample = SPU_TickChannel(i);

      leftSample  += this.sample;
      rightSample += this.sample;

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
  while (channel->accum >= 35156.25 / (25.0f / 16.0f)) {
    channel->accum -= 35156.25 / (25.0f / 16.0f);
    sampleTicks++;
  }

  if (sampleTicks < 1)
    return channel->sample;

  uint32_t waveAddr = channel->waveAddr | (channel->mode.waveHi << 16);

  for (int32_t i = 0; i < sampleTicks; i++) {
    if (channel->isPlaying) {
      switch (channel->mode.pcmMode) {
      case 0: // 8-bit PCM mode
        channel->sample = (int16_t)(Bus_Load(waveAddr + channel->sampleOffset) >> channel->pcmShift) >> 3;
        channel->pcmShift += 8;
        if (channel->pcmShift > 8) {
          channel->pcmShift = 0;
          channel->sampleOffset++;
        }
        break;

      case 1: // 16-bit PCM mode
        channel->sample = (int16_t)Bus_Load(waveAddr + channel->sampleOffset) >> 4;
        channel->sampleOffset++;
        break;

      case 2:   // ADPCM mode
      case 3: { // ???
        uint16_t nybble = (Bus_Load(waveAddr + channel->sampleOffset) >> channel->pcmShift) & 0xf;
        channel->sample = (int16_t)SPU_GetADPCMSample(ch, nybble);
        channel->pcmShift += 4;
        if (channel->pcmShift >= 16) {
          channel->pcmShift = 0;
          channel->sampleOffset++;
        }
      } break;

      }
    }
  }

  if(Bus_Load(waveAddr+channel->sampleOffset+1) == 0xffff) {
    if (channel->mode.playMode == 1) { // One shot mode
      SPU_StopChannel(ch);
    } else {
      channel->sampleOffset = channel->loopAddr | (channel->mode.loopHi << 16);
      channel->pcmShift = 0;
      channel->adpcmLastSample = 0;
      channel->adpcmStepIndex = 0;
    }
  }

  return channel->sample;
}


int16_t SPU_GetADPCMSample(uint8_t ch, uint8_t nybble) {
  Channel_t* channel = &this.channels[ch];

  int16_t step = adpcmStep[channel->adpcmStepIndex];
  int16_t e = (step >> 3);
  if (nybble & 1) e += (step >> 2);
  if (nybble & 2) e += (step >> 1);
  if (nybble & 4) e += step;
  int16_t offset = (nybble & 8) ? -e : e;
  int16_t sample = channel->adpcmLastSample + offset;

  if (sample >  2047) sample =  2047;
  if (sample < -2048) sample = -2048;

  channel->adpcmLastSample = sample;
  channel->adpcmStepIndex += adpcmStepShift[nybble];
  if (channel->adpcmStepIndex <  0) channel->adpcmStepIndex =  0;
  if (channel->adpcmStepIndex > 48) channel->adpcmStepIndex = 48;

  return sample;
}


void SPU_TriggerChannelIRQ(uint8_t ch) {
  // printf("channel IRQ triggered for channel %d\n", ch);

  if (!(this.regs4[0x02] & (1 << ch))) { // FIQ Stat
    this.regs4[0x03] |= (1 << ch);
    this.channelIrq = true;
    CPU_ActivatePendingIRQs();
  }

  Timer_Adjust(this.channels[ch].timer, this.channels[ch].rate);
}


uint16_t SPU_Read(uint16_t addr) {
  switch (addr & 0x0f00) {
  case 0x0000: {
    uint8_t channel = (addr >> 0x4) & 0xf;
    uint8_t reg     = (addr &  0xf);
    printf("read from channel %d %s (%04x) at %06x\n", channel, registerNames0[reg], addr, CPU_GetCSPC());
  } break;

  case 0x0200: {
    uint8_t channel = (addr >> 0x4) & 0xf;
    uint8_t reg     = (addr &  0x7);
    printf("read from channel %d %s (%04x) at %06x\n", channel, registerNames2[reg], addr, CPU_GetCSPC());
  } break;

  case 0x0400: {
    uint8_t reg = (addr & 0x1f);
    printf("read from channel %s (%04x) at %06x\n", registerNames4[reg], addr, CPU_GetCSPC());
  } break;

  default:
    printf("read from unknown register %04x at %06x\n", addr, CPU_GetCSPC());
  }

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
      channel->sampleOffset = 0;
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
        float phase = (float)(channel->phase | ((channel->phaseHigh & 3) << 16));
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
    default: this.regs4[reg] = data;     return;
    }

    return;
  }

}


void SPU_EnableChannels(uint16_t data) {
  if ((this.regs4[0x00] ^ data) != 0x0000) {

    for (int32_t i = 0; i < 16; i++) {
      uint16_t mask = 1 << i;

      if (data & mask) {
        SPU_StartChannel(i);
      } else {
        SPU_StopChannel(i);
      }

    }

  }

  this.regs4[0x00] = data;
}


void SPU_StartChannel(uint8_t ch) {
  Channel_t* channel = &this.channels[ch];

  this.regs4[0x00] |= (1 << ch); // Channel Enable

  if (this.regs4[0x02] & (1 << ch)) { // Channel FIQ Status
    Timer_Adjust(channel->timer, SYSCLOCK / channel->rate);
  } else {
    Timer_Stop(channel->timer);
  }

  if (!(this.regs4[0x05] & (1 << ch))) { // Channel Stop
    this.regs4[0x0f] |= (1 << ch); // Channel Status
    // printf("starting channel %d...\n", ch);
  }

  channel->isPlaying = true;

}


void SPU_StopChannel(uint8_t ch) {
  // if (this.regs4[0x00] & (1 << ch))
  //   printf("stopping channel %d...\n", ch);

  Channel_t* channel = &this.channels[ch];

  this.regs4[0x00] &= ~(1 << ch); // Channel Enable
  this.regs4[0x0a] &= ~(1 << ch); // Ramp Down
  this.regs4[0x0b] |=  (1 << ch); // Channel Stop
  this.regs4[0x0f] &= ~(1 << ch); // Channel Stat
  this.regs4[0x15] &= ~(1 << ch); // Channel Env Mode
  this.regs4[0x16] &= ~(1 << ch); // Tone Release

  channel->sampleOffset = 0;
  channel->pcmShift = 0;
  channel->adpcmLastSample = 0;
  channel->adpcmStepIndex = 0;

  Timer_Stop(channel->timer);

  channel->isPlaying = false;
}


void SPU_WriteBeatCount(uint16_t data) {
  // printf("write to beat count with %04x\n", data);

  this.regs4[0x5] &= ~(data & 0x4000);
  this.regs4[0x5] &= 0x4000;
  this.regs4[0x5] |= (data & ~0x4000);

  if ((this.regs4[0x5] & 0xc000) == 0xc000) {
    this.irq = true;
    CPU_ActivatePendingIRQs();
  } else {
    this.irq = false;
  }

}


void SPU_TriggerBeatIRQ(uint8_t index) {
  uint16_t beatCount = this.regs4[0x05];

  uint16_t beatsLeft = beatCount & 0x3fff;
  if (beatsLeft > 0)
    beatsLeft--;

  if (beatsLeft == 0) {
    beatsLeft = SPU_Read(0x3404); // Beat Base Count
    beatCount |= 0x4000;

    if ((beatCount & 0xc000) == 0xc000) {
      this.irq = true;
      CPU_ActivatePendingIRQs();
    }
  }

  this.regs4[0x05] = beatCount;

  Timer_Reset(this.beatTimer);
}


uint16_t SPU_GetIRQ() {
  return this.irq;
}


uint16_t SPU_GetChannelIRQ() {
  return this.channelIrq;
}
