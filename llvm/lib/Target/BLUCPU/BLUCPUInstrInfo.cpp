//===-- BLUCPUInstrInfo.cpp - BLUCPU Instruction Information --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the BLUCPU implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "BLUCPUInstrInfo.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

#include "BLUCPU.h"
#include "BLUCPUMachineFunctionInfo.h"
#include "BLUCPURegisterInfo.h"
#include "BLUCPUTargetMachine.h"
#include "MCTargetDesc/BLUCPUMCTargetDesc.h"

#define GET_INSTRINFO_CTOR_DTOR
#include "BLUCPUGenInstrInfo.inc"

namespace llvm {

BLUCPUInstrInfo::BLUCPUInstrInfo(BLUCPUSubtarget &STI)
    : BLUCPUGenInstrInfo(BLUCPU::ADJCALLSTACKDOWN, BLUCPU::ADJCALLSTACKUP), RI(),
      STI(STI) {}

void BLUCPUInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MI,
                               const DebugLoc &DL, MCRegister DestReg,
                               MCRegister SrcReg, bool KillSrc) const {
  const BLUCPURegisterInfo &TRI = *STI.getRegisterInfo();
  unsigned Opc;

  if (BLUCPU::DREGSRegClass.contains(DestReg, SrcReg)) {
    Register DestLo, DestHi, SrcLo, SrcHi;

    TRI.splitReg(DestReg, DestLo, DestHi);
    TRI.splitReg(SrcReg, SrcLo, SrcHi);

    // Emit the copies.
    // The original instruction was for a register pair, of which only one
    // register might have been live. Add 'undef' to satisfy the machine
    // verifier, when subreg liveness is enabled.
    // TODO: Eliminate these unnecessary copies.
    if (DestLo == SrcHi) {
      BuildMI(MBB, MI, DL, get(BLUCPU::MOVRdRr), DestHi)
          .addReg(SrcHi, getKillRegState(KillSrc) | RegState::Undef);
      BuildMI(MBB, MI, DL, get(BLUCPU::MOVRdRr), DestLo)
          .addReg(SrcLo, getKillRegState(KillSrc) | RegState::Undef);
    } else {
      BuildMI(MBB, MI, DL, get(BLUCPU::MOVRdRr), DestLo)
          .addReg(SrcLo, getKillRegState(KillSrc) | RegState::Undef);
      BuildMI(MBB, MI, DL, get(BLUCPU::MOVRdRr), DestHi)
          .addReg(SrcHi, getKillRegState(KillSrc) | RegState::Undef);
    }
  } else {
    if (BLUCPU::GPR8RegClass.contains(DestReg, SrcReg)) {
      Opc = BLUCPU::MOVRdRr;
    } else if (SrcReg == BLUCPU::SP && BLUCPU::DREGSRegClass.contains(DestReg)) {
      Opc = BLUCPU::SPREAD;
    } else if (DestReg == BLUCPU::SP && BLUCPU::DREGSRegClass.contains(SrcReg)) {
      Opc = BLUCPU::SPWRITE;
    } else {
      llvm_unreachable("Impossible reg-to-reg copy");
    }

    BuildMI(MBB, MI, DL, get(Opc), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  }
}

Register BLUCPUInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                           int &FrameIndex) const {
  switch (MI.getOpcode()) {
  case BLUCPU::LDDRdPtrQ:
  case BLUCPU::LDDWRdYQ: { //: FIXME: remove this once PR13375 gets fixed
    if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
        MI.getOperand(2).getImm() == 0) {
      FrameIndex = MI.getOperand(1).getIndex();
      return MI.getOperand(0).getReg();
    }
    break;
  }
  default:
    break;
  }

  return 0;
}

Register BLUCPUInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                          int &FrameIndex) const {
  switch (MI.getOpcode()) {
  case BLUCPU::STDPtrQRr:
  case BLUCPU::STDWPtrQRr: {
    if (MI.getOperand(0).isFI() && MI.getOperand(1).isImm() &&
        MI.getOperand(1).getImm() == 0) {
      FrameIndex = MI.getOperand(0).getIndex();
      return MI.getOperand(2).getReg();
    }
    break;
  }
  default:
    break;
  }

  return 0;
}

void BLUCPUInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool isKill, int FrameIndex, const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI, Register VReg) const {
  MachineFunction &MF = *MBB.getParent();
  BLUCPUMachineFunctionInfo *AFI = MF.getInfo<BLUCPUMachineFunctionInfo>();

  AFI->setHasSpills(true);

  const MachineFrameInfo &MFI = MF.getFrameInfo();

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOStore, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  unsigned Opcode = 0;
  if (TRI->isTypeLegalForClass(*RC, MVT::i8)) {
    Opcode = BLUCPU::STDPtrQRr;
  } else if (TRI->isTypeLegalForClass(*RC, MVT::i16)) {
    Opcode = BLUCPU::STDWPtrQRr;
  } else {
    llvm_unreachable("Cannot store this register into a stack slot!");
  }

  BuildMI(MBB, MI, DebugLoc(), get(Opcode))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addReg(SrcReg, getKillRegState(isKill))
      .addMemOperand(MMO);
}

void BLUCPUInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        Register DestReg, int FrameIndex,
                                        const TargetRegisterClass *RC,
                                        const TargetRegisterInfo *TRI,
                                        Register VReg) const {
  MachineFunction &MF = *MBB.getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  unsigned Opcode = 0;
  if (TRI->isTypeLegalForClass(*RC, MVT::i8)) {
    Opcode = BLUCPU::LDDRdPtrQ;
  } else if (TRI->isTypeLegalForClass(*RC, MVT::i16)) {
    // Opcode = BLUCPU::LDDWRdPtrQ;
    //: FIXME: remove this once PR13375 gets fixed
    Opcode = BLUCPU::LDDWRdYQ;
  } else {
    llvm_unreachable("Cannot load this register from a stack slot!");
  }

  BuildMI(MBB, MI, DebugLoc(), get(Opcode), DestReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO);
}

const MCInstrDesc &BLUCPUInstrInfo::getBrCond(BLUCPUCC::CondCodes CC) const {
  switch (CC) {
  default:
    llvm_unreachable("Unknown condition code!");
  case BLUCPUCC::COND_EQ:
    return get(BLUCPU::BREQk);
  case BLUCPUCC::COND_NE:
    return get(BLUCPU::BRNEk);
  case BLUCPUCC::COND_GE:
    return get(BLUCPU::BRGEk);
  case BLUCPUCC::COND_LT:
    return get(BLUCPU::BRLTk);
  case BLUCPUCC::COND_SH:
    return get(BLUCPU::BRSHk);
  case BLUCPUCC::COND_LO:
    return get(BLUCPU::BRLOk);
  case BLUCPUCC::COND_MI:
    return get(BLUCPU::BRMIk);
  case BLUCPUCC::COND_PL:
    return get(BLUCPU::BRPLk);
  }
}

BLUCPUCC::CondCodes BLUCPUInstrInfo::getCondFromBranchOpc(unsigned Opc) const {
  switch (Opc) {
  default:
    return BLUCPUCC::COND_INVALID;
  case BLUCPU::BREQk:
    return BLUCPUCC::COND_EQ;
  case BLUCPU::BRNEk:
    return BLUCPUCC::COND_NE;
  case BLUCPU::BRSHk:
    return BLUCPUCC::COND_SH;
  case BLUCPU::BRLOk:
    return BLUCPUCC::COND_LO;
  case BLUCPU::BRMIk:
    return BLUCPUCC::COND_MI;
  case BLUCPU::BRPLk:
    return BLUCPUCC::COND_PL;
  case BLUCPU::BRGEk:
    return BLUCPUCC::COND_GE;
  case BLUCPU::BRLTk:
    return BLUCPUCC::COND_LT;
  }
}

BLUCPUCC::CondCodes BLUCPUInstrInfo::getOppositeCondition(BLUCPUCC::CondCodes CC) const {
  switch (CC) {
  default:
    llvm_unreachable("Invalid condition!");
  case BLUCPUCC::COND_EQ:
    return BLUCPUCC::COND_NE;
  case BLUCPUCC::COND_NE:
    return BLUCPUCC::COND_EQ;
  case BLUCPUCC::COND_SH:
    return BLUCPUCC::COND_LO;
  case BLUCPUCC::COND_LO:
    return BLUCPUCC::COND_SH;
  case BLUCPUCC::COND_GE:
    return BLUCPUCC::COND_LT;
  case BLUCPUCC::COND_LT:
    return BLUCPUCC::COND_GE;
  case BLUCPUCC::COND_MI:
    return BLUCPUCC::COND_PL;
  case BLUCPUCC::COND_PL:
    return BLUCPUCC::COND_MI;
  }
}

