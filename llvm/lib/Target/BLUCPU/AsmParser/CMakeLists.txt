add_llvm_component_library(LLVMBLUCPUAsmParser
  BLUCPUAsmParser.cpp

  LINK_COMPONENTS
  BLUCPUDesc
  BLUCPUInfo
  CodeGenTypes
  MC
  MCParser
  Support

  ADD_TO_COMPONENT
  BLUCPU
  )
