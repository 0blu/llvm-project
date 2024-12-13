//===-- BLUCPUInstrInfo.td - BLUCPU Instruction defs -------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the BLUCPU instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

include "BLUCPUInstrFormats.td"

//===----------------------------------------------------------------------===//
// BLUCPU Type Profiles
//===----------------------------------------------------------------------===//

def SDT_BLUCPUCallSeqStart : SDCallSeqStart<[SDTCisVT<0, i16>, SDTCisVT<1, i16>]>;
def SDT_BLUCPUCallSeqEnd : SDCallSeqEnd<[SDTCisVT<0, i16>, SDTCisVT<1, i16>]>;
def SDT_BLUCPUCall : SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;
def SDT_BLUCPUWrapper : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;
def SDT_BLUCPUBrcond
    : SDTypeProfile<0, 2, [SDTCisVT<0, OtherVT>, SDTCisVT<1, i8>]>;
def SDT_BLUCPUCmp : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def SDT_BLUCPUTst : SDTypeProfile<0, 1, [SDTCisInt<0>]>;
def SDT_BLUCPUSelectCC
    : SDTypeProfile<1, 3,
                    [SDTCisSameAs<0, 1>, SDTCisSameAs<1, 2>, SDTCisVT<3, i8>]>;

//===----------------------------------------------------------------------===//
// BLUCPU Specific Node Definitions
//===----------------------------------------------------------------------===//

def BLUCPUretglue : SDNode<"BLUCPUISD::RET_GLUE", SDTNone,
                        [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def BLUCPUretiglue : SDNode<"BLUCPUISD::RETI_GLUE", SDTNone,
                         [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def BLUCPUcallseq_start : SDNode<"ISD::CALLSEQ_START", SDT_BLUCPUCallSeqStart,
                              [SDNPHasChain, SDNPOutGlue]>;
def BLUCPUcallseq_end : SDNode<"ISD::CALLSEQ_END", SDT_BLUCPUCallSeqEnd,
                            [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def BLUCPUcall : SDNode<"BLUCPUISD::CALL", SDT_BLUCPUCall,
                     [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue, SDNPVariadic]>;

def BLUCPUWrapper : SDNode<"BLUCPUISD::WRAPPER", SDT_BLUCPUWrapper>;

def BLUCPUbrcond
    : SDNode<"BLUCPUISD::BRCOND", SDT_BLUCPUBrcond, [SDNPHasChain, SDNPInGlue]>;
def BLUCPUcmp : SDNode<"BLUCPUISD::CMP", SDT_BLUCPUCmp, [SDNPOutGlue]>;
def BLUCPUcmpc : SDNode<"BLUCPUISD::CMPC", SDT_BLUCPUCmp, [SDNPInGlue, SDNPOutGlue]>;
def BLUCPUtst : SDNode<"BLUCPUISD::TST", SDT_BLUCPUTst, [SDNPOutGlue]>;
def BLUCPUselectcc : SDNode<"BLUCPUISD::SELECT_CC", SDT_BLUCPUSelectCC, [SDNPInGlue]>;

// Shift nodes.
def BLUCPUlsl : SDNode<"BLUCPUISD::LSL", SDTIntUnaryOp>;
def BLUCPUlsr : SDNode<"BLUCPUISD::LSR", SDTIntUnaryOp>;
def BLUCPUrol : SDNode<"BLUCPUISD::ROL", SDTIntUnaryOp>;
def BLUCPUror : SDNode<"BLUCPUISD::ROR", SDTIntUnaryOp>;
def BLUCPUasr : SDNode<"BLUCPUISD::ASR", SDTIntUnaryOp>;
def BLUCPUlslhi : SDNode<"BLUCPUISD::LSLHI", SDTIntUnaryOp>;
def BLUCPUlsrlo : SDNode<"BLUCPUISD::LSRLO", SDTIntUnaryOp>;
def BLUCPUasrlo : SDNode<"BLUCPUISD::ASRLO", SDTIntUnaryOp>;
def BLUCPUlslbn : SDNode<"BLUCPUISD::LSLBN", SDTIntBinOp>;
def BLUCPUlsrbn : SDNode<"BLUCPUISD::LSRBN", SDTIntBinOp>;
def BLUCPUasrbn : SDNode<"BLUCPUISD::ASRBN", SDTIntBinOp>;
def BLUCPUlslwn : SDNode<"BLUCPUISD::LSLWN", SDTIntBinOp>;
def BLUCPUlsrwn : SDNode<"BLUCPUISD::LSRWN", SDTIntBinOp>;
def BLUCPUasrwn : SDNode<"BLUCPUISD::ASRWN", SDTIntBinOp>;
def BLUCPUlslw : SDNode<"BLUCPUISD::LSLW", SDTIntShiftDOp>;
def BLUCPUlsrw : SDNode<"BLUCPUISD::LSRW", SDTIntShiftDOp>;
def BLUCPUasrw : SDNode<"BLUCPUISD::ASRW", SDTIntShiftDOp>;

// Pseudo shift nodes for non-constant shift amounts.
def BLUCPUlslLoop : SDNode<"BLUCPUISD::LSLLOOP", SDTIntShiftOp>;
def BLUCPUlsrLoop : SDNode<"BLUCPUISD::LSRLOOP", SDTIntShiftOp>;
def BLUCPUrolLoop : SDNode<"BLUCPUISD::ROLLOOP", SDTIntShiftOp>;
def BLUCPUrorLoop : SDNode<"BLUCPUISD::RORLOOP", SDTIntShiftOp>;
def BLUCPUasrLoop : SDNode<"BLUCPUISD::ASRLOOP", SDTIntShiftOp>;

// SWAP node.
def BLUCPUSwap : SDNode<"BLUCPUISD::SWAP", SDTIntUnaryOp>;

//===----------------------------------------------------------------------===//
// BLUCPU Operands, Complex Patterns and Transformations Definitions.
//===----------------------------------------------------------------------===//

def imm8_neg_XFORM : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(-N->getAPIntValue(), SDLoc(N), MVT::i8);
}]>;

def imm16_neg_XFORM : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(-N->getAPIntValue(), SDLoc(N), MVT::i16);
}]>;

