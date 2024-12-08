//===-- BLUCPUMCExpr.cpp - BLUCPU specific MC expression classes ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "BLUCPUMCExpr.h"

#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCValue.h"

namespace llvm {

namespace {

const struct ModifierEntry {
  const char *const Spelling;
  BLUCPUMCExpr::VariantKind VariantKind;
} ModifierNames[] = {
    {"lo8", BLUCPUMCExpr::VK_BLUCPU_LO8},       {"hi8", BLUCPUMCExpr::VK_BLUCPU_HI8},
    {"hh8", BLUCPUMCExpr::VK_BLUCPU_HH8}, // synonym with hlo8
    {"hlo8", BLUCPUMCExpr::VK_BLUCPU_HH8},      {"hhi8", BLUCPUMCExpr::VK_BLUCPU_HHI8},

    {"pm", BLUCPUMCExpr::VK_BLUCPU_PM},         {"pm_lo8", BLUCPUMCExpr::VK_BLUCPU_PM_LO8},
    {"pm_hi8", BLUCPUMCExpr::VK_BLUCPU_PM_HI8}, {"pm_hh8", BLUCPUMCExpr::VK_BLUCPU_PM_HH8},

    {"lo8_gs", BLUCPUMCExpr::VK_BLUCPU_LO8_GS}, {"hi8_gs", BLUCPUMCExpr::VK_BLUCPU_HI8_GS},
    {"gs", BLUCPUMCExpr::VK_BLUCPU_GS},
};

} // end of anonymous namespace

const BLUCPUMCExpr *BLUCPUMCExpr::create(VariantKind Kind, const MCExpr *Expr,
                                   bool Negated, MCContext &Ctx) {
  return new (Ctx) BLUCPUMCExpr(Kind, Expr, Negated);
}

void BLUCPUMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  assert(Kind != VK_BLUCPU_None);
  OS << getName() << '(';
  if (isNegated())
    OS << '-' << '(';
  getSubExpr()->print(OS, MAI);
  if (isNegated())
    OS << ')';
  OS << ')';
}

bool BLUCPUMCExpr::evaluateAsConstant(int64_t &Result) const {
  MCValue Value;

  bool isRelocatable =
      getSubExpr()->evaluateAsRelocatable(Value, nullptr, nullptr);

  if (!isRelocatable)
    return false;

  if (Value.isAbsolute()) {
    Result = evaluateAsInt64(Value.getConstant());
    return true;
  }

  return false;
}

bool BLUCPUMCExpr::evaluateAsRelocatableImpl(MCValue &Result,
                                          const MCAssembler *Asm,
                                          const MCFixup *Fixup) const {
  MCValue Value;
  bool isRelocatable = SubExpr->evaluateAsRelocatable(Value, Asm, Fixup);

  if (!isRelocatable)
    return false;

  if (Value.isAbsolute()) {
    Result = MCValue::get(evaluateAsInt64(Value.getConstant()));
  } else {
    if (!Asm || !Asm->hasLayout())
      return false;

    MCContext &Context = Asm->getContext();
    const MCSymbolRefExpr *Sym = Value.getSymA();
    MCSymbolRefExpr::VariantKind Modifier = Sym->getKind();
    if (Modifier != MCSymbolRefExpr::VK_None)
      return false;
    if (Kind == VK_BLUCPU_PM) {
      //Modifier = MCSymbolRefExpr::VK_BLUCPU_PM;
    }

    Sym = MCSymbolRefExpr::create(&Sym->getSymbol(), Modifier, Context);
    Result = MCValue::get(Sym, Value.getSymB(), Value.getConstant());
  }

  return true;
}

