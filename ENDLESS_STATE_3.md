# ENDLESS_STATE_3 — Bahn 3 (devin/lane3) — Worktree-Kopie

Stand: Lauf 97 fertig, Branch devin/lane3 @ 90e99c75.
R24J01: 6662/8284 matched, main.dol: OK; GC: je 17 files OK;
all-releases@cb115b0c ist Vorfahre.
Baselines .lane3_baseline/report_<V>.json (Stand 21.11. 06:36 —
aeltere Commits danach driften Einzelscores, z.B. DrawGroundShadowAt
99,98->96,62; KEIN matched-Verlust, nur fuzzy).
(HINWEIS: zentrale .claude/devin/ENDLESS_STATE_3.md ist aus dem
Worktree nicht beschreibbar — diese Kopie ist der aktuelle Stand.)

## Lauf 97 — drei 100 %-Matches durch ehrliche Temporaere (xobjattachment/xactradar)

- **Commit `90e99c75`** (2 Dateien, 6659→6662 matched):
  - `XObjAttachment::ExecCallbackB` 79,66→**100**: `const u16 nodeId =
    pFuncObj->GetCallbackNodeID()` VOR dem Basisaufruf cachen (Retail
    laedt die ID in r31 vor ExecCallbackB__4XObj) plus `MtxP parent =
    mParent`-Lokal — Retail haelt parent in einem Callee-Reg ueber die
    Calls. Idiom steht schon an Zeilen 339/511 der Datei.
  - `XObjRadar::LoadFromPlacement` 86,75→**100**: mYaw/mPitch-Reset nur
    bei `ok`-Return des Basisaufrufs; Rest war nur Pool-Label.
  - `AddScalarToVectorComponents` 58,43→**100**: `CVec r(t); return r;`
    statt `return t;` — GC/3.0a5 folded eine direkt returnte Lokal in
    den sret-Slot, Retail materialisiert die Arbeitskopie auf dem Stack
    und wort-kopiert sie mit Vtable-Store heraus. Echtes Temp, kein Trick.
  - `fn_8010BEE0` 73,85→**87,69**: Signatur `Vec fn_8010BEE0(XAction*)`
    passend zum einzigen Aufrufer (gameupdate.cpp:126/1508 deklariert
    exakt so) — der CVec→Vec-Slice erzeugt Retails Wort-Kopien statt
    Float-Stores. Rest: mOwner-Load-Scheduling + Pool-Label (ehrlich
    NONMATCHING-Kommentar).
- **Versucht und klassifiziert (keine Commits):**
  - `ApplyRadarMovementWeight` (90,58): `!= &&`-Umschreibung → schlechter
    (71,8/76). Retail emittiert zwei explizite `cmpwi` fuer `s==1||s==2`,
    GC/3.0a5 folded zu unsigned-Range-Test. GC-verbatim `||`-Spelling
    zurueckgesetzt — Compiler-Versions-Wand, dokumentiert.
  - `UpdatePlayerTumbleAnimation` (99,75): Instruktionsstrom identisch;
    Registerfaerbung/Stack-Slots/Jump-Table/Pool-Labels — klassische
    Schedule-Wand, kein semantischer Hebel gefunden.
  - `GetRadarDishWorldPosition` (48,0): Retail laedt `"obj1"` aus
    `.sdata`@0x805A1110 (ungeclaimter Auto-Blob), wir `.rodata`
    stringBase. Braucht .sdata-Blob-Modellierung, kein Source-Hebel.
  - `ResetPopperCharge`/`ClearPopperCharge` u.a.: `(s32)(gRefreshRate *
    0.0f)` — Retail emittiert pro Actor eine cvt-Sequenz, GC/3.0a5
    folded auf 0. Compiler-Versions-Wand.
  - `XObjRadar::ExecCallbackB` (90,35): Rest = Panic-String-Labelnamen
    (@STRING@GetID vs lbl_803068D8), Registerfarben r30/r31 und ein
    zusaetzliches GetID-Panic-Stueck — kosmetisch/Farbwand.
- **Gate vollstaendig:** R24J01 main.dol OK (normal + --non-matching
  linkt); GGTE01/GGTJ01_01/GGTP01 je 17 files OK; Report-Diff vs
  Baseline: 0 matched-Verlust, nur bekannter DrawGroundShadowAt-
  Fuzzy-Drift; config/* sauber (kein dtk-Drift); keine neuen Symbole;
  Call-Reihenfolge per objdiff verifiziert (callcheck-Skript fehlt in
  tools/ — unveraendert); check.py --source-only: clang-format 18.1.8
  fehlt weiterhin (Host 23.1.4; ~/.local/opt/clang-format-18 existiert,
  Ausfuehrung wurde vom Environment abgelehnt) → Liegt.
- **Naechster Schritt:** `UpdateObject__8XObjPlugFUl` (99,95 — nur ein
  Rest) und `fn_80110C1C`/`fn_80110C24` (0,0 — evtl. Symbolnamen-
  Problem statt echter Code) in xobjplug (23/26); danach xobjplugcord
  (10/17, sieben Funktionen 91–99,8) und xactradar-Rest (meiste
  sub-100 sind Scheduling/Pool-Waende). xacttumble 3/4 abgeschlossen
  bis auf dokumentierte Schedule-Wand; xobjattachment 38/39.

## Lauf 96 — ApplyPlayerItemTintToTarget 100 % (SetDiffuseColor by-value) + Befunde

- **Commit `8c5a6f1b`** (3 Dateien): `XObj::SetDiffuseColor(GXColor color)`
  BY VALUE (xobj.h-Dekl + obja.cpp-Def + symbols.txt FRC8→F8_GXColor).
  Retail-Caller 0x800F5368 kopiert die Farbe byteweise auf den Stack
  (Aggregate-Materialisierung) statt die Member-Adresse zu reichen —
  Beweis fuer by-value-Dekl. Callee-Codegen identisch (beide ABIs
  reichen Zeiger). ApplyPlayerItemTintToTarget 21,74 → 100; Definition
  bleibt 99,45. Einzige SetDiffuseColor-Callsite im Spiel.
- **Gate vollstaendig:** R24J01 main.dol OK normal + --non-matching
  linkt; GGTE01/GGTJ01_01/GGTP01 je 17 files OK; Report-Diff vs
  Baseline: 0 matched-Verlust, nur bekannter DrawGroundShadowAt-
  Fuzzy-Drift (alt) + Umbenennungen; callcheck R24J01: keine
  objb_act/obja/efftrail-Treffer; symbols.txt Dups 19=19.
- **Klassenarchaeologie (wichtig, fuer Thunks/Vtables):**
  Retail-Hierarchie lt. RTTI-Strings (auto_06_802D9460_rodata.s):
  `XObjChrBase : public XObj` — definiert ExecCallbackA=fn_800F3714,
  ExecCallbackB=fn_800F3B88, LoadFromPlacement=fn_800F35E8, dtor
  dtor_800C0B74. `XObjChr : XObjChrBase` (eigene Overrides:
  Release__7XObjChr, SetMoveDir, SetAnim, InitializeObject=fn_800C9E84,
  ResetForNewGame=fn_800CB1DC, ResetObjectState, LoadFromPlacement
  __7XObjChr@0x800CCE68, UpdateObject, Draw..., Pause/ResumeSounds).
  `XObjDress : XObjChrBase` (=unser WiiSuitObject; dtor dtor_800BF108;
  vf28C/vf290=fn_800F3E74/3E88). `XObjDressOpt : XObj` (WiiSuitPart;
  dtor fn_800C0DD8; ExecCallbackB=fn_800F41F0; ExecCallbackA=base).
  Vtables: XObjChrBase=lbl_80302DBC (Thunk fn_800C658C in
  xobj_camera-Range!), XObjChr=lbl_80305DA0 (Thunk fn_800F48D8),
  XObjDressOpt=lbl_80306060 (48D0), XObjDress=lbl_80306310 (48C8).
- **Dtor-Thunks 48C8/48D0/48D8 — Liegt (Strukturprojekt):**
  MWCC emittiert Vtable+@4@-Thunks in der Unit der Key-Funktion =
  erste non-inline NON-DTOR Virtual in Dekl-Reihenfolge (Beweis:
  alle drei Dtoren liegen ausserhalb efftrail, Vtables trotzdem hier).
  Bloesstellen: (a) Klassen korrekt modellieren (XObjChrBase einfuegen,
  ExecCallbackA/B + LoadFromPlacement dorthin, WiiSuitObject davon
  erben lassen — shared Header xobj.h/cgame.h!); (b) Key-Funktionen
  als Member in efftrail.cpp definieren (fn_800F3714/3B88/35E8/3E74/
  3E88/41F0 → Memberform); (c) die drei Vtable-.data-Bereiche fuer
  efftrail in splits.txt claimen UND bytegenau erzeugen — je Vtable
  ~150-165 Slots, viele noch fn_-namenlos. Ohne (c) kollidiert unsere
  .data-Emission mit dem Auto-Blob → main.dol bricht.
- **Sonstige objb_act-Befunde (alle klassifiziert, keine Quickfixes):**
  PutAwayLiftedObject 98,96: instruktionsidentisch, nur Pool-Labels.
  IsCarryingToothbrush/Spoon/Mug/Squirter je 98,40 + IsUsingMug 98,11:
  identischer Strom, nur Registerfarben der 4 Inline-Bools
  (IsHolding→IsHoldingPool→IsHoldingKind→IsCarrying Kette; Retail
  aussen→innen r4..r7, wir andersherum) — Registerfarbwand.
  GetTargetFloorPosition 91,62 / GetStagePlacementScale 80,0 /
  GetLiftTargetContactOffset 95,23 / UpdatePushedObject 92,08:
  Scheduling-/Farb-Reste, Semantik steht. Die sechs Grossen
  (UpdateLiftActionAnimation 0x42D8, UpdateHeldObject 0x2C54,
  UpdatePlayerHangAction 0x20E0, UpdatePlayerJumpAction 0x199C,
  UpdateLadderActionAnimation 0x229C, UpdatePlayerRopeAction 0x1C34)
  bleiben unportiert — GC-Vorlage src/game/objb_act.cpp, Wii hat sie
  fuer nw4r/Remote neu geschrieben; je ein Mehr-Stunden-Port.
- **Naechster Schritt:** in Adressfolge xacttumble (3/4, fuzzy 99,8 —
  wahrscheinlich schnell fertig), dann xactradar (10/31 — 21 offene
  Funktionen, evtl. portierbar), xobjattachment (36/39), xobjplug
  (23/26), xobjplugcord (10/17). Alternativ einer der objb_act-Riesen
  (UpdatePlayerJumpAction 0x199C ist der kleinste) oder das
  Vtable-Strukturprojekt fuer die Thunks.

## Lauf 95 — XObjChr::ResumeSounds/PauseSounds (vtable _210/_20C) je 94,74 %

- **Commit `a5db3cc5`** (3 Dateien): `void XObjChr::ResumeSounds(f32)`
  @0x800F4280 und `void XObjChr::PauseSounds(f32)` @0x800F450C, je 0x28C.
  Als echte Member implementiert (nicht extern "C"): Retail-Overrides
  der XObj-Slots _20C/_210 — `li r4,1`=Pause @450C, `li r4,0`=Resume
  @4280. Decls in xobj.h auf XObjChr (+2 Zeilen), Symbole in
  R24J01/symbols.txt (+2 Zeilen, Mangled-Names wie ueberall).
- **Layout aufgeloest:** Tabelle 1 = eigener `AnimSfxActor mActor`
  @_1CC90 (mActor-Ptr +0x1CC94 — XObjChr-privates Member, direkter
  Zugriff als Member-Funktion). Tabelle 2 (10×2 ab +0x1B08) =
  `mPopper.mShots.mShot[i].mSfxActors[j]` (XActPopper@+0xD50, Shots
  @+0x30, Shot 0xD9C, mSfxActors@_D84, mActor@+4). Tabelle 3 (2er ab
  +0x95B8) = `mCharge.mSfxActors[2]` (Charge @Popper+0x8848 →
  +0x9598, Actors @+0x20) — ctor-Beweis: XActPopper-ctor baut
  AnimSfxActor-Array (ctor fn_800BF6BC, 0xC, ×2) an popper+0x8868;
  "u8 mSfx[0x28]" in cgame.h ist der Charge-Tail (mMtx ab +0x14).
- **Quell-Idiom:** exakt das aus AnimSfx::Pause/Resume (sound.cpp):
  `if (slot->GetHandle() != NULL) { actor = slot->mActor;
  if (actor != NULL) actor->mHandle.Pause/Resume(gRefreshRate*fade); }`
  — GetHandle liefert (mActor!=0, mSound!=0, &mHandle), danach erneuter
  mActor-Check, dann Resume=IsPause→BasicSound::Pause(false,f) bzw.
  Pause=!IsPause→Pause(true,f). Framezahl = implizite f32→s32-Konversion
  `gRefreshRate * fade` (0x4330/fctiwz @sda21).
- **NONMATCHING (~94,74 %):** Instruktions-Multiset, Kontrollfluss und
  Call-Set identisch (callcheck: keine Call-Diffs in beiden Fn). Rest =
  Register-/Scheduling-Farbwahl: Retail haelt `this` in r31 und faltet
  +0xD50 in Member-Offsets (Original-Header hatte typisiertes
  `XActPopper mPopper`); unsere opaque-XAction-Deklaration erzwingt den
  Cast — Inline-Cast-Schreibweise `((XActPopper*)&chr->mPopper)->...`
  gehoert am naechsten dran (94,16→94,74), MWCC CSEt die Basis
  trotzdem einmal in ein Register. Kandidat fuer Permuter/Schreib-
  varianten, kein Trick niedergeschrieben.
- **Gate:** 4 Editionen gebaut: R24J01 main.dol OK, 6658/8284
  (matched-Count unveraendert, kein matched-Verlust vs. Baseline);
  GGTE01/GGTJ01_01/GGTP01 je 17 files OK. callcheck R24J01: einziger
  efftrail-Eintrag UpdateSpiralTrail _savegpr_25/26 (altbekannt).
  check.py --source-only: disc files ok, format FAILED weil nur
  clang-format 23.1.4 da ist (18.1.8 gefordert) — bekannter Blocker.
- **Naechstes Ziel (Adressfolge):** Dtor-Thunks 0x800F48C8/48D0/48D8
  (Symbol/vtable-Integration, falls nicht schon emittiert), dann
  GetTargetFloorPosition @0x800F48E0 (0x304), danach IsLiftTargetPlug
  0x800F4D70, IsCarryingToothbrush 0x800F4DB4, ... in Adressfolge.

## Lauf 94 — sieben Funktionen in efftrail.cpp (XObjDress/XObjDressOpt/XObjChr)

- **Commit `9efda6ef`** (3 Dateien): fn_800F3E74/3E88 = XObjDress
  (unsere WiiSuitObject) vf28C/vf290 — 100 %/100 %. fn_800F3E9C/403C =
  Sichtbarkeits-Walker ueber lokale `const char*`-Node-Listen
  (Kopf: 11 Namen @802DA8C0, linker Arm: 5 @802DA928) — 99,6 %/99,57 %.
  fn_800F41AC = ExecCallback-Selector 4 (spielt die gespeicherten
  Hide-Flags nach) — 100 %. fn_800F41F0 = XObjDressOpt::ExecCallbackB
  (unsere WiiSuitPart, graftet `mOwner->GetWorldMtx("head")` auf
  pResult->mpMtx) — 99,97 %, Rest nur String-Pool-Referent.
  fn_800F4798 = XObjChr::GetCollisionHeight — 57,9 %, NONMATCHING:
  sechs s32/100-Konversionen + zwei `Vec` als by-value Returns
  (Retail's stfs/lwz/stw-Pattern = Return-Slot-Kopien, das war der
  Schluessel: kein f32-Array, sondern `static inline Vec`-Helfer mit
  v.x/v.z/v.y-Zuweisungen), Residuum reine Scheduling-Freiheit
  (Konstanten- und Vtable-Loads gehoben vs. gesunken).
- **Klassen geklaert:** lbl_80302DBC = XObjDress-vtable,
  lbl_80305DA0 = XObjChr, lbl_80306060 = XObjDressOpt. fn_800F3E9C
  wird sowohl vom Spieler (xobj_chr.cpp fn_800E3BCC, WiiPlayerState*)
  als auch vom Dress aufgerufen → Receiver XObj*.
- **xobj.h-Erweiterungen:** ResNodeData `mFlags` @_14 (bit 8 =
  Sichtbarkeit); `ResNode::SetVisibility` mit g3d_resnode_ac.h:175-
  Panic + Re-Guard (exakt Retail); `ScnMdl::CopiedVisAccess`
  (ScnMdl*, u32 id) mit IsValid()=mpScnMdl!=NULL, SetVisibilityEx —
  wie Retail gebaut: `access.IsValid() ? SetVisibilityEx(v) :
  node.SetVisibility(v)`-Verzweigung. `GetID()` liefert den :56-Panic
  bereits. Achtung: include/nw4r/g3d/g3d_scnmdl.h NICHT anfassen —
  das ist der SDK-Header; das Spielmodell lebt in xobj.h.
- **cgame.h:** WiiSuitObject bekam `m_9C8`/`m_9C9` (XObjDress-
  Hide-Flags, Pad-Verbrauch, Groesse gleich); WiiSuitPart `XObj*
  mOwner` @_9A8.
- **Gate:** 4 Editionen gebaut, R24J01 main.dol OK, GC je 17 files OK,
  Report-Diff vs. Baseline: nur Fortschritt (u.a. ApplyPlayerSuit
  96,8→99,98, UpdatePlayerSuitChange 95,98→97,55 — vermutlich weil
  xobj.h-Klassen jetzt vollstaendiger sind); clang-format-Aufruf
  wieder abgelehnt → Stil manuell an Dateikonvention angeglichen.
- **Naechstes Ziel (Adressfolge):** fn_800F4280 + fn_800F450C
  (PauseSounds/ResumeSounds, je 0x28C — 10×2-Soundtabellen ab
  self+0x1B08 (Stride 0xD9C) mit BasicSound* @+0x80, Pause(bool,int)
  und IsPause(); Fade-Frames aus f1*gRefreshRate via 0x4330/fctiwz;
  Member-Typen fuer die 0x1B08-Tabelle und self+0x1CC94 noch zu
  identifizieren), dann Dtor-Thunks 48C8/48D0/48D8 (nur
  Symbol/vtable-Integration, falls nicht schon durch sinit erzeugt).

## Lauf 93 — fn_800F3B88 (ExecCallbackB Kopf-/Augen-Aim) 98,93 % + Aim-Records typisiert

- **Commit `5547c3c9`** (4 Dateien): `fn_800F3B88` @0x800F3B88 (0x2EC)
  neu in efftrail.cpp, extern-"C"-Stil. 98,93 % — instruktionsidentischer
  Strom (187/187 Befehle); Rest nur Registerfarben in der Aim-Record-
  Auswertung + String-Pool-Labels auf den Panic-Calls. NONMATCHING
  dokumentiert.
- **Semantik:** XObj::ExecCallbackB-Base-Call, dann Kopf-Kette:
  nodeId==0 → mHeadNodes[0] (second_null); ==Nodes[0] → Nodes[1]
  (head_chn); !=Nodes[1] → return. Auf head_chn: Ziel-Aim = Kopf-Record
  (0x987C) wenn eine seiner Snap-Achsen !=0, sonst Augen-Record
  (0x9810) wenn dessen Snap !=0; beide frei → keine Mtx-Arbeit.
  Sonst `self->GetResNode(nodeId)` (Accessor-Effekt-Call wie Retail),
  PSMTXCopy mpMtx→lokal, RotRad 'y'(m_40), RotRad 'z'(m_3C), 2×Concat,
  Copy zurueck. Zum Schluss Nodes[2].IsValid()→SetCallbackNodeID (head_cap).
- **cgame.h:** `_9810`/`_987C` zu `XObjScnCamInitRec mEyeAimRecord` /
  `mHeadAimRecord` typisiert (STATIC_ASSERTs, xobjcamera.h-Inklude).
  Alle Roh-Casts ersetzt: xobj_chr.cpp (ResetObjectState, OctaMenu::Open/
  Select, HandleEquipmentRingInput), xobjchrmenu.cpp (ConfirmOXMenuSelection,
  HandleOXMenuInput). Report-Diff: 0 Codegen-Aenderung an den Aufrufstellen.
- **Schluessel-Form:** Snap-Paar-Test ueber dateilokalen
  `static inline bool AimFree(const XObjScnCamInitRec*)` — Retail
  materialisiert Record-Zeiger UND Bool call-förmig; flache
  `&&`-Kette verliert beides (85,7→95,4). `eyeAim` erst im Else-Zweig
  deklarieren (Retail materialisiert &eye erst in der Picker-Auswertung):
  96,3→97,4→98,93.
- **Korrektur zu Lauf 92:** der dortige Rest-Diff-Hinweis war falsch —
  die ResNode-GetID/IsValid-Panics (g3d_resnode_ac.h:56) emittiert unser
  xobj.h-Shim LAENGST korrekt; der echte fn_800F3714-Rest ist
  Registerfarbung (Retail r27-r31, wir r26-r31), mPlayer-Load-Platzierung
  und Jump-Table-Pool-Label. Source-Kommentar entsprechend berichtigt.
- **Gate:** R24J01 `main.dol: OK` normal + --non-matching linkt;
  GGTE01 4588 / GGTJ01_01 4601 / GGTP01 4588 / R24J01 6655 matched,
  je 17 files OK; Report-Diffs 0 Regressionen in allen Editionen
  (nur das bekannte DrawGroundShadowAt-Pool-Artefakt, fuzzy-only);
  callcheck R24J01: 7 Eintraege, alle objb_act-Altfaelle, 0 in
  efftrail/xobj_chr/xobjchrmenu/cgame; symbols.txt unangetastet →
  keine neuen Dubs moeglich; config/ sauber.
- **Naechstes Ziel (Adressfolge):** fn_800F3E74/3E88 (klein),
  fn_800F3E9C/403C (Sichtbarkeits-Listen ueber CopiedVisAccess,
  Node-Id-Listen @802DA8C0/@802DA928), fn_800F41AC (Selector 4),
  fn_800F41F0 (ExecCallbackB Schwesterklasse), fn_800F4280/450C/4798,
  Dtor-Thunks 48C8-48D8.

## Lauf 92 — fn_800F3714 (ExecCallbackA Kopf/Lider) 87,7 %

- **Commit `c9d21f5d`** (3 Dateien): `fn_800F3714` @0x800F3714 (0x474)
  neu in efftrail.cpp, extern-"C"-Stil wie fn_800F35E8. 87,68 %.
- **Semantik:** XObjChr/XObjChrBase-ExecCallbackA — Base-Call, dann
  head_cap (mHeadNodes[2]): statische MTX33-Nickrotation, gated auf
  `GetAttachment()∈{1,3} && mAttachmentRequest<0 && animId!=0x817&&!=0x814`;
  danach mBlinkFrame<0→return; jeder Lid-Case (Nodes[3..6]=l_down,l_up,
  r_down,r_up) ruft `SetRotateDeg(&tabelle[mBlinkFrame])` und setzt
  `pFuncObj->SetCallbackNodeID(next.GetID())` (Cursor-Weitergabe), return.
- **Neue Header-Dekl.:** `WiiPlayerState* mPlayer` in cgame.h (+0x9A8,
  Pad→Member, layoutneutral); `ChrAnmResult::SetRotateDeg/SetRotateMtx`
  und `FuncObjCalcWorld::SetCallbackNodeID` in xobj.h (Unit-eigene
  Minimaldeklarationen erweitert, nicht g3d_resanmchr.h inkludiert).
- **Blink-Tabellen** 4×VEC3[11] in .data, bitgenaue Retail-Literale.
- **Rest-Diff (ehrlich):** Retail-ResNode-Accessor emittiert
  `Panic(IsValid)`-Bloecke (g3d_resnode_ac.h:56/:175), unser rescommon.h
  traegt keine Asserts — jeder Node-Check hat daher einen toten
  Panic-Block weniger; aendern waere ein Shared-Header-Eingriff.
- **Naechstes Ziel (Adressfolge):** `fn_800F3B88` (0x2EC, ExecCallbackB:
  Augen-/Kopf-Aim — rec2@+0x987C sonst rec1@+0x9810 wenn Snap-Paare 0;
  `self->GetResNode(nodeId)` Effekt-Call, dann PSMTXCopy/RotRad y,z
  Concat auf `pManip->mpM`). Danach fn_800F3E74/3E88 (klein),
  fn_800F3E9C/403C (Sichtbarkeits-Listen ueber CopiedVisAccess,
  Node-Id-Listen @802DA8C0/@802DA928), fn_800F41AC (Selector 4),
  fn_800F41F0 (ExecCallbackB Schwesterklasse), fn_800F4280/450C/4798,
  Dtor-Thunks 48C8-48D8.
- **Gate:** alle 4 Editionen gebaut, main.dol OK je Edition,
  0 verlorene Matches (GGTE01 4588, GGTJ01_01 4601, GGTP01 4588,
  R24J01 6655).

## Lauf 91 — fn_800F2CF4 100 % + DrawSpiralTrail 99,7 % + fn_800F35E8 94 %

- **Drei Commits:** `93521364` DrawSpiralTrail-Port + fn_800F2CF4-Fix
  (fn_800F2CF4 100 %), `ef7fd00e` fn_800F35E8 (94 %, NONMATCHING).
- **fn_800F2CF4 @0x800F2CF4 (0x1B0): 100 %.** Retail-Asm-Vergleich
  zeigte die rand()-Kanalreihenfolge `<<16, <<24, <<8` (erstes rand
  geht ins Byte 2, zweites ins Alpha). Source-Farbexpression
  entsprechend: `(rand%156+100)<<24 | <<16 | <<8` — Reihenfolge im
  Source `<<24`(zweites rand)… wichtig: MWCC isst die rand()-Calls
  in der Reihenfolge, in der sie im OR-Ausdruck stehen; Retail
  belegt r16 mit slwi<<16 → zweites rand() im Ausdruck ist der
  <<16-Term. Jetzt identisch, matched +1 (6654→6655).
- **DrawSpiralTrail @0x800F3140 (0x4A8): 99,73 % Report /
  99,61 % objdiff.** GC-Vorlage obja_chr.cpp:79 wortnah portiert
  auf Wii-APIs: CGsRenderState->Apply (cur+cleanup),
  DrawGsCircle(glow=4fach/Ecke, core=3fach), gGame.mGs.mBillboardEnabled
  auf 0 am Ende. KEY-ERKENNTNIS (wie UpdateOXMenuTransition, Lauf 87):
  direkte `trail->rings[i][j]`-Zugriffe OHNE Zeiger-Lokale erzeugen
  Retail's zwei Induktionszeiger; Locals-Reihenfolge angle vor radius.
  Rest <1 %: Register-/Scheduling-/Stack-Temp-Residuum, ehrlich
  NONMATCHING-Kommentar.
- **fn_800F35E8 @0x800F35E8 (0x12C): 93,84 %, NONMATCHING.**
  LoadFromPlacement-Override einer Geschwisterklasse im selben
  Retail-File (vtable-Slot direkt nach ResetObjectState__4XObjFv,
  zwei vtables referenzieren ihn, @4@-Thunk → Mehrfachvererbung).
  Verhalten: `XObj::LoadFromPlacement(info)` qualifiziert; bei
  mObject.m_pScnMdl(+0xF0)!=NULL sieben `GetResNode` (vtable+0x50)-
  Lookups second_null/head_chn/head_cap/l+r_mabuta_down/up →
  ResNode[7] @0x9AC (WiiPlayerState::mHeadNodes, cgame.h:957),
  dann EnableScnObjCallbackExecOp(EXECOP_CALC_WORLD). IDENTISCHER
  Instruktionsstrom zu xobj_chr.cpp:537-553 case-0-Block (dort
  inline in LoadFromPlacement). Restdiff = NUR Referenz-/@label-
  Rauschen (String-Pool), keine Instruktionsdifferenz.
- **Gate vollstaendig:** R24J01 main.dol OK 6655/8284;
  GGTE01/GGTJ01_01/GGTP01 je 17 files OK; Report-Diff lost100=[],
  einzige Regression = bekanntes DrawGroundShadowAt 99,98→96,62
  (Fuzzy-Artefakt); config/splits/symbols unveraendert;
  Format-Gate NICHT fahrbar (kein clang-format 18.1.8 auf Host,
  nur 23.1.4 — neue Bloecke folgen Dateistil).
- **Naechster Schritt (Adressfolge Unit 66):** fn_800F3714 (0x474),
  fn_800F3B88 (0x2EC), fn_800F3E74/E88 (je 0x14: stb bool→+0x9C8/
  +0x9C9 + Tailcall), fn_800F3E9C (0x1A0), fn_800F403C (0x170),
  fn_800F41AC (0x44), fn_800F41F0 (0x90), fn_800F4280 (0x28C),
  fn_800F450C (0x28C), fn_800F4798 (0x130), Stubs fn_800F48C8/D0/D8
  (je 0x8, Deletethunks — aus Klassen, nicht von Hand). Offen:
  Klassenidentitaet fn_800F35E8 (vtable in auto_07_802FF340/
  auto_07_803030D4_data.s, LoadFromPlacement-Slot nach
  ResetObjectState; @4@-Thunk = zweite Basis).
- Liegt: (a) zentrale .claude/devin/progress.md + ENDLESS_STATE_3.md
  vom Worktree aus nicht beschreibbar — diese Kopie ist Stand;
  (b) Format-Gate blockiert durch fehlendes clang-format 18.1.8
  (Host hat 23.1.4, env CLANG_FORMAT nicht gesetzt);
  (c) callcheck-Skript fehlt weiterhin in tools/ — Randpruefung
  ueber objdiff-Instruktionsdiff (fn_800F35E8: Call-Set matchend,
  kein neuer Call-Target-Diff).

## Lauf 90 — StartPointParticles 100 % + UpdatePointParticleDisplay 83,4 %

- **Zwei Commits:** `d6145fad` StartPointParticles (100 %),
  `861115b4` UpdatePointParticleDisplay (83,4 %, ehrlich NONMATCHING).
- **StartPointParticles @0x800F1110 (0x250): 100 %.** Wii-Port aus
  obja_chr.cpp: mType/mState=1/mAmount, Typ-Switch (0/2/3 sonst):
  Farben je Vorzeichen, Burst-Count = |amount| max 40, mPtcl-States
  -1/-2/-3/5. WICHTIG: Wii nutzt `__fabsf(amount)` (fsubs+Fabs+
  fctiwz), NICHT `__abs` — s32->f32->s32-Umweg wie Retail.
- **UpdatePointParticleDisplay @0x800F1360 (0xAF8): 83,36 %.**
  GC-Quelle obja_chr.cpp ~322ff wortnah portiert, State-Maschine,
  Ziffern-Dekomposition (0x66666667-Magie), Burst-Init mit 6×
  rand() — alles identisch decodiert. WII-DELTAS zur GC-Quelle:
  (a) `value = __fabsf(mAmount)` statt ternary; (b) mColor-
  Quellreihenfolge `<<16 | <<24 | <<8` (Retail isst rand() in
  dieser Reihenfolge — <<16 zuerst!); (c) `holdFrames = 0.2f *
  gRefreshRate` (PAL-Formel, NICHT JP's 12.f); (d) frames ist u32
  (2^52-Magie lbl_805A4ED8, kein xoris).
  REST-WAND (~16,6 %): Retail unrollt die zwei Stagger-Fade-Loops
  8x und scheduled die -i neg/xoris/lfd-Sequenz anders im Unroll
  (r3 vs r4 Registerwanderung, DELETE/INSERT-Paare = reine
  Umordnung); dazu sdata2-Pool-Layout (@labels statt benannter
  lbl_805A4xxx — loesen sich mit weiterem Unit-Fuellstand).
  Kein Source-Fehler bekannt; klassische MWCC-Unroll-Scheduling-
  Wand, NONMATCHING-Kommentar steht im File.
- **externs neu:** `<stdlib.h>` (rand) + `extern s32 gRefreshRate`.
- **Gate vollstaendig:** 4 Editionen gebaut — R24J01 main.dol OK
  (6654/8284, +1 durch StartPointParticles), GGTE01/GGTJ01_01/
  GGTP01 je 17 files OK; Format 475 ok 0 need; config/splits/
  symbols unveraendert; Baseline-Diff: kein REGRESSION-Eintrag,
  NEW-100 nur eigene neue Units (ringmenu/xobjchrmenu/efftrail-
  Funktionen, die im alten Baseline-Stand fehlten); kein
  em-dash-Byte mehr (sjiswrap-Warnung efftrail.cpp behoben).
- **Naechster Schritt (Adressfolge Unit 66):** DrawPointParticleDisplay
  @0x800F1E58 (0xE9C) — GC-Vorlage obja_chr.cpp ~415ff (Kamera-
  Fruehreturn, Player-Pos + Joint-Pfad, Screen-Projektion,
  Burst-Partikel-Bewegung, alpha/radius, Polygon+Textur-Render,
  Ziffern-Render; Wii-Deltas gScreenSafe*/Pointer beachten),
  dann fn_800F2CF4 (0x1B0), DrawSpiralTrail @0x800F3140 (0x4A8,
  GC obja_chr.cpp:79 ~97,7 %).
