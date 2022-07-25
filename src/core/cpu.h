#ifndef CPU_H
#define CPU_H

#include "../common.h"
#include "vsmile.h"

typedef void (*Func_t)();

typedef union {
  uint16_t raw;
  struct {
    uint16_t cs : 6;
    uint16_t c  : 1;
    uint16_t s  : 1;
    uint16_t n  : 1;
    uint16_t z  : 1;
    uint16_t ds : 6;
  };
} SR_t;


typedef union {
  uint16_t raw;
  uint8_t  imm : 6;
  struct {
    uint16_t opB : 3;
    uint16_t opN : 3;
    uint16_t op1 : 3;
    uint16_t opA : 3;
    uint16_t op0 : 4;
  };
} Ins_t;


/*
 * Emulates the Sunplus μnSP CPU
 */
typedef struct CPU_t {
  // CPU registers
  union {
    uint16_t r[8];
    struct {
      uint16_t sp;             // Stack pointer
      uint16_t r1, r2, r3, r4; // General purpose registers
      uint16_t bp;             // Base pointer
      SR_t     sr;             // Status register
      uint16_t pc;             // Program counter
    };
  };

  Ins_t    ins;         // CPU instruction
  uint16_t opr1, opr2;  // Operands for ALU instruction
  uint32_t result;      // Result of ALU instruction
  uint32_t aluAddr;     // Where to store result of ALU instruction if necessary
  uint8_t  sb;          // Holds bits that get shifted out of shift instruction
  uint8_t  sbBanked[3]; // Banked shifted bits

  bool irqEnabled, fiqEnabled; // Are interrupts enabled?
  bool irqActive, fiqActive;   // Are interrupts active?
  bool irqPending;             // Is there a potential interrupt to handle?

  int32_t cycles; // cycles taken to run current instruction

} CPU_t;

bool CPU_Init();
void CPU_Cleanup();

void CPU_PrintCPUState();

void CPU_Reset();
int32_t CPU_Tick();

// Standard operations
void CPU_OpADD();
void CPU_OpADC();
void CPU_OpSUB();
void CPU_OpSBC();
void CPU_OpCMP();
void CPU_OpNEG();
void CPU_OpXOR();
void CPU_OpLD();
void CPU_OpOR();
void CPU_OpAND();
void CPU_OpTST();
void CPU_OpSTR();
void CPU_OpSPC();

// Special operations
void CPU_OpMULU();
void CPU_OpMULS();
void CPU_OpCALL();
void CPU_OpJMPF();
void CPU_OpMISC();

// Branch operations
void CPU_OpJCC();
void CPU_OpJCS();
void CPU_OpJSC();
void CPU_OpJSS();
void CPU_OpJNE();
void CPU_OpJE();
void CPU_OpJPL();
void CPU_OpJMI();
void CPU_OpJBE();
void CPU_OpJA();
void CPU_OpJLE();
void CPU_OpJG();
void CPU_OpJVC();
void CPU_OpJVS();
void CPU_OpJMP();

// Push/pop operations
void CPU_OpPOP();
void CPU_OpPSH();

// Bad instruction
void CPU_OpBAD();

// Addressing modes
void CPU_AddrBPImm6();
void CPU_AddrImm6();
void CPU_AddrUnknown();
void CPU_AddrIndirect();
void CPU_AddrExtended();
void CPU_AddrShift();
void CPU_AddrRotate();
void CPU_AddrImm6Addr();

// Extended addressing modes
void CPU_AddrExRb();
void CPU_AddrExImm16();
void CPU_AddrExImm16Addr();
void CPU_AddrExImm16AddrReverse();
void CPU_AddrExASR();

// Misc. register operations
uint32_t CPU_GetCSPC();
void CPU_SetCSPC(uint32_t newPC);
void CPU_IncCSPC(int8_t offset);
uint32_t CPU_GetDS(uint8_t index);
void CPU_SetDS(uint8_t index, uint32_t data);
void CPU_IncDS(uint8_t index, int16_t offset);
uint32_t CPU_GetDataSegment();
void CPU_SetDataSegment(uint32_t data);
uint16_t CPU_FetchNext();
void CPU_Branch();
void CPU_Push(uint16_t data, uint8_t index);
uint16_t CPU_Pop(uint8_t index);

// IRQ handling
void CPU_ActivatePendingIRQs();
void CPU_TestIRQ();
void CPU_DoIRQ(uint8_t irqNum);


#endif // CPU_H
