#include "BLUCPUISelLowering.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"

#include "BLUCPU.h"
#include "BLUCPUMachineFunctionInfo.h"
#include "BLUCPUSubtarget.h"
#include "BLUCPUTargetMachine.h"
#include "MCTargetDesc/BLUCPUMCTargetDesc.h"

namespace llvm {

#include "BLUCPUGenCallingConv.inc"

static const MCPhysReg RegListGeneral[] = {
  BLUCPU::R9,
  BLUCPU::R8,
  BLUCPU::R7,
  BLUCPU::R6,
  BLUCPU::R5,
  BLUCPU::R4,
  BLUCPU::R3,
  BLUCPU::R2,
  BLUCPU::R1,
  BLUCPU::R0,
};

/// Count the total number of bytes needed to pass or return these arguments.
template <typename ArgT>
static unsigned
getTotalArgumentsSizeInBytes(const SmallVectorImpl<ArgT> &Args) {
  unsigned TotalBytes = 0;

  for (const ArgT &Arg : Args) {
    TotalBytes += Arg.VT.getStoreSize();
  }
  return TotalBytes;
}

/// Analyze incoming and outgoing value of returning from a function.
/// The algorithm is similar to analyzeArguments, but there can only be
/// one value, possibly an aggregate, and it is limited to 8 bytes.
template <typename ArgT>
static void analyzeReturnValues(const SmallVectorImpl<ArgT> &Args, CCState &CCInfo) {
  unsigned NumArgs = Args.size();
  unsigned TotalBytes = getTotalArgumentsSizeInBytes(Args);
  // CanLowerReturn() guarantees this assertion.

  // Choose the proper register list for argument passing according to the ABI.
  ArrayRef<MCPhysReg> RegList8 = RegListGeneral;

  // GCC-ABI says that the size is rounded up to the next even number,
  // but actually once it is more than 4 it will always round up to 8.
  if (TotalBytes > 4) {
    TotalBytes = 8;
  } else {
    TotalBytes = alignTo(TotalBytes, 4);
  }

  // The index of the first register to use.
  int RegIdx = TotalBytes - 1;
  for (unsigned i = 0; i != NumArgs; ++i) {
    MVT VT = Args[i].VT;
    unsigned Reg;
    if (VT == MVT::i32) {
      Reg = CCInfo.AllocateReg(RegList8[RegIdx]);
    } else {
      llvm_unreachable("calling convention can only manage i32 types");
    }
    assert(Reg && "register not available in calling convention");
    CCInfo.addLoc(CCValAssign::getReg(i, VT, Reg, VT, CCValAssign::Full));
    // Registers sort in increasing order
    RegIdx -= VT.getStoreSize();
  }
}

/// Analyze incoming and outgoing function arguments. We need custom C++ code
/// to handle special constraints in the ABI.
/// In addition, all pieces of a certain argument have to be passed either
/// using registers or the stack but never mixing both.
template <typename ArgT>
static void analyzeArguments(TargetLowering::CallLoweringInfo *CLI, const Function *F, const DataLayout *TD, const SmallVectorImpl<ArgT> &Args, SmallVectorImpl<CCValAssign> &ArgLocs, CCState &CCInfo) {
  // Choose the proper register list for argument passing according to the ABI.
  ArrayRef<MCPhysReg> RegList8 = RegListGeneral;

  unsigned NumArgs = Args.size();
  // This is the index of the last used register, in RegList*.
  // -1 means R26 (R26 is never actually used in CC).
  int RegLastIdx = -1;
  // Once a value is passed to the stack it will always be used
  bool UseStack = false;
  for (unsigned i = 0; i != NumArgs;) {
    MVT VT = Args[i].VT;
    // We have to count the number of bytes for each function argument, that is
    // those Args with the same OrigArgIndex. This is important in case the
    // function takes an aggregate type.
    // Current argument will be between [i..j).
    unsigned ArgIndex = Args[i].OrigArgIndex;
    unsigned TotalBytes = VT.getStoreSize();
    unsigned j = i + 1;
    for (; j != NumArgs; ++j) {
      if (Args[j].OrigArgIndex != ArgIndex)
        break;
      TotalBytes += Args[j].VT.getStoreSize();
    }
    // Round up to even number of bytes.
    TotalBytes = alignTo(TotalBytes, 4);
    // Skip zero sized arguments
    if (TotalBytes == 0)
      continue;
    // The index of the first register to be used
    unsigned RegIdx = RegLastIdx + TotalBytes;
    RegLastIdx = RegIdx;
    // If there are not enough registers, use the stack
    if (RegIdx >= RegList8.size()) {
      UseStack = true;
    }
    for (; i != j; ++i) {
      MVT VT = Args[i].VT;

      if (UseStack) {
        auto evt = EVT(VT).getTypeForEVT(CCInfo.getContext());
        unsigned Offset = CCInfo.AllocateStack(TD->getTypeAllocSize(evt),
                                               TD->getABITypeAlign(evt));
        CCInfo.addLoc(
            CCValAssign::getMem(i, VT, Offset, VT, CCValAssign::Full));
      } else {
        unsigned Reg;
        if (VT == MVT::i32) {
          Reg = CCInfo.AllocateReg(RegList8[RegIdx]);
        } else {
          llvm_unreachable("calling convention can only manage i32 types");
        }
        assert(Reg && "register not available in calling convention");
        CCInfo.addLoc(CCValAssign::getReg(i, VT, Reg, VT, CCValAssign::Full));
        // Registers inside a particular argument are sorted in increasing order
        // (remember the array is reversed).
        RegIdx -= VT.getStoreSize();
      }
    }
  }
}

BLUCPUTargetLowering::BLUCPUTargetLowering(const BLUCPUTargetMachine &TM,
                                           const BLUCPUSubtarget &STI)
    : TargetLowering(TM), Subtarget(STI) {
  addRegisterClass(MVT::i32, &BLUCPU::GP_REGSRegClass);

  // Compute derived properties from the register classes.
  computeRegisterProperties(Subtarget.getRegisterInfo());

  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);
  setSchedulingPreference(Sched::RegPressure);
  setStackPointerRegisterToSaveRestore(BLUCPU::SP);
  setSupportsUnalignedAtomics(true);

