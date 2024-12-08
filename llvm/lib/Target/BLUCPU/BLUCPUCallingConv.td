//===-- BLUCPUCallingConv.td - Calling Conventions for BLUCPU ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for BLUCPU architecture.
// Normal functions use a special calling convention, solved in code.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// BLUCPU Return Value Calling Convention
//===----------------------------------------------------------------------===//

// Special return value calling convention for runtime functions.
//def RetCC_BLUCPU_BUILTIN : CallingConv<[
//  CCIfType<[i8], CCAssignToReg<[R24, R25]>>,
//  CCIfType<[i16], CCAssignToReg<[R23R22, R25R24]>>
//]>;

//===----------------------------------------------------------------------===//
// BLUCPU Argument Calling Conventions
//===----------------------------------------------------------------------===//

// The calling conventions are implemented in custom C++ code

// Calling convention for variadic functions.
def ArgCC_BLUCPU_Vararg : CallingConv<[
  // VarArgs always passed by stack.
  CCAssignToStack<2, 1>
]>;

//===----------------------------------------------------------------------===//
// Callee-saved register lists.
//===----------------------------------------------------------------------===//

def CSR_Normal : CalleeSavedRegs<(add R9, R8, R7, R6, R5)>;
def CSR_NoRegs : CalleeSavedRegs<(add)>;