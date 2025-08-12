//===-- BLUCPUExpandPseudoInsts.cpp - Expand pseudo instructions -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions. This pass should be run after register allocation but before
// the post-regalloc scheduling pass.
//
//===----------------------------------------------------------------------===//

#include "BLUCPU.h"
#include "BLUCPUInstrInfo.h"
#include "BLUCPUTargetMachine.h"
#include "MCTargetDesc/BLUCPUMCTargetDesc.h"

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"

using namespace llvm;

#define BLUCPU_EXPAND_PSEUDO_NAME "BLUCPU pseudo instruction expansion pass"

namespace {

/// Expands "placeholder" instructions marked as pseudo into
/// actual BLUCPU instructions.
class BLUCPUExpandPseudo : public MachineFunctionPass {
public:
  static char ID;

  BLUCPUExpandPseudo() : MachineFunctionPass(ID) {
    initializeBLUCPUExpandPseudoPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return BLUCPU_EXPAND_PSEUDO_NAME; }

private:
  typedef MachineBasicBlock Block;
  typedef Block::iterator BlockIt;

  const BLUCPURegisterInfo *TRI;
  const TargetInstrInfo *TII;

  bool expandMBB(Block &MBB);
  bool expandMI(Block &MBB, BlockIt MBBI);
  template <unsigned OP> bool expand(Block &MBB, BlockIt MBBI);

  MachineInstrBuilder buildMI(Block &MBB, BlockIt MBBI, unsigned Opcode) {
    return BuildMI(MBB, MBBI, MBBI->getDebugLoc(), TII->get(Opcode));
  }

  MachineInstrBuilder buildMI(Block &MBB, BlockIt MBBI, unsigned Opcode,
                              Register DstReg) {
    return BuildMI(MBB, MBBI, MBBI->getDebugLoc(), TII->get(Opcode), DstReg);
  }

  MachineRegisterInfo &getRegInfo(Block &MBB) {
    return MBB.getParent()->getRegInfo();
  }

  bool expandArith(unsigned OpLo, unsigned OpHi, Block &MBB, BlockIt MBBI);
  bool expandLogic(unsigned Op, Block &MBB, BlockIt MBBI);
  bool expandLogicImm(unsigned Op, Block &MBB, BlockIt MBBI);
  bool isLogicImmOpRedundant(unsigned Op, unsigned ImmVal) const;
  bool isLogicRegOpUndef(unsigned Op, unsigned ImmVal) const;

  template <typename Func> bool expandAtomic(Block &MBB, BlockIt MBBI, Func f);

  template <typename Func>
  bool expandAtomicBinaryOp(unsigned Opcode, Block &MBB, BlockIt MBBI, Func f);

  bool expandAtomicBinaryOp(unsigned Opcode, Block &MBB, BlockIt MBBI);

  /// Specific shift implementation for int8.
  bool expandLSLB7Rd(Block &MBB, BlockIt MBBI);
  bool expandLSRB7Rd(Block &MBB, BlockIt MBBI);
  bool expandASRB6Rd(Block &MBB, BlockIt MBBI);
  bool expandASRB7Rd(Block &MBB, BlockIt MBBI);

  /// Specific shift implementation for int16.
  bool expandLSLW4Rd(Block &MBB, BlockIt MBBI);
  bool expandLSRW4Rd(Block &MBB, BlockIt MBBI);
  bool expandASRW7Rd(Block &MBB, BlockIt MBBI);
  bool expandLSLW8Rd(Block &MBB, BlockIt MBBI);
  bool expandLSRW8Rd(Block &MBB, BlockIt MBBI);
  bool expandASRW8Rd(Block &MBB, BlockIt MBBI);
  bool expandLSLW12Rd(Block &MBB, BlockIt MBBI);
  bool expandLSRW12Rd(Block &MBB, BlockIt MBBI);
  bool expandASRW14Rd(Block &MBB, BlockIt MBBI);
  bool expandASRW15Rd(Block &MBB, BlockIt MBBI);

  // Common implementation of ROLBRdR1 and ROLBRdR17.
  bool expandROLBRd(Block &MBB, BlockIt MBBI);
};

char BLUCPUExpandPseudo::ID = 0;

bool BLUCPUExpandPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  BlockIt MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    BlockIt NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool BLUCPUExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  bool Modified = false;

  const BLUCPUSubtarget &STI = MF.getSubtarget<BLUCPUSubtarget>();
  TRI = STI.getRegisterInfo();
  TII = STI.getInstrInfo();

  for (Block &MBB : MF) {
    bool ContinueExpanding = true;
    unsigned ExpandCount = 0;

    // Continue expanding the block until all pseudos are expanded.
    do {
      assert(ExpandCount < 10 && "pseudo expand limit reached");
      (void)ExpandCount;

      bool BlockModified = expandMBB(MBB);
      Modified |= BlockModified;
      ExpandCount++;

      ContinueExpanding = BlockModified;
    } while (ContinueExpanding);
  }

  return Modified;
}

bool BLUCPUExpandPseudo::expandArith(unsigned OpLo, unsigned OpHi, Block &MBB,
                                  BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  buildMI(MBB, MBBI, OpLo)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg, getKillRegState(DstIsKill))
      .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI =
      buildMI(MBB, MBBI, OpHi)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandLogic(unsigned Op, Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO =
      buildMI(MBB, MBBI, Op)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill))
          .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  // SREG is always implicitly dead
  MIBLO->getOperand(3).setIsDead();

  auto MIBHI =
      buildMI(MBB, MBBI, Op)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::isLogicImmOpRedundant(unsigned Op,
                                            unsigned ImmVal) const {

  // ANDI Rd, 0xff is redundant.
  if (Op == BLUCPU::ANDIRdK && ImmVal == 0xff)
    return true;

  // ORI Rd, 0x0 is redundant.
  if (Op == BLUCPU::ORIRdK && ImmVal == 0x0)
    return true;

  return false;
}

bool BLUCPUExpandPseudo::isLogicRegOpUndef(unsigned Op, unsigned ImmVal) const {
  // ANDI Rd, 0x00 clears all input bits.
  if (Op == BLUCPU::ANDIRdK && ImmVal == 0x00)
    return true;

  // ORI Rd, 0xff sets all input bits.
  if (Op == BLUCPU::ORIRdK && ImmVal == 0xff)
    return true;

  return false;
}

