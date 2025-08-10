
#ifndef LLVM_BLUCPU_H
#define LLVM_BLUCPU_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class BLUCPUTargetMachine;
class FunctionPass;
class PassRegistry;

Pass *createBLUCPUShiftExpandPass();
FunctionPass *createBLUCPUISelDag(BLUCPUTargetMachine &TM, CodeGenOptLevel OptLevel);
FunctionPass *createBLUCPUExpandPseudoPass();
FunctionPass *createBLUCPUFrameAnalyzerPass();
FunctionPass *createBLUCPUBranchSelectionPass();

void initializeBLUCPUDAGToDAGISelLegacyPass(PassRegistry &);
void initializeBLUCPUExpandPseudoPass(PassRegistry &);
void initializeBLUCPUShiftExpandPass(PassRegistry &);

/// Contains the BLUCPU backend.
namespace BLUCPU {


} // end of namespace BLUCPU

} // end namespace llvm

#endif // LLVM_BLUCPU_H
