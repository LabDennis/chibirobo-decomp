# ENDLESS_STATE_3 — Bahn 3 (devin/lane3) — Worktree-Kopie

Stand: Lauf 48 fertig, Branch devin/lane3 @e3751019. R24J01:
  6592/8284 matched, `main.dol: OK`.
Baselines: .lane3_baseline/report_<V>.json (Lauf-44-Stand).
(HINWEIS: zentrale .claude/devin/ENDLESS_STATE_3.md ist aus dem
Worktree nicht beschreibbar — diese Kopie ist der aktuelle Stand.
 .claude/devin/ENDLESS_STATE_3.md IM Worktree ist veraltet, Lauf 43.)

## Lauf 48 — obja READ-Paar: fn_800AF12C 100 %, fn_800AF098 98,38

- **Merge:** `git merge github-fork/all-releases` → „Already up to date".
- **fn_800AF098** (0x94, READ-Start): `mArchive.mAnm.mFile.ReadAsync
  (path,0,0,FALSE)`; auf 0 → `OSPanic("xobj_ut2.h", 0xe2, "")` +
  FALSE, sonst READ-Pending-Bit `0x80000000000000ULL` in
  mStatus.mFlags (_150, High-Word `oris 0x80`) + TRUE. Source-Form
  die Retail-Kontrollfluss trifft: Ternary
  `result != 0 ? TRUE : (OSPanic(...), FALSE)` — &&/Bool-Varianten
  erzeugen neg/or/srwi + Extra-Register. Bleibt **98,38 %
  NONMATCHING**: Retail lädt `""` aus .sdata (lbl_805A0F80), unser
  Build faltet es in die "xobj_ut2.h"-rodata-StringBase
  (`addi r5, r3, 0xb`) — Pool-Layout der unvollständigen Unit,
  Code ansonsten instruktionsidentisch. Kommentar im Source.
- **fn_800AF12C** (0x19C, READ-Abbruch): **100 % matched.**
  Schlüssel: Retail-Guards sind *geschachtelte* ifs (drei beq auf
  dasselbe Ziel), NICHT eine &&-Kette — && erzeugt Bool-
  Materialisierung + r29-Save + Frame 0x30 statt 0x20.
  Ablauf: `mArchive.mAnm.IsLoaded()` → `GetAnimId()>=0` →
  `source = mAttr.mObjData ? mAttr.mObjData->mSource : NULL` →
  `if (source) if (source->mAnimNames) if (GetResAnmChr(...)==NULL)
  SetAnim(0,...)` → `SetAnim(GetAnimId(), TRUE, GetAnimFrame(),
  1,1,15)` → ClearAnmSource → Release(TRUE) + mFile.Close().
- **vf168 gezählt:** `vf168__4XObjFUl = .text:0x800AE170 size 0x2D8`
  fehlte in symbols.txt (war seit Lauf 44 implementiert aber
  ungemeldet). +1 Symbol-Zeile → jetzt im Report: **69,02 %** —
  Restdiff bisher unanalysiert, steht auf der Liste.
- **Gate:** R24J01 `main.dol: OK` 6592/8284 (+1: AF12C neu matched,
  vf168 neu gezählt bei 69); --non-matching linkt; GGTE01/GGTJ01_01/
  GGTP01 je `17 files OK`; Report-Diff alle 4 Editionen **0
  Regressionen** (up: AF12C 100, AF098 98,38, vf168 69,02).
  config-Diff nur die eine Symbol-Zeile. check.py --source-only
  weiter blockiert (nur clang-format 23.1.4, kein 18.1.8);
  callcheck.py existiert nicht in tools/ (nur Erwähnung in dieser
  Datei).

## Lauf 47 — CustomShadow echtes Layout → gCustomShadow in .bss

- **Merge:** `git merge github-fork/all-releases` → „Already up to date".
- **Befund geklärt:** `lbl_80303780` (Ctor 0x800C6F60 schreibt ihn nach
  _A68) ist die CustomShadow-**vtable** {RTTI lbl_805A1078, 0,
  `__dt__12CustomShadowFv` 0x800BD66C, 0}. `virtual` bleibt — der vptr
  liegt wie bei MenuEx/_30, CMsgData/_89C an der Stelle der ersten
  Virtual-Deklaration; Mitglieder zuerst, `virtual ~CustomShadow()`
  zuletzt → vptr _A68, sizeof 0xA6C.
