# Devin GC-Bahn 2 — Übergabe

Stand: 18.11. (spät, dritter Lauf), Branch `devin/gc2`, HEAD `d3f3785d`
(= Merge `github-fork/all-releases`@`cb115b0c` auf `b28bff94`; nicht gepusht).
Baselines: `/tmp/gc2run/reports/report_<V>.json` + `build/<V>/report_base.json`.

## Dieser Lauf — xobjfree-Merge committed + HBox-Wand kartiert

- **Commit `b28bff94`:** xobjfree-Originaldatei fertig gestellt und
  committed (Free/Dust/HBox/Coin/Flags/Dtors in einer TU,
  `-inline noauto` wegen CheckDustScatterHeading-Call). Danach sauberer
  Merge `d3f3785d` auf `cb115b0c` (obja_chr/objb_act/cjobj.h/xobjactortoy
  — keine Überschneidung).
- **Gate (nach Merge):** GGTE01/GGTJ01_01/GGTP01 je „17 files OK",
  R24J01 „main.dol: OK", `--non-matching` linkt, **0 Funktionsregressionen**,
  callcheck GGTE01: 8 Diffen, alle bekannt (objb_act-Inline-Wände,
  gamestart — nicht meine Units). `UpdateObject__8XObjFreeFUl` und
  `ReadValidatedSaveIntoCardBuffer` stehen inzwischen bei 100 %.
- **`UpdateObject__8XObjHBoxFUl` 98.36 (fdiff; 3948 B):** Wand kartiert —
  Retail-Rahmen 0x300 vs 0x2f0; vor `SetPos` kopiert Retail `pos` (0xc)
  und das `HSD_MtxGetRotation`-Ergebnis (**0x10 Bytes** → kein `Vec`) in
  zweite Temps; `SetRotationY` liest Kopie+4 (= `.y` in Quaternion-Layout).
  **Versuche (beide schlechter, zurückgenommen):** `Quaternion rot` →
  richtige Größe, MWCC kopiert das Objekt aber nicht; `CVec rot` → +16 B,
  aber Vtbl-Stores die Retail nicht hat. Vermutlich by-value/sret-Temp
  aus einer Schreibweise, die wir nicht sehen (Kandidat: anderer
  Rotations-Helper oder `mRotate`-Member eines anderen Objekts; Wii liest
  `mMoveRotate`!). Regeln: kein compound literal/qualifier → offen.
- **Nächster Anker:** `LoadFromPlacement__8XObjItem` 90.42 ist
  befehlsidentisch, reiner Registerfall (r30↔r31-Vertauschung) — bis zu
  2 Anläufe erlaubt; danach memcard-Rest + kleine Funktionen.

## Vorheriger Lauf — memcard `SetAngle(Vec&)` out-of-line (Retail-Struktur)

- **Befund:** Retail `memcard.o` definiert `SetAngle__7CCameraFRC3Vec`
  GLOBAL bei 0x80087C88 (zwischen RestoreSavedPlayerAndCamera und
  RestoreGameSaveData) und lässt `__vt__4CVec`/`__vt__14CCameraMemento`
  UND — in allen drei GC-Editionen identisch. Unser Build emittierte
  dieselben Bytes, aber weak (Fuzzy war ohnehin 100 %).
- **Umsetzung:** `include/game/ccamera.h`: `virtual void SetAngle(const
  Vec& angle);` — Body entfernt (Retail-Deklaration: `ccamera.o` UNDet das
  Symbol trotz eigener Vtable-Emission). `src/game/memcard.cpp`:
  `void CCamera::SetAngle(const Vec& angle) { SetAngle(angle.x, angle.y,
  angle.z); }` — **unqualifizierter** Aufruf → virtueller `bctrl` über
  Slot `_60`, wie Retail. (Der Wii-Baum nutzt dasselbe Muster in
  `wii/ccamera.cpp`, dort qualifiziert wegen anderer Vtable-Lage.
  R24J01 nutzt `wii/cgame.h`, nicht `ccamera.h` → kein Wii-Blast-Radius.)
