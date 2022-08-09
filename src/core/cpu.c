#include "cpu.h"

static CPU_t this;

// Register names
static const char* regNames[] = {
  "sp",
  "r1", "r2", "r3", "r4",
  "bp", "sr", "pc"
};


// ALU operation look-up table
static Func_t CPU_OperationFunctions[4][16] = {
  { // Standard ALU operations
    CPU_OpADD, CPU_OpADC, CPU_OpSUB, CPU_OpSBC,
    CPU_OpCMP, CPU_OpBAD, CPU_OpNEG, CPU_OpBAD,
    CPU_OpXOR, CPU_OpLD,  CPU_OpOR,  CPU_OpAND,
    CPU_OpTST, CPU_OpSTR, CPU_OpBAD, CPU_OpSPC
  },
  { // Branch operations
    CPU_OpJCC, CPU_OpJCS, CPU_OpJSC, CPU_OpJSS,
    CPU_OpJNE, CPU_OpJE,  CPU_OpJPL, CPU_OpJMI,
    CPU_OpJBE, CPU_OpJA,  CPU_OpJLE, CPU_OpJG,
    CPU_OpJVC, CPU_OpJVS, CPU_OpJMP, CPU_OpSPC,
  },
  { // Push and pop operations
    CPU_OpBAD, CPU_OpBAD, CPU_OpBAD, CPU_OpBAD,
    CPU_OpBAD, CPU_OpBAD, CPU_OpBAD, CPU_OpBAD,
    CPU_OpBAD, CPU_OpPOP, CPU_OpBAD, CPU_OpBAD,
    CPU_OpBAD, CPU_OpPSH, CPU_OpBAD, CPU_OpSPC
  },
  { // Standard ALU operations
    CPU_OpADD, CPU_OpADC, CPU_OpSUB, CPU_OpSBC,
    CPU_OpCMP, CPU_OpBAD, CPU_OpNEG, CPU_OpBAD,
    CPU_OpXOR, CPU_OpLD,  CPU_OpOR,  CPU_OpAND,
    CPU_OpTST, CPU_OpSTR, CPU_OpBAD, CPU_OpSPC
  }
};


// Special operation look-up table
static Func_t SpecialFunctions[] = {
  CPU_OpMULU, CPU_OpCALL, CPU_OpJMPF, CPU_OpBAD,
  CPU_OpMULS, CPU_OpMISC, CPU_OpBAD,  CPU_OpBAD
};


// Addressing mode look-up table
static Func_t CPU_OperandFunctions[] = {
  CPU_AddrBPImm6,   CPU_AddrImm6,  CPU_AddrUnknown,  CPU_AddrIndirect,
  CPU_AddrExtended, CPU_AddrShift, CPU_AddrRotate,   CPU_AddrImm6Addr
};


// Extended addressing mode look-up table
static Func_t CPU_OperandFunctionsEx[] = {
  CPU_AddrExRb,  CPU_AddrExImm16, CPU_AddrExImm16Addr, CPU_AddrExImm16AddrReverse,
  CPU_AddrExASR, CPU_AddrExASR,   CPU_AddrExASR,       CPU_AddrExASR
};


// Initialize CPU state
bool CPU_Init() {
  memset(&this, 0, sizeof(CPU_t));
  return true;
}


// Cleanup CPU state
void CPU_Cleanup() {

}


// Reset the CPU
void CPU_Reset() {
  memset(&this, 0, sizeof(CPU_t));

  this.fiqSource = FIQSRC_NONE;

  this.pc = Bus_Load(0xfff7);
}

 // bool memDebug[BUS_SIZE];

// Run one instruction and return the number of cycles it took to complete
int32_t CPU_Tick() {
  this.cycles = 0;

  this.ins.raw = CPU_FetchNext();

  // Encode instruction type as an index into the operation array
  // Regular  = 0b00 and 0b11
  // Branch   = 0b01 (opA = 7 && op1 < 2)
  // Push/Pop = 0b10 (op1 = 2)
  uint8_t type = ((this.ins.op1 == 2) << 1) | (this.ins.opA == 7 && this.ins.op1 < 2);
  CPU_OperationFunctions[type][this.ins.op0]();

  if (this.irqPending)
    CPU_TestIRQ();

  // Safety net to prevent infinite loop condition
  if (this.cycles < 1) {
    VSmile_Warning("invalid cycle count %d", this.cycles);
    this.cycles = 1;
  }

  // uint32_t cspc = CPU_GetCSPC();
  // if (!memDebug[cspc]) {
    // printf("%06x\n", cspc);
  //   memDebug[cspc] = true;
  // }

  return this.cycles;
}


