//===-- BLUCPUTargetInfo.cpp - BLUCPU Target Implementation ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/BLUCPUTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
namespace llvm {
Target &getTheBLUCPUTarget() {
  static Target TheBLUCPUTarget;
  return TheBLUCPUTarget;
}
} // namespace llvm

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeBLUCPUTargetInfo() {
  llvm::RegisterTarget<llvm::Triple::blucpu> X(llvm::getTheBLUCPUTarget(), "blucpu",
                                            "BLUCPU Target", "BLUCPU");
}
