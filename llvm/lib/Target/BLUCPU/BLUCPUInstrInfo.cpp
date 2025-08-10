#include "BLUCPUInstrInfo.h"

#define GET_INSTRINFO_CTOR_DTOR
#include "BLUCPUGenInstrInfo.inc"

namespace llvm {
  BLUCPUInstrInfo::BLUCPUInstrInfo(BLUCPUSubtarget &STI) : STI(STI) {}
} // namespace llvm