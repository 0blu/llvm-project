//===-- BLUCPUTargetObjectFile.cpp - BLUCPU Object Files ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "BLUCPUTargetObjectFile.h"
#include "BLUCPUTargetMachine.h"

#include "llvm/BinaryFormat/ELF.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"

#include "BLUCPU.h"

namespace llvm {
void BLUCPUTargetObjectFile::Initialize(MCContext &Ctx, const TargetMachine &TM) {
  Base::Initialize(Ctx, TM);
  ProgmemDataSection =
      Ctx.getELFSection(".progmem.data", ELF::SHT_PROGBITS, ELF::SHF_ALLOC);
  Progmem1DataSection =
      Ctx.getELFSection(".progmem1.data", ELF::SHT_PROGBITS, ELF::SHF_ALLOC);
  Progmem2DataSection =
      Ctx.getELFSection(".progmem2.data", ELF::SHT_PROGBITS, ELF::SHF_ALLOC);
  Progmem3DataSection =
      Ctx.getELFSection(".progmem3.data", ELF::SHT_PROGBITS, ELF::SHF_ALLOC);
  Progmem4DataSection =
      Ctx.getELFSection(".progmem4.data", ELF::SHT_PROGBITS, ELF::SHF_ALLOC);
  Progmem5DataSection =
      Ctx.getELFSection(".progmem5.data", ELF::SHT_PROGBITS, ELF::SHF_ALLOC);
}

MCSection *BLUCPUTargetObjectFile::SelectSectionForGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {
  // Global values in flash memory are placed in the progmem*.data section
  // unless they already have a user assigned section.
  const auto &BLUCPUTM = static_cast<const BLUCPUTargetMachine &>(TM);
  if (BLUCPU::isProgramMemoryAddress(GO) && !GO->hasSection() &&
      Kind.isReadOnly()) {
    // The BLUCPU subtarget should support LPM to access section '.progmem*.data'.
    // if (!BLUCPUTM.getSubtargetImpl()->hasLPM()) {
    //   // TODO: Get the global object's location in source file.
    //   getContext().reportError(
    //       SMLoc(),
    //       "Current BLUCPU subtarget does not support accessing program memory");
    //   return Base::SelectSectionForGlobal(GO, Kind, TM);
    // }
    // // The BLUCPU subtarget should support ELPM to access section
    // // '.progmem[1|2|3|4|5].data'.
    // if (!BLUCPUTM.getSubtargetImpl()->hasELPM() &&
    //     BLUCPU::getAddressSpace(GO) != BLUCPU::ProgramMemory) {
    //   // TODO: Get the global object's location in source file.
    //   getContext().reportError(SMLoc(),
    //                            "Current BLUCPU subtarget does not support "
    //                            "accessing extended program memory");
    //   return ProgmemDataSection;
    // }
    switch (BLUCPU::getAddressSpace(GO)) {
    case BLUCPU::ProgramMemory: // address space 1
      return ProgmemDataSection;
    case BLUCPU::ProgramMemory1: // address space 2
      return Progmem1DataSection;
    case BLUCPU::ProgramMemory2: // address space 3
      return Progmem2DataSection;
    case BLUCPU::ProgramMemory3: // address space 4
      return Progmem3DataSection;
    case BLUCPU::ProgramMemory4: // address space 5
      return Progmem4DataSection;
    case BLUCPU::ProgramMemory5: // address space 6
      return Progmem5DataSection;
    default:
      llvm_unreachable("unexpected program memory index");
    }
  }

  // Otherwise, we work the same way as ELF.
  return Base::SelectSectionForGlobal(GO, Kind, TM);
}
} // end of namespace llvm
