//===-- BLUCPUTargetMachine.cpp - Define TargetMachine for BLUCPU ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the BLUCPU specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#include "BLUCPUTargetMachine.h"

#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"

#include "BLUCPU.h"
#include "BLUCPUMachineFunctionInfo.h"
#include "BLUCPUTargetObjectFile.h"
#include "MCTargetDesc/BLUCPUMCTargetDesc.h"
#include "TargetInfo/BLUCPUTargetInfo.h"

#include <optional>

namespace llvm {

static const char *BLUCPUDataLayout =
    "e-P1-p:16:8-i8:8-i16:8-i32:8-i64:8-f32:8-f64:8-n8-a:8";

/// Processes a CPU name.
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
    : LLVMTargetMachine(T, BLUCPUDataLayout, TT, getCPU(CPU), FS, Options,
                        getEffectiveRelocModel(RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL),
      SubTarget(TT, std::string(getCPU(CPU)), std::string(FS), *this) {
  this->TLOF = std::make_unique<BLUCPUTargetObjectFile>();
  initAsmInfo();
}

namespace {
/// BLUCPU Code Generator Pass Configuration Options.
class BLUCPUPassConfig : public TargetPassConfig {
public:
  BLUCPUPassConfig(BLUCPUTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  BLUCPUTargetMachine &getBLUCPUTargetMachine() const {
    return getTM<BLUCPUTargetMachine>();
  }

  void addIRPasses() override;
  bool addInstSelector() override;
  void addPreSched2() override;
  void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *BLUCPUTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new BLUCPUPassConfig(*this, PM);
}

void BLUCPUPassConfig::addIRPasses() {
  // Expand instructions like
  //   %result = shl i32 %n, %amount
  // to a loop so that library calls are avoided.
  addPass(createBLUCPUShiftExpandPass());

  TargetPassConfig::addIRPasses();
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeBLUCPUTarget() {
  // Register the target.
  RegisterTargetMachine<BLUCPUTargetMachine> X(getTheBLUCPUTarget());

  auto &PR = *PassRegistry::getPassRegistry();
  initializeBLUCPUExpandPseudoPass(PR);
  initializeBLUCPUShiftExpandPass(PR);
  initializeBLUCPUDAGToDAGISelLegacyPass(PR);
}

const BLUCPUSubtarget *BLUCPUTargetMachine::getSubtargetImpl() const {
  return &SubTarget;
}

const BLUCPUSubtarget *BLUCPUTargetMachine::getSubtargetImpl(const Function &) const {
  return &SubTarget;
}

MachineFunctionInfo *BLUCPUTargetMachine::createMachineFunctionInfo(
    BumpPtrAllocator &Allocator, const Function &F,
    const TargetSubtargetInfo *STI) const {
  return BLUCPUMachineFunctionInfo::create<BLUCPUMachineFunctionInfo>(Allocator, F,
                                                                STI);
}

//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//

bool BLUCPUPassConfig::addInstSelector() {
  // Install an instruction selector.
  addPass(createBLUCPUISelDag(getBLUCPUTargetMachine(), getOptLevel()));
  // Create the frame analyzer pass used by the PEI pass.
  addPass(createBLUCPUFrameAnalyzerPass());

  return false;
}

void BLUCPUPassConfig::addPreSched2() {
  addPass(createBLUCPUExpandPseudoPass());
}

void BLUCPUPassConfig::addPreEmitPass() {
  // Must run branch selection immediately preceding the asm printer.
  addPass(&BranchRelaxationPassID);
}

} // end of namespace llvm
