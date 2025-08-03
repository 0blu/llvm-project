//===-- BLUCPUFrameLowering.h - Define frame lowering for BLUCPU ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BLUCPU_FRAME_LOWERING_H
#define LLVM_BLUCPU_FRAME_LOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

/// Utilities for creating function call frames.
class BLUCPUFrameLowering : public TargetFrameLowering {
public:
  explicit BLUCPUFrameLowering();
  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  bool hasFP(MachineFunction const &MF) const override;
};

} // end namespace llvm

#endif // LLVM_BLUCPU_FRAME_LOWERING_H