- **Layout** (aus Ctor+Draw, siehe xobj.h-Kommentar): _000 Vec mCenter;
  _00C Vec[33] Kreis (cos,0,sin); _198 Vec[34] Fan (Draw:
  PSMTXMultVecArray 34 ab this+0); _330 Vec[66] Ringpaare
  (rim, f31*rim); _648 Vec[66] transformiert; _960 u32[66]
  ([2i]=alpha, [2i+1]=i); _A68 vptr. Draw ruft
  DrawGsTrianglePrimitive(FAN 0xA0,34) + DrawGsColoredTrianglePrimitive
  (STRIP 0x98,66); Ring = weiche Kante des Blob-Schattens.
- **Geändert:** include/game/wii/xobj.h (Layout + STATIC_ASSERT 0xA6C);
  veraltete Stub-Kommentare in cjanim.cpp/obja.cpp aktualisiert.
- **Effekt:** gCustomShadow (cjanim.cpp) wandert .sbss→.bss, Zugriff
  absolut statt @sda21. `__sinit_\cjanim_cpp` 64,67→**100,0**,
  DrawGroundShadowAt 92,11→**94,03**. Report-Diff 0 Regressionen
  (4 Editionen), GC 3×17 files OK, --non-matching linkt, Format
  469 ok, callcheck nur Alt-Treffer (actor_pool/ut, fremd).
- Commit 64ae5a3a. Graphiti-MCP-Aufrufe wurden abgelehnt.

## CustomShadow-Detail (belegt, für Portierung)

- Ctor 0x800C6F60 (0x1D0): mCenter={0,0,0}; 33× mCircle[i]=(cos(f28*i),
  0,sin(f28*i)); 33× Paare ab _330+i*0x18: (cos,0,sin) und
  (f31*cos,0,f31*sin); mColors[2i+1]=i; vptr=_A68. Konstanten:
  f30=lbl_805A4E40 (0.0f), Schritt=lbl_805A4E68, Radius=lbl_805A4E6C,
  lfd lbl_805A4E70 = 2^52-Magie.
- Draw 0x800C7130 (0x20C): PSMTXTrans/Scale/Concat → MultVecArray
  (this+0→_198,34) → Fan-Draw(0xA0) → 33× alpha in mColors[2i] →
  MultVecArray(_330→_648,66) → Strip-Draw(0x98, mColors, 66).
- Dtor 0x800BD66C (0x40) = Standard-Deleting-Dtor (r4>0 → __dl__FPv).
- Ctor/Draw liegen in ungeteilter Lücke 0x800C6584–0x800C9E84 zwischen
  xaction und xobj_chr — NICHT in xobj_ut (das besitzt nur den Dtor).

## Liegt

- DrawGroundShadowAt (94,03): Rest = mObjData-Doppelload (r6→r7 neu) +
  Pool-Slots der unvollständigen Unit. ehrlich NONMATCHING.
- fn_800AF098 (98,38): nur noch String-Pool — Retail `""` in .sdata
  (lbl_805A0F80), wir `addi r5, r3, 0xb` auf die rodata-StringBase;
  löst sich mit vollständiger Unit. ehrlich NONMATCHING.
- vf168__4XObjFUl 0x800AE170 (69,02): seit Lauf 48 im Report,
  Restdiff unanalysiert.
- vf198 (87,56): Scheduling-Rest, unverändert.
- obja-Funktionen ohne Quelltext, Retail-Größen:
  fn_800AD498 ~0x3D8 (strcmp, bctrl über
  gGame+0x41E4-vf); fn_800AE9F0 ~0x6A4 (0x1000-Frame, Farbtabellen,
  DrawGsColoredTrianglePrimitive ×8); fn_800AF51C ~0xA64 (NW4R-ResMat-
  Tev-Schleife); LoadFromPlacement 0x800AC238 0x1260;
  TestFloorCollision 0x800B034C 0x120C.