def imm0_63_neg : PatLeaf<(imm), [{
  int64_t val = -N->getSExtValue();
  return val >= 0 && val < 64;
}], imm16_neg_XFORM>;

def uimm6 : PatLeaf<(imm), [{ return isUInt<6>(N->getZExtValue()); }]>;

// imm_com8_XFORM - Return the complement of a imm_com8 value
def imm_com8_XFORM : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(
      ~((uint8_t) N->getZExtValue()), SDLoc(N), MVT::i8);
}]>;

// imm_com8 - Match an immediate that is a complement
// of a 8-bit immediate.
// Note: this pattern doesn't require an encoder method and such, as it's
// only used on aliases (Pat<> and InstAlias<>). The actual encoding
// is handled by the destination instructions, which use imm_com8.
def imm_com8_asmoperand : AsmOperandClass { let Name = "ImmCom8"; }
def imm_com8 : Operand<i8> { let ParserMatchClass = imm_com8_asmoperand; }

def ioaddr_XFORM : SDNodeXForm<imm, [{
  uint8_t offset = Subtarget->getIORegisterOffset();
  return CurDAG->getTargetConstant(
      uint8_t(N->getZExtValue()) - offset, SDLoc(N), MVT::i8);
}]>;

def iobitpos8_XFORM : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(
      Log2_32(uint8_t(N->getZExtValue())), SDLoc(N), MVT::i8);
}]>;

def iobitposn8_XFORM : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(
      Log2_32(uint8_t(~N->getZExtValue())), SDLoc(N), MVT::i8);
}]>;

def ioaddr8 : PatLeaf<(imm), [{
  uint8_t offset = Subtarget->getIORegisterOffset();
  uint64_t val = N->getZExtValue() - offset;
  return val < 0x40;
}], ioaddr_XFORM>;

def lowioaddr8 : PatLeaf<(imm), [{
  uint8_t offset = Subtarget->getIORegisterOffset();
  uint64_t val = N->getZExtValue() - offset;
  return val < 0x20;
}], ioaddr_XFORM>;

def ioaddr16 : PatLeaf<(imm), [{
  uint8_t offset = Subtarget->getIORegisterOffset();
  uint64_t val = N->getZExtValue() - offset;
  return val < 0x3f;
}], ioaddr_XFORM>;

def iobitpos8 : PatLeaf<(imm), [{
  return isPowerOf2_32(uint8_t(N->getZExtValue()));
}], iobitpos8_XFORM>;

def iobitposn8 : PatLeaf<(imm), [{
  return isPowerOf2_32(uint8_t(~N->getZExtValue()));
}], iobitposn8_XFORM>;

def MemriAsmOperand : AsmOperandClass {
  let Name = "Memri";
  let ParserMethod = "parseMemriOperand";
}

/// Address operand for `reg+imm` used by STD and LDD.
def memri : Operand<iPTR> {
  let MIOperandInfo = (ops DREGS, i16imm);

  let PrintMethod = "printMemri";
  let EncoderMethod = "encodeMemri";
  let DecoderMethod = "decodeMemri";

  let ParserMatchClass = MemriAsmOperand;
}

// Address operand for `SP+imm` used by STD{W}SPQRr
def memspi : Operand<iPTR> {
  let MIOperandInfo = (ops GPRSP, i16imm);
  let PrintMethod = "printMemspi";
}