bool BLUCPUExpandPseudo::expandLogicImm(unsigned Op, Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  unsigned Imm = MI.getOperand(2).getImm();
  unsigned Lo8 = Imm & 0xff;
  unsigned Hi8 = (Imm >> 8) & 0xff;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  if (!isLogicImmOpRedundant(Op, Lo8)) {
    auto MIBLO =
        buildMI(MBB, MBBI, Op)
            .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
            .addReg(DstLoReg, getKillRegState(SrcIsKill))
            .addImm(Lo8);

    // SREG is always implicitly dead
    MIBLO->getOperand(3).setIsDead();

    if (isLogicRegOpUndef(Op, Lo8))
      MIBLO->getOperand(1).setIsUndef(true);
  }

  if (!isLogicImmOpRedundant(Op, Hi8)) {
    auto MIBHI =
        buildMI(MBB, MBBI, Op)
            .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
            .addReg(DstHiReg, getKillRegState(SrcIsKill))
            .addImm(Hi8);

    if (ImpIsDead)
      MIBHI->getOperand(3).setIsDead();

    if (isLogicRegOpUndef(Op, Hi8))
      MIBHI->getOperand(1).setIsUndef(true);
  }

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ADDWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(BLUCPU::ADDRdRr, BLUCPU::ADCRdRr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ADCWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(BLUCPU::ADCRdRr, BLUCPU::ADCRdRr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::SUBWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(BLUCPU::SUBRdRr, BLUCPU::SBCRdRr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::SUBIWRdK>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO =
      buildMI(MBB, MBBI, BLUCPU::SUBIRdK)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(SrcIsKill));

  auto MIBHI =
      buildMI(MBB, MBBI, BLUCPU::SBCIRdK)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(SrcIsKill));

  switch (MI.getOperand(2).getType()) {
  case MachineOperand::MO_GlobalAddress: {
    const GlobalValue *GV = MI.getOperand(2).getGlobal();
    int64_t Offs = MI.getOperand(2).getOffset();
    unsigned TF = MI.getOperand(2).getTargetFlags();
    MIBLO.addGlobalAddress(GV, Offs, TF | BLUCPUII::MO_NEG | BLUCPUII::MO_LO);
    MIBHI.addGlobalAddress(GV, Offs, TF | BLUCPUII::MO_NEG | BLUCPUII::MO_HI);
    break;
  }
  case MachineOperand::MO_Immediate: {
    unsigned Imm = MI.getOperand(2).getImm();
    MIBLO.addImm(Imm & 0xff);
    MIBHI.addImm((Imm >> 8) & 0xff);
    break;
  }
  default:
    llvm_unreachable("Unknown operand type!");
  }

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::SBCWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(BLUCPU::SBCRdRr, BLUCPU::SBCRdRr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::SBCIWRdK>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  unsigned Imm = MI.getOperand(2).getImm();
  unsigned Lo8 = Imm & 0xff;
  unsigned Hi8 = (Imm >> 8) & 0xff;
  unsigned OpLo = BLUCPU::SBCIRdK;
  unsigned OpHi = BLUCPU::SBCIRdK;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO =
      buildMI(MBB, MBBI, OpLo)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(SrcIsKill))
          .addImm(Lo8);

  // SREG is always implicitly killed
  MIBLO->getOperand(4).setIsKill();

  auto MIBHI =
      buildMI(MBB, MBBI, OpHi)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(SrcIsKill))
          .addImm(Hi8);

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ANDWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandLogic(BLUCPU::ANDRdRr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ANDIWRdK>(Block &MBB, BlockIt MBBI) {
  return expandLogicImm(BLUCPU::ANDIRdK, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ORWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandLogic(BLUCPU::ORRdRr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ORIWRdK>(Block &MBB, BlockIt MBBI) {
  return expandLogicImm(BLUCPU::ORIRdK, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::EORWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandLogic(BLUCPU::EORRdRr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::COMWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  unsigned OpLo = BLUCPU::COMRd;
  unsigned OpHi = BLUCPU::COMRd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO =
      buildMI(MBB, MBBI, OpLo)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill));

  // SREG is always implicitly dead
  MIBLO->getOperand(2).setIsDead();

  auto MIBHI =
      buildMI(MBB, MBBI, OpHi)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(2).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::NEGWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register ZeroReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Do NEG on the upper byte.
  auto MIBHI =
      buildMI(MBB, MBBI, BLUCPU::NEGRd)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, RegState::Kill);
  // SREG is always implicitly dead
  MIBHI->getOperand(2).setIsDead();

  // Do NEG on the lower byte.
  buildMI(MBB, MBBI, BLUCPU::NEGRd)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg, getKillRegState(DstIsKill));

  // Do an extra SBC.
  auto MISBCI =
      buildMI(MBB, MBBI, BLUCPU::SBCRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addReg(ZeroReg);
  if (ImpIsDead)
    MISBCI->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MISBCI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::CPWRdRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  unsigned OpLo = BLUCPU::CPRdRr;
  unsigned OpHi = BLUCPU::CPCRdRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Low part
  buildMI(MBB, MBBI, OpLo)
      .addReg(DstLoReg, getKillRegState(DstIsKill))
      .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
                   .addReg(DstHiReg, getKillRegState(DstIsKill))
                   .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(2).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::CPCWRdRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  unsigned OpLo = BLUCPU::CPCRdRr;
  unsigned OpHi = BLUCPU::CPCRdRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
                   .addReg(DstLoReg, getKillRegState(DstIsKill))
                   .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  // SREG is always implicitly killed
  MIBLO->getOperand(3).setIsKill();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
                   .addReg(DstHiReg, getKillRegState(DstIsKill))
                   .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(2).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LDIWRdK>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  unsigned OpLo = BLUCPU::LDIRdK;
  unsigned OpHi = BLUCPU::LDIRdK;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO =
      buildMI(MBB, MBBI, OpLo)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead));

  auto MIBHI =
      buildMI(MBB, MBBI, OpHi)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead));

  switch (MI.getOperand(1).getType()) {
  case MachineOperand::MO_GlobalAddress: {
    const GlobalValue *GV = MI.getOperand(1).getGlobal();
    int64_t Offs = MI.getOperand(1).getOffset();
    unsigned TF = MI.getOperand(1).getTargetFlags();

    MIBLO.addGlobalAddress(GV, Offs, TF | BLUCPUII::MO_LO);
    MIBHI.addGlobalAddress(GV, Offs, TF | BLUCPUII::MO_HI);
    break;
  }
  case MachineOperand::MO_BlockAddress: {
    const BlockAddress *BA = MI.getOperand(1).getBlockAddress();
    unsigned TF = MI.getOperand(1).getTargetFlags();

    MIBLO.add(MachineOperand::CreateBA(BA, TF | BLUCPUII::MO_LO));
    MIBHI.add(MachineOperand::CreateBA(BA, TF | BLUCPUII::MO_HI));
    break;
  }
  case MachineOperand::MO_Immediate: {
    unsigned Imm = MI.getOperand(1).getImm();

    MIBLO.addImm(Imm & 0xff);
    MIBHI.addImm((Imm >> 8) & 0xff);
    break;
  }
  default:
    llvm_unreachable("Unknown operand type!");
  }

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LDSWRdK>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  unsigned OpLo = BLUCPU::LDSRdK;
  unsigned OpHi = BLUCPU::LDSRdK;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO =
      buildMI(MBB, MBBI, OpLo)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead));

  auto MIBHI =
      buildMI(MBB, MBBI, OpHi)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead));

  switch (MI.getOperand(1).getType()) {
  case MachineOperand::MO_GlobalAddress: {
    const GlobalValue *GV = MI.getOperand(1).getGlobal();
    int64_t Offs = MI.getOperand(1).getOffset();
    unsigned TF = MI.getOperand(1).getTargetFlags();

    MIBLO.addGlobalAddress(GV, Offs, TF);
    MIBHI.addGlobalAddress(GV, Offs + 1, TF);
    break;
  }
  case MachineOperand::MO_Immediate: {
    unsigned Imm = MI.getOperand(1).getImm();

    MIBLO.addImm(Imm);
    MIBHI.addImm(Imm + 1);
    break;
  }
  default:
    llvm_unreachable("Unknown operand type!");
  }

  MIBLO.setMemRefs(MI.memoperands());
  MIBHI.setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LDWRdPtr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(1).isKill();
  const BLUCPUSubtarget &STI = MBB.getParent()->getSubtarget<BLUCPUSubtarget>();

  // DstReg has an earlyclobber so the register allocator will allocate them in
  // separate registers.
  assert(DstReg != SrcReg && "Dst and Src registers are the same!");

  Register DstLoReg, DstHiReg;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Load low byte.
  buildMI(MBB, MBBI, BLUCPU::LDRdPtr)
      .addReg(DstLoReg, RegState::Define)
      .addReg(SrcReg)
      .setMemRefs(MI.memoperands());

  // Load high byte.
  buildMI(MBB, MBBI, BLUCPU::LDDRdPtrQ)
      .addReg(DstHiReg, RegState::Define)
      .addReg(SrcReg, getKillRegState(SrcIsKill))
      .addImm(1)
      .setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LDWRdPtrPi>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsDead = MI.getOperand(1).isKill();
  unsigned OpLo = BLUCPU::LDRdPtrPi;
  unsigned OpHi = BLUCPU::LDRdPtrPi;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBLO =
      buildMI(MBB, MBBI, OpLo)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(SrcReg, RegState::Define)
          .addReg(SrcReg, RegState::Kill);

  auto MIBHI =
      buildMI(MBB, MBBI, OpHi)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(SrcReg, RegState::Define | getDeadRegState(SrcIsDead))
          .addReg(SrcReg, RegState::Kill);

  MIBLO.setMemRefs(MI.memoperands());
  MIBHI.setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LDWRdPtrPd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsDead = MI.getOperand(1).isKill();
  unsigned OpLo = BLUCPU::LDRdPtrPd;
  unsigned OpHi = BLUCPU::LDRdPtrPd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBHI =
      buildMI(MBB, MBBI, OpHi)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(SrcReg, RegState::Define)
          .addReg(SrcReg, RegState::Kill);

  auto MIBLO =
      buildMI(MBB, MBBI, OpLo)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(SrcReg, RegState::Define | getDeadRegState(SrcIsDead))
          .addReg(SrcReg, RegState::Kill);

  MIBLO.setMemRefs(MI.memoperands());
  MIBHI.setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LDDWRdPtrQ>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  unsigned Imm = MI.getOperand(2).getImm();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(1).isKill();
  const BLUCPUSubtarget &STI = MBB.getParent()->getSubtarget<BLUCPUSubtarget>();

  // Since we add 1 to the Imm value for the high byte below, and 63 is the
  // highest Imm value allowed for the instruction, 62 is the limit here.
  assert(Imm <= 62 && "Offset is out of range");

  // DstReg has an earlyclobber so the register allocator will allocate them in
  // separate registers.
  assert(DstReg != SrcReg && "Dst and Src registers are the same!");

  Register DstLoReg, DstHiReg;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Load low byte.
  buildMI(MBB, MBBI, BLUCPU::LDDRdPtrQ)
      .addReg(DstLoReg, RegState::Define)
      .addReg(SrcReg)
      .addImm(Imm)
      .setMemRefs(MI.memoperands());

  // Load high byte.
  buildMI(MBB, MBBI, BLUCPU::LDDRdPtrQ)
      .addReg(DstHiReg, RegState::Define)
      .addReg(SrcReg, getKillRegState(SrcIsKill))
      .addImm(Imm + 1)
      .setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <typename Func>