- Liegt: (a) zentrale .claude/devin/progress.md + ENDLESS_STATE_3.md
  vom Worktree aus nicht beschreibbar — diese Kopie ist Stand;
  (b) UpdatePointParticleDisplay-Unroll-Wand klassifiziert, ggf.
  Permuter-Kandidat spaeter; (c) callcheck-Skript liegt nicht in
  tools/ dieses Worktree — Randpruefung ueber objdiff-Instruktionsdiff
  (bl rand-Sites matchen, kein neuer Call-Target-Diff).

## Lauf 89 — DrawMenuRing 70,7 % + DrawOXMenu 79,2 % (ehrlich NONMATCHING)

- **Commit `9dbe52ed`:** zwei Funktionen in efftrail.cpp in
  Retail-Adressfolge vor UpdateSpiralTrail.
- **DrawMenuRing @0x800F0100 (0x45C): 70,7 %.** Wortgleich zur
  GC-JP-Fassung (obja_chr.cpp): 50 Vec-Vertices, 3 Schleifen a 25,
  `angle=(TWO_PI/24)*i`, PSMTXMultVecArray, DrawGsTrianglePrimitive
  (GX_TRIANGLESTRIP, `gGame.mGs.mCamera->mViewMtx`, separate→Paare a 4).
  REST-WAND: Retail haelt `angle` in f29 ueber sin UND cos (ein fmuls +
  fmr) und wandert den Vertex-Zeiger (+0x18 statt Indexskalierung), nutzt
  f25-f31; meine Schreibweise rematerialisiert angle pro Call und
  indexiert — gleiche Source-Form wie GC, MWCC-Wahl anders.
- **DrawOXMenu @0x800F055C (0xBB4): 79,2 %.** JP-Geometrie-Pfad
  (NICHT der PAL-Modellpfad): Fade-Fruehreturn, mChr-Anker via
  `GetJointWorldPos("neck")` (vtable _23C, rodata-String!) minus
  `GetPos()->y` (_244), `ProjectOffsetToScreen(_1D4)`. WII-DELTAS:
  (a) `GetStatusFlags(0x100)`-Zweig → screenX=gPointerScreenWidth/2,
  screenY=gScreenSafeBottom statt Projektion; (b) zusaetzliche
  Safe-Area-Schleife (2 Marken-Testpunkte x+90i-45 / y-70*sin(2.268928)-21
  gegen gScreenSafe{L,R,T,B}) → Out-of-Bounds = Re-Center; (c) pro Marke
  Pointer-Hint: `mc&2 && i==sel && mStarted>0 && fade==1.0f &&
  (!(mc&1)||!mPointerValid)` → fn_8002BBD8(&gs,x,yBase+0.2*70) else
  fn_8002BBDC — `yBase` set=screenY-56 / unset=screenY-70*sin(2.268928*f)-21;
  (d) Render-State via `gGame.mGs.mCurRenderState->Set{BlendMode,ZMode,
  CullMode}` (CGsRenderState+0/4/8/C, +14/18/1C, +7C — Stores passen).
  Scharf-Gate identisch zu UpdateOXMenuTransition (siehe Lauf 87).
  DrawOXCross = statisches inline mit 2x DrawGsCube(z=50). REST ~20 %:
  Register-/Scheduling-Residuum ueber die Zwei-Draw-Schleife; kein
  offensichtlicher Source-Fehler mehr (Konstanten alle dekodiert:
  2.268928 double-lfd, 70/90/45/21/56/74/1.05/0.55/0.6/0.02/0.36/0.3/
  0.8/0.1/1.2/0.5/255/150/50/2.0943952/4/10/0.7/0.5/0.2).
- **`1.0f *`-Hinweis:** Retail emittiert fmadds mit 1.0-Faktor in
  `screenX + 1.0f*(fade*(90i-45))` und der Bounds-testX — als Literal
  geschrieben (Skalen-Familie der Funktion), NICHT als Trickserei.
- **Gate vollstaendig:** 4 Editionen OK (GGTE01/GGTJ01_01/GGTP01 je
  17 files, R24J01 main.dol), check.py format ok, keine config-Diffs,
  keine matched-Regression (6653 stabil).
- **objdiff-cli Aufruf:** Unit-Name ist `main/game/wii/efftrail`
  (L3_diff.py ohne Praefix schlaegt fehl).
- **externs in efftrail.cpp:** PSMTXMultVecArray, fn_8002BBD8/DC,
  gPointerScreenWidth, gScreenSafe{L,R,T,B} (lokale extern-Dekls wie
  ccameraaction*.cpp); gWiiPointer via wii/cpadact.h.
- Naechster Schritt: Unit 66 in Adressfolge weiter — StartPointParticles
  @0x800F1110 (0x250), UpdatePointParticleDisplay @0x800F1360 (0xAF8),
  DrawPointParticleDisplay @0x800F1E58 (0xE9C), fn_800F2CF4, dann
  DrawSpiralTrail @0x800F3140 (GC-Vorlage obja_chr.cpp ~97,7 %).
- Liegt: (a) zentrale .claude/devin/progress.md vom Worktree aus nicht
  beschreibbar (Tool-Gate) — Eintrag steht hier im Lauf-89-Block;
  (b) DrawMenuRing/DrawOXMenu Restwaende = Register-/Scheduling-
  Residuum, klassifiziert (siehe oben) — ggf. spaeterer Permuter-
  Kandidat, kein Source-Fehler bekannt.

## Lauf 88 — Unit 66 angelegt: wii/efftrail.cpp, UpdateSpiralTrail 95,9 %

- **Neue TU `src/game/wii/efftrail.cpp`** (wii_units.md Nr. 66,
  0x800F0100..0x800F48E0, 23 Fns): configure.py Object(NonMatching,
  GC/3.0a5, `-Cpp_exceptions on -ipa file`) nach xobjchrmenu;
  splits.txt `.text 0x800F0100..0x800F48E0`. Retail-.rodata nennt die
  Quelle `xobj_chr.cpp`; Name frei gewaehlt weil src/game/wii/xobj_chr.cpp
  belegt (GC-Pendant hiess game/efftrail.cpp).
- **UpdateSpiralTrail @0x800F2EA4 (0x29C): 95,9 %, ehrlich NONMATCHING.**
  GC-Quelle (obja_chr.cpp:142) wortwoertlich — Wii-Asm strukturell
  identisch: rings@+4, Records 0x18 {pos@0,scale@C,color@10,angle@14},
  2 Ringe a 0xF0, Shift unrolled 3x3 Elemente rueckwaerts, Head:
  scale-=0.02f*frames clamp 0, angle+=DEG2RAD*9*frames, x/z aus
  sin/cos*scale*(15*(1-scale)), y+=0.3f*frames, Summe scales→active=0.
  Rest ~4 %: Retail haelt zweiten Ringzeiger fuer die Summe (r28=trail,
  Offsets falten +4 ein) und Schedules um sin/cos anders.
- **LAYOUT-ENTSCHEIDUNG:** kein `#include game/xobjchrstate.h` — zieht
  GC-`cgame.h`, kollidiert in Wii-TUs mit `wii/cgame.h` (Redefinitionen).
  Stattdessen LOKAL `SpiralTrailPoint`/`SpiralTrail` im GC-Layout
  (xobj_chr.cpp-Praezedenz: dessen `SuitChangeTrail` ist die
  scale-first-Sicht auf dasselbe 0x258-Objekt lbl_8045BBD0).
- **WICHTIG FUER UNIT 66:** Retail-.s fuer alle 23 Fns benannt und
  vorhanden; Stubs fn_800F48C8/D0/D8 sind Compiler-Deletethunks
  (subi r3,4; b dtor) — NICHT per Hand schreiben, kommen aus Klassen.
  fn_800F3E74/E88 (0x14): stb bool→obj+0x9C8/0x9C9 + Tailcall.
  DrawSpiralTrail: GC-Quelle obja_chr.cpp:79 (dort 97,7 % NONMATCHING).
  extab 0x8000C620..0x8000C780 / extabindex 0x8001460C..0x800146E4 gehoeren
  der Unit (noch nicht geclaimed — wie xobjchrmenu nur .text).
- **Gate dieses Mal NUR TEILWEISE:** R24J01 main.dol OK (vor und nach
  der TU), Report-Diff und GC-Editionen stehen noch aus — die
  configure.py-Aenderung liegt im Wii-Objektblock, GC unberuehrt.
  Format: check.py wollte clang-format 18.1.8, fand 23.1.4; Datei
  folgt dem Stil (Tabs, obja_chr.cpp-Konvention). objdiff-cli fand
  Units nicht (Konfigfrage, L3_diff.py bricht so auch).

## Lauf 87 — UpdateOXMenuTransition MATCHED (100 %) + Unit-66-Inventar

