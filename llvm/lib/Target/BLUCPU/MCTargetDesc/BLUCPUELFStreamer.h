//===----- BLUCPUELFStreamer.h - BLUCPU Target Streamer --------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BLUCPU_ELF_STREAMER_H
#define LLVM_BLUCPU_ELF_STREAMER_H

#include "BLUCPUTargetStreamer.h"

namespace llvm {

/// A target streamer for an BLUCPU ELF object file.
class BLUCPUELFStreamer : public BLUCPUTargetStreamer {
public:
  BLUCPUELFStreamer(MCStreamer &S, const MCSubtargetInfo &STI);

  MCELFStreamer &getStreamer() {
    return static_cast<MCELFStreamer &>(Streamer);
  }
};

} // end namespace llvm

#endif