/* Standard operations */


// Add two operands together
void CPU_OpADD() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = this.opr1 + this.opr2;

  // Update NZSC
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
    this.sr.s = ((this.result >> 16) & 1) != (((this.opr1 ^ this.opr2) >> 15) & 1);
    this.sr.c = (this.result > 0xffff);

  }

  // [imm16] = ...
  if (this.ins.op1 == 0x4 && this.ins.opN == 0x3) {
   Bus_Store(this.aluAddr, this.result & 0xffff);
   return;
  }

  // Ra = ...
  this.r[this.ins.opA] = this.result & 0xffff;
}


// Add two operands together with carry flag
void CPU_OpADC() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = this.opr1 + this.opr2 + this.sr.c;

  // Update NZSC
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
    this.sr.s = ((this.result >> 16) & 1) != (((this.opr1 ^ this.opr2) >> 15) & 1);
    this.sr.c = (this.result > 0xffff);
  }

  // [imm16] = ...
  if (this.ins.op1 == 0x4 && this.ins.opN == 0x3) {
   Bus_Store(this.aluAddr, this.result & 0xffff);
   return;
  }

  // Ra = ...
  this.r[this.ins.opA] = this.result & 0xffff;
}


// Subract two operands from each other
void CPU_OpSUB() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = this.opr1 + (~this.opr2 & 0xffff) + 1;

  // Update NZSC
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
    this.sr.s = ((this.result >> 16) & 1) != (((this.opr1 ^ ~this.opr2) >> 15) & 1);
    this.sr.c = (this.result > 0xffff);
  }

  // [imm16] = ...
  if (this.ins.op1 == 0x4 && this.ins.opN == 0x3) {
   Bus_Store(this.aluAddr, this.result & 0xffff);
   return;
  }

  // Ra = ...
  this.r[this.ins.opA] = this.result & 0xffff;
}


// Subtract two operands using carry flag
void CPU_OpSBC() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = this.opr1 + (~this.opr2 & 0xffff) + this.sr.c;

  // Update NZSC
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
    this.sr.s = ((this.result >> 16) & 1) != (((this.opr1 ^ ~this.opr2) >> 15) & 1);
    this.sr.c = (this.result > 0xffff);
  }

  // [imm16] = ...
  if (this.ins.op1 == 0x4 && this.ins.opN == 0x3) {
   Bus_Store(this.aluAddr, this.result & 0xffff);
   return;
  }

  // Ra = ...
  this.r[this.ins.opA] = this.result & 0xffff;
}


// Compare two operands (same as SUB, but doesn't store the result)
void CPU_OpCMP() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = this.opr1 + (~this.opr2 & 0xffff) + 1;

  // Update NZSC
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
    this.sr.s = ((this.result >> 16) & 1) != (((this.opr1 ^ ~this.opr2) >> 15) & 1);
    this.sr.c = (this.result > 0xffff);
  }
}


// Negate the second operand
void CPU_OpNEG() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = -this.opr2;

  // Update NZ
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
  }

  // [imm16] = ...
  if (this.ins.op1 == 0x4 && this.ins.opN == 0x3) {
   Bus_Store(this.aluAddr, this.result & 0xffff);
   return;
  }

  // Ra = ...
  this.r[this.ins.opA] = this.result & 0xffff;
}


// Perform bitwise exclusive OR on two operands
void CPU_OpXOR() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = this.opr1 ^ this.opr2;

  // Update NZ
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
  }

  // [imm16] = ...
  if (this.ins.op1 == 0x4 && this.ins.opN == 0x3) {
   Bus_Store(this.aluAddr, this.result & 0xffff);
   return;
  }

  // Ra = ...
  this.r[this.ins.opA] = this.result & 0xffff;
}


