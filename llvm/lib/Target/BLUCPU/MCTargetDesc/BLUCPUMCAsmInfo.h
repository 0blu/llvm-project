#ifndef LLVM_BLUCPU_ASM_INFO_H
#define LLVM_BLUCPU_ASM_INFO_H

#include "llvm/MC/MCAsmInfo.h"

namespace llvm {

class Triple;

/// Specifies the format of BLUCPU assembly files.
class BLUCPUMCAsmInfo : public MCAsmInfo {
public:
  explicit BLUCPUMCAsmInfo(const Triple &TT, const MCTargetOptions &Options);
};

} // end namespace llvm

#endif // LLVM_BLUCPU_ASM_INFO_H