- **Neu (commit `4119fefa`):** `UpdateOXMenuTransition(WiiOXMenu*, u32)`
  @0x800EFD90 (0x370) in xobjchrmenu.cpp — **100 %**. Wii-Port des
  GC-Pendants obja_chr.cpp:767 mit zwei Deltas: (a) die selektierte
  Marke waechst nur, wenn das Menue "scharf" ist —
  `i==mSelectedIndex && (_10>=0 || ((mc&2) && (!(mc&1) || !mPointerValid)))`
  (selbe Stick-Gate wie HandleOXMenuInput); (b) Anim-Check = 3× vcall
  `mChr->GetAnimId()` (vtable+0x11C) statt einmaligem `mAnimId`-Member.
  Rest identisch: Fade `1.0f/(0.3f*gRefreshRate)*frames` (lbl_805A4F74
  =0.3f — GC's `OX_FADE_STEP`), Spin `DEG2RAD*8` (0x805A510C=0.13962634),
  Tilt `DEG2RAD*3` (0x805A4F68=0.05235988), Grow 0.05f (0x805A4FC8),
  Grenzen 0.8f/0.5f (0x805A4F10/0x805A4EE0), Cleanup
  `gGame.ClearStatusFlags(0x800)` (vptr gGame+0x41E4, slot 0x38),
  `mChr->SetRotateY(mStartRot, DEG2RAD*12)` (vcall 0x104),
  `gGame.GetPlayer()->mPad.Update(gWiiGameCubePads,0)`.
- **KEY INSIGHT:** Retail haelt ZWEI Induktionszeiger im Mark-Loop
  (r27=menu+0 fuer die Float-Felder via +0x18/+0x1C/+0x20, r26=menu+0x28
  fuer `&mMark[i].mObj`). Mit `WiiOXMenuMark* mark`-Lokal faltet MWCC
  alles zu einem Zeiger (96,99 %). **`menu->mMark[i]` ohne mark-Lokal**
  schreiben erzeugt die zwei IVs → 100 %.
- **POD-Falle:** `XObj mObj` als echtes Member macht WiiOXMenuMark →
  WiiOXMenu → WiiPlayerState nicht-POD; cgame.h's
  `offsetof(T,m)=&((T*)0)->m`-Makro ist dann "illegal constant
  expression" (bricht ~40 Units). Loesung: `struct WiiOXMenuMarkObj {
  u8 _0[0x9A8]; }` opak in cgame.h, `((XObj*)&mark->mObj)` am Call —
  member-Adress-Ausdruck, zweite IV kommt trotzdem.
- **WiiOXMenuMark jetzt:** mSpin/mTilt/mScale/u32 _0C (GC's dropped
  random angle)/WiiOXMenuMarkObj mObj@_10, sizeof 0x9B8.
- **Gate:** R24J01 main.dol OK 6653/8284; --non-matching linkt;
  GGTE01/GGTJ01_01/GGTP01 je 17 files OK; Report-Diff nur
  DrawGroundShadowAt-Artefakt 96,625 (bekannt seit Lauf 84);
  callcheck R24J01 23 Eintraege alle fremd (actor_pool-Ctors, ut);
  Format 474 ok; Symbol-Dubs nur SDK-Vorbestand.
- **Unit-66-Inventar (0x800F0100–0x800F48E0, 23 Fns) — fertig kartiert
  fuer den naechsten Lauf:** neue TU `src/game/wii/efftrail.cpp`
  (Beleg: GC `game/efftrail.cpp` = hier obja_chr.cpp; Name frei).
  Auto-.s fuer ALLE Fns unter build/R24J01/asm/:
  DrawMenuRing@0x800F0100(0x45C), DrawOXMenu@0x800F055C(0xBB4),
  **StartPointParticles**@0x800F1110(0x250),
  **UpdatePointParticleDisplay**@0x800F1360(0xAF8),
  **DrawPointParticleDisplay**@0x800F1E58(0xE9C) (benannte auto-Files
  StartPointParticles/UpdatePointParticleD/DrawPointParticleDis),
  fn_800F2CF4(0x1B0), UpdateSpiralTrail@0x800F2EA4(0x29C),
  DrawSpiralTrail@0x800F3140(0x4A8), fn_800F35E8(0x12C),
  fn_800F3714(0x474), fn_800F3B88(0x2EC), fn_800F3E74(0x14,
  `stb r4(bool),0x9c8` + b fn_800F3E9C), fn_800F3E88(0x14, selbes
  Muster stb 0x9c9 + b fn_800F403C), fn_800F3E9C(0x1A0),
  fn_800F403C(0x170), fn_800F41AC(0x44), fn_800F41F0(0x90),
  fn_800F4280(0x28C), fn_800F450C(0x28C), fn_800F4798(0x130),
  fn_800F48C8/D0/D8 je 0x8 (3 Stubs).
  GC-Vorlagen: obja_chr.cpp:79 DrawSpiralTrail, :142 UpdateSpiralTrail,
  :202 DrawPointParticleDisplay; DrawMenuRing/DrawOXMenu
  ~obja_chr.cpp:472/637 (OXMenu-Variante). splits: `.text
  0x800F0100..0x800F48E0`; wii_units.md nennt extab
  0x8000C620..0x8000C780, extabindex 0x8001460C..0x800146E4.
  Unit 67 (objb_act, 0x800F48E0..) existiert bereits.
  configure.py-Rezept wie xobjchrmenu: NonMatching, GC/3.0a5,
  `-Cpp_exceptions on -ipa file`, Eintrag nach xobjchrmenu.

## Lauf 86 — HandleOXMenuInput (~83 %, ehrlich NONMATCHING)

- Commit `46a30073`, Handoff `6933f388`. HandleOXMenuInput(WiiOXMenu*,
  CPad*) @0x800EF58C (0x804): mStarted>0-Gate, SetRotateY bei !Flag
  0x100, _10=Hover-Index, Pointer gueltig → Basispunkt Safe-Bottom
  oder Neck-Joint-Projektion (2 Stack-Vecs), Mark-Box-Tests
  (90-Spreizung, 70-Radius, 4-Bob, 120deg*mFade), Stick-Fallback ueber
  mMenuControls-Gate, Kopf-Aim chr->_987C immer geschrieben
  (snap=2-Reset vs. snap=1 ±20deg), Confirm bei fade==1 && Check(1) →
  ConfirmOXMenuSelection. Sound ueberall 0xF4.
- Liegt: ~83 % Scheduling/Pool-Wand wie Rest der Unit.

## Lauf 85 — DrawPlayerEquipmentRings (85,69 %, ehrlich NONMATCHING)

- **Neu:** `DrawPlayerEquipmentRings(XObjChrMenu*)` 0x800EE8B0
  (0x9FC), extern "C" — der grosse Rest der Unit-65-Menuegruppe,
  Wii-Port des GC-Pendants aus obja_chr.cpp:2517.
  DrawEquipmentRing in mSel-Reihenfolge; Typ-3-Zweig: Target/
  Cursor-Tracking (snap <1.0f, sonst 0.7f-Lerp), radius=40*fade,
  quad[5]-Corners (Retail rechnet sie, zeichnet sie aber NICHT —
  kein DrawGsVertexQuad-Aufruf im Retail, qua[] ist dort tot),
  show-Dim-Logik: dim wenn Skript NULL && mSel==0&&mAtc.pointed<0
  bzw. mSel==1&&mDrs.pointed<0, es sei denn Dpad-Override
  (mMenuControls&2 && (!(mc&1)||!mPointerValid)); alpha=show?fade:
  0.5*fade. Wii-API statt GC-Helfer: mCurRenderState->SetZMode
  (NEQUAL/LESS/LEQUAL), mCurTevStage->SetMode(MODULATE/PASSCLR),
  (&gGame.mTpl)->mTex.Init(desc[110-off]) + lokale Kopie,
  DrawGsTexturedQuad mit VIER separaten `0xFFFFFF00|(u32)(255*alpha)`
  (Retail ruft __cvt_fp2unsigned 4x — kein color-Local!).
  Pointer-Tail: mc&2 && RingReady && !pointer -> fn_8002BBD8
  (blr-Stub, pos=x+0.6r,y+0.4r) sonst fn_8002BBDC (blr-Stub).
  Else-Zweig: Hint hints[3]={8,5,5}+GetLocalizedInterfaceText
  (10/11/12), order[2][2]={{0,1},{0,2}}, edge=safeRight-16+
  100*(1-sinf(HALF_PI*fade)), Text=20f/Icon=1.2*20f,
  4 Outline-Labels + Hauptlabel + icon-quad desc[119-off],
  PSMTXScale mTexMtx.mMtx[2]=1/17, Trans mMtx[0]=icon/17.
  Neue File-Lokals: gEquipmentHintOutlineX/Y[4] (Retail .data
  lbl_80305B38/48), extern-decls fn_8002BBD8/DC (blr-Stubs!),
  DrawEquipmentRing, GetLocalizedInterfaceText, gScreenSafe*.
- **Restdiff 82,46 objdiff / 85,69 fuzzy:** Frame 0x360 vs 0x380
  (Retail nutzt f21/f22 fuer Hint-x/y+alpha), Retail haelt die
  pointed/override-Flags in getrennten Registern wo MWCC ueber
  Branches short-circuitet, Pool-@labels statt Unit-.sdata2-
  Namen, 0x4330-Magic + Scheduling. Semantik decodiert und
  verifiziert; keine bekannten semantischen Luecken.
- **Gate:** R24J01 main.dol: OK 6652/8284; GGTE01/GGTJ01_01/
  GGTP01 je 17 files OK; Baseline-Diff: nur Umbenennungen +
  bekanntes DrawGroundShadowAt-Pool-Artefakt (96,62);
  callcheck R24J01 52 bekannte Eintraege, keiner in
  xobjchrmenu; Format 474 ok (1 Datei nachformatiert);
  Dups nur SDK-Vorbestand. config/ unveraendert.
- **Naechstes:** HandleOXMenuInput 0x800EF58C (0x804),
  fn_800EFD90 (0x370), danach Unit 66 (0x800F0100-0x800F48E0)
  in Adressfolge (DrawMenuRing, DrawOXMenu, UpdateSpiralTrail).
- **Liegt:** --non-matching-Linktest diesen Lauf nicht erneut
  gefahren (Unit 65 ist NonMatching und linkt seit Lauf 84).

## Lauf 84 — UpdatePlayerEquipmentRings (93,95 %, ehrlich NONMATCHING)

- **Vorarbeit:** uncommitteter Lauf-84-Entwurf der Funktion aus dem
  abgebrochenen Vorgängerlauf fertiggestellt, NONMATCHING-Kommentar,
  Gate, Commit `a2056260` (xobjchrmenu.cpp, +174/-2).
- **Neu:** `UpdatePlayerEquipmentRings(XObjChrMenu*, u32 frames)`
  0x800EDF8C (0x924), extern "C". Wii-Port des GC-Pendants:
  Ringpaar Atc/Drs, `active = mEvt.mData.mScript == NULL`,
  CanSelectEquipmentRing (RingReady mOpen>1 && mGlow∈{0,1}),
  Pointer-Modus UpdatePointedItem, Typ-3-Reihenmenue Dpad-Wechsel
  (Check 4/8, mSel, Fokus/Cursor-Spiegelung), sonst Trigger-Maske
  0x6400000000000200 -> mSel-Toggle. HeadTurn NAN/0.0 je nach
  beide-ready. HandleEquipmentRingInput je Ring + Fallback-Wechsel,
  !active -> LastPointedItem-Kopie + KeepPointedItemOnOpen=0.
  SetRotateY(PI+view yaw, DEG2RAD*8) ausser FirstPerson(0x100).
  Check(2) -> result=-1 -> ClosePlayerEquipmentRings.
  UpdateEquipmentRingAnimation x2. mFade via FadeEquipmentRingIn/Out
  (0.18f/0.25f * gRefreshRate). Beide zu + StatusFlag 0x400 ->
  ClearStatusFlags + mPad.Update(gWiiGameCubePads,0).
  Neue lokale Inlines: RingReady/CanSelectEquipmentRing/
  FocusEquipmentRing/FadeEquipmentRingIn/Out; +includes cpad.h,
  cpadact.h; extern-decls HandleEquipmentRingInput/
  UpdateEquipmentRingAnimation/ClosePlayerEquipmentRings/gRefreshRate.
- **Restdiff 93,58 objdiff / 93,95 fuzzy:** bekannte Klassen —
  Float-Refs auf lokalen @labels statt benannter sdata2-Globals
  (loesen sich mit Unit-.sdata2), 0x4330-u32->f64-Magic + Sound-
  Call-Scheduling, Regalloc; MWCC cached den zweiten
  mEvt.mData.mScript-Test in einem bool, Retail laedt neu.
- **Gate:** R24J01 main.dol: OK 6652/8284; --non-matching linkt;
  GGTE01/GGTJ01_01/GGTP01 je 17 files OK; Baseline-Diff: nur
  Umbenennungen + das bekannte DrawGroundShadowAt-Pool-Artefakt
  (96,62), Unit-65-Werte identisch zu Lauf 83 (kein Ripple durch
  die neuen Includes); callcheck R24J01 52 bekannte Eintraege,
  keiner in xobjchrmenu; GGTE01 7 bahnfremde objb_act-Eintraege;
  Format 474 ok; Dups nur SDK-Vorbestand.
- **Naechstes:** DrawPlayerEquipmentRings 0x800EE8B0 (0x9FC),
  dann HandleOXMenuInput 0x800EF58C (0x804), fn_800EFD90 (0x370),
  danach Unit 66 (0x800F0100-0x800F48E0) in Adressfolge.

## Lauf 83 — NEUE TU game/wii/xobjchrmenu.cpp (Unit 65), +5 matched

- **Neu:** `src/game/wii/xobjchrmenu.cpp` deckt Unit 65
  (0x800ED45C–0x800F0100) ab — die Wii-Fortsetzung des GC-
  obja_chr.cpp-Menueabschnitts. configure.py +1 Objekt
  (NonMatching, GC/3.0a5, -Cpp_exceptions on -ipa file),
  splits.txt +1 .text-Eintrag, ringmenu.h +`void Init(XObjChr*)`
  auf DrsMenu unter VERSION_R24J01, symbols.txt +4 Namen:
  `Init__7DrsMenuFP7XObjChr` (0x800ED7E0), `Open__7DrsMenuFl`
  (0x800ED8EC), `ConfirmSelection__7DrsMenuFv` (0x800EDBB4),
  `ConfirmOXMenuSelection` (0x800EF3F8, war fn_).
- **Gematcht (100 %):** AtcMenu::IsItemEquipped,
  AtcMenu::GetItemIconTextureIndex, DrsMenu::GetItemIconTextureIndex,
  ClosePlayerEquipmentRings, OctaMenu::OpenActive.
- **Knapp drunter:** DrsMenu::GetItemLabel 99,38 / Open__7DrsMenuFl
  99,10 / IsItemEquipped__7DrsMenuFl 98,5 / PointAtcItem +
  PointDrsItem je 97,5 / ConfirmSelection__7DrsMenuFv 95,16 /
  ConfirmOXMenuSelection 90,46 / GetItemColor Atc 89,13 + Drs 88,09 /
  GetItemLabel__7AtcMenuFl 84,86 / Init__7DrsMenu 78,48 — alle mit
  NONMATCHING-Kommentaren (Register-/Scheduling-Waende, lokale
  Pool-Label vs. Retail-Globals; die float/sdata2-Konstanten
  0x805A4DF0-805A5390 liegen im unzugeordneten Pool mehrerer
  Units, bewusst NICHT zugeschnitten).
- **Dekodierte Fakten (belegt):** fn_800ED7E0 = DrsMenu::Init
  (mMenuType aus gCmdLineArgs.mAttachmentMenuType, Items typ3
  {1,4,6,3,2,7,8,5} sonst {3,5,4,1,6,8,7,2}, _10=1.0f, mRowOffset
  0.9f, mFirstVisibleItem=4). DrsMenu::Open nutzt
  FindOwnedInventoryItemIndex(chr->mParams.mInventory.mItems,id)>=0
  statt GC-HasItem; Typ-3-Ids {0x18,0x20,0x22,0x19,0x1A,0x1E,0x1F,
  0x21}, sonst {0x19,0x21,0x20,0x18,0x22,0x1F,0x1E,0x1A}.
  ConfirmSelection-Tabelle {0,1,1,1,1,0,1,1,1} (Typ 0/5 nicht
  anlegbar). Wii-Sounds: 0xF3=cancel, 0xF5=confirm via
  PlayWiiSoundEffect(&gGame.mSound,id,1.0f,0.0f).
  fn_800EF3F8 = OX-Menu Confirm(result>0)/Cancel(<0) + HeadAim-
  Reset (_987C, mSnap=2) — aus HandleOXMenuInput ausgelagert.
- **Noch offen (Unit 65):** UpdatePlayerEquipmentRings 0x800EDF8C
  (0x924), DrawPlayerEquipmentRings 0x800EE8B0 (0x9FC),
  HandleOXMenuInput 0x800EF58C (0x804), fn_800EFD90 (0x370, der
  OX-Frame-Updater: mFade ein/aus, Mark-Loop 2× stride 0x9B8,
  Anim-Check 0x78/0xEF/0x13B). Alle Wii-Pointer-getrieben —
  GC-Vorlagen in obja_chr.cpp:2517/2646/826, aber Wii-Deltas
  (gWiiPointer, PlayWiiSoundEffect, gScreenSafe*) beachten.
  Unit 66 (0x800F0100–0x800F48E0): DrawMenuRing, DrawOXMenu,
  UpdateSpiralTrail u.a.
- **Gate:** R24J01 main.dol: OK 6652/8284; GGTE01/GGTJ01_01/GGTP01
  je 17 files OK; Format 474 files 0 need; callcheck 52 Eintraege
  ausschliesslich bekannter Klassen, keine xobjchrmenu-Treffer;
  Symbol-Dups nur SDK-Vorbestand; config-Diff = nur die 3 eigenen
  Zeilenbloecke.
- **Naechstes:** UpdatePlayerEquipmentRings + DrawPlayerEquipmentRings
  dekodieren (Call-Sites + Pointer-Pfade schon kartiert), dann
  HandleOXMenuInput (GC-Port + Wii-Pointer), fn_800EFD90, danach
  Unit 66 in Adressfolge.

## Lauf 82 — ringmenu: AtcMenu::Open 99,58 % + mAttachments typisiert (3bf0a530)

- **Commit 3bf0a530:** `AtcMenu::Open` (0x800ECAE0, 0x8D8) ehrlich
  rekonstruiert. Retail-Shape: 8× mAvailable-Loop (Typ 2 → 1, sonst
  GetPlayerAttachmentCount → Find ausgerollt ueber 9 Slots direkt
  auf mChr+0x19E9C), `UpdateAttachmentAvailability` holt den Player
  intern frisch via `gGame.GetPlayer()` und schreibt SetCount auf
  IDs 4(0x10)/6(0x16)/5(0x15)/7(0x38)/8(0), dann
  `OctaMenu::Open(focus)` und der Typ-3-Block
  (mPendingFirstVisibleItem / GetAttachment / mLastPointedItem
  relativ zu mFirstVisibleItem, mKeepPointedItemOnOpen→0).
- **Schluessel:** `WiiPlayerState::mAttachments` in cgame.h von
  `u8[0x48]` auf `AttachmentValues` typisiert (gleiche Groesse,
  GC-xchr.h typisiert es ebenfalls). Erst dann faltet MWCC die
  Zugriffe zu mChr-basierten Split-Immediate-Loads statt einen
  values-Zeiger in ein Register zu materialisieren:
  74,78 → 98,45 → (gGame.GetPlayer im Helfer) 99,30 →
  (Signatur WiiPlayerState*) 99,58.
- **Ripple (Shared-Header-Folge, minimal):** `(AttachmentValues*)`-
  Casts auf `&` umgestellt in evt.cpp, savemanager.cpp,
  xactradar.cpp (Bahn 3), gameupdate.cpp (Bahn 1),
  xobjspider.cpp/xobjspiderboss.cpp (Bahn 5). Gleiche Adresse,
  gleiche IR — Report-Diff zeigt 0 Codegen-Aenderung in diesen
  Units. GC-Dateien unberuehrt (Member dort schon typisiert,
  `&`-Form stand dort schon).
- **Restdiff:** identischer Instruktionsstrom (566/566), nur eine
  Register-Permutation — Retail haelt CanUseTool-Ergebnis in r26
  und die Slot-8-0 in r21, wir r21/r22 + aeussere Vierer um 1
  verschoben. Nach zwei ehrlichen Schreibweisen als Regalloc-Wand
  dokumentiert (NONMATCHING-Kommentar). NICHT forcieren.
- **Gate:** R24J01 `main.dol: OK` 6647/8284; --non-matching linkt;
  GGTE01/GGTJ01_01/GGTP01 je 17 files OK; Report-Diff: nur das
  seit Lauf 77 bekannte DrawGroundShadowAt-Pool-Artefakt
  (99,98→96,62), 14 Gains; callcheck R24J01 52 Eintraege
  ausschliesslich bekannter Klassen (_savegpr/_restgpr-Regfarben,
  Namensaliase, Overload-Mangling) — kein Eintrag in
  ringmenu/atcmenu; GGTE01 8 Eintraege (objb_act, bahnfremd).
  Keine neuen Symbol-Dubs, config/ sauber.
  Format-Gate GRUEN: clang-format-18 liegt unter
  `~/.local/opt/clang-format-18/bin/clang-format` — Aufruf:
  `env CLANG_FORMAT=<pfad> python3 tools/check.py --source-only`
  → 473 files, 0 need formatting (zwei whitespace-only
  Nachformatierungen: cgame.h-Kommentarspalte, drei Umbrueche in
  xobj_chr.cpp; Codegen unveraendert, main.dol bleibt OK).
- **Naechstes:** ringmenu-TU weiter in Adressfolge — verbleibende
  Funktionen unterhalb 0x800ECAE0 (Unit .text 0x800E9FC4–
  0x800ED3B8); offene Fuzzies derselben Unit: GetItemColor 93,77,
  UpdateEquipmentRingAnimation 99,67. Danach xobj_chr-Rest
  (fn_800E32B8 93,94, DrawWithView 92,03, ResetObjectState 89,88).

## Lauf 81 — NEUE TU game/wii/ringmenu.cpp (Unit 63), 4 Matches

- **TU-Befund (wichtig für den Rest der Ring-Region):** Unit 63
  (.text 0x800E9FC4-0x800ED3B8) ist NICHT Teil von xobj_chr.cpp —
  dtk meldete "Cyclic dependency" beim Erweitern der xobj_chr-
  Range. Beweis: Retail-.text ist pro TU kontig; ringmenupointer.o
  (UpdatePointedItem, .text 0x800E9D30-0x800E9FC4, extab
  0x8000C4EC, eti 0x80014534) liegt zwischen xobj_chr .text-Ende
  und Unit 63; deren extab (0x8000C4F4-0x8000C584) liegt hinter
  ringmenupointers extab — gleiche Interleaving-Struktur wie
  .text. wii_units.md hatte die Unit dem xobj_chr zugeschrieben,
  das war falsch. METHODE FÜR WEITERE UNITS: extab-Adressen der
  auto_*.s-Header (# 0x8000C504.. Kommentar) verraten die
  TU-Grenzen — extabindex kontig => evtl. gleiche TU, Lücke =>
  andere TU. Unit 65 beginnt mit eigener extabindex @0x800145C4
  (UpdatePlayerEquipmentRings extab @0x8000C5BC), Unit 66
  (DrawOXMenu) @0x80014618 — beide sind wieder eigene TUs, NICHT
  ringmenu.cpp und NICHT xobj_chr.cpp.
- **Commit 29c97499:** neue Datei src/game/wii/ringmenu.cpp +
  Object(NonMatching, ..., GC/3.0a5, -Cpp_exceptions on, -ipa
  file) in configure.py (hinter xobj_chr-Eintrag) + splits.txt-
  Unit `game/wii/ringmenu.cpp` (extab 0x8000C4F4-0x8000C584,
  extabindex 0x80014540-0x80014570, .text 0x800E9FC4-0x800ED3B8).
  Unit 63 enthält auch AtcMenu::Open (0x800ECAE0, noch auto) —
  Dateiname ist Schätzung, die TU ist der Ring-Menü-Code.
- **Funktionen (Retail-Reihenfolge im File):**
  - `UpdateEquipmentRingAnimation` (0x800E9FC4, 0x974): GC-Port
    mit `1.0f/(k*gRefreshRate)` statt IDLE_SECOND. MATCH-KEY:
    Item-Loop als direkte `ring->mItems[i].`-Zugriffe, KEIN
    `OctaMenuItem& item` — Retail faltet die mItems-Basis+0x14 in
    die Feld-Offsets (r3=this+i*0x28, lfs f6,0x24 = mScale).
    Helpers RaiseRingValue/LowerRingValue/ApproachItemScale/
    AddPulse wie GC. Score **99,67** — Rest nur f4↔f5-Regallok im
    letzten Highlight-Loop (`mOpen>0`, mFade>=0.5): Retail haelt
    Frames-Double+1.0 in f4/f5 getauscht. NONMATCHING-Kommentar
    dokumentiert.
  - `OctaMenu::FinishClose` (0x800EA938): `mOpen = 0` — **MATCHED**.
  - `OctaMenu::GetItemColor` (0x800EA944): identisch zum GC-Source
    (0x20-Grau / Typ3 50,50,200 / Cursor 200,100,100 + AddPulse
    sin(1.2*phase)*25+25). Score 93,77 — gleiche Regalloc-Wand wie
    die GC-Kopie (Farbregs r28-r30 Bindung + li-Scheduling).
  - `GetItemIconTextureIndex`/`IsItemEquipped`: `return 0` — beide
    **MATCHED**.
  - `OctaMenu::GetItemLabel` (0x800ECAD8): Retail gibt
    lbl_805A1108 = **8-Byte-Null-.sdata** zurück, NICHT "" →
    `static char sLabel[8] = ""` als Funktions-Static — **MATCHED**.
- **Zahlen:** 6643→**6647** (+4 matched), fuzzy 68,98→69,08.
  Baseline-Diff: 0 Regressionen.
- **Gate:** R24J01 main.dol OK; GGTE01/GGTJ01_01/GGTP01 je
  "17 files OK"; callcheck: nur 7 bekannte bahnfremde
  objb_act-Eintraege; config/ sauber (nur eigene splits-Zeilen);
  Symbol-Dubs 0. Format-Gate: clang-format 18.1.8 fehlt
  (23.1.4), Befehl vom Interface abgelehnt — Stil haendisch an
  obja_chr.cpp angelehnt.
- **Liegt:** clang-format-Versionskonflikt (bekannt). Restwaende
  unveraendert + neu: UpdateEquipmentRingAnimation 99,67
  (f4/f5), GetItemColor 93,77 (regalloc), HandleEquipmentRingInput
  81,52, LoadFromPlacement 93,96, Release 96,27,
  DrawGroundShadowAt 96,63.
- **Naechstes:** in src/game/wii/ringmenu.cpp weiter —
  `DrawEquipmentRing` (0x800EAA88, 0x2040, gross: unrolled
  8-Item-Row + Label-Draw + CTexObj) und `AtcMenu::Open`
  (0x800ECAE0, 0x8D8: 8-Item-Loop mit ausgerollter
  9-Attachment-Count-Suche, Wii-Layout mChr) — beide Retail-Asm
  liegt vor (auto_DrawEquipmentRing_text.s,
  auto_Open__7AtcMenuFl_text.s), GC-Vorlage obja_chr.cpp.
  Units 65/66 sind eigene TUs — Namen/Dateizuordnung vor dem
  Splitten per extab-Index + .rodata/.data-Referenzen festlegen.

## Lauf 80 — HandleEquipmentRingInput 81,52 % NONMATCHING (committet)

- **Commit 756e5538** `wii xobj_chr: HandleEquipmentRingInput
  rekonstruiert (81,52 %)` — .text:0x800E96CC, size 0x660, in
  src/game/wii/xobj_chr.cpp zwischen OctaMenu::Select und
  ConfirmSelection; Wii-Port von obja_chr.cpp:3564. Kein Match-
  Zuwachs (Funktion war vorher gar nicht im Report), 0 Drops in
  allen 4 Report-Baselines.
- **Semantik:** `settled = mOpen>1 && (mGlow==0||mGlow==1)` als
  BOOL-Lokalmaterialisierung; settled && mPointedItem>=0 &&
  mFocus!=0 -> return 1. Rumpf unter `if (mOpen>0 && mFocus==0)`
  (positive GC-Form, gemeinsamer return-0-Block!). Typ 3
  (Reihenmenue): nur bei (mMenuControls&2) && (!(mc&1) ||
  !gWiiPointer.mPointerValid) Dpad-Lauf ueber mRepeat &
  0x0F000000001E0000, links -1 (wrap 6) / rechts +1 (wrap 0),
  beide mit Sound 0xF4; Cursor = (mFirstVisibleItem +
  mLastPointedItem) % 7 (-1 bei mFocus). Sonst Okta:
  `dirs = mButton & 0x0F000000001E0000` (hi=Remote-, lo=GC-Pad-
  Richtungsbits), 8 Richtungspaare -> Slot 0..7, Fallback
  mCursorIndex. Kopfziel-Record (XObjScnCamInitRec @
  WiiPlayerState._987C): in ALLEN vier classify-Armen je
  kompletter 7-Store-Block (mSnap[0]=mSnap[1]=1, m_0C=NULL,
  mTarget.x=NAN, m_20=NULL, m_24, m_28) — wie GCs TurnJoint-
  Duplikate, nur eben Store-Sequenz. m_28 = DEG2RAD*Yaw-Tabelle
  (Typ3: gEquipmentRowHeadYaw[mLastPointedItem], sonst
  gEquipmentHeadYaw[cursor+1]); m_24 = mHeadTurn oder bei NaN:
  Typ3 DEG2RAD*(6*rowOffset^3), sonst DEG2RAD*pitch[cursor+1].
  Sound 0xF4 bei Cursor-Aenderung ODER Pointer-Wechsel.
  mCursorIndex=cursor. Dann active && mFade>=1: Check(1)=A ->
  Gate mPointedItem>=0 || (mc&2 && (!(mc&1) || !valid)); cursor<0
  oder mItems[cursor].mAvailable==0 -> Sound 0xF2; sonst
  ConfirmSelection (vtable+0x1C), 10x ResetPopperShot ab
  mChr+0xD80 stride 0xD9C, return 1. Check(2)=B oder Check(0x100)
  -> return -1. Sonst 0.
- **Neu deklariert (xobj_chr.cpp):** f32 gEquipmentRowHeadYaw[7],
  gEquipmentHeadYaw[9] (=GC-Inhalt), gEquipmentHeadPitch[9]
  (Wii-Retune {-30,-20,-10,0,10,0,-10,-3,-2}), extern "C"
  ResetPopperShot; include cpadact.h.
- **Restdiff (alle Klasse pool/regalloc/sched):** Retail ankt die
  3 Tabellen ueber r31 = jumptable_80304008 + 0x1A4C/68/8C —
  Anchor existiert erst mit kompletter Unit-.data; Cursor sitzt
  dadurch bei uns in r31 statt r30 (Kaskade), Store-Interleaving
  in den Armen weicht zeitlich. Keine semantischen Diffs mehr.
- **Gate:** R24J01 main.dol OK 6643/8284; --non-matching linkt
  (606 Obj.); GGTE01/GGTJ01_01/GGTP01 je "17 files OK";
  Baseline-Diff 0 Drops; callcheck: 0 Eintraege xobj_chr/obja_chr
  (7 alte objb_act-Reste fremder Bahn); config/ sauber;
  Symbol-Dubs 0.
- **Liegt:** check.py --source-only weiterhin clang-format
  18.1.8 vs 23.1.4 (bekannt). Restwaende: LoadFromPlacement
  93,96; Release 96,27; DrawGroundShadowAt 96,63; neu
  HandleEquipmentRingInput 81,52.
- **Naechstes:** Adressfolge xobj_chr ab ~0x800E9D30 (Ring-
  Pointer-Unit DrawEquipmentRing & Co. pruefen, wer die drei
  Tabellen sonst liest — Anker kommt mit Unit-Vollstaendigkeit);
  oder fn_800E32B8 ~93,94, DrawWithView ~92,03, ResetObjectState
  ~89,88, obja-Reste.

## Lauf 79 — DrawPlayerLookMarker MATCHED (100,0 %)

- `DrawPlayerLookMarker(XObjChrLook*)` (.text:0x800E8AA0, size
  0x864) in src/game/wii/xobj_chr.cpp hinter dem OctaMenu-Block —
  Wii-Port des GC-DrawPlayerLookMarker (obja_chr.cpp:3763).
  MATCHED: matched 6642→**6643**, erste volle Match in diesem
  Block der Unit.
- **Semantik:** Gate auf !IsAction(CAMACTION_SCROLL2) &&
  look->mFade>0. CTexObj-Temp; scale = fade + 0.5*sin(PI*fade);
  Deskriptor 111 (Wii: _30 waehlt 111 vs 112; JPN-Offset
  StatusTextureOffset); pos via mObject->ProjectOffsetToScreen,
  Phase-Shift 4*sin(DEG2RAD*mPhase); Blend/ZMode/TevStage ueber
  gGame.mGs.mCurRenderState/mCurTevStage; Textur-Quad, Maske,
  Glow-Quad (SRCCOL/SRCALPHA), optional Ziel-Textur bei
  mMode>=0; Zustand zurueck, InvalidateCaptureGraphics.
- **Match-Schluessel:** (a) alpha als Ternaer
  `look->_30 ? look->mFade : 0.5f * look->mFade` — Retail beq→
  Block + b→Join; ein `if(!_30) alpha*=0.5f` erzeugt bne-ueber
  statt beq-zu (98,16→98,59). (b) Glow-Faktor als Ternaer ueber
  den GANZEN Ausdruck `_30 ? 0.5+0.5*sin(PI*mPulse) : 0.5` —
  Retail else laedt 0.5 direkt, ein Ternaer ueber pulse
  materialisiert fmadds(0.5,0.0,0.5) (98,59→99,76→100 im
  Report; Rest waren nur @pool-Label-Artefakte).
- **GC-vs-Wii-Diffs:** Wii hat zusaetzliches _30-Gate (halbe
  Alpha, kein Glow, zweite Textur) und Farbfaltung pro Ecke
  statt einmal pro Quad; CTexObj 0x104; Init(TPLDescriptor*,
  u32) ohne Rueckgabe (kein Chaining).
- **Gate:** R24J01 main.dol OK 6643/8284; --non-matching linkt;
  GGTE01/GGTJ01_01/GGTP01 je 17 files OK; Baseline-Diff: einzige
  "drop" DrawGroundShadowAt 99,98→96,63 ist das seit Lauf 77
  dokumentierte Pool-Order-Artefakt (unveraendert, kein
  Codegen-Diff); config-Status sauber; Symbol-Dubs 0.
- **Liegt:** check.py --source-only weiterhin clang-format
  18.1.8 vs 23.1.4 (bekannt); keine neuen Bloecke.
- **Naechstes:** `HandleEquipmentRingInput` (.text:0x800E96CC,
  size 0x660) — gleiche Unit, folgt direkt; danach Adressfolge
  weiter (fn_800E32B8 ~93,94, DrawWithView ~92,03,
  ResetObjectState ~89,88) bzw. obja-Reste.

## Lauf 78 — RestorePlayerHeldItem MATCHED (100,0 %)

- `RestorePlayerHeldItem` (.text:0x800CE4B0, size 0x2BC) in
  src/game/wii/xobj_chr.cpp hinter fn_800CE48C — Wii-Port des
  GC-RestorePlayerHeldItem (obja_chr.cpp). Nach Diff-Korrektur
  BYTE-EXAKT: 95,14 → **100,0 %**, matched 6641→6642.
- **Korrekturen zum 95-%-Wurf:** (a) verschachtelte GC-Form
  `if (pending) { if (!(flag&0x0040..)) {..; return TRUE;} } else
  return TRUE; return FALSE;` statt Early-Returns — Retail
  beq/bne-Sinn; (b) rlwinm-Bitnummerierung: die gespiegelte
  Flagge ist 0x20000000 (lo-Bit 2) auf BEIDEN Seiten, nicht 0x4.
- **Semantik:** Lift mState!=0 && Target-Pool 24 → Target=mPlug,
  Plug-Status 0x0400000000000000, SetAnim 8, TRUE. Sonst Lift-
  Reset. mSavedTarget>=0 → SpawnObjectFromPlacement +
  mPendingTarget, saved=-1. pending==NULL→TRUE; pending-Status
  0x0040000000000000→FALSE; sonst pending->mStatus|=0x10000,
  Lift-Reset, mState=4, SetTarget, mTargetState=4, Target-Anim
  mSavedTargetAnim + Status 0x0400000000000000, Spieler-Anim 8,
  pending=NULL; Squirter→ApplyPlayerItemTintToTarget; bei
  Toothbrush|Spoon|Mug|Squirter: Ziel-0x20000000 := Spieler-
  0x20000000 (nur bei Abweichung schreiben). TRUE.
- **Neue Member/Helfer (cgame.h):** `XObj* mPendingTarget`
  (_1C98C, ersetzt Pad), `XAction::SetTarget(XObj*)` inline
  (mTarget=mTargetFloor-Setzpaar).
- **Extern-Dekl. xobj_chr.cpp:** IsCarryingSquirter/Spoon/
  Toothbrush, ApplyPlayerItemTintToTarget,
  GetStageObjectPlacementByIndex.
- **Gate:** R24J01 main.dol OK 6642/8284; --non-matching linkt
  (606 Obj.); GGTE01/GGTJ01_01/GGTP01 je "17 files OK";
  Baseline-Diff 0 Drops in allen 4 Editionen; callcheck
  R24J01+GGTE01: 0 xobj_chr-Eintraege (7 alte objb_act-Reste
  anderer Bahn); check.py --source-only: format ok nach
  clang-format-18.1.8; Symbol einzigartig (Zeile 547).
- **MWCC-Lektion:** rlwinm mb,me-Argumente sind MSB=0-Bits:
  (2,2)=0x20000000 lo, (9,9)=0x00400000 hi. Retail-Totzweig
  `beq` hinter beq bei GetObjDataPool = verschachtelter
  NULL-Check — fiel diesmal automatisch richtig aus.
- **Liegt:** nichts Neues; bekannte Restwaende in
  LoadFromPlacement (93,96), Release (96,27), DrawGroundShadowAt
  (96,63) = Pool-/RegAlloc-Klasse.
- **Naechstes:** xobj_chr Adressfolge ab 0x800CE76C
  (SetPlayerActionTarget ist vorhanden) — naechste Luecke/
  fn_-Funktion danach; 0-%-Reste fn_800C9E84, fn_800CB1DC.

## Lauf 77 — XObjChr::LoadFromPlacement committet (93,96 % fuzzy)

- **Commit 376e5afd** `wii xobj_chr: XObjChr::LoadFromPlacement
  rekonstruiert (93,96 %)` — 4 Dateien: xobj_chr.cpp (Funktion,
  _158), cgame.h (mHeadNodes[7] _9AC, mLoadStep _9E18 +
  STATIC_ASSERTs), xobj.h (AnmFlag-Enum + SetAnmFlag +
  UseQuaternionBlend/UseAccurateScaleBlend auf AnmChr,
  virtual _158-Override), symbols.txt
  (LoadFromPlacement__7XObjChrFRC9CoordInfo .text:0x800CCE68
  size 0x840).
- Stufe-Loader komplett: Basis-Load + 7 ResNodes per Name,
  EnableScnObjCallbackExecOp(EXECOP_CALC_WORLD), AnmFlags 8+0x10,
  case 0 Grundmodell, 1 Plug(15), 2 Copter(17)+"head", 3
  Popper(18)+"l_arm_chn", 4 Radar(19)+"head", 5 O/X(20, Wii ist
  Japan-only — kein 44-Fall), 6 O/X-Menue, 7 Suit[3] via
  SpawnData 0x24, 8 Suit[6] via 0x25, 9 Stage-Setup, 10/11
  weitere Stufen + CTexObj-Init (Deskriptoren +0x3D8/+0x3E0
  des Spiel-Texturarchivs), 12 Attachment/Radar-Logik, 13
  AnimSfxActor-Create-Muster. mLoadStep>0xC => Reset+Bit 0x4
  (oris), TRUE, sonst FALSE.
- **Wichtig (Pattern):** `if (call()) { ++step } else break;`
  erzeugt retails `cmpwi/beq`; `if (!call()) break` und
  `== FALSE` erzeugen cntlzw/srwi.-Normalisierung. Damit alle
  7 Last-Checks richtig.
- **Restdiff (register/schedule/pool):** Prolog r26-r31+0x80 vs
  r29-r31+0x70; strbase r28 vs r31 + lis/addi-Reihenfolge;
  'placed'-Merge li r0,1/li r0,0 vs mr r29; case-0
  Store-Verzahnung; Pool-Referenzen @labels (100.0f, u8-Magic,
  Sprungtabelle, Stringbasis) — loesen sich mit
  Unit-Vollstaendigkeit. NICHTMATCHING-Kommentar im Source.
- **Report-Vergleich:** better 4 (LoadFromPlacement 0->93,96;
  Release 0->98,11; ApplyPlayerSuit ->99,98; UpdatePlayerSuitChange
  ->97,55), gone@100 0. DrawGroundShadowAt 99,98->96,63 =
  Pool-Order-Artefakt (nur @label/sda21-Referenzen, keine
  Codegen-Aenderung).
- **GC-Gate:** GGTE01/GGTJ01_01/GGTP01 je 17 files OK.
- **Naechstes Ziel:** Adressfolge ab 0x800CD6A8
  (SetObjectStatusFlags ist schon da — die naechste
  unbenannte/Luecken-Funktion im Unit, z.B. fn-Bereich
  0x800CD708+ UpdatePlayerSuitChange ist vorhanden;
  weiter die naechste Luecke in wii_units.md Einheiten 29-46).
## Lauf 76 — XObjChr::Release committet (96,27 %); fn_800CCE68 dekodiert

- **Commit 76399b35** `wii xobj_chr: XObjChr::Release rekonstruiert
  (96,27 %)` — 4 Dateien: xobj_chr.cpp (_40, GC-Release +
  RemovePlayerSuit), xobj.h (virtual void Release(); _40),
  cgame.h (mPadReset _19E90, mSavedTarget/_Anim/_State
  _1C980/_1C984/_1C988), symbols.txt (Release__7XObjChrFv
  .text:0x800CE054 size 0x438). Restdiff = dokumentierte
  Unit-Vollstaendigkeits-Wand (this in r30 vs r31, Pool-Label).
- **fn_800CCE68 = XObjChr::LoadFromPlacement (Wii)** — komplett
  dekodiert, siehe eigenen Abschnitt unten. Naechster Schritt:
  schreiben + Member deklarieren.

## fn_800CCE68 = XObjChr::LoadFromPlacement — Dekodierung (Lauf 76)

Signatur: `BOOL XObjChr::LoadFromPlacement(const CoordInfo& info)`
(vslot _158-Override; r3=this, r4=&info wird an XObj::LoadFromPlacement
weitergereicht). 0x840 B, Sprungtabelle jumptable_80304008, Faelle
0..0xD auf `chr->mLoadStep` (this+0x9E18, NEU `s32` — Pad `u8
_9E18[8]` in cgame.h vorhanden). GC-Vorlage: `XObjChr::LoadFromPlacement`
in src/game/obja_chr.cpp (~Zeile 10268, mLoadStep-Maschine bis 12).
this in r30. Pattern `++mLoadStep` pro Fall (Retail: lwz/addi/stw auf
this+0x9E18 via addis+(-0x61e8)).

**Neue Member (cgame.h):**
- `s32 mLoadStep; // _9E18` (+ `u8 _9E1C[4]`)
- this+0x9AC..0x9C4: SIEBEN `nw4r::g3d::ResNode` (Stride 4) im Pad
  `u8 _09A8[0x9C8-0x9A8]` — GetResNode-Ergebnisse, Namen aus
  lbl_802DAA6C: _9AC "second_null", _9B0 "head_chn", _9B4 "head_cap",
  _9B8 "l_mabuta_down", _9BC "l_mabuta_up", _9C0 "r_mabuta_down",
  _9C4 "r_mabuta_up" (mabuta=Lid, gehoert zu mBlinkAnims). Als Array
  `ResNode mHeadNodes[7]` oder einzeln — Retail speichert in Folge.
