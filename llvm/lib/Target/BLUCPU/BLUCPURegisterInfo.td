// https://github.com/0blu/blucpu

class BLUCPUReg<bits<16> num, string name> : Register<name> {
  field bits<16> Num = num;

  let HWEncoding = num;
  let Namespace = "BLUCPU";
}

def R0    : BLUCPUReg<0, "r0">,     DwarfRegNum<[0]>;
def R1    : BLUCPUReg<1, "r1">,     DwarfRegNum<[1]>;
def R2    : BLUCPUReg<2, "r2">,     DwarfRegNum<[2]>;
def R3    : BLUCPUReg<3, "r3">,     DwarfRegNum<[3]>;
def R4    : BLUCPUReg<4, "r4">,     DwarfRegNum<[4]>;
def R5    : BLUCPUReg<5, "r5">,     DwarfRegNum<[5]>;
def R6    : BLUCPUReg<6, "r6">,     DwarfRegNum<[6]>;
def R7    : BLUCPUReg<7, "r7">,     DwarfRegNum<[7]>;
def R8    : BLUCPUReg<8, "r8">,     DwarfRegNum<[8]>;
def R9    : BLUCPUReg<9, "r9">,     DwarfRegNum<[9]>;
def SP    : BLUCPUReg<10, "sp">,    DwarfRegNum<[10]>;
def BP    : BLUCPUReg<11, "bp">,    DwarfRegNum<[11]>;
def IP    : BLUCPUReg<12, "ip">,    DwarfRegNum<[12]>;
def CTRL1 : BLUCPUReg<13, "ctrl1">, DwarfRegNum<[13]>;
def CTRL2 : BLUCPUReg<14, "ctrl2">, DwarfRegNum<[14]>;
def FLAGS : BLUCPUReg<15, "flags">, DwarfRegNum<[15]>;

// Main 32-bit register class.
def GP_REGS : RegisterClass<"BLUCPU", [i32], 8, (add R9, R8, R7, R6, R5, R4, R3, R2, R1, R0)>;
