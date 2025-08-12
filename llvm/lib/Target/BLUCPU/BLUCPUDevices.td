class FeatureSet<string name, string desc, list<SubtargetFeature> i>
    : SubtargetFeature<name, "HasFeatureSet"#NAME, "true", desc, i>;

class Family<string name, list<SubtargetFeature> i>
    : FeatureSet<name, !strconcat("The device is a part of the ", name, " family"), i>;

class ELFArch<string name>
    : SubtargetFeature<"", "ELFArch", !strconcat("ELF::", name), "">;

class Device<string Name, Family Fam, ELFArch Arch,
             list<SubtargetFeature> ExtraFeatures = []>
    : Processor<Name, NoItineraries, !listconcat([Fam, Arch], ExtraFeatures)>;

def FeatureDummyAlways : SubtargetFeature<"dummyfeature", "HasDummyAlways", "true", "Just a dummy feature for use in AssemblerPredicate">;

def FamilyBLUCPU0 : Family<"blucpu0", [FeatureDummyAlways]>;
def ELFArchBLUCPU0 : ELFArch<"EF_AVR_ARCH_AVR1">;
def : Device<"blucpu", FamilyBLUCPU0, ELFArchBLUCPU0>;
