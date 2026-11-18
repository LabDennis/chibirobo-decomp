# ENDLESS_STATE_5 — Übergabe Bahn 5 (devin-5, devin/lane5)

Stand: Lauf vom 18.11. (Fortsetzung). Letzter Commit: `3292427b`
"R24J01: wire unit 77 xobjcoinupdate.cpp, decode coin update/contact tail".
Davor: `afd4fa0b` (Groundwork: Wii-Klassen + Symbole + Quelldatei).

## Zahlen R24J01 (build/R24J01/report.json nach dem Commit)

- All: 56.33 % fuzzy, 47.27 % matched, 37.89 % linked (493/1727)
- Code: 6042/8278 Funktionen (vorher 6029)
- Game Code: 1277/2239 (vorher 1264)
- main.dol: OK. GC: GGTE01/GGTJ01_01/GGTP01 je "17 files OK".
- Funktionsvergleich gegen Baseline (/tmp/lane5_baseline/R24J01.json): 0 Regressionen;
  17 fn_*-Autonamen durch echte Symbole ersetzt.

## Unit 77 — src/game/wii/xobjcoinupdate.cpp (0x801141C0–0x80116528)

In configure.py nach game/wii/inventory.cpp verdrahtet
(`mw_version="GC/3.0a5"`, extra_cflags `-Cpp_exceptions on -ipa file -i include/rvl_msl`;
rvl_msl nötig, weil xobjflags.h über animsfx.h NW4R-snd zieht). Splits-Block inkl.
extab/extabindex steht in config/R24J01/splits.txt.

Erreichte Scores (NonMatching-Unit, Diagnose): 13× 100 % —
InitializeObject/UpdateObject/ResumeSounds/PauseSounds (XObjSound),
ResetObjectState/UpdateObject/SetUseAnim (XObjItem), InitializeObject/Release/
HandleEventCommand (XObjCoin), UpdateFallingCoin, InitializeObject/ResetForNewGame
(XObjHBox).

Offene Restwerte (alle im Code als NONMATCHING-Kommentar oder noch offen):
- SoundUpdate__9XObjSoundFf 91.4 % — AnimSfx::Update(s32,f32,u8) vs. Retail f2-Volume;
  Signatur gehört Bahn der sound.cpp, absichtlich unverändert.
- ResetObjectState__9XObjSoundFv 96.1 %
- GetFlagIndex__8XObjItemFv / __8XObjCoinFv je 99.4 % (kleiner Residuen-Diff)
- LoadFromPlacement__8XObjItemFRC9CoordInfo 95.9 %
- LoadFromPlacement__8XObjCoinFRC9CoordInfo 94.5 %
- ResetObjectState__8XObjCoinFv 81.8 %
- UpdateObject__8XObjCoinFUl 85.9 % — Milestone-Sparkle-Blöcke + Handle-Release +
  Actor-Updates jetzt retail-dekodiert drin; Rest: Scheduling/Inline-Residuen
  (GetPos()-Aufrufpositionen, cror-Kettenform).
- OnPlayerContact__8XObjCoinFll 97.5 % — SpawnedEffect-TRS-Fill real eingebaut;
  Batterie-Weg über player->mParams.SetBattery() korrekt (WiiPlayerParams-Felder
  _0C/_10 = player+0x99A8/0x99AC; "0x3344/0x3348" aus alter Notiz war Fehlinterpretation).
  Scrap-Fallback war bereits korrekt (attr.mObjData->GetItem() nutzt den
  NULL-selbstsicheren Inline von UnkObjData).
- vf208__8XObjCoinFPC3VecPC3Vecif 76.8 % — NONMATCHING dokumentiert: Status-Check
  via gGame+0x41E4 Slot _40 mit 64-bit-Maske nicht modelliert; GetStatusFlags(u64)
  modelliert die Signatur nicht.

## Architekturentscheidungen (stehen so)

- Wii-Klassen XObjSound/XObjFlags/XObjCoin/XObjItem liegen in
  include/game/wii/xobjflags.h (NEU), nicht in xobj.h — animsfx.h dort würde
  NW4R in alle xobj.h-Konsumenten ziehen (cobjvec/gxb/xobj_chr brachen).
- xobj_ut.cpp hat `#include "game/wii/xobjflags.h"` + `-i include/rvl_msl`
  bekommen (Grenzfile, braucht die Klassen; diff minimal gehalten).
- xobj.h MTX34-Shim (Zeile ~52) bleibt: xobjflags.h muss vor xobj.h inkludiert
  werden, damit der echte math_types.h-Typ zuerst steht.
- SpawnedEffect ist file-lokal in xobjcoinupdate.cpp (mit _60-MtxP-Feld) wie in
  xobjhboxpart.cpp; bewusst nicht in geteiltem Header.
- sCoinSparkles: static const Tabelle {s32 effect; const char* joint} —
  Retail .data 0x803080B8, Werte 14/16/17 + coin_gold/silver/copper_body1.

## Liegt (blockiert)

- **tools/check.py --source-only / clang-format**: alle direkten clang-format-
  und check.py-Aufrufe wurden vom Host rejected (exec-Deny). Formatierung der
  geänderten Dateien ist UNVERIFIZIERT — nächster Lauf zuerst
  `python3 tools/check.py --source-only` mit clang-format 18.1.8 nachholen.
- **git fetch github-fork**: weiterhin denied. Lokales all-releases (8a3860d3)
  ist laut rev-list komplett gemergt — 0 Commits hinten. Remote-Stand unbekannt.
- **Main-Checkout-Übergabe** .claude/devin/ENDLESS_STATE_5.md: Schreibzugriff
  wiederholt denied → diese Datei hier im Worktree ist die aktuelle Version;
  bei Gelegenheit dorthin kopieren.
- UpdateObject-Residuen (85.9 %) und die <100 %-Funktionen oben: weitere
  Dekodier-/Match-Arbeit, kein Blocker.

## Nächste Schritte

1. Formatcheck nachholen (s. Liegt) — ggf. clang-format-18 auf
   xobjcoinupdate.cpp/xobjflags.h laufen lassen.
2. Unit 79: game/wii/xobjhbox.cpp, 0x8011701C–0x801178E4 (xobjhboxpart.cpp ist
   bis 0x8011701C registriert — danach liegt das nächste Stück).
3. Danach Units 80–98, dann 116–122 laut ENDLESS.md-Aufteilung.
4. Verbleibende Unit-77-Residuen (s.o.) können parallel zu 79 verbessert werden.

## Basen/Reports

- Baseline-Vergleichsreports: /tmp/lane5_baseline/{R24J01,GGTE01}.json
- GC-Baselines von 17.11.: /tmp/gc2run/reports/ bzw. /tmp/codex-prompt1-baseline/
- Commit-Identität: LabDennis <341377186+LabDennis@users.noreply.github.com>