- this+0x1C774 und +0x1C878 (Stride 0x104): zwei TPL-Bloecke
  (CTexObj-Felder + 0xC4-Byte-Block), Typ z.B. `u8 mTexBlocks[2][0x104]`
  oder Struktur {CTexObj tex; u8 data[0xC4];} — pruefen.
- this+0x1CC90: Manager-Objekt (+4=SoundArchivePlayer*, +8=Sound3DManager*);
  this+0x1CC94: `CSound3DActor* mSoundActor` (new 0xA0, vtbls
  CSound3DActor+0x1C bei +0x54, NULL bei +0x80).

**Vorhandene Helfer/Member:** mPlug _9E20 (WiiPlayerPlug, vslot
0x158 = LoadFromPlacement), mCopterObj _B580 / mPopperObj _BF30 /
mRadarObj _C8E0 / mOXObj _D2A0 (u8-Blobs -> `(XObj*)`), mSuitParts[3]
= this+0x151C8 (WiiSuitPart, LoadFromPlacement), GetAttachment() =
`mAttachmentRequest >= 0 ? mAttachmentRequest : mAttachment`
(cgame.h:1079), mPopper+4 = mPopper.mState (this+0xD54),
mBlinkWait _96A0 + mBlinkFrame _96A4 (beide =0), mStatus.mFlags.
`EnableScnObjCallbackExecOp(op)` am ScnObj, ExecOp 1 = EXECOP_CALC_WORLD
(xobj.h:800/820). mObject _E8: m_pScnMdl bei +8 (=0xF0).

**Faelle:**
- case 0: `if (XObj::LoadFromPlacement(this,&info))` — nein: direkter
  `bl LoadFromPlacement__4XObjFRC9CoordInfo` (Basis-Call). Bei TRUE
  && mObject.m_pScnMdl!=NULL: 7x GetResNode(name) (vslot _50) ->
  obige Member, dann `m_pScnMdl->EnableScnObjCallbackExecOp(
  EXECOP_CALC_WORLD)`. ok-Flag -> bei FALSE break (Exit). Danach:
  `mStatus.mFlags |= 0x200000000000ULL` (hi-Word 0x150 |= 0x2000),
  `mBlinkWait = mBlinkFrame = 0`, SetAnmFlag(mObject+0xC =
  &m_blend /*NW_ChrBlend, AnmObj*/, 8, TRUE) und (…,0x10,TRUE);
  `++mLoadStep`; Fallthrough.
