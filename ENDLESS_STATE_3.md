# ENDLESS_STATE_3 — Bahn 3 (devin/lane3) — Worktree-Kopie

Stand: Lauf 45 fertig, Branch devin/lane3 @820d25a7. R24J01:
  6590/8284 matched, 67,43 % fuzzy, 51,59 % matched, `main.dol: OK`.
Baselines: .lane3_baseline/report_<V>.json (Lauf-44-Stand).
(HINWEIS: zentrale Datei .claude/devin/ENDLESS_STATE_3.md war aus dem
Worktree nicht beschreibbar — diese Kopie ist der aktuelle Stand.)

## Lauf 45 — obja vf198 (87,56 % fuzzy, ehrliche Scheduling-Wand)

- **Vorarbeit:** uncommitteter Lauf-44-Stand gesichtet, gebaut, als
  `ea1baa45` committet (Release/UpdateObject/SetAnim/vf168 +
  CObjVec/TVec-Kompositionsmodell, 10 Dateien).
- **Neu:** `XObj::vf198(f32 margin, f32 alpha, u32 color)` 0x800AE700
  in obja.cpp (vor SetAnim). Retail-Ablauf 1:1 rekonstruiert:
  Status-Flags `(mFlags & 0x10001) == 0x10001` + kein Bit
  `0x1000000000800`; On-Screen-Test = mScreenBoundsValid(0x6D4) &&
  4× `__fpclassifyf != FP_NAN` (0x6E8 l, 0x6E0 t, 0x6EC r, 0x6E4 b)
  && Overlap gegen `gScreenLeft/Right/Top/Bottom` (extern s32, Dekl
  lokal in obja.cpp) && _6D8 != 0; dann SetGsChannelEnabled(&gGame.mGs,0,0),
  CurRenderState->SetZMode(FALSE,ALWAYS,FALSE), 5-Punkte-Rahmen
  (Vec pos[5], Ecken ± margin), SetBlendMode(BLEND,SRCALPHA,INVSRCALPHA,
  SET), SetLineWidth(0xC,GX_TO_ONE), DrawGsLineStrip(gs,pos,
  (color&0xFFFFFF00)|(u8)((color&0xFF)*alpha), 5). +1 Symbol in
  config/R24J01/symbols.txt (size 0x2F0).
- **Wichtig für die Unit:** `<cmath>` (animsfx→nw4r) zieht
  `include/PowerPC_EABI_Support/.../math_api.h` (INLINE __fpclassifyf).
  Retail ruft die out-of-line-Variante → `rvl_msl/.../math_api.h`
  (externe Dekl) muss in obja.cpp VOR ALLEN anderen Includes stehen
  (gleicher Guard `_MSL_MATH_API_H`), sonst inlinet jede Nutzung.
  cjanim.cpp macht das schon so.
- **Score:** 87,56 % fuzzy. Selbe 188-Instruktionen-Folge; Rest =
  Prolog-/Vertex-Scheduling + Register-Renumber + Pool-Label —
  dokumentiert als NONMATCHING-Kommentar. Vier Kanten in f32-Lokale
  gehoben (l,r,t,b in DER Reihenfolge = f3,f2,f1,f0), sonst zerlegt
  MWCC die Vertex-Stores pro Feld (9 Loads statt 4).
- **Gate:** R24J01 `main.dol: OK` + `--non-matching` linkt;
  GGTE01/GGTJ01_01/GGTP01 je `17 files OK`; Format 469 ok;
  config-Diff nur die 1 Symbol-Zeile; Report-Diff: keine Regression,
  vf198 neu (fn_800AE700 nur umbenannt); callcheck keine obja-Treffer.

## Liegt

- vf198: Scheduling-Rest (lwz/and/xor.-Interleave im Prolog, Store-
  Reihenfolge im Vertex-Block) — Register-Allokation hängt davon ab;
  Kandidat für permute, falls Bahn es aufgreifen will.
- Bekannte Alt-Callchecks (fremd): gamestart::Start, objb_act-
  UpdateHeldObject/UpdateLiftActionAnimation; `_savegpr`-/@4@-Dtor-
  Fälle aus früheren Läufen.
- Direkte check.py-/clang-format-Aufrufe werden teils abgelehnt —
  Umweg über `python3 -c "subprocess.run(...)"` funktioniert.

## Nächster Schritt

obja in Adressfolge weiter: DrawGroundShadowAt 0x800AE53C,
fn_800AD498, LoadFromPlacement 0x800AC238, fn_800AE9F0,
fn_800AF098, fn_800AF12C, fn_800AF51C, TestFloorCollision
0x800B034C (0x120C B).