bool BLUCPUExpandPseudo::expandAtomic(Block &MBB, BlockIt MBBI, Func f) {
  MachineInstr &MI = *MBBI;
  const BLUCPUSubtarget &STI = MBB.getParent()->getSubtarget<BLUCPUSubtarget>();

  // Store the SREG.
  buildMI(MBB, MBBI, BLUCPU::INRdA)
      .addReg(STI.getTmpRegister(), RegState::Define)
      .addImm(STI.getIORegSREG());

  // Disable exceptions.
  buildMI(MBB, MBBI, BLUCPU::BCLRs).addImm(7); // CLI

  f(MI);

  // Restore the status reg.
  buildMI(MBB, MBBI, BLUCPU::OUTARr)
      .addImm(STI.getIORegSREG())
      .addReg(STI.getTmpRegister());

  MI.eraseFromParent();
  return true;
}

template <typename Func>
bool BLUCPUExpandPseudo::expandAtomicBinaryOp(unsigned Opcode, Block &MBB,
                                           BlockIt MBBI, Func f) {
  return expandAtomic(MBB, MBBI, [&](MachineInstr &MI) {
    auto Op1 = MI.getOperand(0);
    auto Op2 = MI.getOperand(1);

    MachineInstr &NewInst =
        *buildMI(MBB, MBBI, Opcode).add(Op1).add(Op2).getInstr();
    f(NewInst);
  });
}

