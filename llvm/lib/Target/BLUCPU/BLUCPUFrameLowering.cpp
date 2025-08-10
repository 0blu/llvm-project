#include "BLUCPUFrameLowering.h"

#include "BLUCPU.h"
#include "BLUCPUInstrInfo.h"
#include "BLUCPUMachineFunctionInfo.h"
#include "BLUCPUTargetMachine.h"
#include "MCTargetDesc/BLUCPUMCTargetDesc.h"

#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"


llvm::BLUCPUFrameLowering::BLUCPUFrameLowering()
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(4), -4) {}

void llvm::BLUCPUFrameLowering::emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = (MBBI != MBB.end()) ? MBBI->getDebugLoc() : DebugLoc();
  const BLUCPUSubtarget &STI = MF.getSubtarget<BLUCPUSubtarget>();
  const BLUCPUInstrInfo &TII = *STI.getInstrInfo();

  BuildMI(MBB, MBBI, DL, TII.get(BLUCPU::RET));
}

void llvm::BLUCPUFrameLowering::emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const {

}

bool llvm::BLUCPUFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  return false;
}