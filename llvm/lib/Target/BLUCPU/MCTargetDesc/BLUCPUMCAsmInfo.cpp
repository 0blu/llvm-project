#include "BLUCPUMCAsmInfo.h"

llvm::BLUCPUMCAsmInfo::BLUCPUMCAsmInfo(const Triple& TT, const MCTargetOptions& Options) {
  CodePointerSize = 2;
  CalleeSaveStackSlotSize = 2;
  CommentString = ";";
  SeparatorString = "$";
  PrivateGlobalPrefix = ".L";
  PrivateLabelPrefix = ".L";
  UsesELFSectionDirectiveForBSS = true;
  SupportsDebugInformation = true;
}
