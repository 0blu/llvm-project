//===-- BLUCPUMCTargetDesc.cpp - BLUCPU Target Descriptions ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides BLUCPU specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "BLUCPUMCTargetDesc.h"
#include "BLUCPUELFStreamer.h"
#include "BLUCPUInstPrinter.h"
#include "BLUCPUMCAsmInfo.h"
#include "BLUCPUMCELFStreamer.h"
#include "BLUCPUTargetStreamer.h"
#include "TargetInfo/BLUCPUTargetInfo.h"

#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "BLUCPUGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "BLUCPUGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "BLUCPUGenRegisterInfo.inc"
#include "BLUCPUMCCodeEmitter.h"

using namespace llvm;

MCInstrInfo *llvm::createBLUCPUMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitBLUCPUMCInstrInfo(X);

  return X;
}
MCCodeEmitter *llvm::createBLUCPUMCCodeEmitter(const MCInstrInfo &MCII,
                                               MCContext &Ctx) {
  return new BLUCPUMCCodeEmitter(MCII, Ctx);
}

static MCRegisterInfo *createBLUCPUMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitBLUCPUMCRegisterInfo(X, 0);

  return X;
}

static MCSubtargetInfo *createBLUCPUMCSubtargetInfo(const Triple &TT,
                                                 StringRef CPU, StringRef FS) {
  return createBLUCPUMCSubtargetInfoImpl(TT, CPU, /*TuneCPU*/ CPU, FS);
}

static MCInstPrinter *createBLUCPUMCInstPrinter(const Triple &T,
                                             unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0) {
    return new BLUCPUInstPrinter(MAI, MII, MRI);
  }

  return nullptr;
}

static MCStreamer *createMCStreamer(const Triple &T, MCContext &Context,
                                    std::unique_ptr<MCAsmBackend> &&MAB,
                                    std::unique_ptr<MCObjectWriter> &&OW,
                                    std::unique_ptr<MCCodeEmitter> &&Emitter) {
  return createELFStreamer(Context, std::move(MAB), std::move(OW),
                           std::move(Emitter));
}

static MCTargetStreamer *
createBLUCPUObjectTargetStreamer(MCStreamer &S, const MCSubtargetInfo &STI) {
  return new BLUCPUELFStreamer(S, STI);
}

static MCTargetStreamer *createMCAsmTargetStreamer(MCStreamer &S,
                                                   formatted_raw_ostream &OS,
                                                   MCInstPrinter *InstPrint) {
  return new BLUCPUTargetAsmStreamer(S);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeBLUCPUTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfo<BLUCPUMCAsmInfo> X(getTheBLUCPUTarget());

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(getTheBLUCPUTarget(), createBLUCPUMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(getTheBLUCPUTarget(), createBLUCPUMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(getTheBLUCPUTarget(),
                                          createBLUCPUMCSubtargetInfo);

  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(getTheBLUCPUTarget(),
                                        createBLUCPUMCInstPrinter);

  // Register the MC Code Emitter
  TargetRegistry::RegisterMCCodeEmitter(getTheBLUCPUTarget(),
                                        createBLUCPUMCCodeEmitter);

  // Register the obj streamer
  TargetRegistry::RegisterELFStreamer(getTheBLUCPUTarget(), createMCStreamer);

  // Register the obj target streamer.
  TargetRegistry::RegisterObjectTargetStreamer(getTheBLUCPUTarget(),
                                               createBLUCPUObjectTargetStreamer);

  // Register the asm target streamer.
  TargetRegistry::RegisterAsmTargetStreamer(getTheBLUCPUTarget(),
                                            createMCAsmTargetStreamer);

  // Register the asm backend (as little endian).
  TargetRegistry::RegisterMCAsmBackend(getTheBLUCPUTarget(), createBLUCPUAsmBackend);
}
