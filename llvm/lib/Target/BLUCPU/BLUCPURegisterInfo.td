//===-- BLUCPURegisterInfo.td - BLUCPU Register defs -------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the BLUCPU register file
//===----------------------------------------------------------------------===//

// 8-bit General purpose register definition.
class BLUCPUReg<bits<16> num, string name, list<Register> subregs = [],
             list<string> altNames = []> : RegisterWithSubRegs<name, subregs> {
  field bits<16> Num = num;

  let HWEncoding = num;
  let Namespace = "BLUCPU";
  let SubRegs = subregs;
  let AltNames = altNames;
}

let Namespace = "BLUCPU" in { def ptr : RegAltNameIndex; }

//===----------------------------------------------------------------------===//
//  8-bit general purpose registers
//===----------------------------------------------------------------------===//

def R0 : BLUCPUReg<0, "r0">, DwarfRegNum<[0]>;
def R1 : BLUCPUReg<1, "r1">, DwarfRegNum<[1]>;
def R2 : BLUCPUReg<2, "r2">, DwarfRegNum<[2]>;
def R3 : BLUCPUReg<3, "r3">, DwarfRegNum<[3]>;
def R4 : BLUCPUReg<4, "r4">, DwarfRegNum<[4]>;
def R5 : BLUCPUReg<5, "r5">, DwarfRegNum<[5]>;
def R6 : BLUCPUReg<6, "r6">, DwarfRegNum<[6]>;
def R7 : BLUCPUReg<7, "r7">, DwarfRegNum<[7]>;
def R8 : BLUCPUReg<8, "r8">, DwarfRegNum<[8]>;
def R9 : BLUCPUReg<9, "r9">, DwarfRegNum<[9]>;
def SP : BLUCPUReg<10, "sp">, DwarfRegNum<[32]>;
def BP : BLUCPUReg<11, "bp">, DwarfRegNum<[33]>;

//===----------------------------------------------------------------------===//
// Register Classes
//===----------------------------------------------------------------------===//

// Main general purpose 32-bit register class.
def GPR8 : RegisterClass<"BLUCPU", [i32], 8, (add R9, R8, R7, R6, R5)>;

// Main 32-bit register class.
def DREGS : RegisterClass<"BLUCPU", [i32], 8, (add R9, R8, R7, R6, R5, R4, R3, R2, R1, R0)>;

// Register class used for the stack read pseudo instruction.
def GPRSP : RegisterClass<"BLUCPU", [i32], 8, (add SP)>;