bool BLUCPUExpandPseudo::expandAtomicBinaryOp(unsigned Opcode, Block &MBB,
                                           BlockIt MBBI) {
  return expandAtomicBinaryOp(Opcode, MBB, MBBI, [](MachineInstr &MI) {});
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::AtomicLoad8>(Block &MBB, BlockIt MBBI) {
  return expandAtomicBinaryOp(BLUCPU::LDRdPtr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::AtomicLoad16>(Block &MBB, BlockIt MBBI) {
  return expandAtomicBinaryOp(BLUCPU::LDWRdPtr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::AtomicStore8>(Block &MBB, BlockIt MBBI) {
  return expandAtomicBinaryOp(BLUCPU::STPtrRr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::AtomicStore16>(Block &MBB, BlockIt MBBI) {
  return expandAtomicBinaryOp(BLUCPU::STWPtrRr, MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::AtomicFence>(Block &MBB, BlockIt MBBI) {
  // On BLUCPU, there is only one core and so atomic fences do nothing.
  MBBI->eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::STSWKRr>(Block &MBB, BlockIt MBBI) {
  const BLUCPUSubtarget &STI = MBB.getParent()->getSubtarget<BLUCPUSubtarget>();
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg;
  Register SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  auto MIB0 = buildMI(MBB, MBBI, BLUCPU::STSKRr);
  auto MIB1 = buildMI(MBB, MBBI, BLUCPU::STSKRr);

  switch (MI.getOperand(0).getType()) {
  case MachineOperand::MO_GlobalAddress: {
    const GlobalValue *GV = MI.getOperand(0).getGlobal();
    int64_t Offs = MI.getOperand(0).getOffset();
    unsigned TF = MI.getOperand(0).getTargetFlags();

    // Write the high byte first for traditional devices.
    MIB0.addGlobalAddress(GV, Offs + 1, TF);
    MIB1.addGlobalAddress(GV, Offs, TF);

    break;
  }
  case MachineOperand::MO_Immediate: {
    unsigned Imm = MI.getOperand(0).getImm();

    // Write the high byte first for traditional devices.
    MIB0.addImm(Imm + 1);
    MIB1.addImm(Imm);

    break;
  }
  default:
    llvm_unreachable("Unknown operand type!");
  }

  // Write the high byte first for traditional devices.
  MIB0.addReg(SrcHiReg, getKillRegState(SrcIsKill))
      .setMemRefs(MI.memoperands());
  MIB1.addReg(SrcLoReg, getKillRegState(SrcIsKill))
      .setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::STWPtrRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool DstIsUndef = MI.getOperand(0).isUndef();
  bool SrcIsKill = MI.getOperand(1).isKill();
  const BLUCPUSubtarget &STI = MBB.getParent()->getSubtarget<BLUCPUSubtarget>();

  //: TODO: need to reverse this order like inw and stsw?

  Register SrcLoReg, SrcHiReg;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  buildMI(MBB, MBBI, BLUCPU::STDPtrQRr)
      .addReg(DstReg, getUndefRegState(DstIsUndef))
      .addImm(1)
      .addReg(SrcHiReg, getKillRegState(SrcIsKill))
      .setMemRefs(MI.memoperands());
  buildMI(MBB, MBBI, BLUCPU::STPtrRr)
      .addReg(DstReg, getUndefRegState(DstIsUndef))
      .addReg(SrcLoReg, getKillRegState(SrcIsKill))
      .setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::STWPtrPiRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(2).getReg();
  unsigned Imm = MI.getOperand(3).getImm();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(2).isKill();
  unsigned OpLo = BLUCPU::STPtrPiRr;
  unsigned OpHi = BLUCPU::STPtrPiRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
                   .addReg(DstReg, RegState::Define)
                   .addReg(DstReg, RegState::Kill)
                   .addReg(SrcLoReg, getKillRegState(SrcIsKill))
                   .addImm(Imm);

  auto MIBHI =
      buildMI(MBB, MBBI, OpHi)
          .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstReg, RegState::Kill)
          .addReg(SrcHiReg, getKillRegState(SrcIsKill))
          .addImm(Imm);

  MIBLO.setMemRefs(MI.memoperands());
  MIBHI.setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::STWPtrPdRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(2).getReg();
  unsigned Imm = MI.getOperand(3).getImm();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(2).isKill();
  unsigned OpLo = BLUCPU::STPtrPdRr;
  unsigned OpHi = BLUCPU::STPtrPdRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
                   .addReg(DstReg, RegState::Define)
                   .addReg(DstReg, RegState::Kill)
                   .addReg(SrcHiReg, getKillRegState(SrcIsKill))
                   .addImm(Imm);

  auto MIBLO =
      buildMI(MBB, MBBI, OpLo)
          .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstReg, RegState::Kill)
          .addReg(SrcLoReg, getKillRegState(SrcIsKill))
          .addImm(Imm);

  MIBLO.setMemRefs(MI.memoperands());
  MIBHI.setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::STDWPtrQRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  const BLUCPUSubtarget &STI = MBB.getParent()->getSubtarget<BLUCPUSubtarget>();

  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsKill = MI.getOperand(0).isKill();
  unsigned Imm = MI.getOperand(1).getImm();
  Register SrcReg = MI.getOperand(2).getReg();
  bool SrcIsKill = MI.getOperand(2).isKill();

  // STD's maximum displacement is 63, so larger stores have to be split into a
  // set of operations.
  // For blucputiny chips, STD is not available at all so we always have to fall
  // back to manual pointer adjustments.
  if (Imm >= 63) {
    // Add offset. The offset can be 0 when expanding this instruction from the
    // more specific STWPtrRr instruction.
    if (Imm != 0) {
      buildMI(MBB, MBBI, BLUCPU::SUBIWRdK, DstReg)
          .addReg(DstReg, RegState::Kill)
          .addImm(0x10000 - Imm);
    }

    // Do the store. This is a word store, that will be expanded further.
    buildMI(MBB, MBBI, BLUCPU::STWPtrPiRr, DstReg)
        .addReg(DstReg, getKillRegState(DstIsKill))
        .addReg(SrcReg, getKillRegState(SrcIsKill))
        .addImm(0)
        .setMemRefs(MI.memoperands());

    // If the pointer is used after the store instruction, subtract the new
    // offset (with 2 added after the postincrement instructions) so it is the
    // same as before.
    if (!DstIsKill) {
      buildMI(MBB, MBBI, BLUCPU::SUBIWRdK, DstReg)
          .addReg(DstReg, RegState::Kill)
          .addImm(Imm + 2);
    }
  } else {
    Register SrcLoReg, SrcHiReg;
    TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

    buildMI(MBB, MBBI, BLUCPU::STDPtrQRr)
        .addReg(DstReg)
        .addImm(Imm + 1)
        .addReg(SrcHiReg, getKillRegState(SrcIsKill))
        .setMemRefs(MI.memoperands());
    buildMI(MBB, MBBI, BLUCPU::STDPtrQRr)
        .addReg(DstReg, getKillRegState(DstIsKill))
        .addImm(Imm)
        .addReg(SrcLoReg, getKillRegState(SrcIsKill))
        .setMemRefs(MI.memoperands());
  }

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::STDSPQRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  const MachineFunction &MF = *MBB.getParent();
  const BLUCPUSubtarget &STI = MF.getSubtarget<BLUCPUSubtarget>();

  assert(MI.getOperand(0).getReg() == BLUCPU::SP &&
         "SP is expected as base pointer");

  assert(STI.getFrameLowering()->hasReservedCallFrame(MF) &&
         "unexpected STDSPQRr pseudo instruction");
  (void)STI;

  MI.setDesc(TII->get(BLUCPU::STDPtrQRr));
  MI.getOperand(0).setReg(BLUCPU::R29R28);

  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::STDWSPQRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  const MachineFunction &MF = *MBB.getParent();
  const BLUCPUSubtarget &STI = MF.getSubtarget<BLUCPUSubtarget>();

  assert(MI.getOperand(0).getReg() == BLUCPU::SP &&
         "SP is expected as base pointer");

  assert(STI.getFrameLowering()->hasReservedCallFrame(MF) &&
         "unexpected STDWSPQRr pseudo instruction");
  (void)STI;

  MI.setDesc(TII->get(BLUCPU::STDWPtrQRr));
  MI.getOperand(0).setReg(BLUCPU::R29R28);

  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::INWRdA>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  unsigned Imm = MI.getOperand(1).getImm();
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  unsigned OpLo = BLUCPU::INRdA;
  unsigned OpHi = BLUCPU::INRdA;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Since we add 1 to the Imm value for the high byte below, and 63 is the
  // highest Imm value allowed for the instruction, 62 is the limit here.
  assert(Imm <= 62 && "Address is out of range");

  auto MIBLO =
      buildMI(MBB, MBBI, OpLo)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addImm(Imm);

  auto MIBHI =
      buildMI(MBB, MBBI, OpHi)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addImm(Imm + 1);

  MIBLO.setMemRefs(MI.memoperands());
  MIBHI.setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::OUTWARr>(Block &MBB, BlockIt MBBI) {
  const BLUCPUSubtarget &STI = MBB.getParent()->getSubtarget<BLUCPUSubtarget>();
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg;
  unsigned Imm = MI.getOperand(0).getImm();
  Register SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  // Since we add 1 to the Imm value for the high byte below, and 63 is the
  // highest Imm value allowed for the instruction, 62 is the limit here.
  assert(Imm <= 62 && "Address is out of range");

  // 16 bit I/O writes need the high byte first on normal BLUCPU devices,
  // and in reverse order for the XMEGA/XMEGA3/XMEGAU families.
  auto MIBHI = buildMI(MBB, MBBI, BLUCPU::OUTARr)
                   .addImm(Imm + 1)
                   .addReg(SrcHiReg,
                           getKillRegState(SrcIsKill));
  auto MIBLO = buildMI(MBB, MBBI, BLUCPU::OUTARr)
                   .addImm(Imm)
                   .addReg(SrcLoReg,
                           getKillRegState(SrcIsKill));

  MIBLO.setMemRefs(MI.memoperands());
  MIBHI.setMemRefs(MI.memoperands());

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::PUSHWRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg;
  Register SrcReg = MI.getOperand(0).getReg();
  bool SrcIsKill = MI.getOperand(0).isKill();
  unsigned Flags = MI.getFlags();
  unsigned OpLo = BLUCPU::PUSHRr;
  unsigned OpHi = BLUCPU::PUSHRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  // Low part
  buildMI(MBB, MBBI, OpLo)
      .addReg(SrcLoReg, getKillRegState(SrcIsKill))
      .setMIFlags(Flags);

  // High part
  buildMI(MBB, MBBI, OpHi)
      .addReg(SrcHiReg, getKillRegState(SrcIsKill))
      .setMIFlags(Flags);

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::POPWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  unsigned Flags = MI.getFlags();
  unsigned OpLo = BLUCPU::POPRd;
  unsigned OpHi = BLUCPU::POPRd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  buildMI(MBB, MBBI, OpHi, DstHiReg).setMIFlags(Flags); // High
  buildMI(MBB, MBBI, OpLo, DstLoReg).setMIFlags(Flags); // Low

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandROLBRd(Block &MBB, BlockIt MBBI) {
  // In BLUCPU, the rotate instructions behave quite unintuitively. They rotate
  // bits through the carry bit in SREG, effectively rotating over 9 bits,
  // instead of 8. This is useful when we are dealing with numbers over
  // multiple registers, but when we actually need to rotate stuff, we have
  // to explicitly add the carry bit.

  MachineInstr &MI = *MBBI;
  unsigned OpShift, OpCarry;
  Register DstReg = MI.getOperand(0).getReg();
  Register ZeroReg = MI.getOperand(3).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  OpShift = BLUCPU::ADDRdRr;
  OpCarry = BLUCPU::ADCRdRr;

  // add r16, r16
  // adc r16, r1

  // Shift part
  buildMI(MBB, MBBI, OpShift)
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstReg, RegState::Kill)
      .addReg(DstReg, RegState::Kill);

  // Add the carry bit
  auto MIB = buildMI(MBB, MBBI, OpCarry)
                 .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
                 .addReg(DstReg, getKillRegState(DstIsKill))
                 .addReg(ZeroReg);

  MIB->getOperand(3).setIsDead(); // SREG is always dead
  MIB->getOperand(4).setIsKill(); // SREG is always implicitly killed

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ROLBRdR1>(Block &MBB, BlockIt MBBI) {
  return expandROLBRd(MBB, MBBI);
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::RORBRd>(Block &MBB, BlockIt MBBI) {
  // In BLUCPU, the rotate instructions behave quite unintuitively. They rotate
  // bits through the carry bit in SREG, effectively rotating over 9 bits,
  // instead of 8. This is useful when we are dealing with numbers over
  // multiple registers, but when we actually need to rotate stuff, we have
  // to explicitly add the carry bit.

  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();

  // bst r16, 0
  // ror r16
  // bld r16, 7

  // Move the lowest bit from DstReg into the T bit
  buildMI(MBB, MBBI, BLUCPU::BST).addReg(DstReg).addImm(0);

  // Rotate to the right
  buildMI(MBB, MBBI, BLUCPU::RORRd, DstReg).addReg(DstReg);

  // Move the T bit into the highest bit of DstReg.
  buildMI(MBB, MBBI, BLUCPU::BLD, DstReg).addReg(DstReg).addImm(7);

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LSLWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  unsigned OpLo = BLUCPU::ADDRdRr; // ADD Rd, Rd <==> LSL Rd
  unsigned OpHi = BLUCPU::ADCRdRr; // ADC Rd, Rd <==> ROL Rd
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Low part
  buildMI(MBB, MBBI, OpLo)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg, getKillRegState(DstIsKill))
      .addReg(DstLoReg, getKillRegState(DstIsKill));

  auto MIBHI =
      buildMI(MBB, MBBI, OpHi)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addReg(DstHiReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LSLWHiRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // add hireg, hireg <==> lsl hireg
  auto MILSL =
      buildMI(MBB, MBBI, BLUCPU::ADDRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addReg(DstHiReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MILSL->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandLSLW4Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // swap Rh
  // swap Rl
  buildMI(MBB, MBBI, BLUCPU::SWAPRd)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg, RegState::Kill);
  buildMI(MBB, MBBI, BLUCPU::SWAPRd)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg, RegState::Kill);

  // andi Rh, 0xf0
  auto MI0 =
      buildMI(MBB, MBBI, BLUCPU::ANDIRdK)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, RegState::Kill)
          .addImm(0xf0);
  // SREG is implicitly dead.
  MI0->getOperand(3).setIsDead();

  // eor Rh, Rl
  auto MI1 =
      buildMI(MBB, MBBI, BLUCPU::EORRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, RegState::Kill)
          .addReg(DstLoReg);
  // SREG is implicitly dead.
  MI1->getOperand(3).setIsDead();

  // andi Rl, 0xf0
  auto MI2 =
      buildMI(MBB, MBBI, BLUCPU::ANDIRdK)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill))
          .addImm(0xf0);
  // SREG is implicitly dead.
  MI2->getOperand(3).setIsDead();

  // eor Rh, Rl
  auto MI3 =
      buildMI(MBB, MBBI, BLUCPU::EORRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addReg(DstLoReg);
  if (ImpIsDead)
    MI3->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandLSLW8Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // mov Rh, Rl
  buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg);

  // clr Rl
  auto MIBLO =
      buildMI(MBB, MBBI, BLUCPU::EORRdRr)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill))
          .addReg(DstLoReg, getKillRegState(DstIsKill));
  if (ImpIsDead)
    MIBLO->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandLSLW12Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // mov Rh, Rl
  buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg);

  // swap Rh
  buildMI(MBB, MBBI, BLUCPU::SWAPRd)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg, RegState::Kill);

  // andi Rh, 0xf0
  auto MI0 =
      buildMI(MBB, MBBI, BLUCPU::ANDIRdK)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addImm(0xf0);
  // SREG is implicitly dead.
  MI0->getOperand(3).setIsDead();

  // clr Rl
  auto MI1 =
      buildMI(MBB, MBBI, BLUCPU::EORRdRr)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill))
          .addReg(DstLoReg, getKillRegState(DstIsKill));
  if (ImpIsDead)
    MI1->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LSLWNRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned Imm = MI.getOperand(2).getImm();
  switch (Imm) {
  case 4:
    return expandLSLW4Rd(MBB, MBBI);
  case 8:
    return expandLSLW8Rd(MBB, MBBI);
  case 12:
    return expandLSLW12Rd(MBB, MBBI);
  default:
    llvm_unreachable("unimplemented lslwn");
    return false;
  }
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LSRWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  unsigned OpLo = BLUCPU::RORRd;
  unsigned OpHi = BLUCPU::LSRRd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // High part
  buildMI(MBB, MBBI, OpHi)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg, getKillRegState(DstIsKill));

  auto MIBLO =
      buildMI(MBB, MBBI, OpLo)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIBLO->getOperand(2).setIsDead();

  // SREG is always implicitly killed
  MIBLO->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LSRWLoRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // lsr loreg
  auto MILSR =
      buildMI(MBB, MBBI, BLUCPU::LSRRd)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MILSR->getOperand(2).setIsDead();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandLSRW4Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // swap Rh
  // swap Rl
  buildMI(MBB, MBBI, BLUCPU::SWAPRd)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg, RegState::Kill);
  buildMI(MBB, MBBI, BLUCPU::SWAPRd)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg, RegState::Kill);

  // andi Rl, 0xf
  auto MI0 =
      buildMI(MBB, MBBI, BLUCPU::ANDIRdK)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, RegState::Kill)
          .addImm(0xf);
  // SREG is implicitly dead.
  MI0->getOperand(3).setIsDead();

  // eor Rl, Rh
  auto MI1 =
      buildMI(MBB, MBBI, BLUCPU::EORRdRr)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, RegState::Kill)
          .addReg(DstHiReg);
  // SREG is implicitly dead.
  MI1->getOperand(3).setIsDead();

  // andi Rh, 0xf
  auto MI2 =
      buildMI(MBB, MBBI, BLUCPU::ANDIRdK)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addImm(0xf);
  // SREG is implicitly dead.
  MI2->getOperand(3).setIsDead();

  // eor Rl, Rh
  auto MI3 =
      buildMI(MBB, MBBI, BLUCPU::EORRdRr)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill))
          .addReg(DstHiReg);
  if (ImpIsDead)
    MI3->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandLSRW8Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Move upper byte to lower byte.
  buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg);

  // Clear upper byte.
  auto MIBHI =
      buildMI(MBB, MBBI, BLUCPU::EORRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addReg(DstHiReg, getKillRegState(DstIsKill));
  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandLSRW12Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Move upper byte to lower byte.
  buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg);

  // swap Rl
  buildMI(MBB, MBBI, BLUCPU::SWAPRd)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg, RegState::Kill);

  // andi Rl, 0xf
  auto MI0 =
      buildMI(MBB, MBBI, BLUCPU::ANDIRdK)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill))
          .addImm(0xf);
  // SREG is implicitly dead.
  MI0->getOperand(3).setIsDead();

  // Clear upper byte.
  auto MIBHI =
      buildMI(MBB, MBBI, BLUCPU::EORRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addReg(DstHiReg, getKillRegState(DstIsKill));
  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LSRWNRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned Imm = MI.getOperand(2).getImm();
  switch (Imm) {
  case 4:
    return expandLSRW4Rd(MBB, MBBI);
  case 8:
    return expandLSRW8Rd(MBB, MBBI);
  case 12:
    return expandLSRW12Rd(MBB, MBBI);
  default:
    llvm_unreachable("unimplemented lsrwn");
    return false;
  }
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::RORWRd>(Block &MBB, BlockIt MBBI) {
  llvm_unreachable("RORW unimplemented");
  return false;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ROLWRd>(Block &MBB, BlockIt MBBI) {
  llvm_unreachable("ROLW unimplemented");
  return false;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ASRWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  unsigned OpLo = BLUCPU::RORRd;
  unsigned OpHi = BLUCPU::ASRRd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // High part
  buildMI(MBB, MBBI, OpHi)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg, getKillRegState(DstIsKill));

  auto MIBLO =
      buildMI(MBB, MBBI, OpLo)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIBLO->getOperand(2).setIsDead();

  // SREG is always implicitly killed
  MIBLO->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ASRWLoRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // asr loreg
  auto MIASR =
      buildMI(MBB, MBBI, BLUCPU::ASRRd)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIASR->getOperand(2).setIsDead();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandASRW7Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // lsl r24
  // mov r24,r25
  // rol r24
  // sbc r25,r25

  // lsl r24 <=> add r24, r24
  buildMI(MBB, MBBI, BLUCPU::ADDRdRr)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg, RegState::Kill)
      .addReg(DstLoReg, RegState::Kill);

  // mov r24, r25
  buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg);

  // rol r24 <=> adc r24, r24
  buildMI(MBB, MBBI, BLUCPU::ADCRdRr)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg, getKillRegState(DstIsKill))
      .addReg(DstLoReg, getKillRegState(DstIsKill));

  // sbc r25, r25
  auto MISBC =
      buildMI(MBB, MBBI, BLUCPU::SBCRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addReg(DstHiReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MISBC->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MISBC->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandASRW8Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Move upper byte to lower byte.
  buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg);

  // Move the sign bit to the C flag.
  buildMI(MBB, MBBI, BLUCPU::ADDRdRr)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg, RegState::Kill)
      .addReg(DstHiReg, RegState::Kill);

  // Set upper byte to 0 or -1.
  auto MIBHI =
      buildMI(MBB, MBBI, BLUCPU::SBCRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, getKillRegState(DstIsKill))
          .addReg(DstHiReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}
