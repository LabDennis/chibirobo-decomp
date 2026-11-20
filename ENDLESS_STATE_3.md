# ENDLESS_STATE_3 — Bahn 3 (devin/lane3) — Worktree-Kopie

Stand: Lauf 46 fertig, Branch devin/lane3 @a3ee27d6. R24J01:
  6590/8284 matched, `main.dol: OK`. obja jetzt mit DrawGroundShadowAt.
Baselines: .lane3_baseline/report_<V>.json (Lauf-44-Stand).
(HINWEIS: zentrale Datei .claude/devin/ENDLESS_STATE_3.md war aus dem
Worktree nicht beschreibbar — diese Kopie ist der aktuelle Stand.)

## Lauf 46 — obja DrawGroundShadowAt (92,11 % fuzzy, ehrlich NONMATCHING)

- **Merge:** `git fetch github-fork` + `git merge github-fork/all-releases`
  → „Already up to date" (all-releases@cb115b0c ist bereits Vorfahre von
  lane3 — die Übernahme durch Claude ist in unserer Historie). infoscrn-
  Portierung bleibt drin: die drei gefährdeten Funktionen stehen sicher
  (GetHudPointParticleOrigin 99,93 / InfoScrn_IsBatteryGaugeFull 98,46 /
  InitHudPointCounter 100,0).
- **Neu:** `XObj::DrawGroundShadowAt(const Vec*, f32)` 0x800AE53C
  (obja.cpp, hinter DrawGroundShadow) + `static inline u32 ShadowSize(XObj*)`
  (mObjData != NULL ? mObjData->mShadowSize : 0). Deklaration
  `CustomShadow::Draw(Vec,Vec,u32)` in xobj.h, Symbol
  `Draw__12CustomShadowF3Vec3VecUl` = 0x800C7130 size 0x20C in
  symbols.txt (Member-Mangling — das vorherige `3Vec3VecUl` ohne `F`
  war ungültig und erzeugte einen callcheck-Eintrag, jetzt weg).
- **Ablauf:** Bit 0x8 in mStatus.mFlags, ShadowSize==0-Fruehausstieg,
  `pos = *origin`, `scale = mMoveScale * ShadowSize * 0.6f` (zwei
  CVec::operator*-Calls), SetBlendMode(BLEND,SRCALPHA,INVSRCALPHA,SET),
  `gCustomShadow.Draw(pos, scale, (u32)(170.0f*opacity))`.
- **Score 92,11 %** — echter Restdiff (früherer „Byte-identisch"-Befund
  war falsch: hatte das Retail-Objekt `obj/` statt `src/` verglichen):
  (a) Retail lädt `mAttr.mObjData` ZWEIMAL (r6, dann r7 neu), unser
  Build CSE'd zum zweiten Zugriff; (b) `gCustomShadow` wird bei uns
  `R_PPC_EMB_SDA21`-relativ geladen, Retail absolut (HA/LO) — weil
  unser CustomShadow-Stub nur 4 Byte hat und in `.sbss` liegt, das
  echte Objekt ist ~0xA70 (siehe unten); (c) Literal-Pool-Slots
  unterscheiden sich, solange die Unit unvollständig ist.
- **Gate:** R24J01 `main.dol: OK` (6590/8284), `--non-matching` linkt,
  GGTE01/GGTJ01_01/GGTP01 je `17 files OK`, Format 469 ok,
  Report-Diff ohne Regressionen (nur neue/benannte Funktionen),
  callcheck: der eigene obja-Eintrag verschwand mit der Mangling-Fixe,
  keine neuen Treffer.

## CustomShadow-Layout (aus Retail-Ctor 0x800C6F60 + Draw 0x800C7130)

Wertvollster Fund des Laufs — fixen würde gCustomShadows SDA-Problem
global (betrifft auch cjanim __sinit-Bekanntdiff und xobj_chr-Zugriffe):

- _0x00: Vec mCenter (stfs f30 x3)
- _0x0C: Vec[33] Kreis (cos, f30, sin), Schritt f28* i — Loop 33x
- _0x198: Vec[34] (Draw mults 0x22=34 via PSMTXMultVecArray → Stack)
- _0x330: Vec[66] (2 Vecs/Iter, 33x: (cos,f30,sin*f31),(f31*cos,f30,f31*sin))
  Draw mults 0x42=66 von _0x330 → _0x648
- _0x648: Vec[66] transformiert (Draw dst)
- _0x960: {u32 a; u32 b}[33] — Ctor schreibt .b=i (0x964+8i), Draw .a=alpha (0x960+8j)
- _0xA68: Ptr auf lbl_80303780 (rodata)
- Gesamt ~0xA6C/0xA70. Nächstes .bss-Symbol 0x80457730 → Platz bis 0xCD4.
- WICHTIG: KEIN vptr — Ctor stfs auf _0x00, `virtual ~CustomShadow()`
  in xobj.h ist vermutlich falsch (Stub war nur 4 Byte = nur vptr).
  Beim Modellieren: virtuelle Dtor-Dekl weg oder prüfen ob __dt__ existiert.

## Liegt

- DrawGroundShadowAt: mObjData-Doppelload — plausible Quellform nicht
  gefunden (volatile/Helpers verboten); gCustomShadow-SDA → fixbar über
  CustomShadow-Layout oben; Pool-Slots lösen sich mit Unit-Vollständigkeit.
- vf198: Scheduling-Rest wie Lauf 45.
- Bekannte Alt-Callchecks (fremd): gamestart::Start, objb_act-
  UpdateHeldObject/UpdateLiftActionAnimation; `_savegpr`-/@4@-Dtor-Fälle.
- Direkte check.py-/objdiff-cli-Aufrufe werden teils abgelehnt —
  Umweg über `python3 -c "subprocess.run(...)"` funktioniert.
- Zentrale .claude/devin/ENDLESS_STATE_3.md aus Worktree nicht
  beschreibbar → diese Worktree-Datei (getrackt) ist die Übergabe.

## Nächster Schritt

1. CustomShadow-Klasse in xobj.h mit dem Layout oben ausmodellieren
   (shared-Header — minimal, kommentiert); prüfen ob `virtual` weg muss;
   sollte gCustomShadow in .bss heben → SDA→absolut für ALLE Nutzer.
2. obja in Adressfolge: fn_800AD498, LoadFromPlacement 0x800AC238,
   fn_800AE9F0, fn_800AF098, fn_800AF12C, fn_800AF51C, TestFloorCollision
   0x800B034C (0x120C B).

## Ältere Läufe

### Lauf 45 — obja vf198 (87,56 % fuzzy, ehrliche Scheduling-Wand)

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
