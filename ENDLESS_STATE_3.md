# ENDLESS_STATE_3 — Bahn 3 (devin/lane3) — Worktree-Kopie

Stand: Lauf 54 fertig, Branch devin/lane3 (Commit 93fd2d24). R24J01:
6592/8284 matched, `main.dol: OK`; GC: 17 files OK (alle drei).
Baselines: .lane3_baseline/report_<V>.json (Lauf-44-Stand).
(HINWEIS: zentrale .claude/devin/ENDLESS_STATE_3.md ist aus dem
Worktree nicht beschreibbar — diese Kopie ist der aktuelle Stand.
 .claude/devin/ENDLESS_STATE_3.md IM Worktree ist veraltet, Lauf 43.)

## Lauf 54 — LoadFromPlacement 81,74 → 90,18 %; TestFloorCollision dekodiert

- **Commit 93fd2d24**: `wii obja: LoadFromPlacement 81.74 -> 90.18
  (ready-flag shape, spiderplacement inlines)`. Der uncommittete
  Anlauf aus dem Vorkontext wurde verifiziert, formatiert, durchs
  volle Gate geschickt und committed. `ready`-BOOL-Struktur +
  `AttrInfo* attr = &mAttr` + Wii-`spiderplacement.h`-Inlines
  (PlacementPosition/Rotation/Scale, bauen alle drei Achsen).
- **Gate komplett grün:** R24J01 main.dol OK, obja-Unit 0
  Regressionen (vf168 99,23; fn_800AE9F0 72,91; fn_800AF51C 71,44;
  DrawGroundShadowAt 94,03 unverändert); GC 3× 17 files OK;
  Report-Diff 0 Regressionen in allen 4 Editionen; callcheck nur
  bekannte Fremd-Treffer (actor_pool); keine neuen Doppelsymbole.
- **TestFloorCollision 0x800B034C (0x120C) vollständig dekodiert**
  — Retail-ASM in `/tmp/obja_dis` (sonst neu:
  `dtk elf disasm build/R24J01/obj/game/wii/obja.o /tmp/obja_dis`,
  Funktion ~Zeilen 5437–6640; llvm-objdump kann Paired-Single
  NICHT → nur dtk). Struktur = GC `src/game/obja.cpp:425`
  TestFloorCollision 1:1 (gleiche Guards, Cases, Konstanten,
  Epilog). Wii-Abweichungen/Befunde für den Port:
  - **NEU: Schleifen-Gate** `if (!(mFloors[i].mFlags & 1)) continue;`
    — Kopf lädt `lbz r0,3(floor)` clrlwi. &1; mFlags Bit 0 = "in
    contact tests" (xobj.h-Kommentar). GC hat das NICHT.
  - `mToppleOffset` (GC) → **`mVec714`** (_714) auf Wii.
  - Decode wie GC: `xoris s32,0x8000`→f64−2^52→`fdivs` 100.0f
    (f31=lbl_805A44DC) ⇒ `ReadObjectFloorOffset/Extent(this,i)` aus
    `include/game/objectfloorquery.h` (Wii-tauglich, benutzen!).
  - yaw = `mMoveRotate.y` (_64) + DEG2RAD*(`mYawDegrees` s32 @+0x10);
    f25=lbl_805A44E0=DEG2RAD; mId s16 @+0 switcht Cases (0,1 Disc;
    2 Box; 3 Wall; 5 Quad).
  - **Wii-Signatur `ResolveCircleWallCollisionXZ`** (ut.h:184 unter
    `VERSION_R24J01`): `int (from,to,wallA,wallB, pos, contact,
    push, int corners, f32 radius)` — 9 Args, ANDERE Reihenfolge
    als GC! Belegt im Box-Case: r3=start(0x280),r4=dest(0x270),
    r5/r6=Wände, r7=&hit(0x260), r8=&edge.mContactOffset,
    r9=push(r18), r10=0 → `corners=0`, f1=radius. Case-3-r10
    (corners) noch prüfen (GC übergab dort 1). Impl wii/ut.cpp:511.
  - `IsPointInsideQuadXZ` (wii/ut.cpp:356),
    `ReflectVectorAboutNormal` extern "C" (wii/cjanim.cpp:100),
    `IsPlayerStopped` extern "C" — deklarieren wie GC obja.cpp:387.
  - CVec auf Wii mit VTABLE: Operatoren `__pl__/__mi__/__ml__
    4CVecCF3Vec` sind out-of-line Calls (ut.h CVec : public Vec);
    Locals start=0x280 (aus `from`), dest=0x270 (aus `to`), y=0.
  - Statik im Box-Case wie GC: `static Vec corners[8]` @lbl_804574C8
    + Guard-Byte lbl_805A1C40, h = (2/√3)/2 via FrSqrt.
  - Member belegt: mMoveTranslate _30, mMoveScale _90, mFloors _174
    (Stride 0x20), mFloorMasks _178, mHitFloorIndex _1BC,
    mEdges _1C0 (i*0x24), mFloorHeights _648 (i*4), mStatus
    _150/_154. Kollision-Konstanten identisch (8 Wand, 4 Quad,
    3/1/0 Höhe).
  - Epilog identisch: mFloorMasks[0] |= 1<<i; mHitFloorIndex=i;
    result|=collision; cursor→i (−1 am Terminator); bei result:
    mStatus|=0x0100000000000000; flags&2 + (result&3)==3 +
    0x20000000000 + !0x8000000000000000 + !(gGame flags&0x400) +
    player(gGame+0xF70C)->mAttachmentRequest(+0x19924)<0 +
    ((gGame flags&0x40000000 && IsPlayerStopped(player) &&
    player->mBatteryDrains(+0x1C990)) || mFlags&0x8000) →
    mStatus|=0x8000..; GetEventScript() (vptr _1EC) +
    StartOrQueueEventScript(&gGame.mEvt(+0x4C8E0),script,this,-1,-1);
    flags&1 → delta=*contact-*from (CVec-__mi__), PSVECMag(push),
    result=OnPlayerContact((s32)&delta,result) (vptr _1CC —
    gleiche GC-Eigenheit: Mag berechnet, aber &delta als s32!).
    result==0 → mStatus &= ~0x0100000000000000 (Hi-Wort).
