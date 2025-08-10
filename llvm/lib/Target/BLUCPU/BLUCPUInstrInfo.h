#ifndef LLVM_BLUCPU_INSTR_INFO_H
#define LLVM_BLUCPU_INSTR_INFO_H

#include "llvm/CodeGen/TargetInstrInfo.h"

#include "BLUCPURegisterInfo.h"

#define GET_INSTRINFO_HEADER
#include "BLUCPUGenInstrInfo.inc"
#undef GET_INSTRINFO_HEADER

namespace llvm {

class BLUCPUSubtarget;

class BLUCPUInstrInfo : public BLUCPUGenInstrInfo {
public:
  explicit BLUCPUInstrInfo(BLUCPUSubtarget &STI);

  const BLUCPURegisterInfo &getRegisterInfo() const { return RI; }

private:
  const BLUCPURegisterInfo RI;

protected:
  const BLUCPUSubtarget &STI;
};

} // end namespace llvm

#endif // LLVM_BLUCPU_INSTR_INFO_H