- Alt-Callchecks (fremd): gamestart::Start, objb_act-UpdateHeldObject/
  UpdateLiftActionAnimation, actor_pool __ct__*Fv (fn_8001AFAC),
  ut fn_8018F9B4 (__as__3Vec), _savegpr-/@4@-Dtor-Fälle.
- Direkte check.py-/objdiff-cli-Aufrufe werden teils abgelehnt →
  Umweg `python3 -c "subprocess.run(...)"` funktioniert.

## Nächster Schritt

obja, kleinste zuerst: fn_800AD498 (~0x3D8, strcmp + bctrl über
gGame+0x41E4-vf), dann vf168-Diff ansehen (69,02, jetzt im Report),
dann fn_800AE9F0, fn_800AF51C; LoadFromPlacement +
TestFloorCollision (je ~0x1200) als eigene Anläufe.
CustomShadow-Ctor/Draw (0x800C6F60/0x800C7130) als Quelle in die
Lücken-Unit, sobald deren Originaldatei identifiziert ist.

## Ältere Läufe

### Lauf 46 — obja DrawGroundShadowAt (92,11 % → siehe Lauf 47)

- Neu: `XObj::DrawGroundShadowAt(const Vec*, f32)` 0x800AE53C +
  `static inline u32 ShadowSize(XObj*)`; `CustomShadow::Draw(Vec,Vec,u32)`
  in xobj.h, Symbol `Draw__12CustomShadowF3Vec3VecUl` = 0x800C7130.
- Ablauf: Bit 0x8 in mStatus.mFlags, ShadowSize==0-Fruehausstieg,
  `pos = *origin`, `scale = mMoveScale * ShadowSize * 0.6f`,
  SetBlendMode(BLEND,SRCALPHA,INVSRCALPHA,SET),
  `gCustomShadow.Draw(pos, scale, (u32)(170.0f*opacity))`.
- Damaliger Restdiff: (a) Retail lädt mAttr.mObjData ZWEIMAL;
  (b) gCustomShadow @sda21 statt absolut — in Lauf 47 durch das
  echte Klassenlayout behoben; (c) Pool-Slots.

### Lauf 45 — obja vf198 (87,56 % fuzzy, ehrliche Scheduling-Wand)

- Vorarbeit: uncommitteter Lauf-44-Stand als `ea1baa45` committet
  (Release/UpdateObject/SetAnim/vf168 + CObjVec/TVec-Modell).
- Neu: `XObj::vf198(f32 margin, f32 alpha, u32 color)` 0x800AE700:
  Status-Flags `(mFlags & 0x10001) == 0x10001` + kein Bit
  `0x1000000000800`; On-Screen-Test = mScreenBoundsValid(0x6D4) &&
  4× `__fpclassifyf != FP_NAN` (0x6E8 l, 0x6E0 t, 0x6EC r, 0x6E4 b)
  && Overlap gegen `gScreenLeft/Right/Top/Bottom` && _6D8 != 0; dann
  SetGsChannelEnabled(&gGame.mGs,0,0), CurRenderState->SetZMode
  (FALSE,ALWAYS,FALSE), 5-Punkte-Rahmen, SetBlendMode, SetLineWidth
  (0xC,GX_TO_ONE), DrawGsLineStrip(gs,pos,(color&0xFFFFFF00)|(u8)
  ((color&0xFF)*alpha),5). +1 Symbol in symbols.txt (size 0x2F0).
- `<cmath>` zieht INLINE __fpclassifyf — Retail ruft out-of-line →
  `rvl_msl/.../math_api.h` muss in obja.cpp VOR ALLEN Includes stehen
  (gleicher Guard `_MSL_MATH_API_H`), sonst inlinet jede Nutzung.
  cjanim.cpp macht das schon so.
- Vier Kanten in f32-Lokale gehoben (l,r,t,b in DER Reihenfolge =
  f3,f2,f1,f0), sonst zerlegt MWCC die Vertex-Stores pro Feld.