// Copy operand to specified location
void CPU_OpLD() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = this.opr2;

  // Update NZ
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
  }

  // [imm16] = ...
  if (this.ins.op1 == 0x4 && this.ins.opN == 0x3) {
   Bus_Store(this.aluAddr, this.result & 0xffff);
   return;
  }

  // Ra = ...
  this.r[this.ins.opA] = this.result & 0xffff;
}


// Perform bitwise OR on two operands
void CPU_OpOR() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = this.opr1 | this.opr2;

  // Update NZ
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
  }

  // [imm16] = ...
  if (this.ins.op1 == 0x4 && this.ins.opN == 0x3) {
   Bus_Store(this.aluAddr, this.result & 0xffff);
   return;
  }

  // Ra = ...
  this.r[this.ins.opA] = this.result & 0xffff;
}


// Perform bitwise AND on two operands
void CPU_OpAND() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = this.opr1 & this.opr2;

  // Update NZ
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
  }

  // [imm16] = ...
  if (this.ins.op1 == 0x4 && this.ins.opN == 0x3) {
   Bus_Store(this.aluAddr, this.result & 0xffff);
   return;
  }

  // Ra = ...
  this.r[this.ins.opA] = this.result & 0xffff;
}


// Perform bit test (same as AND, but doesn't store the result)
void CPU_OpTST() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  this.result = this.opr1 & this.opr2;

  // Update NZ
  if (this.ins.opA != 7) {
    this.sr.n = ((this.result & 0x8000) != 0);
    this.sr.z = ((this.result & 0xffff) == 0);
  }
}


// Store an operand to memory location
void CPU_OpSTR() {
  // Get operands
  this.opr1 = this.r[this.ins.opA];
  CPU_OperandFunctions[this.ins.op1]();

  // Perform operation
  Bus_Store(this.aluAddr, this.opr1);
}


// Perform special operation
void CPU_OpSPC() {
  SpecialFunctions[this.ins.op1]();
}


/* Special operations */


// Perform unsigned multiply on two operands and store result in r3 and r4
void CPU_OpMULU() {
  if (this.ins.opN == 1 && this.ins.opA == 7)
    VSmile_Error("invalid multiplication at 0x%06x (opN = %01x, opA = %01x)", CPU_GetCSPC(), this.ins.opN, this.ins.opA);

  uint16_t a = this.r[this.ins.opA];
  uint16_t b = this.r[this.ins.opB];

  uint32_t result = a * b;

  if (this.r[this.ins.opB] & 0x8000)
    result -= (this.r[this.ins.opA] << 16);

  this.r3 = result & 0xffff;
  this.r4 = result >> 16;

  this.cycles += 12;
}


// Perform signed multiply on two operands and store result in r3 and r4
void CPU_OpMULS() {
  if (this.ins.opN == 1 && this.ins.opA == 7)
    VSmile_Error("invalid multiplication at 0x%06x (opN = %01x, opA = %01x)", CPU_GetCSPC(), this.ins.opN, this.ins.opA);

  uint16_t a = this.r[this.ins.opA];
  int16_t b = this.r[this.ins.opB];

  uint32_t result = a * b;

  if (this.r[this.ins.opB] & 0x8000)
    result -= (this.r[this.ins.opA] << 16);
  if (this.r[this.ins.opA] & 0x8000)
    result -= (this.r[this.ins.opB] << 16);

  this.r3 = result & 0xffff;
  this.r4 = result >> 16;

  this.cycles += 12;
}


// Push PC and SR to the stack and perform a long jump to a specified location
void CPU_OpCALL() {
  if ((this.ins.opA & 1) != 0)
    VSmile_Error("illegal opcode 0x%04x at 0x%06x", this.ins.raw, CPU_GetCSPC());

  uint16_t lowerPC = CPU_FetchNext();
  CPU_Push(this.pc, 0);
  CPU_Push(this.sr.raw, 0);
  CPU_SetCSPC((this.ins.imm << 16) | lowerPC);

  this.cycles += 9;
}


// Perform a long jump to a specified location
void CPU_OpJMPF() {
  if (this.ins.opA != 7)
    VSmile_Error("illegal opcode 0x%04x at 0x%06x", this.ins.raw, CPU_GetCSPC());

  CPU_SetCSPC((this.ins.imm << 16) | CPU_FetchNext());

  this.cycles += 5;
}