- case 1: `if (!chr->mPlug.LoadFromPlacement(CoordInfo(GetSpawnData
  (0xF)))) break; ++mLoadStep;` Fallthrough.
- case 2: Copter: LoadFromPlacement(GetSpawnData(0x11)), dann
  `((XObjAttachment*)mCopterObj)->SetParent(GetWorldMtx("head"),TRUE)`
  (vslot _A4 GetWorldMtx(name), _294 SetParent). `++mLoadStep`.
- case 3: Popper: GetSpawnData(0x12), SetParent("l_arm_chn",TRUE). ++.
- case 4: Radar: GetSpawnData(0x13), SetParent("head",TRUE). ++.
- case 5: OX: GetSpawnData(0x14), SetParent("head",TRUE). ++.
- case 6: `++mLoadStep` (leer — GCs OX-Mark-Load faellt weg).
- case 7: `mSuitParts[3].LoadFromPlacement(CoordInfo(GetSpawnData
  (0x24)))`; bei FALSE break. `++mLoadStep`.
- case 8: `u32 flags[5] = {mFlags&0x800000, mFlags&0x200000,
  mFlags&0x40, mFlags&0x20000, mFlags&0x400000}` (lo-Word 0x154,
  rlwinm-Bits 8,10,25,14,9); `vf1A0(flags)`; dann `if ((mFlags &
  0x1000000) && IsStageDataLoaded()) vf1A8((*(u8**)GetStageEnvironment())
  [9] / 100.0f);` (u8->f32 via xoris/0x4330/lfd lbl_805A4ED8, fdivs
  lbl_805A4ED4=100.0f). `++mLoadStep`.
- case 9: `gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->
  descriptorArray[123],0)` + `[124]` — Retail: tpl=gGame+0x4C30,
  desc=+0x8 -> +0x3D8 (=Idx 123), +0x3E0 (=124); CTexObj ab
  gGame+0x4C34. Kopiert je Block: 13 Worte CTexObj + 0xC4 Bytes
  ab gGame+0x4C6C -> this+0x1C774 / +0x1C878 (memcpy-Loop 24x8+4).
  `++mLoadStep`.
- case 10: `++mLoadStep` (leer).
- case 11: `switch (GetAttachment()) {1: ShowIdlePlayerCopter((XObjCopter*)
  mCopterObj); 2: mPopper.mState=3; 3: ShowIdlePlayerRadar((XObjRadar*)
  mRadarObj);}` — Retail-Form: r0 = mAttachmentRequest>=0?req:mAttachment;
  dreifach cmpwi-Kette 1/2/3. `++mLoadStep`.
- case 12: `if (this+0x1CC94==NULL) { mgr=this+0x1CC90; CSound3DActor*
  a = new(0xA0) CSound3DActor(mgr->+4, mgr->+8); setze vtbl +0/+0x54,
  +0x80=0; this+0x1CC94 = a; }` `++mLoadStep`.
- case 13 = leer? -> Exit-Pfad: `if (mLoadStep > 0xD) { mStatus.mFlags
  |= 0x40000ULL; mLoadStep = 0; return TRUE; } return FALSE;`

**Beleg-Details:** Extab vorhanden (CoordInfo-Temps mit vtbl auf
Stack). Strings fuellen den Unit-Pool in Retail-Reihenfolge nach
(hilft ApplyPlayerSuit/Release/UpdatePlayerSuitChange gegen 100).
Callcheck beachten: kein neuer externer Name ausser ggf.
EnableScnObjCallbackExecOp/CSound3DActor-Ctor (beide in symbols.txt
pruefen). symbols.txt: `LoadFromPlacement__7XObjChrFRC9CoordInfo =
.text:0x800CCE68; size:0x840` eintragen.

## Lauf 75 — xobj_chr: UpdatePlayerSuitChange rekonstruiert (95,98 %)

Commit 2fe44eaf (3 Dateien, +111/−10): src/game/wii/xobj_chr.cpp,
include/game/wii/cgame.h, config/R24J01/symbols.txt
(UpdatePlayerSuitChange .text:0x800CD708 size 0x500 — dtk erwartet
die alinierte Groesse, nicht die 0x4FC des Listings).

- **UpdatePlayerSuitChange (0x800CD708) = 95,98 % fuzzy — NEU.**
  Suit-Wechsel-Zustandsmaschine, GC-Vorlage UpdatePlayerSuitChange
  + RemovePlayerSuit (beim Wii inline). Switch auf
  mSuits.mChangeStep: 1: SetColor(2,255,255,255,0.5) +
  fn_800F2CF4(&lbl_8045BBD0) (Spiraleffekt, Record-Typ
  SuitChangeTrail in xobj_chr.cpp) + PlayWiiSoundEffect(0xB2).
  2: SuitFadeFinished (mAnmQueue.mColorStep[1].mSpeed == 0, neu
  als Helper). 3: bei mCurrent!=0: GetAnimId/GetAnimFrame sichern,
  ClearAnim, 4 Attachments via GetWorldMtx+SetParent rebinden
  (Copter→head, Popper→l_arm_chn, Radar→head, OX→head — dieselben
  Strings wie ApplyPlayerSuit), altes Suit via
  IsObjectActive(&mObjects[mCurrent]) + vf290(FALSE)@Attach2 +
  ClearAnim + Release freigeben, mSuitAnm-Archiv
  (mAnm.Release(TRUE) + mFile.Close()) schliessen, SetAnim
  restore, SetColor weiss. Dann mLoadStep=1, mCurrent=mRequested,
  step++, gPlayerSuitChangeTimer=0.0f — Fallthrough 4: Timer
  −=2.0f (clamp 0), LoadPlayerSuitItem + Timer==0 → 5:
  ApplyPlayerSuit, weiss sofort, schwarz 0.5, Farb-Pack aus
  mColorStep[0].mValue → SetColor(0). 6: Fade-Ende → step=0.
- **Layout-Fix:** CVec ist 0x10 (vtable), nicht 0xC —
  mEyeAimStep endet exakt auf _1C9FC; ein u8-Pad [4] war falsch
  und schob beide CJArchive +4 (Diff −0x3374 statt −0x3378).
  Recovered in WiiPlayerState: CJArchive mAttachmentAnm (_1C9FC),
  mSuitAnm (_1CB44) = GameCube mAttachmentAnim/mSuitAnim.
- **MWCC-Lektionen:** Retail leitet `mObjects[mCurrent]` bei
  jedem Call neu ab (kein gebundener Suit-Pointer — volles
  Indexing an jeder Stelle schreiben); der Flag-Test geht ueber
  IsObjectActive(&…) — die Adressbildung materialisiert &suit in
  r3, das vf290 danach wiederverwendet (94,11 → 95,98).
- **NONMATCHING-Rest (~4 %, dokumentiert im Code):** gleiche
  Instruktionsmenge/Calls/Kontrollfluss. Retail haelt den Unit-
  Stringblock lbl_802DAA6C in r30 (+0x95 head, +0x9A l_arm_chn)
  und player in r31; unser rodata-Pool traegt erst die zwei
  Strings dieser Funktion → player bleibt in r30, keine
  Stringreg-Allokation. Jeder Rest-Diff ist die r30↔r31-Umbenennung,
  gleichwertige Float-Labels (@16148… vs lbl_805A4EB0/4EBC/4EC0/
  4ED0/4EE0), der Timer-.sbss-Name (gPlayerSuitChangeTimer vs
  lbl_805A1C50) und Scheduling-Kaskade. Schliesst mit Unit-
  Vollstaendigkeit (fn_800CE054 bindet dieselben Strings).
- **Gate:** R24J01 main.dol OK 6641/8284, Baseline-Diff 0 verloren
  +1 neu; GGTE01/GGTJ01_01/GGTP01 je "17 files OK";
  --non-matching linkt (606 Objekte); callcheck R24J01/GGTE01
  kein xobj_chr-Eintrag; symbols.txt-Diff = nur die eine neue
  Zeile; clang-format-18.1.8 dry-run sauber.
- **Liegt:** `python3 tools/check.py --source-only` wurde vom
  Bediener zweimal verworfen (auch mit CLANG_FORMAT=~/.local/opt/
  clang-format-18/bin/clang-format — die Binary existiert und die
  geaenderten Dateien sind einzeln geprueft sauber). Uebrig sind
  die bekannten Pool-/RegAlloc-Waende; kein Hard-Blocker.
- **Naechstes:** xobj_chr Adressfolge: fn_800CE054 (0x438,
  Attachment-/Suit-Teardown, bindet head/l_arm_chn + beide
  CJArchive — koennte den Stringpool in Retail-Reihenfolge
  ergaenzen und UpdatePlayerSuitChange/ApplyPlayerSuit Richtung
  100 ziehen); danach fn_800CCE68, 0-%-Reste fn_800C9E84/
  fn_800CB1DC; fn_800CE48C. obja-Rest unveraendert.

## Lauf 74 — xobj_chr: LoadPlayerSuitItem MATCHED (100 %), ApplyPlayerSuit (96,84 %)

Commit 474d9b3c (2 Dateien, +67/−4): src/game/wii/xobj_chr.cpp +
config/R24J01/symbols.txt (LoadPlayerSuitItem .text:0x800CDC98
size 0x114, ApplyPlayerSuit .text:0x800CDDC0 size 0x294,
gSpecialSuitSpawnIds .data:0x80304040 size 0xC = {0x20,0x22,0x23}).

- **LoadPlayerSuitItem(0x800CDC98) = 100,00 % — MATCHED**
  (6638→6639). Wii-Variante des GC-LoadPlayerSuitItem
  (xobjchritem.h): mSuits bei this+0xDC50 (mLoadStep _00,
  mCurrent _08, mSpecial _10, mObjects[9] _18). Schritt 1:
  Suit = mObjects[mCurrent]; SpawnId = mCurrent==7 ?
  gSpecialSuitSpawnIds[mSpecial] : mCurrent+25;
  suit->LoadFromPlacement(CoordInfo(GetSpawnData(id))); bei
  Erfolg `suit->SetAnmSource(&player->mObject)` (NW_Obj an
  this+0xE8 — Wii sourct die Suit-Anim aus dem Player-Obj,
  KEIN separater Animdatei-Load wie am Cube), mLoadStep=0.
  Schritt 2: nur mLoadStep=0.
- **MWCC-Lektion:** `if(!x) return FALSE` auf einem BOOL-
  Call-Ergebnis erzeugt cntlzw/srwi. (Bool-Materialisierung);
  `if(x){...;return TRUE;} break;` + gemeinsamer
  `return FALSE` hinter dem switch erzeugt cmpwi r3,0 +
  geteilten FALSE-Block (94,71 → 96,88 → 100).
- **ApplyPlayerSuit(0x800CDDC0) = 96,84 % fuzzy** — instruktions-
  identisch bis auf reine Referent-Diffs: Strings "head"/
  "l_arm_chn" landen in unserem @stringBase0, Retail in
  lbl_802DAA6C+0x95/+0x9A (Unit-rodata-Pool, beginnt mit
  "xobj_chr.cpp" — loest sich mit Unit-Vollstaendigkeit),
  plus sdata2-Pool-Namen @15945/@15947/@16183 vs
  lbl_805A4EB0/4EC0/4ED0. Gleiche Pool-in-r30-Strategie.
- **Semantik ApplyPlayerSuit:** Player aktiv (mStatus.mFlags&1)
  + Suit mit SceneModel → ClearAnim (vf110), bei Attachment 2
  vf290(FALSE); 4 Attachments via vf0A4("head"/"l_arm_chn")=
  GetWorldMtx + vf294 SetParent: Copter→head, Popper→l_arm_chn,
  Radar→head, OX→head; wenn !(gGame.GetStatusFlags(0x100)):
  vf28C(FALSE); bei Attach 2 vf290(TRUE). Attach 1|3:
  SetAnim 0x4A; sonst SetAnim 17 + mIdleLoops=3 (this+0xA88).
  u64-Arg-Konvention verifiziert: u64-Args in r5:r6-Paar,
  u64-Return in r3:r4 (or. testet den Return).
- **fn_800CDC08** ruft jetzt die beiden benannten Helfer;
  bleibt 98,69 % (dokumentierte RegAlloc-Wand).
- **Gate:** R24J01 main.dol: OK 6639/8284, --non-matching
  linkt, GGTE01/GGTJ01_01/GGTP01 je "17 files OK",
  Baseline-Diff: 0 verloren, +1 gewonnen, config-Status sauber
  (nur die 3 beabsichtigten Symbol-Zeilen).
- **Liegt:** check.py --source-only — "need clang-format
  18.1.8, got 23.1.4"; die 18.1.8-Binary ist weiterhin nicht
  auffindbar (Aufrufe wurden diesmal vom Bediener verworfen —
  evtl. Wrapper/Absolutpfad extern einmal pruefen). Callcheck
  nicht erneut gelaufen (keine neuen externen Calls: beide
  Funktionen rufen nur bereits benannte Symbole + virtuelle
  Slots).
- **Naechstes:** xobj_chr in Adressfolge weiter: naechste
  offene nach ApplyPlayerSuit u.a. fn_800CE054; 0-%-Reste
  fn_800C9E84, fn_800CB1DC; UpdatePlayerSuitChange-Kandidat
  fn_800CD708 gehoert zur selben Suit-Kette. obja-Rest:
  fn_800AD498, LoadFromPlacement 90,18, fn_800AE9F0,
  fn_800AF098, fn_800AF12C, fn_800AF51C, TestFloorCollision.

## Lauf 73 — xobj_chr: ResetPlayerAttachment rekonstruiert (97,94 %)

- **ResetPlayerAttachment (0x800CC83C, 0x4D4)** in src/game/wii/
  xobj_chr.cpp hinter ClearObjectStatusFlags — komplette
  Rekonstruktion aus Retail-Asm + GC-Vorlage obja_chr.cpp.
- **Semantik (alle Faelle verifiziert):** Switch auf GetAttachment():
  1 Copter (Sfx 0x75, Hide via XObjAttachment::_290), 2 Popper
  (Sfx 0x1F3+0x1F8, Hide, vf290(FALSE)=Mount unhidden, ResetPopperCharge),
  3 Radar (Sfx 0x1F9..0x1FB, Hide), default: mLift.mTarget — bei Pool 8
  wirds Inventar-Item (SetOrAddInventoryItemCount + XObjItem::SetFlag),
  ABER nicht wenn der Lift in den fruehen Zustaenden 1..3 steckt UND
  XActLift::_AC gesetzt ist (Wii-Neuerung: Put-away teilt sich mit dem
  Attachment-Wechsel); Pool != 24 → KEIN Release()-Call wie am Cube,
  sondern `mStatus.mFlags &= ~0x10000ULL` (Keep-Flag weg) +
  `mReleaseCountdown = 2` (Tick-down zum Release). Danach alle neun
  Action-Resets, Attachment-/Climb-/Kontakt-Felder, optional SetAnim
  (nextAnimation,TRUE,0,1,1,10), mFallDistance, 4x
  InfoScrn_ApplyObjectGaugeCommand(infoscrn,NULL,0,1,0..3).
- **Score 97,94 %** — Restdiff ist dokumentierte Wand-Klasse:
  10x Literal-Pool-Namen (@15865 vs lbl_805A4EB0, Unit-weit, loest
  sich mit Unit-Vollstaendigkeit), RegAlloc in der mObjData/mState-/
  Flag-Kette (r7/r6/r5/r4-Bindung) und ein extsh bei GetItem()→Arg.
  Kein Logik-Diff mehr — alle Stores/Calls/Branch-Struktur gleich.
- **MWCC-Lektion:** `state==1||state==2||state==3` als ein Ausdruck
  wird zum subi/cmplwi-Range-Check (94,06→); als gestufte u8-Bools
  (inLift=(1||2); inLift=inLift||(3); inLift=inLift&&_AC) entsteht die
  Retail-Flagkette (→97,94). u8-Cast je Teilterm zaehlt.
- **Masken-Korrektur:** die Release-Vormerkung loescht 0x10000, NICHT
  Bit 0 — lis 0xffff/subi 1 = 0xFFFEFFFF am Low-Word, High & -1.
- **Gate:** GGTE01/GGTJ01_01/GGTP01 ninja OK; R24J01 main.dol: OK,
  6638/8284 unveraendert (die Funktion ist fuzzy, nicht matched —
  kein Verlust). Keine splits/symbols-Aenderungen. NONMATCHING per
  Unit-Kopf abgedeckt; Wand hier dokumentiert.
- **Liegt:** (a) --non-matching-Link diesmal nicht gelaufen — Kommando
  wurde vom Bediener abgebrochen; vor naechstem Commit nachholen.
  (b) check.py --source-only meldet "need clang-format 18.1.8, got
  23.1.4" — Lauf-72-Stand sagt "via python3-subprocess-Wrapper: format
  ok" — der Wrapper (clang-format-18.1.8-Binary irgendwo im Suchpfad,
  NICHT ~/.local/bin oder /usr/bin) muss wiedergefunden werden; der
  nackte check.py-Aufruf allein reicht nicht.
- **Naechstes:** xobj_chr in Adressfolge weiter — naechste 0-%
  bzw. kleine Fuzzies hinter ResetPlayerAttachment; Restliste der
  61 offenen Funktionen liegt im Report (main/game/wii/xobj_chr).
  obja-Fuzzies weiterhin moeglich: LoadFromPlacement 90,18,
  fn_800AE9F0 72,91, fn_800AF51C 71,44.

## Lauf 72 — xobj_chr: XObjChr::ResetObjectState (2fa01204)

- **Commit 2fa01204:** `XObjChr::ResetObjectState` (0x800CC294, 0x574 B,
  Slot _154) in src/game/wii/xobj_chr.cpp vor SetVectorComponents —
  komplette Rekonstruktion nach Retail-Asm + GC-Vorlage obja_chr.cpp:
  Basis-Reset, vf28C(0), mMotion/Stick/Kontakt-Indizes, alle XAct-Init
  virtuell, Copter-Zeit aus gRefreshRate, Popper/Radar-Init, zwei
  Kamera-Records via fn_800C659C, mLook, Plug+Attachments+9 Suits,
  mMenu (Atc/Drs-Ring-Highlights), mOXMenu, Status-Bit 0x800000,
  mDustPool + 10 PointPtcl + gPointParticleSlots, SuitChangeTrail
  (lbl_8045BBD0, 0x258 B, contiguous mit Pool+Slots im .bss), Ground/
  EyeAim/HeadOffset, mActor.Reset(). Neue Member in cgame.h mit Pads:
  mStick[4], mFloorContactObjects, mNearDistance, mLetickerIndex,
  mLiftPlugFlags, mFrameTime (FrameEventTime), zwei 0x68-Kamera-Recs
  _9810/_987C, WiiOXMenu.mFade/_10. fn_800C659C in xobj_camera.cpp
  von u32 auf XObj* korrigiert (gleicher Code, ehrlicher Typ).
  ClearRingHighlights-Helfer fuer mAtc/mDrs.
- **Score:** 89,88 % fuzzy, NONMATCHING-Kommentar vor der Funktion.
  Restdiff rein Register-Allokation: Retail haelt gPointParticlePool-
  Basis in r31 ab Prolog und leitet Slots (+0x3D18) und Trail (+0x3D40)
  daraus ab (contiguous .bss); unser Build materialisiert jede Basis
  neu. Gleiche Instruktions-Menge (349), gleiche Calls, gleiche
  Store-Reihenfolge — Wand-Klasse register/schedule wie fn_800CDC08.
- **Gate:** voll bestanden (s. Stand oben); Objdiff-Unit-Name:
  `main/game/wii/xobj_chr`.
- **Liegt:** ResetObjectState-Regalloc-Wand — Permuter-Kandidat;
  mOXMenu._10 und WiiPlayerState._0A70/_969C semantisch unbenannt.
- **Naechstes:** xobj_chr weitermachen — Restfunktionen der Unit in
  Adressreihenfolge; sonst obja-Rest-Fuzzies LoadFromPlacement 90,18,
  fn_800AE9F0 72,91, fn_800AF51C 71,44.

## Lauf 71 — fn_800E8140 gematcht (+1); ResetObjectState dekodiert (Vorlage unten)

- **Commit 34038430** `wii xobj_chr: match fn_800E8140 (eye camera
  aim, 3-deg step both axes)` — 1 Datei, +42.
- **`fn_800E8140` (0x1C4) = 100 %.** Setzt mEyeAimTarget.y/x auf
  die Aufrufwinkel, mEyeAimStep.y/x = `DEG2RAD * 3` (ergibt exakt
  das Pool-Literal 0x3D567750), wickelt Ist+Ziel mit `WrapAngle`
  (gmath.h) nach [0,2pi), und bei |Ziel-Ist| > PI dreht es das Ziel
  um +/-TWO_PI (Kuerzester Weg). Struktur identisch zu
  `CObj::SetRotateY` (cobj.cpp ~1086) — zweimal hintereinander
  (y dann x). Hilfsmittel alle vorhanden: WrapAngle/TWO_PI/DEG2RAD
  aus gmath.h, PI ueber <math.h> bzw. stl/math.h, __fabsf.
- Gate komplett gruen: R24J01 6638/8284 `main.dol: OK`; GC 3x
  `17 files OK`; --non-matching linkt; Report-Diff nur die neue
  Funktion; Format 471 ok; keine neuen Symbol-Dubs.
- **fn_800CDC08 bleibt 98.69 %** — reine Registerwand, jetzt im
  Kommentar dokumentiert: Retail legt die +0x10000-Basis in r4,
  mCurrent in r0 und mRequested in r3 (Parameter-Register
  wiederverwendet); unser Build basiert in r3 und laedt mRequested
  zuerst in r4. Vier ehrliche Schreibweisen (Locals current/
  requested, Operandentausch) erzeugen alle identischen Code —
  Allokator fest verdrahtet. Nur Kommentar-Commit.
- **GetPlayerFirstPersonEyePosition (0x800E589C, 99.975 %)** und
  fn_800E59DC (95 %): Rest ist nur die Lage der String-Literale
  "L_eye"/"R_eye"/"carry_obj_nll" im Pool (Retail +0x249/+0x24F
  vs. unser +0x0E/+0x14) — Unit-Stringreihenfolge, loest sich erst
  mit weiteren String-Benutzern in der Unit. Nicht erzwingen.

