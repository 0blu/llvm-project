add_llvm_component_library(LLVMBLUCPUDesc
  BLUCPUAsmBackend.cpp
  BLUCPUELFObjectWriter.cpp
  BLUCPUELFStreamer.cpp
  BLUCPUInstPrinter.cpp
  BLUCPUMCAsmInfo.cpp
  BLUCPUMCCodeEmitter.cpp
  BLUCPUMCELFStreamer.cpp
  BLUCPUMCExpr.cpp
  BLUCPUMCTargetDesc.cpp
  BLUCPUTargetStreamer.cpp

  LINK_COMPONENTS
  MC
  BLUCPUInfo
  Support
  TargetParser

  ADD_TO_COMPONENT
  BLUCPU
)

add_dependencies(LLVMBLUCPUDesc BLUCPUCommonTableGen)