- **Wichtig:** GC/2.6 lehnt out-of-line-Redefinition eines in-class
  Inline ab („redefined") — der Header muss bodiless sein. GC/3.0a5 (Wii)
  toleriert es.
- **Gate:** GGTE01/GGTJ01_01/GGTP01 je „17 files OK", R24J01 „main.dol:
  OK". `SetAngle__7CCameraFRC3Vec` 100 % + jetzt GLOBAL in allen drei GC-
  Editionen, `RestoreSavedPlayerAndCamera` weiter 100 %, null Regress.
- **Scheinregression entkräftet:** `xobjplugcord::UpdatePlugCordSimulation`
  96.23→88.66 in der Baseline — Objekt vor/nach Änderung byteidentisch
  (objdiff .text 100 %). Großer vorbestehender Reg-/Stack-Diff, stale
  Baseline, nicht von diesem Lauf.

## Großer Befund fürs Projekt (für nächsten Lauf/Orchestrator)

**MWCC-Vtable-Heimregel:** die Vtable landet in der TU mit dem ersten
nicht-inlinen MEMBER (nicht nur virtuell). Retail-Belege (je genau ein
GLOBAL-DEF, Rest UND):
- `__ct__4CVecFRC4CVec` + `__ct__4CVecFfff` → `game/obja_chr.o` → dort
  sitzt `__vt__4CVec`; 107 unserer Objekte tragen sie derzeit weak.
- `__ct__14CCameraMementoFv` → `game/evt.o` → `__vt__14CCameraMemento`
  (6 weak-Emittenten).
- `__dt__4CVecFv` → `game/gamecgenerator.o` (schon als `extern "C"`
  vorhanden), `__dt__14CCameraMementoFv` → `game/cjobj.o` (dto.).
- Setter: `SetEye__7CCameraFRC3Vec+Ffff` → `gamemain.o`,
  `SetInterest…` → `gameccameraset.o`, `SetDistance`+`SetFov` →
  `gameccameraproj.o`, `SetAngle__7CCameraFfff` → `gxa.o` — die Codebase
  hat dafür bereits `extern "C"`-Handdefinitionen; Retail-Deklarationen
  waren aber vermutlich bodiless.
- Destruktoren bleiben inline-leer: Retail `RestoreSavedPlayerAndCamera`
  hat KEINE `bl __dt__…`-Aufrufe, nur vptr-Stores → `~CVec`/`~CCameraMemento`
  NICHT bodiless machen (sonst Regress in 100%-Funktionen).
- Konsequenz fürs Aufräumen: `CVec(const CVec&)`/`CVec(f32,f32,f32)` bodiless
  + Definitionen in `obja_chr.cpp`; `CCameraMemento()` bodiless + Definition
  in `evt.cpp`. Blast radius: jede CVec-Konstruktion/Kopie wird `bl` —
  Retail hat diese Calls (obja_chr-Beleg), Matching-Funktionen ohne sie
  bleiben unberührt. Memcard-`.sdata2` (derzeit 28 statt 8 Bytes:
  RTTI-Namen `CVec`/`Vec`) und die weak-Emissionen in ~100 Units räumt das
  ab. Vorher JP/PAL-Symbole spiegeln.

## memcard-Rest (identisch zur Baseline)

- `IsValidSavePreview` 98.71, `RestoreGameSaveData` 99.47 — Regwalls.
- `.sdata2`-Mehrbytes = RTTI-Emission oben, nicht lokal lösbar.
- Verbraucht: `stage = GetStageNum() + stage - 99`-Hypothese → MWCC
  reassoziiert `a += b - c` bereits zu `(a+b)-c`, wurde schlechter
  (98.56), zurückgenommen. Nicht wiederholen.

## xobjfree/xobjactor-Restlücken (18.11. dritter Lauf)

- `LoadFromPlacement__8XObjItemFRC9CoordInfo` 90.42 (reiner
  Registerfall, Befehle identisch),
  `UpdateObject__8XObjHBoxFUl` 98.36 (Strukturwand oben),
  `OnPlayerContact__8XObjCoinFll` 98.97, `SetAnim…` 99.18,
  `UpdateDustEvent` 99.89, `UpdateGroundMovement` 99.92.
- Jetzt 100 %: `UpdateObject__8XObjFreeFUl`,
  `ReadValidatedSaveIntoCardBuffer`.

## Nächste Schritte

1. `xobjfree::LoadFromPlacement` — Registerfall, max. 2 Anläufe.
2. Kleine Funktionen: `OnPlayerContact` 98.97, `SetAnim` 99.18,
   `UpdateDustEvent`/`UpdateGroundMovement` 99.9x, memcard
   `IsValidSavePreview` 98.71 / `RestoreGameSaveData` 99.47.
3. Optional: CVec-Ctors/Memento-Ctor-Umbau oben (groß, Gates nötig).
4. `obja_chr::HandleEventCommand` 96.28, `xactradar::DrawRadarTargetBeam` 93.75.

Claude-Dateien (NICHT anfassen): xobjspiderboss*, xobjdust*, xobjhbox*,
xobjcoin*, xobjactor3*, cinterfacecameraaction, cinterpolation*,
objb_attach/objb_plug/xobjplugcord/xobjactortoy/xobjactormama.
Dolphin nie.

## Liegt (Blocker)

- `tools/check.py`/clang-format-Aufrufe werden vom Tool-Gate abgelehnt
  (steht so seit Vorgängerlauf). Header-Zeile manuell auf Spalte 101
  ausgerichtet; neuer Code hält Tab-Stil.
- Untracked Scratch: diese Datei, scratch_*.py/md/patch, probe.sh.

---

## Vorgänger-Läufe (Details)

- Merge 9420e3b7: zwei Originaldateien fertig (xobjactor `5ed52e74`,
  xobjfree `7564a053`; `xobjflagsinit.cpp` bleibt eigene Unit —
  `__sinit_xobjflagsinit_cpp`-Namensbeweis). Gate: 4× OK, 0 Regress.
- memcard-Trio-Anker-Muster: Retail hält `card+0x10000`/`gGame+0x10000`
  in einem Register; unsere Builds materialisieren den Anker später —
  Regalloc-Wand.
- xobjinit `LoadFromPlacement`: `initial` ist `Vec[3]` in der
  dateiübergreifenden Null-Template-Region (.bss 0x80378148..0x803781C0).
- `report.json` fuzzy zählt `@pool@sda21`-vs-`lbl_…`-Referenten nicht.