int64_t BLUCPUMCExpr::evaluateAsInt64(int64_t Value) const {
  if (Negated)
    Value *= -1;

  switch (Kind) {
  case BLUCPUMCExpr::VK_BLUCPU_LO8:
    Value &= 0xff;
    break;
  case BLUCPUMCExpr::VK_BLUCPU_HI8:
    Value &= 0xff00;
    Value >>= 8;
    break;
  case BLUCPUMCExpr::VK_BLUCPU_HH8:
    Value &= 0xff0000;
    Value >>= 16;
    break;
  case BLUCPUMCExpr::VK_BLUCPU_HHI8:
    Value &= 0xff000000;
    Value >>= 24;
    break;
  case BLUCPUMCExpr::VK_BLUCPU_PM_LO8:
  case BLUCPUMCExpr::VK_BLUCPU_LO8_GS:
    Value >>= 1; // Program memory addresses must always be shifted by one.
    Value &= 0xff;
    break;
  case BLUCPUMCExpr::VK_BLUCPU_PM_HI8:
  case BLUCPUMCExpr::VK_BLUCPU_HI8_GS:
    Value >>= 1; // Program memory addresses must always be shifted by one.
    Value &= 0xff00;
    Value >>= 8;
    break;
  case BLUCPUMCExpr::VK_BLUCPU_PM_HH8:
    Value >>= 1; // Program memory addresses must always be shifted by one.
    Value &= 0xff0000;
    Value >>= 16;
    break;
  case BLUCPUMCExpr::VK_BLUCPU_PM:
  case BLUCPUMCExpr::VK_BLUCPU_GS:
    Value >>= 1; // Program memory addresses must always be shifted by one.
    break;

  case BLUCPUMCExpr::VK_BLUCPU_None:
    llvm_unreachable("Uninitialized expression.");
  }
  return static_cast<uint64_t>(Value) & 0xff;
}

BLUCPU::Fixups BLUCPUMCExpr::getFixupKind() const {
  BLUCPU::Fixups Kind = BLUCPU::Fixups::LastTargetFixupKind;

  switch (getKind()) {
  case VK_BLUCPU_LO8:
    Kind = isNegated() ? BLUCPU::fixup_lo8_ldi_neg : BLUCPU::fixup_lo8_ldi;
    break;
  case VK_BLUCPU_HI8:
    Kind = isNegated() ? BLUCPU::fixup_hi8_ldi_neg : BLUCPU::fixup_hi8_ldi;
    break;
  case VK_BLUCPU_HH8:
    Kind = isNegated() ? BLUCPU::fixup_hh8_ldi_neg : BLUCPU::fixup_hh8_ldi;
    break;
  case VK_BLUCPU_HHI8:
    Kind = isNegated() ? BLUCPU::fixup_ms8_ldi_neg : BLUCPU::fixup_ms8_ldi;
    break;

  case VK_BLUCPU_PM_LO8:
    Kind = isNegated() ? BLUCPU::fixup_lo8_ldi_pm_neg : BLUCPU::fixup_lo8_ldi_pm;
    break;
  case VK_BLUCPU_PM_HI8:
    Kind = isNegated() ? BLUCPU::fixup_hi8_ldi_pm_neg : BLUCPU::fixup_hi8_ldi_pm;
    break;
  case VK_BLUCPU_PM_HH8:
    Kind = isNegated() ? BLUCPU::fixup_hh8_ldi_pm_neg : BLUCPU::fixup_hh8_ldi_pm;
    break;
  case VK_BLUCPU_PM:
  case VK_BLUCPU_GS:
    Kind = BLUCPU::fixup_16_pm;
    break;
  case VK_BLUCPU_LO8_GS:
    Kind = BLUCPU::fixup_lo8_ldi_gs;
    break;
  case VK_BLUCPU_HI8_GS:
    Kind = BLUCPU::fixup_hi8_ldi_gs;
    break;

  case VK_BLUCPU_None:
    llvm_unreachable("Uninitialized expression");
  }

  return Kind;
}

void BLUCPUMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}

const char *BLUCPUMCExpr::getName() const {
  const auto &Modifier =
      llvm::find_if(ModifierNames, [this](ModifierEntry const &Mod) {
        return Mod.VariantKind == Kind;
      });

  if (Modifier != std::end(ModifierNames)) {
    return Modifier->Spelling;
  }
  return nullptr;
}

BLUCPUMCExpr::VariantKind BLUCPUMCExpr::getKindByName(StringRef Name) {
  const auto &Modifier =
      llvm::find_if(ModifierNames, [&Name](ModifierEntry const &Mod) {
        return Mod.Spelling == Name;
      });

  if (Modifier != std::end(ModifierNames)) {
    return Modifier->VariantKind;
  }
  return VK_BLUCPU_None;
}

} // end of namespace llvm