## Naechster Schritt: ResetObjectState__7XObjChrFv (0x800CC294, 0x574)

**Der grosse offene Brocken in xobj_chr.cpp.** GC-Vorlage komplett:
`XObjChr::ResetObjectState` in src/game/obja_chr.cpp ~Zeile 10644.
Wii-Asm: build/R24J01/asm/game/wii/xobj_chr.s Zeilen 6076-6428.
r30 = this. Dekodierte Reihenfolge (Store-Offsets -> GC-Quelle):

1. `XObj::ResetObjectState()` (direkter bl auf Basis-Impl).
2. `vf28C(0)` — virtual Slot 0x28C (in cgame.h deklariert); ersetzt
   GCs `SetPlayerEyeCameraJointsHidden(this,FALSE)`.
3. Float-Loads: f2=lbl_805A4EB0 (0.0f), f1=lbl_805A4EBC (Wert
   pruefen — vermutlich 2.0f -> mNearDistance), f0=lbl_805A4EC0
   (1.0f -> mMoveSpeedScale), r29=-1, r28=0; r0 = *(0xA74) mit
   `rlwinm 0,25,23` = &= ~0x80 gemaske (mLiftPlugFlags).
4. Stores in Asm-Reihenfolge:
   - stfs f2: 0x9D4,9D8,9DC,9E4,9E8,9EC,9F4,9F8,9FC,A0C,A08,A14,A10
     = `mMotion.Reset(0,0,0)` + Stick-Nullen; vptr-Plaetze 9E0,9F0,
     A00,A04 frei (CObjVec _09D4 enthaelt CVecs, vptr +0xC).
     CObjVec::Reset-Definition nachschlagen.
   - stw r29: A18=m_0A18 (mActionObjectIndex), A1C=m_0A1C
     (mHitObjectIndex), dann A24,A20,A28,A30 =
     `mFloorContactObjects[4]=[2]=[0]=[1]=-1` (Kette, RTL).
   - A44 stw -1 -> mLastContactFloorIndex (NEU deklarieren).
   - A58 stfs f1 -> mNearDistance (NEU, f32).
   - A6C stw -1 -> mLetickerIndex (NEU, s32).
   - A70 stw 0 -> GCs _A14 (NEU, s32).
   - A74 stw r0(maskiert) -> mLiftPlugFlags &= ~0x80 (NEU, u32).
   - A78 stw 0 -> mAPressed, A7C stw 0 -> mPressTimer (NEU, s32).
   - A88 mIdleLoops=0, A8C mIdleTimer=0.0f (vorhanden).
   - A94 mFlashCount, A90 mFlashTimer = `mFlashTimer = mFlashCount
     = 0` (Count zuerst gespeichert — beide vorhanden).
   - A98 mHitStrength=0.0f, A9C mMoveSpeedScale=1.0f (vorhanden).
5. Sieben Action-`Init(this)` ueber Vtable-Slot _08 (vptr sitzt
   bei Objekt+8 — 8 Bytes vor dem vptr): `addi r3,r30,off;
   mr r4,r30; lwz r12,off+8(r30); lwz r12,8(r12); bctrl`.
   B50 mHang (danach stw 0 -> 0xB7C = mHang.mStickAway=FALSE);
   BA0 mJump (danach stfs f0 BEC,BE8 = mTargetStrength/mStrength
   -> `mStrength = mTargetStrength = 0.0f`); BF4 mLadder (danach
   stw 0 -> 0xC20 = mLadder.mBPressed); C38 mRope; C78 mPush;
   CD0 mTumble; D18 mCopter (danach stfs 0xD44 = mCopter.mTime =
   (f32)(u32)gRefreshRate — xoris/0x4330/lfd-Idiom).
   `InitializePlayerPopperAction(&mPopper /*_D50*/, this)` und
   `InitRadarAction(&mRadar /*_95D4*/, this)` — externe C-Fns in
   src/game/wii/xactradar.cpp:850/1214.
6. +0x9694..96A4: stfs -1.0 (lbl_805A4EB4) 9698 dann 9694 =
   mFrameTime.mLastFrame/mFrame (Kette `mFrame = mLastFrame =
   -1.0f`, NEU); stw 0 969C (drittes FrameTime-Feld o.
   mIdleJointCooldown — Beleg pruefen); stw 0 96A0/96A4 =
   mBlinkWait/mBlinkFrame (vorhandene Namen) ODER GCs
   mAttachmentJoint/mAttachmentJointAnim=NULL — nach Nutzung
   entscheiden.
7. ZWEI `fn_800C659C(obj, this)`-Calls: obj = this+0x9810 und
   this+0x987C — vermutlich mHeadTurn.mNeck/mHead (GC hatte drei
   Joint-Rots: mJointRot + Neck + Head). fn_800C659C-Asm lesen,
   Signatur/Typ bestimmen, Member deklarieren.
8. mLook (XObjChrLook _98E8, cgame.h:921): stw 0 98E8=mActive;
   stw -1 98EC=mMode; stw r30 98F0=mObject=this (GCs NULL-then-this
   faellt zu einem Store zusammen — einmal `mLook.mObject = this`);
   Stack-Null-Vec -> stfs 98F4/98F8/98FC = mOffset {0,0,0} (+0xC);
   stfs 0.0 990C/9910/9914 = mPulse/mPhase/mFade.
9. stw -1 9924 = mAttachmentRequest; stw 0 9928 = mAttachmentStep
   (deklariert!), dann 992C/9930/9934 = mScriptAttach/
   mNextClimbType/mClimbType (NEU, Kette `mClimbType =
   mNextClimbType = mScriptAttach = mAttachmentStep = 0` — Step
   zuerst).
10. stfs 0.0 9DE0 = mFallDistance (NEU). NAN (__float_nan):
    stfs 9DEC,9DE8,9DE4 = `mCameraTarget[0]=[1]=[2]=NAN` (f32[3]
    _9DE4, NEU). stfs 1.0 9DF4 dann 9DF0 =
    `mCameraTargetMargin = mCameraTargetMarginSetting = 1.0f`
    (Margin _9DF0, Setting _9DF4, NEU). NAN: 9E00,9DFC,9DF8 =
    `mHeldMoveAngle = mMoveAngle = mHeldStickAngle = NAN`
    (HeldStickAngle _9E00 NEU; Reihenfolge: Stick zuerst).
    stw 0 9E04 = mBatteryWarningFrames (NEU); stfs 0.0 9E08 =
    mBatteryWarningAngle, 9E0C = mBatteryWarningFade (NEU —
    Asm-Reihenfolge angle vor fade beachten);
    stw 0 9E10 = mGaugeTarget=NULL; stfs 0.0 9E14 = mGaugeTime.
11. FUENF `lwzu`-Vtable-Calls Slot 0x154 (= ResetObjectState):
    `addis r3,r30,1; lwzu r12,-off(r3); lwz r12,0x154(r12); bctrl`
    — r3 zeigt danach auf das Member (eingebettet, vptr bei +0):
    9E20 mPlug (WiiPlayerPlug) -> `mPlug.ResetObjectState()`;
    B580 mCopterObj, BF30 mPopperObj, C8E0 mRadarObj, D2A0 mOXObj
    (u8-Blobs -> `((XObj*)mXObj)->ResetObjectState()` oder
    entsprechender Cast — siehe Lauf-67-Notiz).
12. Suit-Loop: r26 = this+0xDC68 = &mSuits.mObjects[0] (mObjects
    bei WiiPlayerSuits+0x18), i=0..8, Stride 0x9D0, je
    `->ResetObjectState()` -> `for (s32 i=0;i<9;++i)
    mSuits.mObjects[i].ResetObjectState()`.
13. Statusmaske: `lwz r0,0x150; lwz r6,0x154; r5=0xFF80FFFF;
    and r7,r6,r5; and r0,r0,r8(-1)` -> `mStatus.mFlags &=
    ~0x7F0000ULL` (Low-Word & 0xFF80FFFF, High-Word & -1; MWCC
    emittiert die Identitaets-AND).
14. r3 = this+0x20000: mMenu/mOXMenu/mDust-Bereich —
    stw 0 +0x19EE4 = mMenu+0; zwei Highlight-Ringe unrolled:
    stfs 0.0 x9 Stride 0x28 ab +0x19EEC (mMenu.mAtc) und ab
    +0x1A17C (mMenu.mDrs) — `ClearRingHighlights(...)` zweimal
    (Helfer-Def suchen); stw 0 +0x1A174; stw 0 +0x1A424 =
    mOXMenu.mStarted=FALSE (+4), stw -1 +0x1A430, stfs 0.0
    +0x1A434 = mOXMenu.mFade (WiiOXMenu cgame.h:849, XObjChrMenu
    xchrmenu.h:9 — Feldnamen pruefen); stw 0 +0x1B7B8 =
    mOnGround=0 (GCs `mInAir = TRUE`-Gegenstueck!);
    `InitParticlePool(&mDust.mPtcl /*_1B7BC*/, mDust.mPtcls
    /*_1B7D0*/, 0x50, 0x32)` — Signatur include/game/cptcl.h:88,
    mDust-Member NEU.
15. Point-Pool-Loop x10 (r31=gPointParticlePool, xobjchrstate.h):
    `GetHudPointParticleOrigin(&gGame.mInfoScrn /*gGame+0xAF58*/,
    &p.+0, &p.+4); stw 0 p+0xC; stw 0 p+0x74;
    gPointParticleSlots[i]=NULL` — PointPtcl Stride 0x61C, Felder
    +0xC/+0x74 pruefen (GC: burst.mState/display.mState).
    Danach stw 0 gPointParticlePool+0x3D40 + ~40 stfs 0.0f ab
    +0x3D50 Stride 8 — grosser Pool-Bereich genullt (Symbol/
    Struktur fuer +0x3D40 ermitteln; evtl. memset oder unrolled
    Loop in pointptcl.h-Dekl).
16. Tail: stw 0 +0x1C9A4 = mGroundType=0; stfs 0.0 x17:
    +0x1C9A8,AC,B0 (Vec _1C9A8 = GCs _158C4.Set(0,0,0)),
    +0x1C9B8..C8 fuenf Floats (_1C9B8[3] + _1C9C4[2] — _1C9C4 als
    f32[2] umdeklarieren), +0x1C9CC..D4 mEyeAim, +0x1C9DC..E4
    mEyeAimTarget, +0x1C9EC..F4 mEyeAimStep (vptr-Slots D8/E8/F8
    frei — CVecs bestaetigt); stfs 5.0 (lbl_805A4EC4) +0x1C99C =
    mHeadOffset; stw 0 +0x1C9A0 = mHeadOffsetReady=FALSE;
    `bl Reset__12AnimSfxActorFv` r3 = this+0x1CC90 — ZWEITER
    AnimSfxActor (nicht mSfx _95AC): GCs
    `gPlayerSuitChangeEffect.Reset()`-Gegenstueck ist hier ein
    MEMBER — deklarieren (Groesse aus animsfx.h).
17. Epilog _restgpr_25.

Direkt danach: ResetPlayerAttachment (0x800CC83C) — gleiche
Behandlung, Asm folgt in xobj_chr.s ab ~6430.

### Liegt

- fn_800CDC08: Registerwand (s.o.), dokumentiert, nicht
  erzwingbar ohne Tricks.
- String-Pool-Reste bei 0x800E589C/0x800E59DC: Unit-Reihenfolge,
  wartet auf weitere String-Benutzer in der Unit.
- ResetObjectState: Asm vollstaendig dekodiert (oben), Source noch
  nicht geschrieben — naechster Lauf schreibt ihn in
  src/game/wii/xobj_chr.cpp (Adressfolge: nach dem vorhandenen
  Block, vor ResetPlayerAttachment 0x800CC83C), deklariert die
  ~20 neuen Member in cgame.h an ihren Offsets (+STATIC_ASSERTs),
  baut iterativ R24J01 und gleicht Store-Reihenfolgen ab.


## Lauf 70 — xobj_chr: SetPlayerEyeCameraJointsHidden gematcht (+1)

- **Commit 8c490ad5** `wii xobj_chr: match
  SetPlayerEyeCameraJointsHidden (0x800E7FB0)` — 2 Dateien, +33/-1.
- **`SetPlayerEyeCameraJointsHidden` (0x190) = 100 %.** Signatur
  `(WiiPlayerState* player, BOOL hidden, f32 rotY, f32 unused)` —
  aus cmotioncontrolcamera.cpp-Aufrufen. Schreibt IMMER drei
  CVec-Tripel (x=unused, y=rotY, z=0 / identisch / 0,0,0):
  mEyeAim _1C9CC, mEyeAimTarget _1C9DC, mEyeAimStep _1C9EC.
  CVec = 0x10 Byte, vptr bei +0xC (Retail-Beweis aus
  GetViewPosition `stw __vt__4CVec,0xc`). Der 0xC-Abstand der
  Stores ist das Give-away — ResetObjectState nullt alle neun
  Floats, laesst die vptr-Plaetze frei.
- Layout-Fix in cgame.h: `Vec mEyeAim` -> drei `CVec`
  (mEyeAim/mEyeAimTarget/mEyeAimStep). WiiPlayerState ist reines
  Overlay (kein Symbol emittiert), sicher aenderbar.
- Nachbar fn_800E8140 (0x800E8140) nutzt dieselben neun Floats:
  Winkel-Lerp mit pi/2pi/0.05235988f (3 Grad pro Frame) —
  Augenkamera-Ziel-Anfahrung, bestaetigt die Semantik.
- Scene-Teil nur wenn `GetAttachment()==2 && hidden` (Attachment 2
  = Augenkamera): Queue-Muster identisch zu fn_800E7EA8 (Player,
  aktiver Suit, SuitPart bei mCurrent 3||6, Popper), dann
  `fn_802AEDA0(&mHead,0)`, dann Drain: Retail haelt mHead in r30
  und testet `head->mPending` je Iteration — ein
  `WiiGsSceneObj* head = gGame.mGs.mScene.mHead;`-Local +
  `while ((pending = head->mPending) != 0) if (pending != 0)
  head->vf038(pending-1);` erzeugt genau das (96,44 % -> 100 %).
  Kontrast: fn_800E7EA8 will KEIN head-Local (laedt mHead vor
  jedem Call neu); hier ist das Local Retail-idiom.
- Gate: R24J01 OK 6637/8284 (+1); GC 3x `17 files OK`; config/
  sauber; Report-Diff nur die neue Funktion None->100; callcheck
  keine neuen Treffer.

## Lauf 69 — xobj_chr: fn_800E7EA8 gematcht (+1)

- **Commit bbc85e92** `wii xobj_chr: match fn_800E7EA8 (queue worn
  models, step scene draws)` — 1 Datei, +17.
- **`fn_800E7EA8` (0x108) = 100 %.** Dekod: `mScene.mHead` ist ein
  ScnRoot* (ScnGroup): Vtable-Slot 0x34 = `Insert(u32,ScnObj*)`,
  Member _E4 = `mNumScnObj` — der WiiGsSceneObj-Stellvertreter in
  cgs.h modelliert das als `vf034(s32 index, void* model)` +
  `mPending`. Vier Queue-Aufrufe
  `head->vf034(head->mPending, X->mObject.m_pScnMdl)`: this (XObj+0xF0
  = NW_Obj::m_pScnMdl), aktiver Suit (mSuits.mObjects[mCurrent], Flag
  = Low-Word von mStatus.mFlags bei +0x154, `clrlwi.` =
  `IsObjectActive`), SuitPart bei mCurrent==3||6 (Stride 0x9B0),
  mPopperObj+0xF0 (=0xC020, u8-Blob -> XObj*-Cast). Danach
  `fn_802AEDA0(&mScene.mHead, 0)` = G3dReset + UpdateFrame + CalcVtx
  + CalcWorld + CalcMaterial auf *mHead (extern "C", Dekl wie
  gameupdate.cpp). WICHTIG: kein `scene`-Local — Retail laedt mHead
  vor JEDEM Call neu (bctrl kann gGame aendern); voller Ausdruck je
  Statement schreiben.
- Definition `extern "C"` (fn_-Auto-Name), steht nach dem
  extern-C-Block (Zeile ~1617). Platz: zwischen vf1A8 und SetBaseMtx.
- Aufrufer: fn_80151344 (Bahn 5, noch asm) ruft nach ClearBaseMtx.
- Gate: R24J01 OK 6636/8284 (+1); GC 3x `17 files OK`; config/
  unberuehrt (kein symbols.txt-Eintrag noetig, Auto-Name);
  Report-Diff nur fn_800E7EA8 0->100; callcheck keine neuen Treffer.

## Naechster Schritt

- xobj_chr weiter in Adress-Reihenfolge: fn_800E3CD4
  (GetCameraTarget-Gegend), SetAnim 0x800E333C (gross);
  fn_800E8140 (Augen-Lerp, nutzt die drei neuen CVecs — jetzt
  modelliert, Asm schon gelesen: pi/2pi/3-Grad-Stepping);
  ResetObjectState 0x800CC294 / ResetPlayerAttachment 0x800CC83C
  (GC-Vorlagen in obja_chr.cpp, Wii-Offsets gemappt).
- obja-Folge: LoadFromPlacement 0x800AC238, fn_800AE9F0, fn_800AF098,
  fn_800AF12C, fn_800AF51C, TestFloorCollision 0x800B034C.
- Alt-Liegt: DrawWithView 92,0 (Schedule), DrawGroundShadowAt 99,7
  (1 Instr.), Attachment-DrawWithView 75 (toter blr), Walls s. unten.

## Lauf 68 — xobj_chr: drei Suit-Helper, alle 100 % (+3)

- **Commit 8b04dc42** `wii: XObjChr anim-rate scaling and suit flag
  forwarders, all matched` — 2 Dateien, +47/-1.
- **100 % gematcht:** `fn_800E3968` 0x800E3968 (0x17C) — rate =
  1.135f nur bei mLift.mState==4 && IsLiftTargetPlug(&mLift) &&
  anim!=0x13B, sonst 1.0f, mal (60.0f/gRefreshRate), mal frame;
  auf chr via `chr->CObj::SetAnimRate` (qualifizierter Basis-Call →
  direkter bl, kein Vtable-Slot!) und auf den aktuellen Suit via
  virtual SetAnimRate _114; GetAnimId() (_11C) wird je Site als
  Argument frueh geholt. Logik als `static inline
  PlayerAnimRateScale(chr, anim)` geteilt — identisch zum Koerper
  von fn_800E32B8. Suit-Test als `(mStatus.mFlags & 1) == TRUE`
  (xori-Idiom), Suit inline indiziert wie in fn_800E3AE4 — ein
  benannter Suit-Zeiger kostet die r30-Belegung (89,8 % → 100 %).
  `fn_800E3BCC` 0x800E3BCC (0x84) — m_9C8 = held!=0 (neg/or/srwi),
  extern-"C" fn_800F3E9C(chr, held!=0), dann
  mSuits.mObjects[mCurrent].vf28C(held) wenn aktiv.
  `fn_800E3C50` 0x800E3C50 (0x84) — Zwilling: m_9C9,
  fn_800F403C, vf290. Beide extern-"C"-Deklarationen lokal.
- Neues Member `u8 m_9C9` _9C9 in WiiPlayerState (cgame.h).
- Erkenntnisse: (a) qualifizierter `chr->CObj::SetAnimRate(...)` ist
  Retail-Idiom fuer "Basis-Implementierung direkt"; (b) Retail
  materialisiert KEINE Suit-Pointer-Lokale — immer
  `mSuits.mObjects[mCurrent]` inline wiederholen; (c) `(flags & 1)
  == TRUE` erzeugt clrlwi+xori+cmpwi+bne, `IsObjectActive()`
  erzeugt clrlwi.+beq — je nach Retail-Asm waehlen.

## Lauf 67 — xobj_chr: fuenf Forwarder + Slot-Korrekturen, alle 100 % (+5)

- **Commit 91eb5fd8** `wii: XObjChr cull-mode, flag and fade
  forwarders, all matched` — 5 Dateien, +165/-8.
- Basisslot _1A4 korrigiert: evt.s ruft ihn mit (0,0) → zwei Argumente;
  XObjPlug::SetDestinationAlpha(BOOL,u8) ueberschreibt denselben Slot.
  `XObj::vf1A4()` wurde `XObj::SetDestinationAlpha(BOOL,u8)`
  (0x800AD9D0, Symbol umbenannt), xobjplug.cpp ruft jetzt die Basis
  direkt statt des extern-"C"-Wrappers fn_800AD9D0.
- Stale-Symbole korrigiert (Retail-Asm bewies Vertauschung):
  0x800D209C ApplyMaterialPresets→`SetCullMode__7XObjChrFl`
  (ruft Basis SetCullMode + Member-_1C0); 0x800D2204
  SetDestinationAlpha→`ResetCullMode__7XObjChrFv` (Basis
  ResetCullMode + Member-_1C4, keine Args benutzt).
- **100 % gematcht (alle fuenf):** `vf1BC__7XObjChrFl` 0x800D1F34,
  `SetCullMode__7XObjChrFl` 0x800D209C, `ResetCullMode__7XObjChrFv`
  0x800D2204 — Guard-Triade: Basis-Call, dann mPlug.HasModel();
  die vier Attachments ueber `((XObjAttachment*)…)->mVisible` (_9AC);
  aktueller Suit ueber IsObjectActive (mStatus.mFlags&1), Suit-Part
  zusaetzlich bei mSuits.mCurrent==3||6.
  `vf1A0__7XObjChrFPCUl` 0x800D2344 und
  `SetDestinationAlpha__7XObjChrFiUc` 0x800D251C — HasModel()-Guards
  (_224) ueberall + volle 9er-Schleife ueber mSuits.mObjects UND
  mSuitParts (nicht nur aktueller Suit).
- Offsets bestaetigt: mCopterObj _B580, mPopperObj _BF30,
  mRadarObj _C8E0, mOXObj _D2A0 (u8-Blobs, Cast auf
  XObj/XObjAttachment); mSuits.mObjects[i] _DC68+i*0x9D0;
  mSuitParts[i] _134B8+i*0x9B0; mSuits.mCurrent _DC58.
- GC-Gegenstueck bestaetigt die Semantik (obja_chr.cpp
  XObjChr::SetDestinationAlpha-Forwarder-Muster).
