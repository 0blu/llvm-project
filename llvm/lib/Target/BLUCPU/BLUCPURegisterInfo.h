#ifndef LLVM_BLUCPU_REGISTER_INFO_H
#define LLVM_BLUCPU_REGISTER_INFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "BLUCPUGenRegisterInfo.inc"

namespace llvm {
class BLUCPURegisterInfo : public BLUCPUGenRegisterInfo {
public:
  BLUCPURegisterInfo();

public:
  const uint16_t* getCalleeSavedRegs(const MachineFunction *MF = nullptr) const override;
  const uint32_t* getCallPreservedMask(const MachineFunction &MF, CallingConv::ID CC) const override;
  BitVector getReservedRegs(const MachineFunction &MF) const override;

  /// Stack Frame Processing Methods
  bool eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj, unsigned FIOperandNum, RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;

  const TargetRegisterClass * getPointerRegClass(const MachineFunction &MF, unsigned Kind = 0) const override;

};
} // namespace llvm

#endif // LLVM_BLUCPU_REGISTER_INFO_H