def relbrtarget_7 : Operand<OtherVT> {
  let PrintMethod = "printPCRelImm";
  let EncoderMethod = "encodeRelCondBrTarget<BLUCPU::fixup_7_pcrel>";
}

def brtarget_13 : Operand<OtherVT> {
  let PrintMethod = "printPCRelImm";
  let EncoderMethod = "encodeRelCondBrTarget<BLUCPU::fixup_13_pcrel>";
}

def rcalltarget_13 : Operand<i16> {
  let PrintMethod = "printPCRelImm";
  let EncoderMethod = "encodeRelCondBrTarget<BLUCPU::fixup_13_pcrel>";
}

// The target of a 22 or 16-bit call/jmp instruction.
def call_target : Operand<iPTR> {
  let EncoderMethod = "encodeCallTarget";
  let DecoderMethod = "decodeCallTarget";
}

// A 16-bit address (which can lead to an R_BLUCPU_16 relocation).
def imm16 : Operand<i16> { let EncoderMethod = "encodeImm<BLUCPU::fixup_16, 2>"; }

// A 7-bit address (which can lead to an R_BLUCPU_LDS_STS_16 relocation).
def imm7tiny : Operand<i16> {
  let EncoderMethod = "encodeImm<BLUCPU::fixup_lds_sts_16, 0>";
}

/// A 6-bit immediate used in the ADIW/SBIW instructions.
def imm_arith6 : Operand<i16> {
  let EncoderMethod = "encodeImm<BLUCPU::fixup_6_adiw, 0>";
}

/// An 8-bit immediate inside an instruction with the same format
/// as the `LDI` instruction (the `FRdK` format).
def imm_ldi8 : Operand<i8> {
  let EncoderMethod = "encodeImm<BLUCPU::fixup_ldi, 0>";
}

/// A 5-bit port number used in SBIC and friends (the `FIOBIT` format).
def imm_port5 : Operand<i8> {
  let EncoderMethod = "encodeImm<BLUCPU::fixup_port5, 0>";
}

/// A 6-bit port number used in the `IN` instruction and friends (the
/// `FIORdA` format.
def imm_port6 : Operand<i8> {
  let EncoderMethod = "encodeImm<BLUCPU::fixup_port6, 0>";
}

// Addressing mode pattern reg+imm6
def addr : ComplexPattern<iPTR, 2, "SelectAddr", [], [SDNPWantRoot]>;

// AsmOperand class for a pointer register.
// Used with the LD/ST family of instructions.
// See FSTLD in BLUCPUInstrFormats.td
def PtrRegAsmOperand : AsmOperandClass { let Name = "Reg"; }

// A special operand type for the LD/ST instructions.
// It converts the pointer register number into a two-bit field used in the
// instruction.
def LDSTPtrReg : Operand<i16> {
  let MIOperandInfo = (ops DREGS);
  let EncoderMethod = "encodeLDSTPtrReg";

  let ParserMatchClass = PtrRegAsmOperand;
}

// A special operand type for the LDD/STD instructions.
// It behaves identically to the LD/ST version, except restricts
// the pointer registers to Y and Z.
def LDDSTDPtrReg : Operand<i16> {
  let MIOperandInfo = (ops DREGS);
  let EncoderMethod = "encodeLDSTPtrReg";

  let ParserMatchClass = PtrRegAsmOperand;
}

//===----------------------------------------------------------------------===//
// BLUCPU predicates for subtarget features
//===----------------------------------------------------------------------===//

def HasSRAM : Predicate<"Subtarget->hasSRAM()">,
              AssemblerPredicate<(all_of FeatureSRAM)>;

def HasJMPCALL : Predicate<"Subtarget->hasJMPCALL()">,
                 AssemblerPredicate<(all_of FeatureJMPCALL)>;

def HasIJMPCALL : Predicate<"Subtarget->hasIJMPCALL()">,
                  AssemblerPredicate<(all_of FeatureIJMPCALL)>;

def HasEIJMPCALL : Predicate<"Subtarget->hasEIJMPCALL()">,
                   AssemblerPredicate<(all_of FeatureEIJMPCALL)>;

def HasADDSUBIW : Predicate<"Subtarget->hasADDSUBIW()">,
                  AssemblerPredicate<(all_of FeatureADDSUBIW)>;

def HasSmallStack : Predicate<"Subtarget->HasSmallStack()">,
                    AssemblerPredicate<(all_of FeatureSmallStack)>;

def HasMOVW : Predicate<"Subtarget->hasMOVW()">,
              AssemblerPredicate<(all_of FeatureMOVW)>;

def HasLPM : Predicate<"Subtarget->hasLPM()">,
             AssemblerPredicate<(all_of FeatureLPM)>;

