
// Scratch registers, must be saved by caller
def SCRATCH_REGS : RegisterClass<"BLUCPU", [i32], 32, (add R4, R3, R2, R1, R0)>;