bool BLUCPUInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                 MachineBasicBlock *&TBB,
                                 MachineBasicBlock *&FBB,
                                 SmallVectorImpl<MachineOperand> &Cond,
                                 bool AllowModify) const {
  // Start from the bottom of the block and work up, examining the
  // terminator instructions.
  MachineBasicBlock::iterator I = MBB.end();
  MachineBasicBlock::iterator UnCondBrIter = MBB.end();

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugInstr()) {
      continue;
    }

    // Working from the bottom, when we see a non-terminator
    // instruction, we're done.
    if (!isUnpredicatedTerminator(*I)) {
      break;
    }

    // A terminator that isn't a branch can't easily be handled
    // by this analysis.
    if (!I->getDesc().isBranch()) {
      return true;
    }

    // Handle unconditional branches.
    //: TODO: add here jmp
    if (I->getOpcode() == BLUCPU::RJMPk) {
      UnCondBrIter = I;

      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        continue;
      }

      // If the block has any instructions after a JMP, delete them.
      MBB.erase(std::next(I), MBB.end());

      Cond.clear();
      FBB = nullptr;

      // Delete the JMP if it's equivalent to a fall-through.
      if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
        TBB = nullptr;
        I->eraseFromParent();
        I = MBB.end();
        UnCondBrIter = MBB.end();
        continue;
      }

      // TBB is used to indicate the unconditinal destination.
      TBB = I->getOperand(0).getMBB();
      continue;
    }

    // Handle conditional branches.
    BLUCPUCC::CondCodes BranchCode = getCondFromBranchOpc(I->getOpcode());
    if (BranchCode == BLUCPUCC::COND_INVALID) {
      return true; // Can't handle indirect branch.
    }

    // Working from the bottom, handle the first conditional branch.
    if (Cond.empty()) {
      MachineBasicBlock *TargetBB = I->getOperand(0).getMBB();
      if (AllowModify && UnCondBrIter != MBB.end() &&
          MBB.isLayoutSuccessor(TargetBB)) {
        // If we can modify the code and it ends in something like:
        //
        //     jCC L1
        //     jmp L2
        //   L1:
        //     ...
        //   L2:
        //
        // Then we can change this to:
        //
        //     jnCC L2
        //   L1:
        //     ...
        //   L2:
        //
        // Which is a bit more efficient.
        // We conditionally jump to the fall-through block.
        BranchCode = getOppositeCondition(BranchCode);
        unsigned JNCC = getBrCond(BranchCode).getOpcode();
        MachineBasicBlock::iterator OldInst = I;

        BuildMI(MBB, UnCondBrIter, MBB.findDebugLoc(I), get(JNCC))
            .addMBB(UnCondBrIter->getOperand(0).getMBB());
        BuildMI(MBB, UnCondBrIter, MBB.findDebugLoc(I), get(BLUCPU::RJMPk))
            .addMBB(TargetBB);

        OldInst->eraseFromParent();
        UnCondBrIter->eraseFromParent();

        // Restart the analysis.
        UnCondBrIter = MBB.end();
        I = MBB.end();
        continue;
      }

      FBB = TBB;
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(MachineOperand::CreateImm(BranchCode));
      continue;
    }

    // Handle subsequent conditional branches. Only handle the case where all
    // conditional branches branch to the same destination.
    assert(Cond.size() == 1);
    assert(TBB);

    // Only handle the case where all conditional branches branch to
    // the same destination.
    if (TBB != I->getOperand(0).getMBB()) {
      return true;
    }

    BLUCPUCC::CondCodes OldBranchCode = (BLUCPUCC::CondCodes)Cond[0].getImm();
    // If the conditions are the same, we can leave them alone.
    if (OldBranchCode == BranchCode) {
      continue;
    }

    return true;
  }

  return false;
}

unsigned BLUCPUInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                    MachineBasicBlock *TBB,
                                    MachineBasicBlock *FBB,
                                    ArrayRef<MachineOperand> Cond,
                                    const DebugLoc &DL, int *BytesAdded) const {
  if (BytesAdded)
    *BytesAdded = 0;

  // Shouldn't be a fall through.
  assert(TBB && "insertBranch must not be told to insert a fallthrough");
  assert((Cond.size() == 1 || Cond.size() == 0) &&
         "BLUCPU branch conditions have one component!");

  if (Cond.empty()) {
    assert(!FBB && "Unconditional branch with multiple successors!");
    auto &MI = *BuildMI(&MBB, DL, get(BLUCPU::RJMPk)).addMBB(TBB);
    if (BytesAdded)
      *BytesAdded += getInstSizeInBytes(MI);
    return 1;
  }

  // Conditional branch.
  unsigned Count = 0;
  BLUCPUCC::CondCodes CC = (BLUCPUCC::CondCodes)Cond[0].getImm();
  auto &CondMI = *BuildMI(&MBB, DL, getBrCond(CC)).addMBB(TBB);

  if (BytesAdded)
    *BytesAdded += getInstSizeInBytes(CondMI);
  ++Count;

  if (FBB) {
    // Two-way Conditional branch. Insert the second branch.
    auto &MI = *BuildMI(&MBB, DL, get(BLUCPU::RJMPk)).addMBB(FBB);
    if (BytesAdded)
      *BytesAdded += getInstSizeInBytes(MI);
    ++Count;
  }

  return Count;
}

