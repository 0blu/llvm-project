#ifndef LLVM_BLUCPU_REGISTER_INFO_H
#define LLVM_BLUCPU_REGISTER_INFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "BLUCPUGenRegisterInfo.inc"

namespace llvm {
class BLUCPURegisterInfo : public BLUCPUGenRegisterInfo {
public:
  BLUCPURegisterInfo();
};
} // namespace llvm

#endif // LLVM_BLUCPU_REGISTER_INFO_H
