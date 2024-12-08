//===-- BLUCPUMCAsmInfo.h - BLUCPU asm properties ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the BLUCPUMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BLUCPU_ASM_INFO_H
#define LLVM_BLUCPU_ASM_INFO_H

#include "llvm/MC/MCAsmInfo.h"

namespace llvm {

class Triple;

/// Specifies the format of BLUCPU assembly files.
class BLUCPUMCAsmInfo : public MCAsmInfo {
public:
  explicit BLUCPUMCAsmInfo(const Triple &TT, const MCTargetOptions &Options);
};

} // end namespace llvm

#endif // LLVM_BLUCPU_ASM_INFO_H