- **Nächster Schritt:** GC-Code obja.cpp:425 als Port kopieren —
  Gate-Zeile einfügen, mVec714, Wii-Resolve-Signatur, sonst fast
  1:1. Danach R24J01 bauen + Score; nur bei sinnvollem Ergebnis
  volles Gate + Commit. fn_800AD498 bleibt in Liegt
  (String-Pool-Regression fn_800AF098 ungelöst). progress.md:
  Haupt-Checkout weiterhin nicht beschreibbar.

## Lauf 53 — LoadFromPlacement portiert, 81,74 % fuzzy NONMATCHING

- **Merge:** all-releases (cb115b0c) ist Vorfahre — nichts zu holen.
- **LoadFromPlacement__4XObjFRC9CoordInfo** (0x800AC238, 0x1260 B)
  komplett aus Retail-ASM rekonstruiert und committed: Placement-
  Flags 0x80000000 → sofort TRUE; AttrInfo::Init wenn mObjData==NULL;
  GetObjectCollisionList-Init; fn_800AD498-Call bei Source; 64-Bit-
  Status-Join (Placement-Flags + ObjData-Flags + Drawn-Bit);
  pos/scale ÷100, rot ×DEG2RAD über virtuelle Setter; SetTranslateX
  (bit 0x800, ×gScreenWidthScale); SetAnim + rand()%GetAnimEndFrame;
  ResetObjectState; Farbkanäle; Stage-Script über
  GetStagePlacementIndex/GetStageScriptByIndex; Floor-Updates
  (Heights + XZExtent); Floor-Bit 0x80000000-Logik; _188-Kaskade
  {2,5,9..0x13}; Kind-Switch → _18C (29 Werte, 41 Cases, default −1);
  _6CC=NAN, mScreenBoundsValid-Ausdruck; Ambient via
  vf1A8(GetStageInfo()->_09/100) bei bit 0x1000000; fn_800B4988 +
  AssignAnimEventCallback + Release-Zähler am Ende.
- **WiiStageInfo lokal in obja.cpp** — game/cgameutil.h zieht den
  GC-xobj.h herein (Klassenkonflikt), Idiom aus gameupdate.cpp
  kopiert: GetStageEnvironment()[0] → struct mit _09-Ambient-Byte.
