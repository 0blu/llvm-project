#ifndef LLVM_BLUCPU_TARGET_MACHINE_H
#define LLVM_BLUCPU_TARGET_MACHINE_H

#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"

namespace llvm {

class BLUCPUTargetMachine : public CodeGenTargetMachineImpl {

public:
  BLUCPUTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                   StringRef FS, const TargetOptions &Options,
                   std::optional<Reloc::Model> RM,
                   std::optional<CodeModel::Model> CM, CodeGenOptLevel OL,
                   bool JIT);
};

} // namespace llvm

#endif // LLVM_BLUCPU_TARGET_MACHINE_H
