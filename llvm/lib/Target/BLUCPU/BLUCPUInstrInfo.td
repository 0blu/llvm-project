// Base class for BLUCPU instructions
class BLUCPUInst<string asmstr, dag outs, dag ins>
  : Instruction
{
  let Namespace = "BLUCPU";

  let AsmString = asmstr;
  let OutOperandList = outs;
  let InOperandList  = ins;
}

// ADD reg, reg
def ADDrr : BLUCPUInst<"add $dst, $src",
                       (outs GP_REGS:$dst),
                       (ins GP_REGS:$src)>
{
  let hasSideEffects = 0;
}

// RET (no operands)
def RET : BLUCPUInst<"ret",
                     (outs),
                     (ins)>
{
  let isReturn = 1;
  let hasSideEffects = 1;
}