bool BLUCPUExpandPseudo::expandASRW14Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // lsl r25
  // sbc r24, r24
  // lsl r25
  // mov r25, r24
  // rol r24

  // lsl r25 <=> add r25, r25
  buildMI(MBB, MBBI, BLUCPU::ADDRdRr)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg, RegState::Kill)
      .addReg(DstHiReg, RegState::Kill);

  // sbc r24, r24
  buildMI(MBB, MBBI, BLUCPU::SBCRdRr)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg, RegState::Kill)
      .addReg(DstLoReg, RegState::Kill);

  // lsl r25 <=> add r25, r25
  buildMI(MBB, MBBI, BLUCPU::ADDRdRr)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg, RegState::Kill)
      .addReg(DstHiReg, RegState::Kill);

  // mov r25, r24
  buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg);

  // rol r24 <=> adc r24, r24
  auto MIROL =
      buildMI(MBB, MBBI, BLUCPU::ADCRdRr)
          .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstLoReg, getKillRegState(DstIsKill))
          .addReg(DstLoReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIROL->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MIROL->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return false;
}

bool BLUCPUExpandPseudo::expandASRW15Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // lsl r25
  // sbc r25, r25
  // mov r24, r25

  // lsl r25 <=> add r25, r25
  buildMI(MBB, MBBI, BLUCPU::ADDRdRr)
      .addReg(DstHiReg, RegState::Define)
      .addReg(DstHiReg, RegState::Kill)
      .addReg(DstHiReg, RegState::Kill);

  // sbc r25, r25
  auto MISBC =
      buildMI(MBB, MBBI, BLUCPU::SBCRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, RegState::Kill)
          .addReg(DstHiReg, RegState::Kill);
  if (ImpIsDead)
    MISBC->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MISBC->getOperand(4).setIsKill();

  // mov r24, r25
  buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg);

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ASRWNRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned Imm = MI.getOperand(2).getImm();
  switch (Imm) {
  case 7:
    return expandASRW7Rd(MBB, MBBI);
  case 8:
    return expandASRW8Rd(MBB, MBBI);
  case 14:
    return expandASRW14Rd(MBB, MBBI);
  case 15:
    return expandASRW15Rd(MBB, MBBI);
  default:
    llvm_unreachable("unimplemented asrwn");
    return false;
  }
}

