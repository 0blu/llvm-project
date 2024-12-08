//===--------- BLUCPUMCELFStreamer.h - BLUCPU subclass of MCELFStreamer ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_BLUCPU_MCTARGETDESC_BLUCPUMCELFSTREAMER_H
#define LLVM_LIB_TARGET_BLUCPU_MCTARGETDESC_BLUCPUMCELFSTREAMER_H

#include "MCTargetDesc/BLUCPUMCExpr.h"
#include "MCTargetDesc/BLUCPUMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectWriter.h"

namespace llvm {

const int SIZE_LONG = 4;
const int SIZE_WORD = 2;

class BLUCPUMCELFStreamer : public MCELFStreamer {
  std::unique_ptr<MCInstrInfo> MCII;

public:
  BLUCPUMCELFStreamer(MCContext &Context, std::unique_ptr<MCAsmBackend> TAB,
                   std::unique_ptr<MCObjectWriter> OW,
                   std::unique_ptr<MCCodeEmitter> Emitter)
      : MCELFStreamer(Context, std::move(TAB), std::move(OW),
                      std::move(Emitter)),
        MCII(createBLUCPUMCInstrInfo()) {}

  BLUCPUMCELFStreamer(MCContext &Context, std::unique_ptr<MCAsmBackend> TAB,
                   std::unique_ptr<MCObjectWriter> OW,
                   std::unique_ptr<MCCodeEmitter> Emitter,
                   MCAssembler *Assembler)
      : MCELFStreamer(Context, std::move(TAB), std::move(OW),
                      std::move(Emitter)),
        MCII(createBLUCPUMCInstrInfo()) {}

  void emitValueForModiferKind(
      const MCSymbol *Sym, unsigned SizeInBytes, SMLoc Loc = SMLoc(),
      BLUCPUMCExpr::VariantKind ModifierKind = BLUCPUMCExpr::VK_BLUCPU_None);
};

MCStreamer *createBLUCPUELFStreamer(Triple const &TT, MCContext &Context,
                                 std::unique_ptr<MCAsmBackend> MAB,
                                 std::unique_ptr<MCObjectWriter> OW,
                                 std::unique_ptr<MCCodeEmitter> CE);

} // end namespace llvm

#endif // LLVM_LIB_TARGET_BLUCPU_MCTARGETDESC_BLUCPUMCELFSTREAMER_H