  // setLibcallCallingConv(RTLIB::SDIVREM_I8, CallingConv::C);
  // setLibcallCallingConv(RTLIB::SDIVREM_I16, CallingConv::BLUCPU_BUILTIN);
  // setLibcallCallingConv(RTLIB::UDIVREM_I8, CallingConv::BLUCPU_BUILTIN);
  // setLibcallCallingConv(RTLIB::UDIVREM_I16, CallingConv::BLUCPU_BUILTIN);
}

bool BLUCPUTargetLowering::CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF, bool isVarArg, const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context, const Type *RetTy) const {
  return false;
}

SDValue BLUCPUTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg, const SmallVectorImpl<ISD::OutputArg> &Outs, const SmallVectorImpl<SDValue> &OutVals, const SDLoc &dl, SelectionDAG &DAG) const {
  DAG.dump();

  // CCValAssign - represent the assignment of the return value to locations.
  SmallVector<CCValAssign, 32> RVLocs;

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());

  MachineFunction &MF = DAG.getMachineFunction();

  // Analyze return values.
  if (CallConv == CallingConv::C) {
    CCInfo.AnalyzeReturn(Outs, RetCC_BLUCPU_BUILTIN);
  } else {
    analyzeReturnValues(Outs, CCInfo);
  }

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  // Copy the result values into the output registers.
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");

    Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(), OutVals[i], Glue);

    // Guarantee that all emitted copies are stuck together with flags.
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  // Don't emit the ret/reti instruction when the naked attribute is present in
  // the function being compiled.
  if (MF.getFunction().getAttributes().hasFnAttr(Attribute::Naked)) {
    return Chain;
  }

  const BLUCPUMachineFunctionInfo *AFI = MF.getInfo<BLUCPUMachineFunctionInfo>();

  // if (!AFI->isInterruptOrSignalHandler()) {
  //   // The return instruction has an implicit zero register operand: it must
  //   // contain zero on return.
  //   // This is not needed in interrupts however, where the zero register is
  //   // handled specially (only pushed/popped when needed).
  //   RetOps.push_back(DAG.getRegister(Subtarget.getZeroRegister(), MVT::i8));
  // }

  unsigned RetOpc = BLUCPU::RET;// BLUCPU::RETI_GLUE; // AFI->isInterruptOrSignalHandler() ? BLUCPUISD::RETI_GLUE : BLUCPUISD::RET_GLUE;

  RetOps[0] = Chain; // Update chain.

  if (Glue.getNode()) {
    RetOps.push_back(Glue);
  }

  return DAG.getNode(RetOpc, dl, MVT::Other, RetOps);
}

SDValue BLUCPUTargetLowering::LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv, bool isVarArg, const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &dl, SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  DAG.dump();

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  auto DL = DAG.getDataLayout();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(), ArgLocs, *DAG.getContext());

  // Variadic functions do not need all the analysis below.
  if (isVarArg) {
    CCInfo.AnalyzeFormalArguments(Ins, ArgCC_BLUCPU_Vararg);
  } else {
    analyzeArguments(nullptr, &MF.getFunction(), &DL, Ins, ArgLocs, CCInfo);
  }

  SDValue ArgValue;
  for (CCValAssign &VA : ArgLocs) {
    // Arguments stored on registers.
    if (VA.isRegLoc()) {

      EVT RegVT = VA.getLocVT();
      const TargetRegisterClass *RC;
      if (RegVT == MVT::i32) {
        RC = &BLUCPU::GP_REGSRegClass;
      } else {
        llvm_unreachable("Unknown argument type!");
      }

      Register Reg = MF.addLiveIn(VA.getLocReg(), RC);
      ArgValue = DAG.getCopyFromReg(Chain, dl, Reg, RegVT);

      switch (VA.getLocInfo()) {
        case CCValAssign::Full:
          break;
        default:
          llvm_unreachable("Unknown loc info!");
      }

      InVals.push_back(ArgValue);
    } else {
      llvm_unreachable("TODO non isRegLoc");
    }
  }

  return Chain;
}

SDValue BLUCPUTargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI, SmallVectorImpl<SDValue> &InVals) const {
  return TargetLowering::LowerCall(CLI, InVals);
}

} // end of namespace llvm
