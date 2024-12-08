#include "BLUCPUELFStreamer.h"

#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/TargetParser/SubtargetFeature.h"

#include "BLUCPUMCTargetDesc.h"

namespace llvm {

static unsigned getEFlagsForFeatureSet(const FeatureBitset &Features) {
  unsigned EFlags = 0;

  return EFlags;
}

BLUCPUELFStreamer::BLUCPUELFStreamer(MCStreamer &S, const MCSubtargetInfo &STI)
    : BLUCPUTargetStreamer(S) {
  ELFObjectWriter &W = getStreamer().getWriter();
  unsigned EFlags = W.getELFHeaderEFlags();

  EFlags |= getEFlagsForFeatureSet(STI.getFeatureBits());
  //EFlags |= ELF::EF_BLUCPU_LINKRELAX_PREPARED;

  W.setELFHeaderEFlags(EFlags);
}

} // end namespace llvm
