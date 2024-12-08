//===-- BLUCPUELFObjectWriter.cpp - BLUCPU ELF Writer ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/BLUCPUFixupKinds.h"
#include "MCTargetDesc/BLUCPUMCExpr.h"
#include "MCTargetDesc/BLUCPUMCTargetDesc.h"

#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

/// Writes BLUCPU machine code into an ELF32 object file.
class BLUCPUELFObjectWriter : public MCELFObjectTargetWriter {
public:
  BLUCPUELFObjectWriter(uint8_t OSABI);

  virtual ~BLUCPUELFObjectWriter() = default;

  unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                        const MCFixup &Fixup, bool IsPCRel) const override;
};

BLUCPUELFObjectWriter::BLUCPUELFObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(false, OSABI, ELF::EM_BLUCPU, true) {}

unsigned BLUCPUELFObjectWriter::getRelocType(MCContext &Ctx, const MCValue &Target,
                                          const MCFixup &Fixup,
                                          bool IsPCRel) const {
  llvm_unreachable("invalid fixup kind!");
}

std::unique_ptr<MCObjectTargetWriter> createBLUCPUELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<BLUCPUELFObjectWriter>(OSABI);
}

} // end of namespace llvm
