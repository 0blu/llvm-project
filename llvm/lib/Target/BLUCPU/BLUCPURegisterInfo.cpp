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
  const BLUCPUMachineFunctionInfo *AFI = MF->getInfo<BLUCPUMachineFunctionInfo>();

  return AFI->isInterruptOrSignalHandler() ? CSR_Interrupts_SaveList
                                           : CSR_Normal_SaveList;
}

const uint32_t *
BLUCPURegisterInfo::getCallPreservedMask(const MachineFunction &MF, CallingConv::ID CC) const {
  return CSR_Normal_RegMask;
}

BitVector BLUCPURegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  // Reserve the intermediate result registers r1 and r2
  // The result of instructions like 'mul' is always stored here.
  // R0/R1/R1R0 are always reserved on both blucpu and blucputiny.
  Reserved.set(BLUCPU::R0);
  Reserved.set(BLUCPU::R1);
  Reserved.set(BLUCPU::R1R0);

  // Reserve the stack pointer.
  Reserved.set(BLUCPU::SPL);
  Reserved.set(BLUCPU::SPH);
  Reserved.set(BLUCPU::SP);

  // We tenatively reserve the frame pointer register r29:r28 because the
  // function may require one, but we cannot tell until register allocation
  // is complete, which can be too late.
  //
  // Instead we just unconditionally reserve the Y register.
  //
  // TODO: Write a pass to enumerate functions which reserved the Y register
  //       but didn't end up needing a frame pointer. In these, we can
  //       convert one or two of the spills inside to use the Y register.
  Reserved.set(BLUCPU::R28);
  Reserved.set(BLUCPU::R29);
  Reserved.set(BLUCPU::R29R28);

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

/// Fold a frame offset shared between two add instructions into a single one.
static void foldFrameOffset(MachineBasicBlock::iterator &II, int &Offset,
                            Register DstReg) {
  MachineInstr &MI = *II;
  int Opcode = MI.getOpcode();

  // Don't bother trying if the next instruction is not an add or a sub.
  if (Opcode != BLUCPU::SUBIWRdK) {
    return;
  }

  // Check that DstReg matches with next instruction, otherwise the instruction
  // is not related to stack address manipulation.
  if (DstReg != MI.getOperand(0).getReg()) {
    return;
  }

  // Add the offset in the next instruction to our offset.
  switch (Opcode) {
  case BLUCPU::SUBIWRdK:
    Offset += -MI.getOperand(2).getImm();
    break;
  }

  // Finally remove the instruction.
  II++;
  MI.eraseFromParent();
}