// Miscellaneous operations
void CPU_OpMISC() {
  switch (this.ins.imm) {
  case 0x00: // INT OFF
    this.irqEnabled = false;
    this.fiqEnabled = false;
    break;

  case 0x01: // INT IRQ
    this.irqEnabled = true;
    this.fiqEnabled = false;
    break;

  case 0x03: // INT FIQ, IRQ
    this.irqEnabled = true;
    this.fiqEnabled = true;
    break;

  case 0x04: // FIR_MOV ON
    this.firMov = true;
    break;

  case 0x05: // FIR_MOV OFF
    this.firMov = false;
    break;

  case 0x0c: // FIQ OFF
    this.fiqEnabled = false;
    break;

  case 0x0e: // FIQ ON
    this.fiqEnabled = true;
    break;

  case 0x25: // NOP
    break;

  default: VSmile_Error("unimplemented special instruction at 0x%06x (op1 = 0x%01x, offset = 0x%02x)", CPU_GetCSPC(), this.ins.op1, this.ins.imm);
  }

  this.cycles += 2;
}


/* Branch operations */


// Jump if carry flag is cleared
void CPU_OpJCC() {
  this.cycles += 2;

  if (!this.sr.c)
    CPU_Branch();
}


// Jump if carry flag is set
void CPU_OpJCS() {
  this.cycles += 2;

  if (this.sr.c)
    CPU_Branch();
}


// Jump if sign flag is cleared
void CPU_OpJSC() {
  this.cycles += 2;

  if (!this.sr.s)
    CPU_Branch();
}


// Jump if sign flag is set
void CPU_OpJSS() {
  this.cycles += 2;

  if (this.sr.s)
    CPU_Branch();
}


// Jump if not equal / not zero
void CPU_OpJNE() {
  this.cycles += 2;

  if (!this.sr.z)
    CPU_Branch();
}


// Jump if equal / zero
void CPU_OpJE() {
  this.cycles += 2;

  if (this.sr.z)
    CPU_Branch();
}


// Jump if positive / negative flag is cleared
void CPU_OpJPL() {
  this.cycles += 2;

  if (!this.sr.n)
    CPU_Branch();
}


// Jump if negative / negative flag is set
void CPU_OpJMI() {
  this.cycles += 2;

  if (this.sr.n)
    CPU_Branch();
}


// Jump if not (zero flag is cleared and carry flag is set)
void CPU_OpJBE() {
  this.cycles += 2;

  if ((!this.sr.z && !this.sr.c) || this.sr.z)
    CPU_Branch();
}


// Jump if zero flag is cleared and carry flag is set
void CPU_OpJA() {
  this.cycles += 2;

  if (!this.sr.z && this.sr.c)
    CPU_Branch();
}


// Jump if less than or equal / not greater than
void CPU_OpJLE() {
  this.cycles += 2;

  if (this.sr.z || this.sr.s)
    CPU_Branch();
}


// Jump if not less than or equal / greater than
void CPU_OpJG() {
  this.cycles += 2;

  if (!(this.sr.z || this.sr.s))
    CPU_Branch();
}


// Jump if negative flag equals sign flag
void CPU_OpJVC() {
  this.cycles += 2;

  if (this.sr.n == this.sr.s)
    CPU_Branch();
}


// Jump if negative flag does not equal sign flag
void CPU_OpJVS() {
  this.cycles += 2;

  if (this.sr.n != this.sr.s)
    CPU_Branch();
}


// Unconditionally jump
void CPU_OpJMP() {
  this.cycles += 2;

  CPU_Branch();
}


/* Push / Pop operations */


// Pop a specified number of words off a specified stack (includes RETF and RETI)
void CPU_OpPOP() {
  if (this.ins.opA == 5 && this.ins.opN == 3 && this.ins.opB == 0) { // RETI
    this.sr.raw = CPU_Pop(0);
    this.pc = CPU_Pop(0);

    if (this.fiqActive) {
      this.sbBanked[2] = this.sb;
      this.sb = this.sbBanked[this.irqActive];
      this.fiqActive = false;
    }
    else if (this.irqActive) {
      this.sbBanked[1] = this.sb;
      this.sb = this.sbBanked[0];
      this.irqActive = false;
    }

    this.irqPending = true;

    this.cycles += 8;

  } else {
    uint8_t n = this.ins.opN;
    uint8_t a = this.ins.opA;

    while (n--)
      this.r[++a] = CPU_Pop(this.ins.opB);

    this.cycles += 2*this.ins.opN + 4;
  }
}


