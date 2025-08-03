add_llvm_component_library(LLVMBLUCPUDesc
    BLUCPUMCTargetDesc.cpp

    LINK_COMPONENTS
    MC
    BLUCPUInfo
    Support
    TargetParser

    ADD_TO_COMPONENT
    BLUCPU
)

add_dependencies(LLVMBLUCPUDesc BLUCPUCommonTableGen)
