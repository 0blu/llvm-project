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

void BLUCPUInstrInfo::copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, const DebugLoc &DL, MCRegister DestReg, MCRegister SrcReg, bool KillSrc) const {
}

Register BLUCPUInstrInfo::isLoadFromStackSlot(const MachineInstr &MI, int &FrameIndex) const {
  return 0;
}

Register BLUCPUInstrInfo::isStoreToStackSlot(const MachineInstr &MI, int &FrameIndex) const {
  return 0;
}

void BLUCPUInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool isKill, int FrameIndex, const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI, Register VReg) const {
}

void BLUCPUInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        Register DestReg, int FrameIndex,
                                        const TargetRegisterClass *RC,
                                        const TargetRegisterInfo *TRI,
                                        Register VReg) const {
}

const MCInstrDesc &BLUCPUInstrInfo::getBrCond(BLUCPUCC::CondCodes CC) const {
  llvm_unreachable("Unknown condition code!");
}

BLUCPUCC::CondCodes BLUCPUInstrInfo::getCondFromBranchOpc(unsigned Opc) const {
  return BLUCPUCC::COND_INVALID;
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
    // if (I->getOpcode() == BLUCPU::RJMPk) {
    //   UnCondBrIter = I;
    //
    //   if (!AllowModify) {
    //     TBB = I->getOperand(0).getMBB();
    //     continue;
    //   }
    //
    //   // If the block has any instructions after a JMP, delete them.
    //   MBB.erase(std::next(I), MBB.end());
    //
    //   Cond.clear();
    //   FBB = nullptr;
    //
    //   // Delete the JMP if it's equivalent to a fall-through.
    //   if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
    //     TBB = nullptr;
    //     I->eraseFromParent();
    //     I = MBB.end();
    //     UnCondBrIter = MBB.end();
    //     continue;
    //   }
    //
    //   // TBB is used to indicate the unconditinal destination.
    //   TBB = I->getOperand(0).getMBB();
    //   continue;
    // }

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
  }
  }
}

MachineBasicBlock *
BLUCPUInstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
    llvm_unreachable("unexpected opcode!");
}

bool BLUCPUInstrInfo::isBranchOffsetInRange(unsigned BranchOp,
                                         int64_t BrOffset) const {
  return false;
}

void BLUCPUInstrInfo::insertIndirectBranch(MachineBasicBlock &MBB,
                                        MachineBasicBlock &NewDestBB,
                                        MachineBasicBlock &RestoreBB,
                                        const DebugLoc &DL, int64_t BrOffset,
                                        RegScavenger *RS) const {
}

} // end of namespace llvm