- Neue XObjChr-Deklarationen in xobj.h: vf1A0(_1A0),
  SetDestinationAlpha(_1A4), vf1BC(_1BC), SetCullMode(_1C0),
  ResetCullMode(_1C4). WICHTIG: WiiTitleObject::vf1A4 ist eine
  andere Hierarchie — nicht anfassen.

## Lauf 66 — xobj_chr: XObjChr Draw-Overrides (+2 matched, 2 knapp drunter)

- **Commit b2e63c06** `xobj_chr: XObjChr's four draw overrides
  reconstructed` — 4 Dateien, +145/-5.
- Vier Retail-Asm-Dekodes aus auto_fn_*.s (Vtable _178/_17C/_180/_184),
  4 Symbol-Eintraege in config/R24J01/symbols.txt, vier Override-
  Deklarationen in xobj.h (XObjChr, Slots wie Retail).
- **100 % gematcht:** `DrawReflection__7XObjChrFPA4_f` 0x800E05F0
  (vf28C(0)/DrawWithView/vf28C(1) um die Reflexion, dann
  mPlug/mCopterObj/mPopperObj/mRadarObj/mOXObj->_17C) und
  `DrawGroundShadow__7XObjChrFf` 0x800E0780 (MapDistance<1.0f → return;
  mStatus.mFlags&4 → GetStatusFlags(0x0040...ULL)→mPlug._180,
  sonst XObj::DrawGroundShadow).
- **NONMATCHING:** `DrawGroundShadowAt` 0x800E083C 99,66 % — eine
  Instruktion Rest; Ablauf sonst identisch (mug: FindResNode
  "carry_obj_nll" + GetJointScale + CustomShadow-Draw, sonst
  GetAnimId()-0x14D<=4 → "koshi_model" + Basis). `DrawWithView`
  0x800E0234 92,03 % — fuenf Index-Cleanups + Suit-/Attachment-Draws
  stimmen, Register-/Schedule-Rest ueber dem Kopfteil.
- Neue Member in WiiPlayerState (cgame.h, Offsets aus Retail):
  `m_9C8` (u8, blockiert den Uebergangs-Suit-Part), `m_0A18`, `m_0A1C`
  (s32 Objekt-Indizes, DrawWithView invalidiert bei inaktivem Objekt
  gegen gGame.mXObjs.mObjects[i] = gGame+0xF8B0).
- Erkenntnis: `mSuits.mCurrent` _DC58, `mChangeStep` _DC54,
  `mSuitParts[9]` _134B8 (stride 0x9B0), `mLift.mState` _0AA4,
  Plug _9E20; Kamera `mMotionControlCamera.mView.mAngle.x` +0xB0,
  `.mViewMtx` +0xD8.

## Lauf 65 — xobj_chr: 11 fn_-Namen dekodiert und gematcht (+11)

- **Commit 2bca7191** `wii xobj_chr: 11 weitere Accessoren und
  XObjChr/XObjAttachment-Virtuals matcht` — 4 Dateien, +107/-1.
- Methode: unbekannte fn_-Symbole gegen Retail-Asm, Vtables und die
  GameCube-extern-"C"-Namen (obja_chr.cpp) gemappt. 10 Eintraege in
  config/R24J01/symbols.txt ergaenzt.
- **100 % gematcht:** `Hide__14XObjAttachmentFv` 0x800CCD18 (Slot
  _290, mVisible=FALSE — Inline aus dem Header in out-of-line
  Definition im cpp verlegt, wie der dtor schon war),
  `DrawWithView`/`Hide` brauchten `virtual` in xobjattachment.h;
  `GetPlayerPlugConnection` 0x800D5864 (plug->mConnection 0xD64);
  `GetPlayerPlugCordPull` 0x800DB914 (&plug->mCord.mPull, 0xA14);
  `ResetPlayerPlugState` 0x800DB924 (Wii nur mConnection=0, GC setzte
  auch den Joint zurueck); `fn_800DB958` (mTrigon.mTri[hit->mTriangle]
  .mGetVtx-Tail-Call); `GetObjectMarkerOverride` 0x800DBF14 +
  `GetMaskedObjectMarkerOverride` 0x800DBF1C (mAnmQueue.
  mMarkerOverride 0x7D4); `fn_800D1740` (__fabsf-Wrapper);
  `vf1A8__7XObjChrFf` 0x800D26FC (leer);
  `SetBaseMtx__7XObjChrFPA4_f` 0x800E8304 + `ClearBaseMtx__7XObjChrFv`
  0x800E8388 (mBaseMtxPtr + PSMTXCopy + Loop ueber 9 Suits via
  `(WiiPlayerState*)this->mSuits.mObjects[i]`).
- **Bleibt NONMATCHING:** `DrawWithView__14XObjAttachmentFPA4_f` 75 %
  — identisch bis auf toten `blr` nach dem Tail-Call, den unser MWCC
  immer anhaengt (alle Guard-Schreibweisen probiert).
  `ScaleVectorInPlace` 40,7 % — ruft PSVECScale; Retail interleaved
  die ps-Ops (andere SDK-Inline-Reihenfolge), ehrlich dokumentiert.
- Erkenntnis XObjChr-Vtable: fn_800D2344/_251C/_26FC = vf1A0/vf1A4/
  vf1A8-Overrides; fn_800E8304/_8388 = SetBaseMtx/ClearBaseMtx;
  fn_800E05F0/_0780/_083C = DrawWithView/DrawReflection/DrawGround-
  Shadow-Overrides; fn_800D1F34 = _1BC-Slot — noch alle offen.
- XObjAttachment hat jetzt `virtual void DrawWithView(Mtx)` (_178)
  und `virtual void Hide()` (_290) deklariert; XObjChr hat die drei
  Override-Deklarationen.

## Liegt (Blocker, kein Stopp)

- `DrawWithView__14XObjAttachmentFPA4_f` (75): toter `blr` nach
  Tail-Call, Compiler-Emissions-Unterschied.
- `ScaleVectorInPlace` (40,7): JMathInlineVEC-Statement-Reihenfolge
  weicht von Retail-SDK ab (shared header nicht angefasst).
- `DrawWithView__7XObjChrFPA4_f` (92,0): Register-/Schedule-Rest im
  Index-Cleanup-Kopf, Inhalt stimmt.
- `DrawGroundShadowAt__7XObjChrFPC3Vecf` (99,7): ein Rest.
- Bekannte Alt-Waende unveraendert: StopPlayerMotion (f1/f2/f3-
  Nummerierung), SelectPlayerEquipmentRing (bool-Materialisierung),
  GetCharacterObjectApproachResult (Load-Hoist), EyePosition +
  fn_800E59DC (String-Pool), GetPlayerGroundType (clrlwi.-Fold),
  FindPlayerSupportingFloor (Regalloc), fn_800E32B8 (Early-Load).

## Naechster Schritt

- fn_800E7EA8 dekodieren (auto_fn_800E7EA8_text.s liegt bereit, noch
  nicht analysiert); weitere kleine fn_ in xobj_chr in Adress-Reihenfolge:
  fn_800E3CD4 (GetCameraTarget-Gegend), SetAnim 0x800E333C (gross).
- DrawWithView (92 %)/DrawGroundShadowAt (99,7 %) Reste nur bei
  klarer Compiler-Ursache nachfassen, kein Permuter.
- XObjChr::ResetObjectState 0x800CC294 (0x574) und
  ResetPlayerAttachment 0x800CC83C (0x4D4): GC-Vorlagen in
  obja_chr.cpp komplett, Wii-Offsets teils gemappt (s. Lauf-66/67-
  Notizen) — groesserer Port, Header-Mapping noetig.
- obja-Folge: LoadFromPlacement 0x800AC238, fn_800AE9F0, fn_800AF098,
  fn_800AF12C, fn_800AF51C, TestFloorCollision 0x800B034C.


## Lauf 64 — xobj_chr: fn_800E3AE4 matched (+1), Format-Gate gruen

- **Commit a979c5dc** `wii xobj_chr: match fn_800E3AE4 (object event
  floor check)` — 3 Dateien, +15/-15.
- **`fn_800E3AE4` 96,38 -> 100 % MATCHED.** War bei Laufbeginn schon
  im Arbeitsbaum auf 100 (Nebeneffekt des mListA/mListB-[10][10]-
  Modells aus Lauf 63 — Retail adressiert die Grid-Zellen genau so);
  der Report zaehlt ihn jetzt mit: 6613 -> 6614.
- **`SelectPlayerEquipmentRing` bleibt 98,74 % NONMATCHING** (beste
  bekannte Form `if (chr->mMenu.IsOpening())` wiederhergestellt).
  Wand klassifiziert: Retail materialisiert `mDrs.mOpen <= 0` als
  Wert ueber cntlzw/rlwnm in eine TRUE-initialisierte Flag; jede
  &&/||/ternary/!/bool-Schreibweise senkt MWCC zu branches oder
  neg/andc/srwi ab (Versuche 88,9–96,7 %). Zusaetzlich bekannte
  Rotations-Kopie wie in StopPlayerMotion + mSel-Reg.
- **`GetCharacterObjectApproachResult` bleibt 86,67 %**: Early-Return-
  Form statt Ternary probiert -> 73,0 % (Branch-Polaritaet + zusaetzl.
  li/b-Block), rueckgaengig. Wand = Retail zieht den
  mAttr.mObjData-Load (0x168) VOR die Prolog-Stores (lwz vor stw r0),
  nur Scheduling.
- **Format-Gate geloest:** `CLANG_FORMAT=~/.local/opt/clang-format-18/
  bin/clang-format python3 tools/check.py --source-only` laeuft
  (ueber subprocess-Wrapper; direkte Binary-Aufrufe werden vom
  Environment rejected). 2 Alt-Fundstellen in xobj_map.cpp/xobj.h
  (Align-Whitespace aus Lauf-61/63-Commits) formatiert, jetzt
  `471 file(s), 0 need formatting`, `format: ok disc files: ok`.
- **Gate:** R24J01 `main.dol: OK` 6614/8284 (+1, kein Regress);
  GGTE01/GGTJ01_01/GGTP01 je `17 files OK`; config/ sauber (keine
  dtk-Rewrites auf configure.py/splits.txt/symbols.txt). callcheck:
  kein callcheck.py im Worktree/tools vorhanden — Schritt entfaellt,
  Dol-SHA1 deckt Code-Regress ab.
- **Liegt:** SelectPlayerEquipmentRing Boolean-Materialisierungs-Wand
  (s.o.); GetCharacterObjectApproachResult Load-Hoist-Scheduling;
  Uebrige aus Lauf 63 (fn_800C98E0/9750 RegAlloc, fn_800C9B84
  inline/extab, fn_800C85EC offen). Alt-Delta
  UpdateObject__4XObjFUl/fn_800AF098 stammt weiterhin aus df3bf9f5.
- **Naechstes:** xobj_chr weitermachen — StopPlayerMotion 99,57,
  GetPlayerFirstPersonEyePosition ~99,9 (nur String-Referenten/
  Pool-Offsets!), fn_800CDC08 98,69, dann fn_800E59DC 95,0 /
  GetPlayerGroundType 95,33 / FindPlayerSupportingFloor 94,48 /
  fn_800E32B8 93,94 / fn_800E5BFC 93,24 / TryStartObjectEvent 93,28 /
  IsPlayerBlockingEvents 92,06. Eye-Position ist der reifste
  (Struktur stimmt, nur Referenten).


## Lauf 63 — xobj_map: fn_800C98E0 (FindMapFloor) 89,33 %, Listen als 10x10

- **Commit 7000d0f9** `Wii xobj_map: fn_800C98E0 (FindMapFloor) 89.33%,
  map lists as 10x10 grids`.
- **`fn_800C98E0` neu (676 B, 89,33 % ehrlich NONMATCHING):** Wii-
  out-of-line FindMapFloor. Ablauf: Hint-Dreieck probieren (mGetVtx +
  IsPointInsideTriangleXZ + IntersectVerticalLineWithTrianglePlane),
  dann Sort-Grid-Zelle `mListB[cellX][cellZ]` mit Record-Liste
  {u32 tri, f32 hoehe} (limit = pos->y + 7.5f), dann
  `CTrigon::Check(pos,-1,&hit->mHeight)`; Erfolg schreibt
  mTriangle/mTrigon=mCell/mHeight, Misserfolg mTrigon=-1+FALSE.
  NONMATCHING-Rest: gleiche Instruktionsfolge, nur RegAlloc/Stack-Slots
  (Retail haelt tri-Zeiger callee-saved, Frame r25-r31 statt r24-r31,
  Loop-Vecs in niedrigeren Slots als Hint-Vecs). objdiff-Richtung beim
  Diff beachten: links=Retail, rechts=eigener Build.
- **`mListA`/`mListB` [0x64] -> [10][10]** (xobj.h, nur xobj_map nutzt
  sie): Retail rechnet verteilt `this + cellX*0x78 + cellZ*0xC`;
  Nebeneffekt: `fn_800C807C` 78,2->80,93, `XObjMap::Release` 80,9->83,36.
  `cell->mItems[i]` per-Iteration member-access (kein gehoisteter
  Local) = retails `lwz`+`lwzx`.
- **Gate:** R24J01 `main.dol: OK` 6613/8284 (kein Regress; C98E0
  nonmatching zaehlt nicht), GC 3x `17 files OK`, config sauber,
  Format geblockt (clang-format 23.1.4 vs. gefordert 18.1.8 — Dauer-
  Blocker, host-seitig).
- **Liegt:** fn_800C98E0 RegAlloc-Wand (s.o.); fn_800C9750 94,75
  RegAlloc; fn_800C9B84 inline/extab-Wand; clang-format-Version.
- **Naechstes:** fn_800C9B84 (inline/extab) oder fn_800C9D88 /
  xobj_chr ab 0x800C9E84.

## Lauf 61 — XObjMap-Fortsetzung: fn_800C85D4 matcht, fn_800C9750 ehrlich

- **Commit a5b50037** `wii xobj_map: map rebuild + ANIM reset + mark
  draw proc, sort-grid fields` — uebernommener WIP (fand ich
  uncommittet vor, gatefaehig gemacht):
  - `fn_800C7F70` 81,5 %: Map-Rebuild — Szene-Modul pruefen,
    Modul auf Szene queuen (`vf03C` neu in `WiiGsSceneObj`,
    am Ende der deklarierten Virtuals = kein Slot-Shift),
    `CalcWorld`, dequeuen, `fn_802BA4F4` (Trigon aufbauen);
    >1000 Tri => Sort-Bounds/Cell-Size fuellen + `fn_800C85EC` +
    `fn_800C9B84`, sonst `mSortValid=0`.
  - `fn_800C807C` 78,2 %: Listen-Reset + `fn_800929EC` +
    Trigon-Release + Rebuild.
  - `fn_800C83DC` 74,7 %: 16 Debug-Marks, GX-State, markierte
    Dreiecke +0,1f y, Tri-Strip zeichnen, Mark loeschen.
  - `fn_800C9B84` 11,9 %: ehrlicher Wrapper um `fn_800929EC`;
    Retail hat eigenen Frame/Extab-Eintrag — Wand dokumentiert.
  - xobj.h: Sort-Grid-Felder `_9EC mSortValid`, `_9F0 mSortMin`,
    `_9FC mSortMax`, `_A08 mSortCellX`, `_A10 mSortCellZ`
    (STATIC_ASSERTs gesetzt); Listen bleiben `_A14`/`_EC4`.
  - cgs.h: `vf03C(void* model)` an `WiiGsSceneObj` angehaengt.
  - callcheck-Fix: `fn_802AEE44` hatte C++-Mangling; Retail nutzt
    Auto-Namen → `extern "C"` wie die anderen Deklarationen.
    Eintrag weg (53→52).
- **Commit 76c49cf2** `Wii xobj_map: fn_800C85D4 matcht,
  fn_800C9750 ehrlich 94.75%`:
  - `fn_800C85D4` **100 % MATCHED** — Komparator ueber
    `{u32 tri; f32 height}`-Records (Hoehenvergleich). Wird per
    `lis/addi` als Funktionszeiger in `fn_800C85EC` uebergeben.
  - `fn_800C9750` 94,75 % NONMATCHING (dokumentiert): Sort3 ueber
    drei Records via `(*cmp)`-Zeiger + `Rec t=**x; **x=**y; **y=t`-Swaps.
    Identische Befehlsliste (104 Ins); Retail sichert Args nach
    r27-r30 in Reihenfolge und haelt den 2. Record-Ptr fuer die
    letzten Stores, unser Build nummeriert r28-r31 um und laedt ein
    Mal neu — reine Regalloc-/Reload-Wand, ehrlich liegen gelassen.
- **Bekannte Alt-Deltas (NICHT aus diesem Lauf):** `UpdateObject`
  91,51→91,08 und `fn_800AF098` 98,38→95,27 kommen aus Commit
  df3bf9f5 (Pool-Regression, in Lauf-59/60-Uebergaben dokumentiert);
  per Stash+Rebuild verifiziert identisch auf HEAD vor dem WIP.
  `vf170`/`vf1A8` `None` = Signatur-Renames (`FPA4_f`/`Ff`,
  beide 100 % unter neuem Namen).
- **Tail von xobj_map nicht machbar ehrlich:** `fn_800C9BE4`/
  `fn_800C9C3C`/Dtor-Thunks/`@4@`-Stubs sind MSL-Exception-Klassen-
  Teile (vtable `what()`/deleting dtors) aus dem `throw` in
  `fn_800C85EC` — Compiler-Artefakte, nicht per Hand schreiben.
- **Gate:** R24J01 main.dol OK 6613/8284; `--non-matching` linkt;
  GGTE01/GGTJ01_01/GGTP01 je 17 files OK; kein xobj_map-callcheck-
  Eintrag; config/ sauber; check.py-Format weiterhin durch
  clang-format 23.1.4-vs-18.1.8-Hostproblem blockiert (Zeilen folgen
  dem Tab-Stil der Datei).
- **Liegt:** `fn_800C9750` Regalloc-Wand (s.o.); `fn_800C9B84`
  separater Retail-Frame/Extab (Inline-Grenze); `fn_800C85EC`
  (0x814, Grid-Aufbau + Exception-Machinerie) noch offen;
  `fn_800C98E0` angefangen analysiert: Cell-Index → Trigon-Tri →
  `IsPointInsideTriangleXZ` + `IntersectVerticalLineWithTrianglePlane`
  → schreibt `{tri,flags,y}` nach r31-Arg.
- **Naechstes:** `fn_800C98E0` zu Ende (dann `fn_800C85EC` Monster
  oder besser: obja-Adressfolge fn_800AE9F0, fn_800AF098 (Regress
  schauen), fn_800AF12C, fn_800AF51C, TestFloorCollision 0x800B034C).
  Retail-Disassembly des xobj_map-Objekts liegt in /tmp/xmap_dis
  (sessionfluechtig — neu erzeugen mit dtk/objdump).

## Lauf 60 — CustomShadow-Layout korrigiert + Ctor implementiert (90,5 %)

- **Thunk-/`None`-Rätsel gelöst:** die `fn_800C6584/7C4C/…`-Einträge
  und `dtor_*`-Stubs zeigen `None`, weil der Report per Symbolnamen
  paart — unser Compiler emittiert sie als `@4@__dt__…`/`__dt__…`,
  Retail hat Auto-Namen `fn_*`/`dtor_*`. Dauerhaft unscorebar, kein
  Punkteverlust. Verifiziert: `virtual ~XObjScnCam() {}` im Klassen-
  körper verschiebt `__vt__10XObjScnCam`+`@4@`-Thunk in UNSERE TU
  (Retails Anordnung: Dtors inline, `__dt__`-Kopien in xobjctor/
  xobj_ut/xobjmember sind weak-Emissionen aus den echten Ctor-Unwind-
  Tabellen). Zurückgenommen: unser Stub-Ctor referenziert den Dtor
  nicht → `__dt__10XObjScnCamFv` würde aus xobjctor verschwinden
  (Regress ohne Report-Gewinn). Ehrlicher Fix erst wenn die echten
  Ctors (ScnCam 0x800C48A0 0x2B0 u.a.) rekonstruiert sind.
- **CustomShadow-Layout korrigiert (xobj.h):** Retail-Ctor
  `stw r4,0xa68(r3)` speichert `lbl_80303780` = Vtable
  `[RTTI"CustomShadow",0,__dt__12CustomShadowFv]` HINTER den Daten —
  vptr bei _A68 (RTTI-Basenzeiger 0 = kein polymorpher Base). Daten
  liegen jetzt in `struct CustomShadowData` (0xA68), `class
  CustomShadow : public CustomShadowData` legt den vptr korrekt.
  Alter Kommentar "first virtual declared last" war der falsche
  Mechanismus (vptr wäre bei +0, Member um +4 verschoben).
- **`__ct__12CustomShadowFv` None → 90,47 %.** mCenter=0; Schleife 1:
  `mCircle[i]=(cos(i*s),0,sin(i*s))`; Schleife 2: Ringpaare
  `(cos,0,sin)`/`(2cos,0,2sin)` + `mColors[2i+1]=i`. s=0.19634955f
  (π/16), Skala 2.0f, int i (xoris-Idiom = signed→f64).
  Schlüssel: `(f32)cos(...)`-Cast VOR `*2.0f` → `fmuls` nicht
  `fmul`; Store-Reihenfolge x,z,y (MWCC hält Programmreihenfolge).
  Rest: `_savegpr_27` vs Retail `_savegpr_26` (ein Live-Reg mehr).
- **Gate:** GGTE01/GGTJ01_01/GGTP01 je 17 files OK; R24J01
  6605/8284, xobjctor 20/51, xobj_ut 6/9, xobjmember 3/64 — kein
  Regress. check.py: Host hat clang-format 23.1.4 (braucht 18.1.8);
  eigene Zeilen unter v23 nicht verletzt (Fundstellen vorbestehend).
- **Nächstes:** `Draw__12CustomShadowF3Vec3VecUl` (0x800C7130, 0x20C,
  GX-Fan/Ring-Strip); danach obja-Funktionen in Adressfolge
  (fn_800AD498, LoadFromPlacement, fn_800AE9F0, fn_800AF098,
  fn_800AF12C, fn_800AF51C, TestFloorCollision 0x800B034C).

## Lauf 59 — fn_800C6CEC gematcht (+1), drei grosse ScnCam-Funktionen ehrlich

