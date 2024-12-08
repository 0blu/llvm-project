add_llvm_component_library(LLVMBLUCPUDisassembler
  BLUCPUDisassembler.cpp

  LINK_COMPONENTS
  BLUCPUInfo
  CodeGenTypes
  MC
  MCDisassembler
  Support

  ADD_TO_COMPONENT
  BLUCPU
  )

