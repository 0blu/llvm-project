// Special return value calling convention for runtime functions.
def RetCC_BLUCPU_BUILTIN : CallingConv<[
  CCIfType<[i32], CCAssignToReg<[R1, R2]>>,
]>;

// The calling conventions are implemented in custom C++ code
// Calling convention for variadic functions.
def ArgCC_BLUCPU_Vararg : CallingConv<[
  // i8 are always passed through the stack with a byte slot and byte alignment.
  CCIfType<[i8], CCAssignToStack<1, 1>>,
  // i32 are always passed through the stack with an alignment of 4.
  CCAssignToStack<4, 4>
]>;

// Callee-saved register lists.
//def CSR_Normal : CalleeSavedRegs<(add R29, R28, (sequence "R%u", 17, 2))>;

// Scratch registers, must be saved by caller
def SCRATCH_REGS : CalleeSavedRegs<(add R4, R3, R2, R1, R0)>;