// Push a specified number of words to a specified stack
void CPU_OpPSH() {
  uint8_t n = this.ins.opN;
  uint8_t a = this.ins.opA;

  while (n--)
    CPU_Push(this.r[a--], this.ins.opB);

  this.cycles += 2*this.ins.opN + 4;
}


/* Bad operation */


void CPU_OpBAD() {
  VSmile_Warning("unknown CPU instruction %04x at %06x", this.ins.raw, this.pc);
}


/* Addressing modes */


// [BP+imm6]
void CPU_AddrBPImm6() {
  this.aluAddr = (this.bp + this.ins.imm) & 0xffff;
  if (this.ins.op0 != 0xd)
    this.opr2 = Bus_Load(this.aluAddr);

  this.cycles += 6;
}


// imm6
void CPU_AddrImm6() {
  this.opr2 = this.ins.imm;

  this.cycles += 2;
}


// Invalid addressing mode
void CPU_AddrUnknown() {
  VSmile_Error("unimplemented operand mode at 0x%06x (op1 = 0x%01x)", CPU_GetCSPC(), this.ins.op1);
}


// [DS: Rb] or [Rb]
void CPU_AddrIndirect() {
  switch (this.ins.opN) {
  case 0: // [Rb]
    this.aluAddr = this.r[this.ins.opB];
    if (this.ins.op0 != 0xd)
      this.opr2 = Bus_Load(this.aluAddr);
    break;

  case 1: // [Rb--]
    this.aluAddr = this.r[this.ins.opB];
    if (this.ins.op0 != 0xd)
      this.opr2 = Bus_Load(this.aluAddr);
    this.r[this.ins.opB]--;
    break;

  case 2: // [Rb++]
    this.aluAddr = this.r[this.ins.opB];
    if (this.ins.op0 != 0xd)
      this.opr2 = Bus_Load(this.aluAddr);
    this.r[this.ins.opB]++;
    break;

  case 3: // [++Rb]
    this.r[this.ins.opB]++;
    this.aluAddr = this.r[this.ins.opB];
    if (this.ins.op0 != 0xd)
      this.opr2 = Bus_Load(this.aluAddr);
    break;

  case 4: // [DS: Rb]
    this.aluAddr = CPU_GetDS(this.ins.opB);
    if (this.ins.op0 != 0xd)
      this.opr2 = Bus_Load(this.aluAddr);
    break;

  case 5: // [DS: Rb--]
    this.aluAddr = CPU_GetDS(this.ins.opB);
    if (this.ins.op0 != 0xd)
      this.opr2 = Bus_Load(this.aluAddr);
    CPU_IncDS(this.ins.opB, -1);
    break;

  case 6: // [DS: Rb++]
    this.aluAddr = CPU_GetDS(this.ins.opB);
    if (this.ins.op0 != 0xd)
      this.opr2 = Bus_Load(this.aluAddr);
    CPU_IncDS(this.ins.opB, 1);
    break;

  case 7: // [DS: ++Rb]
    CPU_IncDS(this.ins.opB, 1);
    this.aluAddr = CPU_GetDS(this.ins.opB);
    if (this.ins.op0 != 0xd)
      this.opr2 = Bus_Load(this.aluAddr);
    break;
  }

  this.cycles += (this.ins.opA == 7) ? 7 : 6;
}


// Get operands from extended addressing mode
void CPU_AddrExtended() {
  CPU_OperandFunctionsEx[this.ins.opN]();
}


// Rb LSR n / Rb LSL n
void CPU_AddrShift() {
  if (this.ins.opN & 4) { // LSR
      uint32_t shift = ((this.r[this.ins.opB] << 4) | this.sb) >> (this.ins.opN - 3);
      this.sb = shift & 0xf;
      this.opr2 = (shift >> 4) & 0xffff;
    } else { // LSL
      uint32_t shift = ((this.sb << 16) | this.r[this.ins.opB]) << (this.ins.opN + 1);
      this.sb = (shift >> 16) & 0xf;
      this.opr2 = shift & 0xffff;
    }

   this.cycles += (this.ins.opA == 7) ? 5 : 3;
}