bool BLUCPUExpandPseudo::expandLSLB7Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();

  // ror r24
  // clr r24
  // ror r24

  buildMI(MBB, MBBI, BLUCPU::RORRd)
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstReg, RegState::Kill)
      ->getOperand(3)
      .setIsUndef(true);

  buildMI(MBB, MBBI, BLUCPU::EORRdRr)
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstReg, RegState::Kill)
      .addReg(DstReg, RegState::Kill);

  auto MIRRC =
      buildMI(MBB, MBBI, BLUCPU::RORRd)
          .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIRRC->getOperand(2).setIsDead();

  // SREG is always implicitly killed
  MIRRC->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LSLBNRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned Imm = MI.getOperand(2).getImm();
  switch (Imm) {
  case 7:
    return expandLSLB7Rd(MBB, MBBI);
  default:
    llvm_unreachable("unimplemented lslbn");
    return false;
  }
}

bool BLUCPUExpandPseudo::expandLSRB7Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();

  // rol r24
  // clr r24
  // rol r24

  buildMI(MBB, MBBI, BLUCPU::ADCRdRr)
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstReg, RegState::Kill)
      .addReg(DstReg, RegState::Kill)
      ->getOperand(4)
      .setIsUndef(true);

  buildMI(MBB, MBBI, BLUCPU::EORRdRr)
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstReg, RegState::Kill)
      .addReg(DstReg, RegState::Kill);

  auto MIRRC =
      buildMI(MBB, MBBI, BLUCPU::ADCRdRr)
          .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstReg, getKillRegState(DstIsKill))
          .addReg(DstReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIRRC->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  MIRRC->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::LSRBNRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned Imm = MI.getOperand(2).getImm();
  switch (Imm) {
  case 7:
    return expandLSRB7Rd(MBB, MBBI);
  default:
    llvm_unreachable("unimplemented lsrbn");
    return false;
  }
}

