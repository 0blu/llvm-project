//===-- BLUCPU.td - Describe the BLUCPU Target Machine ----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//
// This is the top level entry point for the BLUCPU target.
//===---------------------------------------------------------------------===//

//===---------------------------------------------------------------------===//
// Target-independent interfaces which we are implementing
//===---------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===---------------------------------------------------------------------===//
// BLUCPU Device Definitions
//===---------------------------------------------------------------------===//

include "BLUCPUDevices.td"

//===---------------------------------------------------------------------===//
// Register File Description
//===---------------------------------------------------------------------===//

include "BLUCPURegisterInfo.td"

//===---------------------------------------------------------------------===//
// Instruction Descriptions
//===---------------------------------------------------------------------===//

include "BLUCPUInstrInfo.td"

def BLUCPUInstrInfo : InstrInfo;

//===---------------------------------------------------------------------===//
// Calling Conventions
//===---------------------------------------------------------------------===//

include "BLUCPUCallingConv.td"

//===---------------------------------------------------------------------===//
// Assembly Printers
//===---------------------------------------------------------------------===//

def BLUCPUAsmWriter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}

//===---------------------------------------------------------------------===//
// Assembly Parsers
//===---------------------------------------------------------------------===//

def BLUCPUAsmParser : AsmParser {
  let ShouldEmitMatchRegisterName = 1;
  let ShouldEmitMatchRegisterAltName = 1;
}

def BLUCPUAsmParserVariant : AsmParserVariant {
  int Variant = 0;

  // Recognize hard coded registers.
  string RegisterPrefix = "$";
  string TokenizingCharacters = "+";
}

//===---------------------------------------------------------------------===//
// Target Declaration
//===---------------------------------------------------------------------===//

def BLUCPU : Target {
  let InstructionSet = BLUCPUInstrInfo;
  let AssemblyWriters = [BLUCPUAsmWriter];

  let AssemblyParsers = [BLUCPUAsmParser];
  let AssemblyParserVariants = [BLUCPUAsmParserVariant];
}