// Rb ROR n / Rb ROL n
void CPU_AddrRotate() {
  uint32_t shift = (((this.sb << 16) | this.r[this.ins.opB]) << 4) | this.sb;

  if (this.ins.opN & 4) { // ROR
    shift >>= (this.ins.opN - 3);
    this.sb = shift & 0xf;
  } else { // ROL
    shift <<= (this.ins.opN + 1);
		this.sb = (shift >> 20) & 0xf;
  }

  this.opr2 = (shift >> 4) & 0xffff;

  this.cycles += (this.ins.opA == 7) ? 6 : 5;
}


// [imm6]
void CPU_AddrImm6Addr() {
  this.aluAddr = this.ins.imm;
  this.opr2 = Bus_Load(this.aluAddr);

  this.cycles += (this.ins.opA == 7) ? 6 : 5;
}


/* Extended addressing modes */


// Rb
void CPU_AddrExRb() {
  this.opr2 = this.r[this.ins.opB];

  this.cycles += (this.ins.opA == 7) ? 5 : 3;
}


// imm16
void CPU_AddrExImm16() {
  this.opr1 = this.r[this.ins.opB];
  this.opr2 = CPU_FetchNext();

  this.cycles += (this.ins.opA == 7) ? 5 : 4;
}


// [imm16]
void CPU_AddrExImm16Addr() {
  this.aluAddr = CPU_FetchNext();
  this.opr1 = this.r[this.ins.opB];
  if (this.ins.op0 != 0xd)
    this.opr2 = Bus_Load(this.aluAddr);

  this.cycles += (this.ins.opA == 7) ? 8 : 7;
}


// [imm16] = ...
void CPU_AddrExImm16AddrReverse() {
  this.aluAddr = CPU_FetchNext();
  this.opr1 = this.r[this.ins.opB];
  this.opr2 = this.r[this.ins.opA];

  this.cycles += (this.ins.opA == 7) ? 8 : 7;
}


// Rb ASR n
void CPU_AddrExASR() {
  uint32_t shift = (this.r[this.ins.opB] << 4) | this.sb;
  if (shift & 0x80000)
    shift |= 0xf00000;
  shift >>= (this.ins.opN - 3);
  this.sb = shift & 0xf;
  this.opr2 = (shift >> 4) & 0xffff;

  this.cycles += (this.ins.opA == 7) ? 5 : 3;
}


/* Misc. register operations */


// Get full code segment / program counter
uint32_t CPU_GetCSPC() {
  return (this.sr.cs << 16) | this.pc;
}


// Set full code segment / program counter
void CPU_SetCSPC(uint32_t newPC) {
  this.pc = newPC & 0xffff;
  this.sr.cs = (newPC >> 16) & 0x3f;
}


void CPU_IncCSPC(int8_t offset) {
  uint32_t cspc = CPU_GetCSPC();
  cspc += offset;
  CPU_SetCSPC(cspc);
}


uint32_t CPU_GetDS(uint8_t index) {
  return (this.sr.ds << 16) | this.r[index];
}


void CPU_SetDS(uint8_t index, uint32_t data) {
  this.sr.ds = (data >> 16) & 0x3f;
  this.r[index] = data & 0xffff;
}


void CPU_IncDS(uint8_t index, int16_t offset) {
  uint32_t ds = CPU_GetDS(index);
  ds += offset;
  CPU_SetDS(index, ds);
}


uint32_t CPU_GetDataSegment() {
  return this.sr.ds;
}


void CPU_SetDataSegment(uint32_t data) {
  this.sr.ds = data & 0x3f;
}


uint16_t CPU_FetchNext() {
  uint16_t data = Bus_Load(CPU_GetCSPC());
  CPU_IncCSPC(1);
  return data;
}


void CPU_Branch() {
  this.cycles += 2;

  if (this.ins.op1)
    CPU_IncCSPC(-this.ins.imm);
  else
    CPU_IncCSPC( this.ins.imm);
}


void CPU_Push(uint16_t data, uint8_t index) {
  Bus_Store(this.r[index], data);
  this.r[index]--;
}


uint16_t CPU_Pop(uint8_t index) {
  this.r[index]++;
  return Bus_Load(this.r[index]);
}