unsigned BLUCPUInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                    int *BytesRemoved) const {
  if (BytesRemoved)
    *BytesRemoved = 0;

  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugInstr()) {
      continue;
    }
    //: TODO: add here the missing jmp instructions once they are implemented
    // like jmp, {e}ijmp, and other cond branches, ...
    if (I->getOpcode() != BLUCPU::RJMPk &&
        getCondFromBranchOpc(I->getOpcode()) == BLUCPUCC::COND_INVALID) {
      break;
    }

    // Remove the branch.
    if (BytesRemoved)
      *BytesRemoved += getInstSizeInBytes(*I);
    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }

  return Count;
}

bool BLUCPUInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 1 && "Invalid BLUCPU branch condition!");

  BLUCPUCC::CondCodes CC = static_cast<BLUCPUCC::CondCodes>(Cond[0].getImm());
  Cond[0].setImm(getOppositeCondition(CC));

  return false;
}

unsigned BLUCPUInstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  unsigned Opcode = MI.getOpcode();

  switch (Opcode) {
  // A regular instruction
  default: {
    const MCInstrDesc &Desc = get(Opcode);
    return Desc.getSize();
  }
  case TargetOpcode::EH_LABEL:
  case TargetOpcode::IMPLICIT_DEF:
  case TargetOpcode::KILL:
  case TargetOpcode::DBG_VALUE:
    return 0;
  case TargetOpcode::INLINEASM:
  case TargetOpcode::INLINEASM_BR: {
    const MachineFunction &MF = *MI.getParent()->getParent();
    const BLUCPUTargetMachine &TM =
        static_cast<const BLUCPUTargetMachine &>(MF.getTarget());
    const TargetInstrInfo &TII = *STI.getInstrInfo();
    return TII.getInlineAsmLength(MI.getOperand(0).getSymbolName(),
                                  *TM.getMCAsmInfo());
  }
  }
}

MachineBasicBlock *
BLUCPUInstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("unexpected opcode!");
  case BLUCPU::JMPk:
  case BLUCPU::CALLk:
  case BLUCPU::RCALLk:
  case BLUCPU::RJMPk:
  case BLUCPU::BREQk:
  case BLUCPU::BRNEk:
  case BLUCPU::BRSHk:
  case BLUCPU::BRLOk:
  case BLUCPU::BRMIk:
  case BLUCPU::BRPLk:
  case BLUCPU::BRGEk:
  case BLUCPU::BRLTk:
    return MI.getOperand(0).getMBB();
  case BLUCPU::BRBSsk:
  case BLUCPU::BRBCsk:
    return MI.getOperand(1).getMBB();
  case BLUCPU::SBRCRrB:
  case BLUCPU::SBRSRrB:
  case BLUCPU::SBICAb:
  case BLUCPU::SBISAb:
    llvm_unreachable("unimplemented branch instructions");
  }
}

bool BLUCPUInstrInfo::isBranchOffsetInRange(unsigned BranchOp,
                                         int64_t BrOffset) const {

  switch (BranchOp) {
  default:
    llvm_unreachable("unexpected opcode!");
  case BLUCPU::JMPk:
  case BLUCPU::CALLk:
    return true;
  case BLUCPU::RCALLk:
  case BLUCPU::RJMPk:
    return isIntN(13, BrOffset);
  case BLUCPU::BRBSsk:
  case BLUCPU::BRBCsk:
  case BLUCPU::BREQk:
  case BLUCPU::BRNEk:
  case BLUCPU::BRSHk:
  case BLUCPU::BRLOk:
  case BLUCPU::BRMIk:
  case BLUCPU::BRPLk:
  case BLUCPU::BRGEk:
  case BLUCPU::BRLTk:
    return isIntN(7, BrOffset);
  }
}

void BLUCPUInstrInfo::insertIndirectBranch(MachineBasicBlock &MBB,
                                        MachineBasicBlock &NewDestBB,
                                        MachineBasicBlock &RestoreBB,
                                        const DebugLoc &DL, int64_t BrOffset,
                                        RegScavenger *RS) const {
  // This method inserts a *direct* branch (JMP), despite its name.
  // LLVM calls this method to fixup unconditional branches; it never calls
  // insertBranch or some hypothetical "insertDirectBranch".
  // See lib/CodeGen/RegisterRelaxation.cpp for details.
  // We end up here when a jump is too long for a RJMP instruction.
  BuildMI(&MBB, DL, get(BLUCPU::JMPk)).addMBB(&NewDestBB);
}

} // end of namespace llvm
