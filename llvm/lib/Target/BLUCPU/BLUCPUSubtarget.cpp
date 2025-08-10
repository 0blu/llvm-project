#include "BLUCPUSubtarget.h"
#include "BLUCPUTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "blucpu-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "BLUCPUGenSubtargetInfo.inc"

BLUCPUSubtarget::BLUCPUSubtarget(const Triple &TT, const std::string &CPU,
                                 const std::string &FS,
                                 const BLUCPUTargetMachine &TM)
    : BLUCPUGenSubtargetInfo(TT, CPU, CPU, FS), TLInfo(TM, *this), InstrInfo(*this) {}