void CPU_ActivatePendingIRQs() {
  this.irqPending = true;
}


void CPU_TestIRQ() {
  if (!this.irqEnabled || this.irqActive)
    return;

  uint16_t maskedIRQ = Bus_Load(0x3d21) & Bus_Load(0x3d22);
  uint16_t gpuMaskedIRQ = Bus_Load(0x2862) & Bus_Load(0x2863);
  uint16_t spuIRQ = SPU_GetIRQ();
  uint16_t spuChannelIRQ = SPU_GetChannelIRQ();

  // If there's no active IRQs to handle, break out early
  // and hint that there's no more IRQs to the CPU
  if (!maskedIRQ && !gpuMaskedIRQ && !spuIRQ && !spuChannelIRQ) {
    this.irqPending = false;
    return;
  }

  if (this.fiq) {
    CPU_DoFIQ();
  }
  else if (gpuMaskedIRQ) { // Video
    CPU_DoIRQ(0);
  }
  // else if (spuChannelIRQ) { // SPU
  //   CPU_DoIRQ(1);
  // }
  else if (maskedIRQ & 0x0c00) { // Timer A, Timer B
    CPU_DoIRQ(2);
  }
  else if (maskedIRQ & 0x6100) { // UART, ADC, SPI
    CPU_DoIRQ(3);
  }
  else if (spuIRQ) { // SPU beat and envelope
    CPU_DoIRQ(4);
  }
  else if (maskedIRQ & 0x1200) { // Controller 1 and 2 RTS signals
    CPU_DoIRQ(5);
  }
  else if (maskedIRQ & 0x0070){ // 1024 Hz, 2048 Hz, 4096 Hz
    CPU_DoIRQ(6);
  }
  else if (maskedIRQ & 0x008b){ // TMB1, TMB2, 4Hz, key change
    CPU_DoIRQ(7);
  }
}


// Execute Interrupt
void CPU_DoIRQ(uint8_t irqNum) {
  if (this.irqActive || this.fiqActive || !this.irqEnabled)
    return;

  this.irqActive = true;

  this.sbBanked[0] = this.sb;
  CPU_Push(this.pc, 0);
  CPU_Push(this.sr.raw, 0);

  this.sb = this.sbBanked[1];
  this.pc = Bus_Load(0xfff8+irqNum);
  this.sr.raw = 0;
}


// Execute Fast Interrupt
void CPU_DoFIQ() {
  if (this.fiqActive || !this.fiqEnabled)
    return;

  this.fiqActive = true;
  this.fiq = false;

  this.sbBanked[this.irqActive] = this.sb;
  CPU_Push(this.pc, 0);
  CPU_Push(this.sr.raw, 0);

  this.sb = this.sbBanked[2];
  this.pc = Bus_Load(0xfff6);
  this.sr.raw = 0;
}

// Configure FIQ Source
void CPU_SetFIQSource(uint16_t fiqSource) {
  this.fiqSource = fiqSource;
}


// Trigger FIQ if the FIQ source matches the selected FIQ source
void CPU_TriggerFIQ(uint16_t fiqSource) {
  if (this.fiqSource == fiqSource)
    this.fiq = true;
}


// Print the CPU state to the console
void CPU_PrintCPUState() {
  printf("ins:\t0x%04x\n\n", this.ins.raw);
  printf("Op0:\t%01x\n", this.ins.op0);
  printf("OpA:\t%01x\n", this.ins.opA);
  printf("Op1:\t%01x\n", this.ins.op1);
  printf("OpN:\t%01x\n", this.ins.opN);
  printf("OpB:\t%01x\n", this.ins.opB);
  printf("Offset:\t%d\n\n", this.ins.imm);
  printf("SR.ds\t%02x\n", this.sr.ds);
  printf("SR.z:\t%d\n", this.sr.z);
  printf("SR.n:\t%d\n", this.sr.n);
  printf("SR.s:\t%d\n", this.sr.s);
  printf("SR.c:\t%d\n", this.sr.c);
  printf("SR.cs:\t%02x\n\n", this.sr.cs);
  for (int32_t i = 0; i < 8; i++)
    printf("%s:\t0x%04x\n", regNames[i], this.r[i]);
  printf("CSPC:\t0x%06x\n", CPU_GetCSPC());

  // getchar();
}
