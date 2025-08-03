//===-- BLUCPURegisterInfo.cpp - BLUCPU Register Information --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the BLUCPU implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "BLUCPURegisterInfo.h"

#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/IR/Function.h"

#include "BLUCPU.h"
#include "BLUCPUInstrInfo.h"
#include "BLUCPUMachineFunctionInfo.h"
#include "BLUCPUTargetMachine.h"
#include "MCTargetDesc/BLUCPUMCTargetDesc.h"

#define GET_REGINFO_TARGET_DESC
#include "BLUCPUGenRegisterInfo.inc"

namespace llvm {

BLUCPURegisterInfo::BLUCPURegisterInfo() : BLUCPUGenRegisterInfo(0) {}

const uint16_t *
BLUCPURegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_Normal_SaveList;
}

const uint32_t *
BLUCPURegisterInfo::getCallPreservedMask(const MachineFunction &MF, CallingConv::ID CC) const {
  return CSR_Normal_RegMask;
}

BitVector BLUCPURegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  // Reserve the stack pointer.
  Reserved.set(BLUCPU::SP);

  // Reserve R2~R17 only on blucputiny.
  return Reserved;
}

const TargetRegisterClass *
BLUCPURegisterInfo::getLargestLegalSuperClass(const TargetRegisterClass *RC,
                                           const MachineFunction &MF) const {
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  if (TRI->isTypeLegalForClass(*RC, MVT::i16)) {
    return &BLUCPU::DREGSRegClass;
  }

  if (TRI->isTypeLegalForClass(*RC, MVT::i8)) {
    return &BLUCPU::GPR8RegClass;
  }

  llvm_unreachable("Invalid register size");
}

bool BLUCPURegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                          int SPAdj, unsigned FIOperandNum,
                                          RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected SPAdj value");

    return false;
  }
Register BLUCPURegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return BLUCPU::SP;
}

const TargetRegisterClass* BLUCPURegisterInfo::getPointerRegClass(const MachineFunction &MF, unsigned Kind) const {
  return &BLUCPU::GPRSPRegClass;
}

bool BLUCPURegisterInfo::shouldCoalesce(
    MachineInstr *MI, const TargetRegisterClass *SrcRC, unsigned SubReg,
    const TargetRegisterClass *DstRC, unsigned DstSubReg,
    const TargetRegisterClass *NewRC, LiveIntervals &LIS) const {
  // if (this->getRegClass(BLUCPU::PTRDISPREGSRegClassID)->hasSubClassEq(NewRC)) {
  //   return false;
  // }

  return TargetRegisterInfo::shouldCoalesce(MI, SrcRC, SubReg, DstRC, DstSubReg,
                                            NewRC, LIS);
}

} // end of namespace llvm
