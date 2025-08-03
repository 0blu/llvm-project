#include "BLUCPUMCTargetDesc.h"
#include "TargetInfo/BLUCPUTargetInfo.h"

#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define GET_REGINFO_MC_DESC
#define GET_REGINFO_ENUM
#include "BLUCPUGenRegisterInfo.inc"

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "BLUCPUGenInstrInfo.inc"

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeBLUCPUTargetMC() {
}
