//===-- BLUCPUMCAsmInfo.cpp - BLUCPU asm properties -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the BLUCPUMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "BLUCPUMCAsmInfo.h"

#include "llvm/TargetParser/Triple.h"

namespace llvm {

BLUCPUMCAsmInfo::BLUCPUMCAsmInfo(const Triple &TT, const MCTargetOptions &Options) {
  CodePointerSize = 2;
  CalleeSaveStackSlotSize = 2;
  CommentString = ";";
  SeparatorString = "$";
  PrivateGlobalPrefix = ".L";
  PrivateLabelPrefix = ".L";
  UsesELFSectionDirectiveForBSS = true;
  SupportsDebugInformation = true;
}

} // end of namespace llvm
