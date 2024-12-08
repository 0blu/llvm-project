//===-- BLUCPUTargetStreamer.h - BLUCPU Target Streamer --------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BLUCPU_TARGET_STREAMER_H
#define LLVM_BLUCPU_TARGET_STREAMER_H

#include "llvm/MC/MCELFStreamer.h"

namespace llvm {
class MCStreamer;

/// A generic BLUCPU target output stream.
class BLUCPUTargetStreamer : public MCTargetStreamer {
public:
  explicit BLUCPUTargetStreamer(MCStreamer &S);
};

/// A target streamer for textual BLUCPU assembly code.
class BLUCPUTargetAsmStreamer : public BLUCPUTargetStreamer {
public:
  explicit BLUCPUTargetAsmStreamer(MCStreamer &S);
};

} // end namespace llvm

#endif // LLVM_BLUCPU_TARGET_STREAMER_H