bool BLUCPUExpandPseudo::expandASRB6Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();

  // bst r24, 6
  // lsl r24
  // sbc r24, r24
  // bld r24, 0

  buildMI(MBB, MBBI, BLUCPU::BST)
      .addReg(DstReg)
      .addImm(6)
      ->getOperand(2)
      .setIsUndef(true);

  buildMI(MBB, MBBI, BLUCPU::ADDRdRr) // LSL Rd <==> ADD Rd, Rd
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstReg, RegState::Kill)
      .addReg(DstReg, RegState::Kill);

  buildMI(MBB, MBBI, BLUCPU::SBCRdRr)
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstReg, RegState::Kill)
      .addReg(DstReg, RegState::Kill);

  buildMI(MBB, MBBI, BLUCPU::BLD)
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstReg, getKillRegState(DstIsKill))
      .addImm(0)
      ->getOperand(3)
      .setIsKill();

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandASRB7Rd(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();

  // lsl r24
  // sbc r24, r24

  buildMI(MBB, MBBI, BLUCPU::ADDRdRr)
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstReg, RegState::Kill)
      .addReg(DstReg, RegState::Kill);

  auto MIRRC =
      buildMI(MBB, MBBI, BLUCPU::SBCRdRr)
          .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstReg, getKillRegState(DstIsKill))
          .addReg(DstReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIRRC->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  MIRRC->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::ASRBNRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned Imm = MI.getOperand(2).getImm();
  switch (Imm) {
  case 6:
    return expandASRB6Rd(MBB, MBBI);
  case 7:
    return expandASRB7Rd(MBB, MBBI);
  default:
    llvm_unreachable("unimplemented asrbn");
    return false;
  }
}

