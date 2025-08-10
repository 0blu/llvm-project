add_llvm_component_group(BLUCPU)

set(LLVM_TARGET_DEFINITIONS BLUCPU.td)

tablegen(LLVM BLUCPUGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM BLUCPUGenCallingConv.inc -gen-callingconv)
tablegen(LLVM BLUCPUGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM BLUCPUGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM BLUCPUGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM BLUCPUGenSubtargetInfo.inc -gen-subtarget)
add_public_tablegen_target(BLUCPUCommonTableGen)

add_llvm_target(BLUCPUCodeGen
  BLUCPUAsmPrinter.cpp
  BLUCPUFrameLowering.cpp
  BLUCPUInstrInfo.cpp
  BLUCPUISelLowering.cpp
  BLUCPURegisterInfo.cpp
  BLUCPUSubtarget.cpp
  BLUCPUTargetMachine.cpp
  BLUCPUTargetObjectFile.cpp
  BLUCPUISelDAGToDAG.cpp

  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
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

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
