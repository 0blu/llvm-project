#ifndef LLVM_BLUCPU_ISEL_LOWERING_H
#define LLVM_BLUCPU_ISEL_LOWERING_H

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class BLUCPUTargetMachine;
class BLUCPUSubtarget;

class BLUCPUTargetLowering : public TargetLowering {
public:
  explicit BLUCPUTargetLowering(const BLUCPUTargetMachine &TM, const BLUCPUSubtarget &STI);

  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF, bool isVarArg, const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context, const Type *RetTy) const override;
  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg, const SmallVectorImpl<ISD::OutputArg> &Outs, const SmallVectorImpl<SDValue> &OutVals, const SDLoc &dl, SelectionDAG &DAG) const override;
  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv, bool isVarArg, const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &dl, SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI, SmallVectorImpl<SDValue> &InVals) const override;

protected:
  const BLUCPUSubtarget& Subtarget;

};

} // end of llvm namespace


#endif // LLVM_BLUCPU_ISEL_LOWERING_H