def HasLPMX : Predicate<"Subtarget->hasLPMX()">,
              AssemblerPredicate<(all_of FeatureLPMX)>;

def HasELPM : Predicate<"Subtarget->hasELPM()">,
              AssemblerPredicate<(all_of FeatureELPM)>;

def HasELPMX : Predicate<"Subtarget->hasELPMX()">,
               AssemblerPredicate<(all_of FeatureELPMX)>;

def HasSPM : Predicate<"Subtarget->hasSPM()">,
             AssemblerPredicate<(all_of FeatureSPM)>;

def HasSPMX : Predicate<"Subtarget->hasSPMX()">,
              AssemblerPredicate<(all_of FeatureSPMX)>;

def HasDES : Predicate<"Subtarget->hasDES()">,
             AssemblerPredicate<(all_of FeatureDES)>;

def SupportsRMW : Predicate<"Subtarget->supportsRMW()">,
                  AssemblerPredicate<(all_of FeatureRMW)>;

def SupportsMultiplication : Predicate<"Subtarget->supportsMultiplication()">,
                             AssemblerPredicate<(all_of FeatureMultiplication)>;

def HasBREAK : Predicate<"Subtarget->hasBREAK()">,
               AssemblerPredicate<(all_of FeatureBREAK)>;

def HasTinyEncoding : Predicate<"Subtarget->hasTinyEncoding()">,
                      AssemblerPredicate<(all_of FeatureTinyEncoding)>;

def HasNonTinyEncoding : Predicate<"!Subtarget->hasTinyEncoding()">,
                         AssemblerPredicate<(any_of (not FeatureTinyEncoding))>;

// BLUCPU specific condition code. These correspond to BLUCPU_*_COND in
// BLUCPUInstrInfo.td. They must be kept in synch.
def BLUCPU_COND_EQ : PatLeaf<(i8 0)>;
def BLUCPU_COND_NE : PatLeaf<(i8 1)>;
def BLUCPU_COND_GE : PatLeaf<(i8 2)>;
def BLUCPU_COND_LT : PatLeaf<(i8 3)>;
def BLUCPU_COND_SH : PatLeaf<(i8 4)>;
def BLUCPU_COND_LO : PatLeaf<(i8 5)>;
def BLUCPU_COND_MI : PatLeaf<(i8 6)>;
def BLUCPU_COND_PL : PatLeaf<(i8 7)>;

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// BLUCPU Instruction list
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

// ADJCALLSTACKDOWN/UP implicitly use/def SP because they may be expanded into
// a stack adjustment and the codegen must know that they may modify the stack
// pointer before prolog-epilog rewriting occurs.
// Pessimistically assume ADJCALLSTACKDOWN / ADJCALLSTACKUP will become
// sub / add which can clobber SREG.
//let Defs = [SP], Uses = [SP] in {
  // def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i16imm:$amt, i16imm:$amt2),
  //                               "#ADJCALLSTACKDOWN",
  //                               [(BLUCPUcallseq_start timm:$amt, timm:$amt2)]>;
//
  // // R31R30 is used to update SP. It is normally free because it is a
  // // call-clobbered register but it is necessary to set it as a def as the
  // // register allocator might use it in rare cases (for rematerialization, it
  // // seems). hasSideEffects needs to be set to true so this instruction isn't
  // // considered dead.
  // let Defs = [R31R30], hasSideEffects = 1 in def ADJCALLSTACKUP
  //     : Pseudo<(outs), (ins i16imm:$amt1, i16imm:$amt2),
  //              "#ADJCALLSTACKUP", [(BLUCPUcallseq_end timm:$amt1, timm:$amt2)]>;
//}

//===----------------------------------------------------------------------===//
// Addition
//===----------------------------------------------------------------------===//
let isCommutable = 1, Constraints = "$src = $rd" in {
  // ADD Rd, Rr
  // Adds two 8-bit registers.
  def ADDRdRr : FRdRr<0b0000, 0b11, (outs DREGS:$rd),(ins DREGS:$src, DREGS:$rr),
                      "add\t$rd, $rr",
                      [(set i32:$rd, (add i32:$src, i32:$rr))]>;

}

//===----------------------------------------------------------------------===//
// Subtraction
//===----------------------------------------------------------------------===//
let Constraints = "$rs = $rd" in {
  // SUB Rd, Rr
  // Subtracts the 8-bit value of Rr from Rd and places the value in Rd.
  def SUBRdRr : FRdRr<0b0001, 0b10, (outs GPR8:$rd), (ins GPR8:$rs, GPR8:$rr),
                      "sub\t$rd, $rr",
                      [(set i32:$rd, (sub i32:$rs, i32:$rr))]>;

}
