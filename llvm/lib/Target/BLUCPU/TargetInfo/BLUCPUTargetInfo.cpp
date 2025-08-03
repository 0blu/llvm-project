#include "TargetInfo/BLUCPUTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

namespace llvm {
Target &getTheBLUCPUTarget() {
  static Target TheBLUCPUTarget;
  return TheBLUCPUTarget;
}
} // namespace llvm

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeBLUCPUTargetInfo() {
  llvm::RegisterTarget<llvm::Triple::blucpu, /*HasJIT=*/false> X(
    llvm::getTheBLUCPUTarget(), "blucpu",
    "BLUCPU Target", "BLUCPU"
  );
}
