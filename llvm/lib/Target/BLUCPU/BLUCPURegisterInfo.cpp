#include "BLUCPURegisterInfo.h"

#include "llvm/ADT/BitVector.h"

#include "BLUCPUMachineFunctionInfo.h"
#include "BLUCPUTargetMachine.h"

#define GET_REGINFO_TARGET_DESC
#include "BLUCPUGenRegisterInfo.inc"

namespace llvm {
  BLUCPURegisterInfo::BLUCPURegisterInfo() : BLUCPUGenRegisterInfo(0) {}

  const uint16_t* BLUCPURegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
    static const MCPhysReg CalleeSavedReg = BLUCPU::NoRegister;
    return &CalleeSavedReg;
  }

  const uint32_t * BLUCPURegisterInfo::getCallPreservedMask(const MachineFunction &MF, CallingConv::ID CC) const {
    // const BLUCPUSubtarget &STI = MF.getSubtarget<BLUCPUSubtarget>();
    return GP_REGSSubClassMask;
  }

  BitVector
  BLUCPURegisterInfo::getReservedRegs(const MachineFunction &MF) const {
    BitVector Reserved(getNumRegs());

    Reserved.set(BLUCPU::SP);

    return Reserved;
  }

  bool BLUCPURegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj, unsigned FIOperandNum, RegScavenger *RS) const {
    return false;
  }

  Register BLUCPURegisterInfo::getFrameRegister(const MachineFunction &MF) const {
    return BLUCPU::SP;
  }

  const TargetRegisterClass * BLUCPURegisterInfo::getPointerRegClass(const MachineFunction &MF, unsigned Kind) const {
  return &BLUCPU::GP_REGSRegClass;
  }

  } // namespace llvm
