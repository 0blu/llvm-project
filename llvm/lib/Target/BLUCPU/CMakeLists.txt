add_llvm_component_group(BLUCPU)

set(LLVM_TARGET_DEFINITIONS BLUCPU.td)

tablegen(LLVM BLUCPUGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM BLUCPUGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM BLUCPUGenCallingConv.inc -gen-callingconv)
tablegen(LLVM BLUCPUGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM BLUCPUGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM BLUCPUGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM BLUCPUGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM BLUCPUGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM BLUCPUGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(BLUCPUCommonTableGen)

add_llvm_target(BLUCPUCodeGen
  BLUCPUAsmPrinter.cpp
  BLUCPUExpandPseudoInsts.cpp
  BLUCPUFrameLowering.cpp
  BLUCPUInstrInfo.cpp
  BLUCPUISelDAGToDAG.cpp
  BLUCPUISelLowering.cpp
  BLUCPUMCInstLower.cpp
  BLUCPURegisterInfo.cpp
  BLUCPUShiftExpand.cpp
  BLUCPUTargetMachine.cpp
  BLUCPUTargetObjectFile.cpp

  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
  BLUCPUDesc
  BLUCPUInfo
  AsmPrinter
  CodeGen
  CodeGenTypes
  Core
  MC
  SelectionDAG
  Support
  Target

  ADD_TO_COMPONENT
  BLUCPU
  )

add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
