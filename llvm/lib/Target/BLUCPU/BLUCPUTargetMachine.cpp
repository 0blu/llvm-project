#include "BLUCPUTargetMachine.h"

#include "TargetInfo/BLUCPUTargetInfo.h"

#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"

namespace llvm {

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeBLUCPUTarget() {
  // Register the target.
  RegisterTargetMachine<BLUCPUTargetMachine> X(getTheBLUCPUTarget());
}

static const char *BLUCPUDataLayout =
    "e-P1-p:16:8-i8:8-i16:8-i32:8-i64:8-f32:8-f64:8-n8-a:8";

static StringRef getCPU(StringRef CPU) {
  if (CPU.empty() || CPU == "generic") {
    return "blucpu";
  }

  return CPU;
}

static Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::Static);
}


BLUCPUTargetMachine::BLUCPUTargetMachine(const Target &T, const Triple &TT,
                                         StringRef CPU, StringRef FS,
                                         const TargetOptions &Options,
                                         std::optional<Reloc::Model> RM,
                                         std::optional<CodeModel::Model> CM,
                                         CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, BLUCPUDataLayout, TT, getCPU(CPU), FS, Options, getEffectiveRelocModel(RM), getEffectiveCodeModel(CM, CodeModel::Small), OL) {}
} // namespace llvm