template <> bool BLUCPUExpandPseudo::expand<BLUCPU::SEXT>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  // sext R17:R16, R17
  // mov     r16, r17
  // lsl     r17
  // sbc     r17, r17
  // sext R17:R16, R13
  // mov     r16, r13
  // mov     r17, r13
  // lsl     r17
  // sbc     r17, r17
  // sext R17:R16, R16
  // mov     r17, r16
  // lsl     r17
  // sbc     r17, r17
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  if (SrcReg != DstLoReg)
    buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
        .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
        .addReg(SrcReg);

  if (SrcReg != DstHiReg) {
    auto MOV = buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
                   .addReg(DstHiReg, RegState::Define)
                   .addReg(SrcReg);
    if (SrcReg != DstLoReg && SrcIsKill)
      MOV->getOperand(1).setIsKill();
  }

  buildMI(MBB, MBBI, BLUCPU::ADDRdRr) // LSL Rd <==> ADD Rd, Rr
      .addReg(DstHiReg, RegState::Define)
      .addReg(DstHiReg, RegState::Kill)
      .addReg(DstHiReg, RegState::Kill);

  auto SBC =
      buildMI(MBB, MBBI, BLUCPU::SBCRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, RegState::Kill)
          .addReg(DstHiReg, RegState::Kill);

  if (ImpIsDead)
    SBC->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  SBC->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <> bool BLUCPUExpandPseudo::expand<BLUCPU::ZEXT>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  // zext R25:R24, R20
  // mov      R24, R20
  // eor      R25, R25
  // zext R25:R24, R24
  // eor      R25, R25
  // zext R25:R24, R25
  // mov      R24, R25
  // eor      R25, R25
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  if (SrcReg != DstLoReg) {
    buildMI(MBB, MBBI, BLUCPU::MOVRdRr)
        .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
        .addReg(SrcReg, getKillRegState(SrcIsKill));
  }

  auto EOR =
      buildMI(MBB, MBBI, BLUCPU::EORRdRr)
          .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
          .addReg(DstHiReg, RegState::Kill | RegState::Undef)
          .addReg(DstHiReg, RegState::Kill | RegState::Undef);

  if (ImpIsDead)
    EOR->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::SPREAD>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  Register DstLoReg, DstHiReg;
  Register DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  unsigned Flags = MI.getFlags();
  unsigned OpLo = BLUCPU::INRdA;
  unsigned OpHi = BLUCPU::INRdA;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Low part
  buildMI(MBB, MBBI, OpLo)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addImm(0x3d)
      .setMIFlags(Flags);

  // High part
  buildMI(MBB, MBBI, OpHi)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addImm(0x3e)
      .setMIFlags(Flags);

  MI.eraseFromParent();
  return true;
}

template <>
bool BLUCPUExpandPseudo::expand<BLUCPU::SPWRITE>(Block &MBB, BlockIt MBBI) {
  const BLUCPUSubtarget &STI = MBB.getParent()->getSubtarget<BLUCPUSubtarget>();
  MachineInstr &MI = *MBBI;
  Register SrcLoReg, SrcHiReg;
  Register SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  unsigned Flags = MI.getFlags();
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  buildMI(MBB, MBBI, BLUCPU::INRdA)
      .addReg(STI.getTmpRegister(), RegState::Define)
      .addImm(STI.getIORegSREG())
      .setMIFlags(Flags);

  buildMI(MBB, MBBI, BLUCPU::BCLRs).addImm(0x07).setMIFlags(Flags);

  buildMI(MBB, MBBI, BLUCPU::OUTARr)
      .addImm(0x3e)
      .addReg(SrcHiReg, getKillRegState(SrcIsKill))
      .setMIFlags(Flags);

  buildMI(MBB, MBBI, BLUCPU::OUTARr)
      .addImm(STI.getIORegSREG())
      .addReg(STI.getTmpRegister(), RegState::Kill)
      .setMIFlags(Flags);

  buildMI(MBB, MBBI, BLUCPU::OUTARr)
      .addImm(0x3d)
      .addReg(SrcLoReg, getKillRegState(SrcIsKill))
      .setMIFlags(Flags);

  MI.eraseFromParent();
  return true;
}

bool BLUCPUExpandPseudo::expandMI(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  int Opcode = MBBI->getOpcode();

#define EXPAND(Op)                                                             \
  case Op:                                                                     \
    return expand<Op>(MBB, MI)

  switch (Opcode) {
    EXPAND(BLUCPU::ADDWRdRr);
    EXPAND(BLUCPU::ADCWRdRr);
    EXPAND(BLUCPU::SUBWRdRr);
    EXPAND(BLUCPU::SUBIWRdK);
    EXPAND(BLUCPU::SBCWRdRr);
    EXPAND(BLUCPU::SBCIWRdK);
    EXPAND(BLUCPU::ANDWRdRr);
    EXPAND(BLUCPU::ANDIWRdK);
    EXPAND(BLUCPU::ORWRdRr);
    EXPAND(BLUCPU::ORIWRdK);
    EXPAND(BLUCPU::EORWRdRr);
    EXPAND(BLUCPU::COMWRd);
    EXPAND(BLUCPU::NEGWRd);
    EXPAND(BLUCPU::CPWRdRr);
    EXPAND(BLUCPU::CPCWRdRr);
    EXPAND(BLUCPU::LDIWRdK);
    EXPAND(BLUCPU::LDSWRdK);
    EXPAND(BLUCPU::LDWRdPtr);
    EXPAND(BLUCPU::LDWRdPtrPi);
    EXPAND(BLUCPU::LDWRdPtrPd);
  case BLUCPU::LDDWRdYQ: //: FIXME: remove this once PR13375 gets fixed
    EXPAND(BLUCPU::LDDWRdPtrQ);
    EXPAND(BLUCPU::AtomicLoad8);
    EXPAND(BLUCPU::AtomicLoad16);
    EXPAND(BLUCPU::AtomicStore8);
    EXPAND(BLUCPU::AtomicStore16);
    EXPAND(BLUCPU::AtomicFence);
    EXPAND(BLUCPU::STSWKRr);
    EXPAND(BLUCPU::STWPtrRr);
    EXPAND(BLUCPU::STWPtrPiRr);
    EXPAND(BLUCPU::STWPtrPdRr);
    EXPAND(BLUCPU::STDWPtrQRr);
    EXPAND(BLUCPU::STDSPQRr);
    EXPAND(BLUCPU::STDWSPQRr);
    EXPAND(BLUCPU::INWRdA);
    EXPAND(BLUCPU::OUTWARr);
    EXPAND(BLUCPU::PUSHWRr);
    EXPAND(BLUCPU::POPWRd);
    EXPAND(BLUCPU::ROLBRdR1);
    EXPAND(BLUCPU::RORBRd);
    EXPAND(BLUCPU::LSLWRd);
    EXPAND(BLUCPU::LSRWRd);
    EXPAND(BLUCPU::RORWRd);
    EXPAND(BLUCPU::ROLWRd);
    EXPAND(BLUCPU::ASRWRd);
    EXPAND(BLUCPU::LSLWHiRd);
    EXPAND(BLUCPU::LSRWLoRd);
    EXPAND(BLUCPU::ASRWLoRd);
    EXPAND(BLUCPU::LSLWNRd);
    EXPAND(BLUCPU::LSRWNRd);
    EXPAND(BLUCPU::ASRWNRd);
    EXPAND(BLUCPU::LSLBNRd);
    EXPAND(BLUCPU::LSRBNRd);
    EXPAND(BLUCPU::ASRBNRd);
    EXPAND(BLUCPU::SEXT);
    EXPAND(BLUCPU::ZEXT);
    EXPAND(BLUCPU::SPREAD);
    EXPAND(BLUCPU::SPWRITE);
  }
#undef EXPAND
  return false;
}

} // end of anonymous namespace

INITIALIZE_PASS(BLUCPUExpandPseudo, "blucpu-expand-pseudo", BLUCPU_EXPAND_PSEUDO_NAME,
                false, false)
namespace llvm {

FunctionPass *createBLUCPUExpandPseudoPass() { return new BLUCPUExpandPseudo(); }

} // end of namespace llvm