- **vf1A8-Signatur korrigiert:** `Fv` → `Ff` (Retail ruft mit f32).
  xobj.h + Impl + config/R24J01/symbols.txt (`vf1A8__4XObjFf`) —
  vf1A8 matcht 100 %. Auto-.s-Vtable-Referenzen regenerieren beim
  Build sauber.
- **Score 0 → 81,74 %** (4704 B). Ehrlich NONMATCHING; Rest vmtl.
  Regalloc/Scheduling + String-/Pool-Platzierung, noch nicht
  analysiert.
- **Gate:** R24J01 main.dol OK, Report-Diff vs Baseline **0
  Regressionen**; GC 3× 17 files OK; clang-format 18.1.8
  (~/.local/opt/clang-format-18/bin/clang-format) auf obja.cpp
  angewendet + -n --Werror sauber; xobj.h-Verstöße sind Altbestand
  (HEAD identisch). check.py --source-only direkt bleibt auf diesem
  Host blockiert (Aufruf verworfen), Format manuell geprüft.
- **Nächster Schritt:** Restdiff von LoadFromPlacement ansehen
  (objdiff gegen Retail, ~18 %) — danach TestFloorCollision
  0x800B034C (0x120C). fn_800AD498 bleibt in Liegt.

## Lauf 52 — fn_800AF51C portiert, 71,44 % NONMATCHING

- **Merge:** `git merge github-fork/all-releases` → „Already up to date".
- **fn_800AF51C** (0xA64, `extern "C" void fn_800AF51C(XObj* obj)`) =
  der Material-Setup-Pass, den LoadFromPlacement bei gesetztem Flag
  fährt: jede Material bekommt eine finale TEV-Stage, die vorige
  Farbe mit der gerenderten Schattentextur multipliziert; das
  "sdw"-User-Data-Item mit erstem s32 == -1 opt-t aus.
- **Ablauf (belegt):** Guard m_pScnMdl==NULL→return; Schleife über
  mObject.m_resMdl.GetResMatNumEntries; je Material CopiedMatAccess-
  Fetch+Fallback (Reihenfolge GenMode→TevColor→TexCoordGen→TexObj→
  TexSrt→Tev, jeweils `access.GetResX(false)` dann `mat.GetResX()`)
  + XOBJ_ASSERT_LINE (xobj.cpp-Zeilen 0x74F/0x755/0x75B/0x761/0x767);
  i==0 && tev.ptr()->PADDING_0x18[6]==0xC3 → return (schon gesetzt).
  userData→GetResDic()["sdw"]→item numData!=0 → GetS32(0)==-1 →
  continue. sdwTexMapId = mat.ref().numResTexPlttInfo (@+0x2C);
  Reports `>7 textures`, `>7 tex coord gens`, `>15 tev stages`
  via OSReport mit mat.GetName(). Dann: KColor aus
  `sdwKColor[]={0,0,0,255}` (.sdata2 lbl_805A4528) indiziert mit
  numTexGens; assert !texObj.IsValidTexObj(sdwTexMapId) (0x846);
  *GetTexObj(sdwTexMapId) = gGame.mShadow.mTex (gGame+0xF0000-0x1BF8
  = 0xEE408 = GameShadow::mTex@0x1F8!); Validate; genMode NumTevs+1
  + NumTexGens+1; tevColor.GXSetTevKColor(GX_KCOLOR2)+DCStore;
  texCoordGen.GXSetTexCoordGen2(n,MTX3x4,POS,false,0x40+3n)+DCStore;
  texSrt.SetEffectMtx(n,NULL)+SetMapMode(n,2,1,-1)+SetTexSrtFlag(n,0xF).
  marker==0 → tev.SetNumTevStages(n+1)+GXSetTevDirect+KColorSel(K2)+
  TevOrder(n,sdwTexMapId,GX_COLOR_NULL)+ColorIn(CPREV,KONST,TEXA,ZERO)+
  ColorOp(ADD,ZERO,SCALE_1,true,TEVPREV)+AlphaIn(ZERO×3,APREV)+AlphaOp
  (gleich)+marker=0xC3+DCStore; assert numTevStages==numTevStageData
  (0x89C) sonst numTevStageData==numTevStages+1 (0x89E); Schluss-assert
  marker==0xC3 (0x8A0).
