#include "BLUCPUTargetMachine.h"
#include "MCTargetDesc/BLUCPUInstPrinter.h"
#include "TargetInfo/BLUCPUTargetInfo.h"

#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLoweringObjectFile.h"

#define DEBUG_TYPE "blucpu-asm-printer"

namespace llvm {

/// An BLUCPU assembly code printer.
class BLUCPUAsmPrinter : public AsmPrinter {
public:
  BLUCPUAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override { return "BLUCPU Assembly Printer"; }
};

} // end of namespace llvm


extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeBLUCPUAsmPrinter() {
  llvm::RegisterAsmPrinter<llvm::BLUCPUAsmPrinter> X(llvm::getTheBLUCPUTarget());
}

