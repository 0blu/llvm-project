//===---------------------------------------------------------------------===//
// BLUCPU Device Definitions
//===---------------------------------------------------------------------===//

// :TODO: Implement the skip errata, see `gcc/config/blucpu/blucpu-arch.h` for details
// :TODO: We define all devices with SRAM to have all variants of LD/ST/LDD/STD.
//        In reality, blucpu1 (no SRAM) has one variant each of `LD` and `ST`.
//        blucpu2 (with SRAM) adds the rest of the variants.

// A feature set aggregates features, grouping them. We don't want to create a
// new member in BLUCPUSubtarget (to store a value) for each set because we do not
// care if the set is supported, only the subfeatures inside the set. We fix
// this by simply setting the same dummy member for all feature sets, which is
// then ignored.
class FeatureSet<string name, string desc, list<SubtargetFeature> i>
    : SubtargetFeature<name, "HasFeatureSet"#NAME, "true", desc, i>;

// A family of microcontrollers, defining a set of supported features.
class Family<string name, list<SubtargetFeature> i>
    : FeatureSet<
          name, !strconcat("The device is a part of the ", name, " family"), i>;

// The device has SRAM, and supports the bare minimum of
// SRAM-relevant instructions.
//
// These are:
// LD - all 9 variants
// ST - all 9 variants
// LDD - two variants for Y and Z
// STD - two variants for Y and Z
// `LDS Rd, K`
// `STS k, Rr`
// `PUSH`/`POP`
def FeatureSRAM : SubtargetFeature<"sram", "HasSRAM", "true",
                                   "The device has random access memory">;

// The device supports the `JMP k` and `CALL k` instructions.
def FeatureJMPCALL : SubtargetFeature<"jmpcall", "HasJMPCALL", "true",
                                      "The device supports the `JMP` and "
                                      "`CALL` instructions">;

// The device supports the indirect branches `IJMP` and `ICALL`.
def FeatureIJMPCALL : SubtargetFeature<"ijmpcall", "HasSRAMIJMPCALL", "true",
                                       "The device supports `IJMP`/`ICALL`"
                                       "instructions">;

// The device supports the extended indirect branches `EIJMP` and `EICALL`.
def FeatureEIJMPCALL : SubtargetFeature<"eijmpcall", "HasEIJMPCALL", "true",
                                        "The device supports the "
                                        "`EIJMP`/`EICALL` instructions">;

// The device supports `ADDI Rd, K`, `SUBI Rd, K`.
def FeatureADDSUBIW : SubtargetFeature<"addsubiw", "HasADDSUBIW", "true",
                                       "Enable 16-bit register-immediate "
                                       "addition and subtraction instructions">;

// The device has an 8-bit stack pointer (SP) register.
def FeatureSmallStack
    : SubtargetFeature<"smallstack", "HasSmallStack", "true",
                       "The device has an 8-bit "
                       "stack pointer">;

// The device supports the 16-bit GPR pair MOVW instruction.
def FeatureMOVW : SubtargetFeature<"movw", "HasMOVW", "true",
                                   "The device supports the 16-bit MOVW "
                                   "instruction">;

// The device supports the `LPM` instruction, with implied destination being r0.
def FeatureLPM : SubtargetFeature<"lpm", "HasLPM", "true",
                                  "The device supports the `LPM` instruction">;

// The device supports the `LPM Rd, Z[+] instruction.
def FeatureLPMX : SubtargetFeature<"lpmx", "HasLPMX", "true",
                                   "The device supports the `LPM Rd, Z[+]` "
                                   "instruction">;

// The device supports the `ELPM` instruction.
def FeatureELPM : SubtargetFeature<"elpm", "HasELPM", "true",
                                   "The device supports the ELPM instruction">;

// The device supports the `ELPM Rd, Z[+]` instructions.
def FeatureELPMX : SubtargetFeature<"elpmx", "HasELPMX", "true",
                                    "The device supports the `ELPM Rd, Z[+]` "
                                    "instructions">;

// The device supports the `SPM` instruction.
def FeatureSPM : SubtargetFeature<"spm", "HasSPM", "true",
                                  "The device supports the `SPM` instruction">;

// The device supports the `SPM Z+` instruction.
def FeatureSPMX : SubtargetFeature<"spmx", "HasSPMX", "true",
                                   "The device supports the `SPM Z+` "
                                   "instruction">;

// The device supports the `DES k` instruction.
def FeatureDES : SubtargetFeature<"des", "HasDES", "true",
                                  "The device supports the `DES k` encryption "
                                  "instruction">;

// The device supports the Read-Write-Modify instructions
// XCH, LAS, LAC, and LAT.
def FeatureRMW : SubtargetFeature<"rmw", "SupportsRMW", "true",
                                  "The device supports the read-write-modify "
                                  "instructions: XCH, LAS, LAC, LAT">;

// The device supports the `[F]MUL[S][U]` family of instructions.
def FeatureMultiplication
    : SubtargetFeature<"mul", "SupportsMultiplication", "true",
                       "The device supports the "
                       "multiplication instructions">;

// The device supports the `BREAK` instruction.
def FeatureBREAK : SubtargetFeature<"break", "HasBREAK", "true",
                                    "The device supports the `BREAK` debugging "
                                    "instruction">;

// The device has instruction encodings specific to the Tiny core.
def FeatureTinyEncoding
    : SubtargetFeature<"tinyencoding", "HasTinyEncoding", "true",
                       "The device has Tiny core specific "
                       "instruction encodings">;

// When writing a 16-bit port or storing a 16-bit word, do the low byte first.
def FeatureLowByteFirst
    : SubtargetFeature<"lowbytefirst", "HasLowByteFirst", "true",
                       "Do the low byte first when writing a 16-bit port or "
                       "storing a 16-bit word">;

// The device has CPU registers mapped in data address space
def FeatureMMR : SubtargetFeature<"memmappedregs", "HasMemMappedGPR", "true",
                                  "The device has CPU registers "
                                  "mapped in data address space">;

class ELFArch<string name>
    : SubtargetFeature<"", "ELFArch", !strconcat("ELF::", name), "">;

// ELF e_flags architecture values

//===---------------------------------------------------------------------===//
// BLUCPU Families
//===---------------------------------------------------------------------===//

// The device has at least the bare minimum that **every** single BLUCPU
// device should have.
def FamilyBLUCPU1 : Family<"blucpu1", []>;

def FeatureSetSpecial
    : FeatureSet<"special",
                 "Enable use of the entire instruction "
                 "set - used for debugging",
                 [
                   FeatureSRAM, FeatureJMPCALL, FeatureIJMPCALL,
                   FeatureEIJMPCALL, FeatureADDSUBIW, FeatureMOVW, FeatureLPM,
                   FeatureLPMX, FeatureELPM, FeatureELPMX, FeatureSPM,
                   FeatureSPMX, FeatureDES, FeatureRMW, FeatureMultiplication,
                   FeatureBREAK, FeatureMMR
                 ]>;

//===---------------------------------------------------------------------===//
// BLUCPU microcontrollers supported.
//===---------------------------------------------------------------------===//

class Device<string Name>
    : Processor<Name, NoItineraries, []>;

// Generic MCUs
// Note that several versions of GCC has strange ELF architecture
// settings for backwards compatibility - see `gas/config/tc-blucpu.c`
// in BLUCPU binutils. We do not replicate this.
def : Device<"BLUCPU">;