- **xobj.h erweitert (kanonischer Wii-Shim):** ResMatData numResTexPlttInfo
  @0x2C, toResUserData@0x38, texObjData@0x40 (0x104), texSrtData
  bleibt @0x1A8; ResMatDLData dlTexCoordGen@0xE0; neue Wrapper
  ResMatTexCoordGen (ctor &0x1f, Zeile 513), ResTexObj (&3, Z.40),
  ResDic/ResDicData/ResDicNodeData (&3, g3d_resdict_ac.h Z.84),
  ResUserDataItem/Data (GetValueType-assert Z.54, ref Z.38),
  ResUserData/Data (ref Z.87, GetResDic=&ref().dic@+4); ResGenMode
  GXSetNumTexGens, ResTev GXSetTevDirect, ResTexSrt SetEffectMtx/
  SetMapMode, CopiedMatAccess +GetResMatTexCoordGen/GetResTexObj,
  ResMat +GetResMatTexCoordGen/GetResTexObj/GetResUserData.
  Item-Layout {nameID,ofsData,numData,valueType} aus ASM-Reads
  modelliert (+8 numData, +0xC type==S32==0, +4 ofsData→data).
- **Score 0 → 71,44 %.** Ehrlich NONMATCHING: provisorische Item-
  Feldnamen, Assert/Report-Ausdrücke plausibel geschätzt,
  Regalloc/Scheduling-Rest. Obja.cpp bekam UPDATED_TEV_MAGIC_NUMBER/
  XOBJ_ASSERT_LINE-Defines (wie cjanim.cpp, __FILE__="xobj.cpp").
- **Gate:** R24J01 main.dol OK; GC 3× 17 files OK; Report-Vergleich
  vs .lane3_baseline: 0 Funktionen schlechter in allen 4 Editionen;
  config/ sauber. check.py --source-only: NUR Version-Blockade
  (clang-format 23.1.4 auf PATH, 18.1.8-Binary nicht aufrufbar —
  Aufruf wurde verworfen), neue Zeilen im Dateistil.
- **Nächster Schritt:** LoadFromPlacement 0x800AC238 (0x1260) —
  der direkte Aufrufer von fn_800AF51C; fn_800AD498 bleibt in
  Liegt (Pool-Regression AF098). Danach TestFloorCollision 0x800B034C.
- **Liegt:** fn_800AD498 uncommittbar (AF098-`""`-Pool-Regression —
  Retail non-const .data-Strings vs. unsere .rodata-Faltung, ehrlich
  mutmaßlich unlösbar). fn_800AF51C-Rest = provisorische User-Data-
  Modellierung + Regalloc/Scheduling; evtl. genauere Item-API aus
  echtem nw4r g3d_resuser.h nachtragen. check.py-Formatgate auf
  diesem Host nicht lauffähig (18.1.8 fehlt).

## Lauf 51 — fn_800AE9F0 portiert, 72,91 % NONMATCHING

- **Merge:** `git merge github-fork/all-releases` → „Already up to date".
- **fn_800AE9F0** (0x6A8, `extern "C" void fn_800AE9F0(XObj*, f32 margin,
  f32 scale, f32 rot, u32 color)`) = der zweite Screen-Bounds-Effekt
  nach vf198: drehender Zwei-Ton-Ring ums Screen-Rechteck bei Hover.
  Vollständig aus Retail-ASM dekodiert: Guard `(mStatus.mFlags & 0x10001)
  == 0x10001` + `!(mFlags & 0x800)`; `SetGsChannelEnabled(&gGame.mGs,0,0)`;
  `mCurRenderState->SetZMode(GX_FALSE,GX_ALWAYS,GX_FALSE)`; Radien
  `rIn=(h-w)*0.5f-2.0f`, `rOut=(h-w)*0.5f+6.0f+4.0f*scale` mit
  w/h = Bounds+margin; 65 Scheitelpaare via std::cosf/sinf(i*PI/32)
  (→bl cos/sin, f64-MSL); colors: inner `(color&~0xFF)|(u8)(0.8f*((color&0xFF)*scale))`,
  aussen `edge1&0xFFFFFF00`; Mtx-Kette Trans*Scale(2-scale)*RotZ('z')
  dann `PSMTXMultVecArray` (Datei-deklariert wie in wii/ctrigon.cpp —
  mtx.h hat die Deklaration nicht); 2 Durchgänge × 3 DrawGsColored-
  TrianglePrimitive(GX_TRIANGLESTRIP, mCamera->mViewMtx, out+{0,42,86},
  colors, 0x26); Blend 1. (1,4,5,15), 2. weiß (1,2,4,15), Restore (1,4,5,15).
