#ifndef LLVM_BLUCPU_MCTARGET_DESC_H
#define LLVM_BLUCPU_MCTARGET_DESC_H

#include "llvm/Support/DataTypes.h"

#include <memory>

namespace llvm {

class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCTargetOptions;
class Target;

} // end namespace llvm

#define GET_REGINFO_ENUM
#include "BLUCPUGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "BLUCPUGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "BLUCPUGenSubtargetInfo.inc"

#endif // LLVM_BLUCPU_MCTARGET_DESC_H
