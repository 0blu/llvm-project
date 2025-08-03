//===-- BLUCPUMCInstLower.cpp - Convert BLUCPU MachineInstr to an MCInst --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower BLUCPU MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "BLUCPUMCInstLower.h"
#include "BLUCPUInstrInfo.h"
#include "MCTargetDesc/BLUCPUMCExpr.h"

#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

void BLUCPUMCInstLower::lowerInstruction(const MachineInstr &MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI.getOpcode());
}

} // end of namespace llvm