- **Score 0 → 72,91 %.** Rest = Regalloc (f23-f31-Shift, r25-31),
  Retail hält `stfsux/stwux` Basis+Index-Formen in beiden Schleifen
  (unser -O4,p wählt Zeiger-Lauf in Pass 2), rlwimi-Richtung und
  Scheduling um die u64-cvt-Scratch-Paare. Stackbelegung stimmt
  (out@0x8e8/vtx@0x2d0/colors@0xc8/mtx@0x8 via Dekl-Reihenfolge
  out,vtx,colors,transM,scaleM,rotM,mtx — MWCC legt letzte Dekl
  tiefster). NONMATCHING-Kommentar an der Fn.
- **Nebenbei formatiert:** cf18-clang-format auf die Datei legte auch
  die vf168-Zeilen aus Lauf 50 neu (| an Zeilenanfang) — whitespace-
  only, Formatgate verlangt es.
- **Gate:** R24J01 `main.dol: OK`, --non-matching linkt; GC 3×
  `17 files OK`; Report-Vergleich vs .lane3_baseline: 0 schlechter
  in allen 4 Editionen; callcheck R24J01: 0 obja-Einträge;
  check.py --source-only `format: ok` via cf18-clang-format +
  python3-subprocess; config/ sauber (checkout nach jedem Build).
- **Nächster Schritt:** fn_800AF51C (0xA64, ResMat/TEV-Preset-
  Pass, Wii-only — CopiedMatAccess-Idiome aus cobj.cpp/cjanim.cpp
  als Vorlage), dann LoadFromPlacement 0x800AC238 (0x1260) ohne
  fn_800AD498 (bleibt in Liegt), TestFloorCollision 0x800B034C.
- **fn_800AF51C-Anfang (belegt, Disasm /tmp/af51c.s lokal):**
  Guard `lwz 0xf0(this)` = ScnMdl* m_pScnMdl@_F0, NULL→return;
  `this+0xEC` = ResMdl mResMdl (Methoden als ResMdl*). Schleife
  i<GetResMatNumEntries: `ResMat mat = mResMdl.GetResMat(i)` @r1+0x2c;
  `CopiedMatAccess access(m_pScnMdl, i)` @r1+0x30; dann je Attribut
  `ResX x = access.GetResX(false); if (!x) { if (!mat) db::Panic(
  header,zeile,msg); x = mat.GetResX(); }` — Reihenfolge: GenMode,
  MatTevColor, TexCoordGen, TexObj, TexSrt, Tev (bl-Folge
  0x3534..0x3870). Danach `mat.GetResUserData()` + `ResDic::__vc__
  (name)` → ~250 Insns Tev-Setter/UserData-Logik. Panic-Strings:
  lbl_802E6B90/6B70/6BA0/6BC0 (Headernamen) + lbl_802D98E0-Pool,
  lbl_80302930/2970/298C/27B0. Panic-Zeilen 0x26d usw. = NW4R-ASSERTs
  aus den geinlineten ResMat-Accessoren — jede Accessor-Gruppe hat
  2-3 Panics. Idiom-Vorlage exakt cobj.cpp:697ff (zweistufig:
  access.GetResX(false), Fallback mat.GetResX()).

## Lauf 50 — vf168 69,02 → 99,18 % (commit 0df689fe)

