// Target-independent interfaces which we are implementing
include "llvm/Target/Target.td"

include "BLUCPURegisterInfo.td"
include "BLUCPUInstrInfo.td"
def BLUCPUInstrInfo : InstrInfo;

include "BLUCPUCallingConv.td"

def BLUCPUAsmWriter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
}

// Target Declaration
def BLUCPU : Target {
  let InstructionSet = BLUCPUInstrInfo;
  let AssemblyWriters = [BLUCPUAsmWriter];
}