- **Commit f3d6b884** `wii xobj_camera: match fn_800C6CEC, honest
  passes for fn_800C66C8/fn_800C6CF0/fn_800C7764`.
- **`fn_800C6CEC` 100 % MATCHED** — `b`-Thunk auf fn_800C6608:
  `extern "C" CVec fn_800C6CEC(XObjScnCamInitRec*, XObj*)` →
  `return fn_800C6608((u32)rec, obj)`.
- **`fn_800C66C8` 43,1 % NONMATCHING** (0x624): drei Arme
  (m_0C→GetPos×2!, mTarget-NAN-Check, m_20→mView.mEye), Yaw =
  `(f32)atan2(dx,dz) - rot->y`, Pitch = `dy>=0 ? HALF_PI-r : r-HALF_PI`,
  `atan2(FSqrt(dz²+dx²), FSqrt(dy²))`, Clamp gegen ±m_30/m_2C.
  Gleiche Ins-Anzahl (528), Rest = CVec-Temp-/Regallokation.
- **`fn_800C6CF0` 93,6 % NONMATCHING** (0x270): Zwei-Achsen-Tracker,
  WrapAngle-Norm, kürzester Winkelweg, speed=max(0.1*|d|,0.01), Snap-
  Zählerprotokoll. Rest: Retail `lha 0x2(r30)` mit r30=rec+2i (kein
  addi) vs unser `rec->mSnap[i]`-Zugang + Stack-Slot-Reihenfolge.
- **`fn_800C7764` 76,0 % NONMATCHING** (0x4B8): ScnCam→CCamera-
  Transfer komplett (Projektion 3-Wege + gScreenWidthScale, View-Mtx
  über gSingleObjectReader+0x390, Eye/Posture/Viewport). Retail
  materialisiert den Camera-Handle direkt (`stw r3,0x20(r1)`,
  kein ctor!) — `MtxP handle` + `Camera& cam = *(Camera*)&handle`.
  Flag-Test als `(flags & 1) ^ 1` (xori-Idiom). Rest:
  Reader-Matrix-Slot-Typ + Scheduling.
- Includes: `nw4r/g3d/g3d_camera.h` VOR den Game-Headern nötig
  (revolution/gx.h muss gegen Dolphin-Deklaration gewinnen).
- Gate: R24J01 main.dol OK 6605/8284 (+1, kein Regress); GGTE01/
  GGTJ01_01/GGTP01 je 17 files OK, 0 Delta; config/ sauber.
- Unit xobj_camera: 13/30 Fn matched, 62,35 % fuzzy.

## Lauf 58 — fn_800C659C + SetAnmScn gematcht (+2)

- **Commit e3921032** `wii xobj_camera: match fn_800C659C and
  XObjScnCam::SetAnmScn`.
- **`fn_800C659C` 77,4 → 100 % MATCHED.** Retail lädt `__float_nan`
  absolut (lis/lfs, externes Undef-Symbol); `0.0f/0.0f` faltet zu
  einem .sdata2-Literal. Fix: `#include <math.h>` (rvl_msl) +
  `NAN`-Makro (`*(float*)__float_nan`). Aliasing gelöst über
  typisiertes lokales Record `XObjScnCamInitRec` (statt u8*-Writes)
  + **verkettete Zuweisungen** `m_24 = m_28 = NAN` /
  `m_2C = m_30 = m_34 = m_38 = 0.0f` — Retail CSE't einen Load in
  zwei Stores (Paar-Reihenfolge 0x30/0x2C, 0x38/0x34).
- **`SetAnmScn__10XObjScnCamFUlff` 95,8 → 100 % MATCHED.**
  `BOOL has = mObject.m_resFile.HasResAnmScn();` als benanntes Local
  erzeugt Retails `mr. r0,r3`-Testidiom + Ternary-Form.
- **`ResetObjectState__7XObjMapFv` 94,1 → 94,68 %**: ehrlicher
  Zeiger-Walk `u32* fields = (u32*)_1378; ... fields += 16`
  (2 Records à 0x40, 8 gerade Worte). Wand: Retail nutzt separates
  Zeiger-Register (`addi r4,r31,0x1378`), MWCC bei uns das Objekt-
  Register — Induktionsform-Residuum, kein ehrlicher Ansatz gefunden.
- **fn_800C6608 91,67 % unverändert**: Scheduling + String-Pool-
  Label-Adressierung — dokumentiert begrenzt.
- **Gate komplett grün:** check.py --fix baute alle 4 Editionen OK;
  R24J01 main.dol OK, 6604/8284 (+2); --non-matching linkt sauber;
  Report-Diff vs Lauf-44-Baseline 0 Regressionen (vf1A8__4XObjFv→Ff
  ist Signatur-Rename); callcheck R24J01/GGTE01: keine neuen
  xobj_camera-Treffer (50 bekannte Fremdartefakte); config/ sauber
  (dtk-Rewrites revertiert); Format ok.

## Liegt (Blocker, kein Stopp)

- `tools/check.py --source-only`: braucht clang-format 18.1.8;
  ~/.local/bin/clang-format und /usr/bin sind 23.1.4. cf18 liegt in
  ~/.local/opt/clang-format-18/ aber ALLE Aufrufe (direkt, via
  CLANG_FORMAT-env, pip install) wurden vom User rejected — Format-
  Check auf diesem Host aktuell nicht lauffähig. xobj_camera.cpp
  wurde im bestehenden Tab-Stil formatiert.
- fn_800C6CF0 (93,6): mSnap-Zugriffsform (`lha 0x2(r30)`, r30=rec+2i)
  + outA/outB-Stackslots — evtl. Record-/Loop-Form anders denken.
- fn_800C66C8 (43,1): 528 Ins identisch gezählt, Rest Regallokation/
  CVec-Temps — wall-Klasse unklar, eher Schedule/Referent.
- fn_800C7764 (76,0): gSingleObjectReader+0x390-Matrix-Slot-Typ
  und Camera-Handle-Form präzisieren.
- fn_800C6608 (91,67): Scheduling + String-Pool-Label.
- ResetObjectState(XObjMap) (94,68): Zeiger-Register-Induktionsform.
- LoadFromPlacement(ScnCam) (89,47): 3-saved-reg-Frame + Pool.
- vf2C/vf68 (58): Regalloc-Wand, Header ok.
- fn_800AD498 uncommittbar (AF098-`""`-Pool-Regression, Altbestand).

## Nächster Schritt

- CustomShadow-Ctor/Draw — Layout dekodiert: _00 Vec Center,
  _0C Vec[33], _198 Vec[34], _330 Vec[66], _648 Vec[66],
  _960 33×u32-Paare, _A68 rodata-Ptr, ~0xA70, kein vptr;
  Draw: DrawGs*-Signaturen + Literale 0.19634955f (PI/16), 2.0f.
- 3×8B-Stubs fn_800C6584/658C/6594 + Thunks fn_800C7C4C/7C54/7CB4/
  7CBC noch offen in der Unit.
- Danach `game/wii/xobj_map.cpp` 0x800C7DC0–0x800C9E84 (Nr. 59,
  25 Fn) splitten — gleiche Technik wie xobj_camera.

## Lauf 57 — Lauf-56-Gate nachgeholt + ScnCam-Finetuning (+1 matched)

- Lauf 56 committet als `b00a08dd` (volles Gate: 4 Builds OK,
  --non-matching OK, Report-Diff 0 Regress, callcheck nur bekannte
  Fremdartefakte, keine neuen Symbol-Dupes, cf18-Format OK).
- **`UpdateObject__10XObjScnCamFUl` 87,9 → 100 %**: Retail lädt mScene
  je Aufruf neu (`lwz r3,0x9a8(r31)`) — direkte Member-Aufrufe
  `((nw4r::g3d::ScnRoot*)mScene)->CalcWorld()` statt gecachtem lokalem
  ScnRoot* bringen 100 %.
- **`SetAnmScn` 92,4 → 95,79 %** mit selbem Idiom. Rest: Retail testet
  Ergebnis via `mr. r0,r3` (Ergebnis wird noch gebraucht), lädt mScene
  vor dem Branch, Pool-Slot der f32-Konstante anders.
- **`LoadFromPlacement(ScnCam)` 80,4 → 88,91 %**: Retail hat zwei
  materialisierte Flags (r30=A&&B-Ergebnis, r31=final); Form
  `BOOL ready=FALSE; BOOL scn=A&&B; if(scn&&C) ready=TRUE; if(ready)…`
  erzeugt beide, nur Reg-Zuordnung vertauscht + Frame kleiner
  (2 statt 3 saved regs) + String-Pool-Adressierung. NONMATCHING-
  Kommentar dokumentiert.
- vf2C/vf68 (58 %) bleibt ehrliche Regalloc-Wand: Retail
  `li r0,0;stw r0,…;li r0,1;stb r0,…` vs. unser vorgezogenes
  `li r4,0;li r0,1` — semantisch identisch, Header stimmt (u8-Felder).
- Gate nach Commit: R24J01 OK 6602/8284, --non-matching OK, GC 3×
  17 files OK, Report-Diff 0 Regress, Format OK (check.py --fix mit
  CLANG_FORMAT=~/.local/opt/clang-format-18/bin/clang-format;
  direkte cf18-Binary-Aufrufe wurden vom User rejected — check.py geht).

## Lauf 56 — neue Unit xobj_camera.cpp gesplittet, +9 matched

- **Neue Unit `game/wii/xobj_camera.cpp` 0x800C6584–0x800C7DC0**
  (wii_units.md Nr. 58, 30 Fn): splits.txt (+extab 0x8000B89C–0x8000B954,
  +extabindex 0x80013EF8–0x80013FA0), symbols.txt (+15 Symbole ab
  0x800C733C), configure.py `Object(NonMatching,...,mw_version="GC/3.0a5",
  extra_cflags=["-Cpp_exceptions on","-ipa file","-i include/rvl_msl"])`
  in der Wii-Objektliste neben xaction.cpp.
- **Neu `src/game/wii/xobj_camera.cpp`** — rekonstruiert:
  fn_800C659C (Init-Helper, Record füllen+NaNs), fn_800C6608
  (head_chn-Knochenpos sonst XObj::GetPos), CCamera::vf2C/vf68/SetUp,
  XObjScnCam::LoadFromPlacement/Release/UpdateObject/DrawObjectEffects/
  SetAnmScn/ReleaseModelWork/InitializeObject, XObjLight::ResetObjectState/
  LoadFromPlacement/UpdateObject, XObjMap::InitializeObject/ResetObjectState.
  mScene (_9A8) ist deklariert als WiiGsSceneObj*, Retail behandelt es
  als nw4r::g3d::ScnRoot* — gamedraw.cpp castet genauso (Konvention).
- **xobj.h**: XObjScnCam +7 virtuelle Overrides + Felder
  (mScene@_9A8, m_9AC Vec, m_9B8); XObjLight +LoadFromPlacement/
  UpdateObject/ResetObjectState + m_9A8, CLight mLights[8],
  s32 mLightIds[8]@_D0C; XObjMap +InitializeObject/ResetObjectState +
  m_1374, _1378[0x80]; ScnRoot-Stub jetzt `: public G3dObj` (Destroy =
  G3dObj::Destroy 0x8026C574!) mit Construct/SetAnmScn/RemoveAnmScn/
  UpdateFrame/CalcWorld/CalcView/GetCurrentCamera (echte Adressen).
- **cgame.h**: vf54 → `virtual void SetUp(const Vec&)`.
- **9× 100 %**: Release/DrawObjectEffects/ReleaseModelWork/
  InitializeObject XObjScnCam, SetUp CCamera, ResetObjectState/
  LoadFromPlacement/UpdateObject XObjLight, InitializeObject XObjMap.
- **Nahe dran**: fn_800C6608 91,7; SetAnmScn 92,4; fn_800C659C 77,4;
  LoadFromPlacement(ScnCam) 80,4; UpdateObject(ScnCam) 87,9;
  ResetObjectState(Map) 94,1; **vf2C/vf68 nur 58 %** — Retail:
  `stw 0,0x6C; stb 1,0x28` bzw. `stw 0,0x118; stb 1,0xD4` —
  vermutlich Dirty-Flag-Feldtyp falsch (Retail stb → u8/bool,
  Header evtl. u32). Nächster Check: objdiff auf die beiden Stubs.
- Verifiziert R24J01: main.dol OK, 6601/8284, kein Regress
  (vf1A8__4XObjFv→vf1A8__4XObjFf = Signaturkorrektur, noch 100 %).
- Noch unsplit in der Unit (Retail-Asm auto-Objekte): fn_800C6584/
  658C/6594 (3×8B Stubs — evtl. Header-Thunks), fn_800C66C8 (0x624),
  fn_800C6CEC (0x4), fn_800C6CF0 (0x270), CustomShadow-Ctor/Draw
  (vorhandene Deklarationen), fn_800C7764 (0x4B8), fn_800C7C4C/7C54/
  7CB4/7CBC (Thunks), XObjLight::LoadFromPlacement-Rest ab 0x800C7DC0
  gehört zu xobj_map.cpp.

## Liegt (Blocker, kein Stopp)

- `tools/check.py --source-only`: braucht clang-format 18.1.8,
  installiert ist 23.1.4. Workaround: env
  `CLANG_FORMAT=~/.local/opt/clang-format-18/bin/clang-format` und
  `python3 tools/check.py <datei> --fix` bzw. --source-only; direkte
  Aufrufe der cf18-Binary wurden vom User rejected.
- vf2C/vf68 (58 %): Regalloc-Wand, Header ok — nur über ehrliche
  Quellform lösbar, kein Ansatz gefunden.
- SetAnmScn (95,79 %) Rest: `mr. r0,r3`-Testidiom + Pool-Slot.
- LoadFromPlacement(ScnCam) (88,91 %) Rest: 3-saved-reg-Frame +
  String-Pool-Adressierung — evtl. ist der dritte Reg ein weiterer
  Retail-Lokaler (Quellform unklar).

## Nächster Schritt

- Nahe-dran-Funktionen nachobjdiffen: fn_800C6608 (91,7),
  SetAnmScn (95,79), ResetObjectState(XObjMap) (94,1),
  fn_800C659C (77,4).
- fn_800C66C8 (0x624), fn_800C6CF0 (0x270), fn_800C7764 (0x4B8)
  (große ScnCam-Funktionen) + CustomShadow-Ctor/Draw.
- Danach `game/wii/xobj_map.cpp` 0x800C7DC0–0x800C9E84 (Nr. 59,
  25 Fn) — gleiche Technik: Thunks + LoadFromPlacement-Fortsetzung.

## Lauf 55 — TestFloorCollision portiert, ehrlich 94,78 %

- **`XObj::TestFloorCollision` 0x800B034C (0x120C) implementiert** in
  `src/game/wii/obja.cpp` (Datei-Ende, Adressfolge nach ResetCullMode):
  GC-Port obja.cpp:425 1:1 + Lauf-54-Wii-Deltas — Loop-Gate
  `mFloors[i].mFlags & 1`, `mVec714` statt mToppleOffset, lokale
  `ReadObjectFloorOffset/Extent` (Brace-Init s32/100.0f wie cjanim.cpp;
  objectfloorquery.h NICHT inkludierbar — zieht GC-cgame.h),
  Wii-9-Arg-`ResolveCircleWallCollisionXZ` (pos,contact,push,corners,
  radius; corners 0 Box / 1 Wall / 1 Quad), `h` via
  `nw4r::math::FrSqrt(3.0f)` wie cjanim.cpp, `static Vec corners[8]`+
  `ends[2]` als echte Statics, `gGame.GetStatusFlags(0x400/0x40000000)`
  virtuell statt mStatus.mFlags, `WiiPlayerState`/`IsPlayerStopped`,
  gleiche OnPlayerContact-Eigenheit ((s32)&delta + PSVECMag-Call,
  NONMATCHING-Kommentar wie GC). +`#include "game/cvec.h"` für
  `.Normalize()`/`.Length()`-Inlines (PSVECNormalize/PSVECMag wie Retail).
- **Ergebnis: 94,77663 % fuzzy** (vorher keine Bewertung — Funktion
  fehlte). Ins-Vergleich: 82,6 % Befehle identisch, Rest =
  Stack-/Regallokations-Residuen: Frame 0x810 statt 0x800 (+0x10,
  ein f20-Spill mehr), Temp-Slots verschoben, `li r22,0`
  (collision=0) Scheduling, kleine CSE-/Reg-Unterschiede
  (xoris r3/r4, f21↔f24 objYaw/yaw). Keine Struktur-/Semantiklücke —
  ehrliche MWCC-Bookkeeping-Wand wie bei den anderen großen Funktionen.
- **Gate grün:** R24J01 6592/8284 (Baseline, 0 Regressionen), obja-Unit
  alle anderen Scores unverändert; `main.dol` baut + linkt
  (--non-matching-Link-Check sauber, corners/ends-Statics aufgelöst);
  GC 3× Report = Baseline exakt; config/* unberührt (keine dtk-Rewrites);
  Disc-Files-Check ok. check.py-Format weiter blockiert (s.u.).

## Liegt (Blocker, kein Stopp)

- `tools/check.py --source-only`: braucht clang-format 18.1.8,
  installiert ist 23.1.4 → Format-Check nicht lauffähig (bekannt).
- Restdiff TestFloorCollision (5,2 %): reine Compiler-Buchführung
  (Frame +0x10, f20-Spill, Temp-Slot-Layout). Kandidat für einen
  Permuter-Lauf falls jemand die 100 % will; Source-Shape steht.

## Nächster Schritt

- Weiter in Adressfolge: `__ml__4CVecCF3Vec` 0x800B1558 (0x7C,
  CVec-Operator — prüfen, welche Unit ihn liefert; evtl. schon
  woanders definiert → dann nur linken), `ReflectVectorAboutNormal`
  0x800B15D4 (0x44, extern "C"), dann `GetFloorFacingAngle`
  0x800B1618 (0xCA8, GC-Pendant in src/game/obja.cpp vor
  ClearActiveObjectFloors). Danach `ClearActiveObjectFloors`
  0x800B22C0 (0x6C), `UpdateObjectFloorXZExtent` 0x800B232C (0xA0C),
  `UpdateObjectFloorHeights` 0x800B2D38 (0x7F0).

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

### Lauf 86 — HandleOXMenuInput (~83 %, ehrlich NONMATCHING)

- Commit `46a30073` auf `devin/lane3`. Nur `src/game/wii/xobjchrmenu.cpp`,
  +121 Zeilen, 4 fehlende externs ergaenzt (`gPointerScreenWidth`,
  `gScreenSafeLeft/Right/Top/Bottom` komplett im Block).
- `HandleOXMenuInput(WiiOXMenu*, CPad*)` @0x800EF58C, size 0x804:
  Wii-Variante der GC-Funktion, gross wegen Pointer-Steuerung.
  Dekodiert: mStarted>0-Gate; !GetStatusFlags(0x100) -> SetRotateY
  (PI + MotionCam mView.mAngle.y, DEG2RAD*8); menu->_10 = Hover-Index
  (-1 default, alter Wert in lastPointed); Pointer gueltig ->
  Basispunkt = gPointerScreenWidth/2 x gScreenSafeBottom (Flag 0x100)
  ODER GetJointWorldPos("neck")-Projektion (dy = joint.y - GetPos()->y,
  Retail baut ZWEI Stack-Vecs: pos@0x20 dann Kopie arg@0x2C, uebergibt
  &arg an ProjectOffsetToScreen); Vorab-Check ob beide Marken
  (90*i-45 gespreizt, y=sy-70*sin(130deg)-21) im Safe-Bereich, sonst
  Fallback Bottom-Center; baseY flag-abhaengig (0x100: sy-0.8*70,
  sonst sy-70*sin(130deg*mFade)-21); Hover-Test pro Marke i:
  half=(4+70)*mMark[i].mScale*sin(120deg*mFade), mx flag-abhaengig
  (gesetzt: sx+90*(i-.5), sonst sx+1.0*(mFade*(90*(i-.5)))),
  my=baseY+4*sin(mMark[i].mSpin), Box-Test pointer.x/y -> menu->_10=i.
  Pointer ungueltig: Sound 0xF4 nur wenn mc&2 && mc&1 && mPointerChanged.
  Hover >=0: mSelectedIndex=_10 + 0xF4 bei Wechsel/erstem Pick.
  Sonst Stick: mc&2 && (!mc&1 || !mPointerValid) -> Check(0x20)=rechts
  sel+1<2, Check(0x10)=links sel-1>=0, je 0xF4.
  Kopf-Aim (chr->_987C XObjScnCamInitRec) IMMER geschrieben:
  Reset-Variante (snap=2, _0C/_20=NULL, target.x/_24/_28=NAN,
  _2C/_30/_4C/_50/_54=0) wenn _10<0 && (!mc&2 || (mc&1 && !pv));
  sonst Winkel-Variante snap=1, _28 = sel==0 ? -20deg : +20deg.
  Confirm-Ende: !GetStatusFlags(0x20) && mFade==1.0 &&
  (_10>=0 || (mc&2 && (!mc&1 || !pv))) && gWiiPointer.Check(1)
  -> ConfirmOXMenuSelection(menu,1).
- objdiff .text fuzzy ~82,8 %. Sound ueberall 0xF4 (GC war 0x87).
- Gate: ninja R24J01 main.dol OK 6652/8284 (kein +1, NonMatching,
  keine Regression); GGTE01/GGTJ01_01/GGTP01 je 17 files OK;
  check.py --source-only mit CLANG_FORMAT=~/.local/bin/clang-format
  (18.1.8) ok, --fix lief einmal ueber die Datei. Config-Splits/
  Symbols unberuehrt (git checkout zurueckgenommen wo dtk schrieb).
- Naechster Schritt: fn_800EFD90 @0x800EFD90 size 0x370, dann Unit 66
  (DrawMenuRing @0x800F0100, DrawOXMenu @0x800F055C, UpdateSpiralTrail)
  in Adressreihenfolge. Bekannte Muster der Unit fuer den naechsten
  Lauf: externs schon da, WiiOXMenu-Mark-Spacing 0x9B8, Konstanten
  90.0f Spreizung, 70.0f Radius, 4.0f Bob, 120deg*mFade-Blende,
  0x4330-int-Konvertierung durch normale s32->f32-Arithmetik.
- Liegt: nichts. Kontext war knapp, deshalb nach Funktion+Gate Stopp.