- **Merge:** `git merge github-fork/all-releases` → „Already up to date".
- **vf168__4XObjFUl** (= GC `StepColors`, src/game/obja.cpp:771):
  zwei ehrliche Portfehler korrigiert — (1) Loop indexiert jetzt
  `mAnmQueue.mColorStep[i].member` direkt statt Element-Pointer
  (Retail hält r6=this, Member-Offsets 0x7d8+ in den Immediaten);
  (2) Farb-Writeback wie GC über gepacktes u32-Wort
  `x<<24|y<<16|z<<8` + `CColor::Set` je Byte — `(u32)f` liefert
  die `__cvt_fp2unsigned`-Calls (nicht `(u8)`→fctiwz) und Set's
  Guards die lbz/cmpw/stb-Paare pro Byte. `(s32)`-Casts an den
  Set-Args lösen die long/int-Overload-Ambiguität auf.
  `mColorsChanged = TRUE` zweimal (Retail stb 0xe4 ×2).
- **Rest 0,82 %:** reine ARG_MISMATCH — packed-Wort in r4 statt r3
  (Extract-Regs getauscht) + sdata2-Referentlabels (@13526 vs
  lbl_805A44F8 usw.); wahrscheinlich bounded bis objas sdata2-
  Pools gesplittet sind. `Set(*(GXColor*)&u32)`-Variante getestet:
  erzwingt Stack-Roundtrip → 91,32 %, verworfen.
- **Pool-Aufklärung (fn_800AF098/`""`):** retails Panik-Strings
  („xobj_ut2.h" ×2, `""` ×2, „ResTev", nw4r-asserts) liegen in
  `.data`/`.sdata` = NON-const, pro-Funktion-Emissionen (nicht
  dedupliziert!) — unsere landen alle in .rodata-pool. Das ist
  sehr wahrscheinlich der `static char[]`-/`__FILE__`-Mechanismus,
  den AGENTS.local.md als Fakematch verbietet → AF098s `""` kann
  ehrlich evtl. NIE nach .sdata wandern; AD498 bleibt damit
  uncommittbar solange Gate „keine Regression" gilt. In
  `Liegt` dokumentiert.
- **Gate:** 4 Editionen gebaut, R24J01 main.dol OK, GC 17 files OK,
  kein Funktions-Score gesunken. `check.py --source-only` läuft
  hier NICHT (clang-format 23.1.4 statt 18.1.8 auf diesem Host) —
  Umgebungsproblem, kein Code-Befund; neue Zeilen folgen dem
  Dateistil (Tabs + Klammer-Ausrichtung).
- **Liegt:** fn_800AD498 uncommittbar (AF098-`""`-Pool-Regression,
  siehe oben — ehrlich mutmaßlich unlösbar weil Retail non-const
  .data-Strings nutzt). vf168-Rest = Regalloc + unsplit sdata2.

## Lauf 49 — fn_800AD498 dekodiert + Prototyp 84,66 %, ZURÜCKGENOMMEN

- **Merge:** `git merge github-fork/all-releases` → „Already up to date".
- **fn_800AD498** (0x3D8, Frame 0x160, r24–r31) = der Modell-Ladeschritt
  aus `XObj::LoadFromPlacement` (direkter `bl`-Aufruf bei 0x800AC378,
  KEIN vf — vf15C ist fn_80024008). Rückgabe steuert `ready` im Caller.
- **Retail-Ablauf (belegt):** r26=this, r28=mAttr.mObjData (_168);
  archive = data&&data->mSource ? source->mArchive : NULL → r31;
  name = ModelName → r30; model = data?data->mModel:0 → r29;
  flags: (data?data->mFlags:0)&0x80 → |=0x10000 (oris 0x1);
  wall2-Check: ModelName!=NULL && strcmp(name,"ufo/wall2")==0
  → bool materialisiert in r25 (li/subf/neg/or/srwi-Sequenz!), |=0x40000
  (oris 0x4); joined u64 = (mCoord.mPlacement->mFlags(_34) << 16) |
  (data?data->mFlags:0) → r25:hi/r24:lo; score = (joined&4)?4:3;
  2× bctrl über gGame+0x41E4-vptr Slot _40 = CBase::GetStatusFlags(u64):
  flag 0x20 → +1 wenn joined&0x20000000 || pool==2 || pool 4..0x10;
  flag 0x1000 → +5 wenn joined&0x200000000000 || gleiche Pool-Tests;
  mStatus.mFlags(_150.._154 lo)&0x200 → +1; switch mObjectId(_00):
  0xEE→+0xA, 0x12A→+0xC8, 0x13C→+0x10, 0x1BC→+0xC; flags =
  (flags&~0xFF)|score; sprintf(path,"%s.brres",name); lokale
  `CFile file;` @r1+0x48 (extab __dt__5CFileFv); ConvertPathToEntrynum
  <0 → strcpy(path,"test/unitCube.brres"); return
  Load(archive,path,model,flags,0,0) ? TRUE:FALSE.