bool BLUCPURegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                          int SPAdj, unsigned FIOperandNum,
                                          RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected SPAdj value");

  MachineInstr &MI = *II;
  DebugLoc dl = MI.getDebugLoc();
  MachineBasicBlock &MBB = *MI.getParent();
  const MachineFunction &MF = *MBB.getParent();
  const BLUCPUTargetMachine &TM = (const BLUCPUTargetMachine &)MF.getTarget();
  const TargetInstrInfo &TII = *TM.getSubtargetImpl()->getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetFrameLowering *TFI = TM.getSubtargetImpl()->getFrameLowering();
  const BLUCPUSubtarget &STI = MF.getSubtarget<BLUCPUSubtarget>();
  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  int Offset = MFI.getObjectOffset(FrameIndex);

  // Add one to the offset because SP points to an empty slot.
  Offset += MFI.getStackSize() - TFI->getOffsetOfLocalArea() + 1;
  // Fold incoming offset.
  Offset += MI.getOperand(FIOperandNum + 1).getImm();

  // This is actually "load effective address" of the stack slot
  // instruction. We have only two-address instructions, thus we need to
  // expand it into move + add.
  if (MI.getOpcode() == BLUCPU::FRMIDX) {
    Register DstReg = MI.getOperand(0).getReg();
    assert(DstReg != BLUCPU::R29R28 && "Dest reg cannot be the frame pointer");

    // Copy the frame pointer.
    Register DstLoReg, DstHiReg;
    splitReg(DstReg, DstLoReg, DstHiReg);
    BuildMI(MBB, MI, dl, TII.get(BLUCPU::MOVRdRr), DstLoReg)
        .addReg(BLUCPU::R28);
    BuildMI(MBB, MI, dl, TII.get(BLUCPU::MOVRdRr), DstHiReg)
        .addReg(BLUCPU::R29);

    assert(Offset > 0 && "Invalid offset");

    // We need to materialize the offset via an add instruction.
    unsigned Opcode;

    II++; // Skip over the FRMIDX instruction.

    // Generally, to load a frame address two add instructions are emitted that
    // could get folded into a single one:
    //  movw    r31:r30, r29:r28
    //  adiw    r31:r30, 29
    //  adiw    r31:r30, 16
    // to:
    //  movw    r31:r30, r29:r28
    //  adiw    r31:r30, 45
    if (II != MBB.end())
      foldFrameOffset(II, Offset, DstReg);

    // Select the best opcode based on DstReg and the offset size.
    switch (DstReg) {
    case BLUCPU::R25R24:
    case BLUCPU::R27R26:
    case BLUCPU::R31R30: {
      [[fallthrough]];
    }
    default: {
      // This opcode will get expanded into a pair of subi/sbci.
      Opcode = BLUCPU::SUBIWRdK;
      Offset = -Offset;
      break;
    }
    }

    MachineInstr *New = BuildMI(MBB, II, dl, TII.get(Opcode), DstReg)
                            .addReg(DstReg, RegState::Kill)
                            .addImm(Offset);
    New->getOperand(3).setIsDead();

    MI.eraseFromParent(); // remove FRMIDX

    return false;
  }

  // On most BLUCPUs, we can use an offset up to 62 for load/store with
  // displacement (63 for byte values, 62 for word values). However, the
  // "reduced tiny" cores don't support load/store with displacement. So for
  // them, we force an offset of 0 meaning that any positive offset will require
  // adjusting the frame pointer.
  int MaxOffset = 62;

  // If the offset is too big we have to adjust and restore the frame pointer
  // to materialize a valid load/store with displacement.
  //: TODO: consider using only one adiw/sbiw chain for more than one frame
  //: index
  if (Offset > MaxOffset) {
    int AddOffset = Offset - MaxOffset;

    // For huge offsets where adiw/sbiw cannot be used use a pair of subi/sbci.
    unsigned AddOpc = BLUCPU::SUBIWRdK;
    unsigned SubOpc = BLUCPU::SUBIWRdK;
    AddOffset = -AddOffset;

    // It is possible that the spiller places this frame instruction in between
    // a compare and branch, invalidating the contents of SREG set by the
    // compare instruction because of the add/sub pairs. Conservatively save and
    // restore SREG before and after each add/sub pair.
    BuildMI(MBB, II, dl, TII.get(BLUCPU::INRdA), STI.getTmpRegister())
        .addImm(STI.getIORegSREG());

    MachineInstr *New = BuildMI(MBB, II, dl, TII.get(AddOpc), BLUCPU::R29R28)
                            .addReg(BLUCPU::R29R28, RegState::Kill)
                            .addImm(AddOffset);
    New->getOperand(3).setIsDead();

    // Restore SREG.
    BuildMI(MBB, std::next(II), dl, TII.get(BLUCPU::OUTARr))
        .addImm(STI.getIORegSREG())
        .addReg(STI.getTmpRegister(), RegState::Kill);

    // No need to set SREG as dead here otherwise if the next instruction is a
    // cond branch it will be using a dead register.
    BuildMI(MBB, std::next(II), dl, TII.get(SubOpc), BLUCPU::R29R28)
        .addReg(BLUCPU::R29R28, RegState::Kill)
        .addImm(Offset - MaxOffset);

    Offset = MaxOffset;
  }

  MI.getOperand(FIOperandNum).ChangeToRegister(BLUCPU::R29R28, false);
  assert(isUInt<6>(Offset) && "Offset is out of range");
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
  return false;
}

Register BLUCPURegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
  if (TFI->hasFP(MF)) {
    // The Y pointer register
    return BLUCPU::R28;
  }

  return BLUCPU::SP;
}

const TargetRegisterClass *
BLUCPURegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                    unsigned Kind) const {
  // FIXME: Currently we're using blucpu-gcc as reference, so we restrict
  // ptrs to Y and Z regs. Though blucpu-gcc has buggy implementation
  // of memory constraint, so we can fix it and bit blucpu-gcc here ;-)
  return &BLUCPU::PTRDISPREGSRegClass;
}

void BLUCPURegisterInfo::splitReg(Register Reg, Register &LoReg,
                               Register &HiReg) const {
  assert(BLUCPU::DREGSRegClass.contains(Reg) && "can only split 16-bit registers");

  LoReg = getSubReg(Reg, BLUCPU::sub_lo);
  HiReg = getSubReg(Reg, BLUCPU::sub_hi);
}

bool BLUCPURegisterInfo::shouldCoalesce(
    MachineInstr *MI, const TargetRegisterClass *SrcRC, unsigned SubReg,
    const TargetRegisterClass *DstRC, unsigned DstSubReg,
    const TargetRegisterClass *NewRC, LiveIntervals &LIS) const {
  if (this->getRegClass(BLUCPU::PTRDISPREGSRegClassID)->hasSubClassEq(NewRC)) {
    return false;
  }

  return TargetRegisterInfo::shouldCoalesce(MI, SrcRC, SubReg, DstRC, DstSubReg,
                                            NewRC, LIS);
}

} // end of namespace llvm
