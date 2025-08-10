#ifndef LLVM_BLUCPU_FRAME_LOWERING_H
#define LLVM_BLUCPU_FRAME_LOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

/// Utilities for creating function call frames.
class BLUCPUFrameLowering : public TargetFrameLowering {
public:
  explicit BLUCPUFrameLowering();

public:
  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

protected:
  bool hasFPImpl(const MachineFunction &MF) const override;
};

} // end namespace llvm

#endif // LLVM_BLUCPU_FRAME_LOWERING_H
