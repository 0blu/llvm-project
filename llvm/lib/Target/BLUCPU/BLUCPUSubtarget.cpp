//===-- BLUCPUSubtarget.cpp - BLUCPU Subtarget Information ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the BLUCPU specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "BLUCPUSubtarget.h"

#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/TargetRegistry.h"

#include "BLUCPU.h"
#include "BLUCPUTargetMachine.h"
#include "MCTargetDesc/BLUCPUMCTargetDesc.h"

#define DEBUG_TYPE "blucpu-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "BLUCPUGenSubtargetInfo.inc"

namespace llvm {

BLUCPUSubtarget::BLUCPUSubtarget(const Triple &TT, const std::string &CPU,
                           const std::string &FS, const BLUCPUTargetMachine &TM)
    : BLUCPUGenSubtargetInfo(TT, CPU, /*TuneCPU*/ CPU, FS), InstrInfo(*this),
      TLInfo(TM, initializeSubtargetDependencies(CPU, FS, TM)) {
  // Parse features string.
  ParseSubtargetFeatures(CPU, /*TuneCPU*/ CPU, FS);
}

BLUCPUSubtarget &
BLUCPUSubtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS,
                                              const TargetMachine &TM) {
  // Parse features string.
  ParseSubtargetFeatures(CPU, /*TuneCPU*/ CPU, FS);
  return *this;
}

} // end of namespace llvm