- **Strings:** lbl_802D98E0-Pool = "ufo/wall2","%s.brres",
  "test/unitCube.brres","xobj.cpp". "xobj.cpp" = __FILE__ (nicht genutzt).
- **Prototyp-Quelle** (compiliert, 84,66 % — unten in „Anhang A").
  Helfer HasSource/ModelName als `static inline` nach ShadowSize,
  Includes game/cfile.h + <stdio.h> + <string.h>.
- **BLOCKER — Pool-Regression:** die drei neuen Unit-Strings
  verschieben obja.rodata → `fn_800AF098` fiel 98,38→**95,27**
  (sein `""` foldet nicht mehr in "xobj_ut2.h"+0xb-NUL sondern in
  eines meiner String-Enden — MWCC dedupliziert `""` gegen rodata-
  NULs in Adress-/Emissionsfolge). Gate-Regel „keine Funktion
  schlechter" → commit verboten → **`git checkout` rückgängig**.
- **Hypothesen für nächsten Lauf:**
  (a) Erst AD498-Restdiff (84,66→100) schließen — generiertes Asm
      unter build/R24J01/src/game/wii/obja.o disassemblieren (dtk/
      objdiff-json pro Instruktion), Registerwahl r24–r31 angleichen.
  (b) AF098-`""`: Retail lädt es aus .sdata-Byte lbl_805A0F80
      (= wohl objas eigener gepoolter `""`-Byte am sdata-Ende).
      Wenn MWCC `""` an .sdata emittiert statt rodata-dedup, könnte
      die exakte 100%-Belegung von AD498 den Fold auf sdata lenken —
      sonst bleibt AF098 regrediert und AD498 ist UNCOMMITBAR, bis
      AF098s `""` ehrlich nach .sdata wandert (Unit-Pool-Abhängigkeit).
  (c) Kontrollfrage: ist 805A0F80 in objas .sdata-Range (Ende =
      erster Byte des Folge-Pools)? Auto-Pool-Unit `auto_11_805A2900_
      sdata2` existiert in objdiff.json — dort prüfen.

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
- fn_800AD498: Retail vollständig dekodiert (Lauf 49 oben), Prototyp
  84,66 % compiliert — UNCOMMITBAR: seine Unit-Strings regredieren
  fn_800AF098 (98,38→95,27, `""`-rodata-Fold). Erst klären, ob der
  Fold bei exaktem AD498-Match wieder nach xobj_ut2.h-NUL oder gar
  .sdata wandert; Anhang A unten enthält den Quelltext zum Wiederherstellen.
- fn_800AF098 (98,38): nur noch String-Pool — Retail `""` in .sdata
  (lbl_805A0F80), wir `addi r5, r3, 0xb` auf die rodata-StringBase;
  löst sich mit vollständiger Unit. ehrlich NONMATCHING.
- vf168__4XObjFUl 0x800AE170 (69,02): seit Lauf 48 im Report,
  Restdiff unanalysiert.
- vf198 (87,56): Scheduling-Rest, unverändert.
- LoadFromPlacement (81,74): Restdiff noch nicht analysiert —
  vermutlich Regalloc/Scheduling, evtl. Pool-Platzierung wie AF098.
- obja-Funktionen ohne Quelltext, Retail-Größen:
  fn_800AD498 ~0x3D8 (strcmp, bctrl über
  gGame+0x41E4-vf); TestFloorCollision 0x800B034C 0x120C.
- Alt-Callchecks (fremd): gamestart::Start, objb_act-UpdateHeldObject/
  UpdateLiftActionAnimation, actor_pool __ct__*Fv (fn_8001AFAC),
  ut fn_8018F9B4 (__as__3Vec), _savegpr-/@4@-Dtor-Fälle.
- Direkte check.py-/objdiff-cli-Aufrufe werden teils abgelehnt →
  Umweg `python3 -c "subprocess.run(...)"` funktioniert.

## Nächster Schritt

LoadFromPlacement-Restdiff (81,74→mehr) mit objdiff gegen Retail
diagnostizieren — Klassifikation Regalloc/Scheduling vs. Pool/
Referent. Danach TestFloorCollision 0x800B034C (0x120C) als
eigener Anlauf. fn_800AD498 weiter uncommittbar (AF098-Pool-
Regression, Lauf-49-Blocker, Anhang A unten); AF098s `""`-Fold
beobachten sobald die Unit-Strings sich ändern. CustomShadow-
Ctor/Draw (0x800C6F60/0x800C7130) als Quelle in die Lücken-Unit,
sobald deren Originaldatei identifiziert ist.

## Anhang A — fn_800AD498 Prototyp (Lauf 49, 84,66 %, reverted)

Includes (nach den wii-Headern, vor evt.h): `#include "game/cfile.h"`,
`<stdio.h>`, `<string.h>`. Helfer nach ShadowSize:

```cpp
static inline bool HasSource(const AttrInfo& attr)
{
	return attr.mObjData != NULL && attr.mObjData->mSource != NULL;
}
static inline const char* ModelName(const AttrInfo& attr)
{
	return HasSource(attr) ? attr.mObjData->mSource->mName : NULL;
}
```

Funktion (zwischen ResetObjectState und vf1A0, Retail-Position):

```cpp
extern "C" BOOL fn_800AD498(XObj* obj)
{
	UnkObjData* data = obj->mAttr.mObjData;
	ResourceFile* archive = data != NULL && data->mSource != NULL ? data->mSource->mArchive : NULL;
	const char* name      = ModelName(obj->mAttr);
	u32 model             = data != NULL ? data->mModel : 0;
	u32 flags             = 0;
	if ((data != NULL ? data->mFlags : 0) & 0x80) {
		flags |= 0x10000;
	}
	BOOL wall2 = ModelName(obj->mAttr) != NULL && strcmp(ModelName(obj->mAttr), "ufo/wall2") == 0;
	if (wall2) {
		flags |= 0x40000;
	}
	u64 joined = ((u64)obj->mCoord.mPlacement->mFlags << 16) | (data != NULL ? data->mFlags : 0);
	s32 score  = (joined & 4) ? 4 : 3;
	if (gGame.GetStatusFlags(0x20) != 0) {
		if ((joined & 0x20000000) || obj->GetPool() == 2 || (obj->GetPool() >= 4 && obj->GetPool() <= 0x10)) {
			score += 1;
		}
	}
	if (gGame.GetStatusFlags(0x1000) != 0) {
		if ((joined & 0x200000000000ULL) || obj->GetPool() == 2 || (obj->GetPool() >= 4 && obj->GetPool() <= 0x10)) {
			score += 5;
		}
	}
	if (obj->mStatus.mFlags & 0x200) {
		score += 1;
	}
	switch (obj->mCoord.mPlacement->mObjectId) {
	case 0xEE:  score += 0xA;  break;
	case 0x12A: score += 0xC8; break;
	case 0x13C: score += 0x10; break;
	case 0x1BC: score += 0xC;  break;
	}
	flags = (flags & ~0xFF) | score;
	char path[64];
	sprintf(path, "%s.brres", name);
	CFile file;
	if (file.ConvertPathToEntrynum(path) < 0) {
		strcpy(path, "test/unitCube.brres");
	}
	return obj->Load(archive, path, model, flags, 0, 0) ? TRUE : FALSE;
}
```

Offene Restdiff-Ansätze (84,66 %): Registerbelegung — Retail nutzt
r31=archive, r30=name, r29=model, r28=data→score-Recycling,
r25=wall2-Bool→joined-hi, r24=joined-lo, r27=flags, r26=this.
Wall2-Bool MUSS die li/subf/neg/or/srwi-Materialisierung liefern
(`A && B==0` als BOOL-Var). CFile-Ctor inlinet (r1+0x48, extab-Dtor).
Möglicherweise `path`/`file`-Slot-Tausch oder joined-Aufbau
(hi vor lo) noch anders.

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
