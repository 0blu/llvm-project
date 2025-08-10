#ifndef LLVM_BLUCPU_TARGET_OBJECT_FILE_H
#define LLVM_BLUCPU_TARGET_OBJECT_FILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {

/// Lowering for an BLUCPU ELF32 object file.
class BLUCPUTargetObjectFile : public TargetLoweringObjectFileELF {
  typedef TargetLoweringObjectFileELF Base;

public:
};

} // end namespace llvm

#endif // LLVM_BLUCPU_TARGET_OBJECT_FILE_H
