#include "BLUCPUMCTargetDesc.h"

#include "BLUCPUMCAsmInfo.h"
#include "TargetInfo/BLUCPUTargetInfo.h"

#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

using namespace llvm;

#define GET_REGINFO_MC_DESC
#define GET_REGINFO_ENUM
#include "BLUCPUGenRegisterInfo.inc"

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "BLUCPUGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "BLUCPUGenSubtargetInfo.inc"

static MCInstrInfo* createBLUCPUMCInstructionInfo() {
  MCInstrInfo* X = new MCInstrInfo();
  InitBLUCPUMCInstrInfo(X);
  return X;
}

static MCRegisterInfo* createBLUCPUMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo* X = new MCRegisterInfo();
  InitBLUCPUMCRegisterInfo(X, 0);
  return X;
}

static MCSubtargetInfo* createBLUCPUMCSubtargetInfo(const Triple& TT, StringRef CPU, StringRef FS) {
  const StringRef TuneCPU = CPU;
  return createBLUCPUMCSubtargetInfoImpl(TT, CPU, TuneCPU, FS);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeBLUCPUTargetMC() {
  RegisterMCAsmInfo<BLUCPUMCAsmInfo> X(getTheBLUCPUTarget());

  TargetRegistry::RegisterMCSubtargetInfo(getTheBLUCPUTarget(), createBLUCPUMCSubtargetInfo);
  TargetRegistry::RegisterMCInstrInfo(getTheBLUCPUTarget(), createBLUCPUMCInstructionInfo);
  // TargetRegistry::RegisterMCAsmInfo(getTheBLUCPUTarget(), createBLUCPUMCAsmInfo);
  TargetRegistry::RegisterMCRegInfo(getTheBLUCPUTarget(), createBLUCPUMCRegisterInfo);
}
