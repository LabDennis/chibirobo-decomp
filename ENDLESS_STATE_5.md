# ENDLESS_STATE_5 — Übergabe Bahn 5 (devin-5, devin/lane5)

Stand: Lauf vom 18.11. (Merge-Lauf). Letzter Commit: `13b44504`
"Merge branch 'all-releases' into devin/lane5" — Merge mit vollem Gate.
Arbeitsbaum sauber. Baselines NACH dem Merge: /tmp/lane5_baseline/{R24J01,GGTE01,GGTJ01_01,GGTP01}.json

## Zahlen R24J01 (nach Merge, build/R24J01/report.json)

- All: 60.36 % fuzzy, 50.19 % matched, 39.40 % linked (507/1720)
- Code: 6352/8281 Funktionen; Game Code: 1529/2242
- main.dol: OK. GC: GGTE01/GGTJ01_01/GGTP01 je "17 files OK".
- Funktionsvergleich gg. Pre-Merge-Baseline: 0 Drops auf Wii; auf GC genau
  `ProcessPendingEvents__12XObjActorToyFUl` 96.58→88.59 — kommt von all-releases
  verbatim (dokumentierte Inline-Grenze), nicht meine Auflösung.
- Symbol-Duplikate identisch mit all-releases (19/101/106/101) — 0 neue.
- check.py --source-only: OK (clang-format via `env`-Form).

## Merge-Entscheidungen (13b44504)

- cgame.h: upstream-Modell `WiiPlayerState : public XObj` übernommen.
- xobj.h: nur Wortlaut-Duplikat mFloorMasks — Kommentar behalten.
- plugcordsimulation.cpp gelöscht (all-releases hat xobjplugcord.cpp vereint);
  lane-Fixes portiert: `s32 old = cord->mMode`, `case 0` vor default,
  unknown-kind → mMode=0; Wii nutzt `GetPos()` nicht `GetPosition()`.
- xobjhboxpart.cpp: Include-Reihenfolge cgame.h vor xobj.h (echter nw4r-MTX34
  muss vor dem xobj.h-Shim stehen).
- rvl_msl-Flag wie bei Unit 77 wo nötig.

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

## Unit 79 — src/game/wii/xobjhbox.cpp (0x8011701C–0x801178E4) — ANALYSE FERTIG

Asm gelesen: build/R24J01/asm/auto_{DrawScreenOverlay__8,OnHitFromPosition__8,
fn_80117340,GetFlagIndex__8XObjD,ResetObjectState__8X,fn_8011768C,03_801178D8}_text.s
GC-Vorlage: src/game/xobjhbox.cpp (gleiche Logik, andere APIs).

1. DrawScreenOverlay__8XObjHBoxFv (0x264): Stack 0xA0, f28-31+r25-31.
   `if (mRing <= 0.0f) return;` mRing=this+0x9AC, 0.0f=lbl_805A5400.
   `vf1D4(&pos,&x,&y,0,0)` — Projektions-Vcall (Muster in ccameraactionscroll2.cpp:
   `obj->vf1D4(&ofs,&x,&y,0,0)`).
   RenderState = `gGame.mGs.mCurRenderState` (gGame+0x388, mGs bei _0008 +
   mCurRenderState _380). CGsRenderState in include/game/wii/cgs.h modelliert:
   SetZMode(GXBool u8, GXCompare u32, GXBool u8) schreibt _14/_18/_1C+GX-Call;
   SetBlendMode schreibt _00.._0C+GX-Call. Retail: SetZMode(0,7,0)=
   (false,GX_ALWAYS,false), SetBlendMode(1,2,4,15), DrawGsCircle inner,
   DrawGsCircle outer, SetBlendMode(1,4,5,15), SetZMode(1,3,0).
   inner=400.0f*mRing, outer=400.0f*(1.0f-mRing), alpha aus 255.0f*mRing,
   Farbe 0xFFFFFF00. Zwei GC-Schleifen laufen je 1× — auf Wii einfach zwei Calls.
2. OnHitFromPosition__8XObjHBoxFRC3Vecf (0xC0): `if (vf11C()/*GetAnimId*/ <= 0) {
   XObj::OnHitFromPosition(pos,f); if ((mStatus.mFlags & 0x04000000ULL) &&
   f >= (s32)GetPlayerActionParameter(31)/100.0f) mStatus.mFlags |= 0x2000...0000ULL; }`
   (oris 0x2000 auf high word @0x150).
3. fn_80117340 = OnPlayerContact__8XObjHBoxFll (0x194): `(flags&3)==1 → return flags`;
   `vf11C()==0 → return flags`; dann mStatus|=0x4000..ULL (oris 0x4000@0x150);
   mPart.mHit=1.0f (stfs @0x1358); PSMTXScale(&m, s,s,s) (s=lbl_805A54B0);
   mPart.vf0A4(&mPart, lbl_802DAEA0) → ret=Mtx; PSMTXConcat(ret,&scaleMtx,&out);
   mPart.vf1B4(&out); mPart.vf108(0,0,f1=0,f2=1,f3=0,f4=lbl_805A5408);
   PlayWiiSoundEffect(gGame+0x22A4, 0x18A, 1.0f, 0.0f); r30=vf1EC();
   `if (!IsPlayerBlockingEvents(player@gGame+0xF70C)) StartOrQueueEventScript(
   &gGame.mEvt /*gGame+0x4C8E0*/, r30, this, -1, -1)`; dann mPart.mSfx-Handle
   @0x1364: `if(h&&*h) (*h)->Stop(0)` (nw4r::snd::detail::BasicSound::Stop(f));
   `*(0x1364)=0`. return 0.
4. GetFlagIndex__8XObjDustFv (0x44): `r31=*(this+0x198); if(r31>0) return
   GetStageCollectionFlagBase(2)+r31; return 0;`
5. ResetObjectState__8XObjDustFv (0x174): XObj::ResetObjectState();
   `*(0x168)`-Objekt: `if(!o)r0=-1 elif(o) r0=lha 0x16(o) else 0` — Muster
   `kind = o ? lha(o+0x16) : -1`; `if (clrlwi(r0,31))` bit0: mField9B8=this+0x34;
   mField9BC=frsp(fmod(rand(),dbl lbl_805A54C0)); mField9C4=0,mField9C8=0;
   mField9C0=(rand()%200+200)/lbl_805A54C8; mField9CC=__float_nan; 9D0=0;9D4=1.0f.
   Dann: s16@-1 @0x9FC,0x9FE; u32 0 @0x9AC; `if (vf224() && mStatus.mFlags&0x1000)`
   → vf108(this, r4=*(0x160)+0x30, 1, 0.0f,1.0f,1.0f, lbl_805A5408) — Tail nicht
   komplett gelesen (0x50 Rest).
6. fn_8011768C (0x24C) = UpdateObject__8XObjDustFUl — NICHT gelesen, GC-Version
   in src/game/xobjhbox.cpp als Referenz.
7. fn_801178D8 (0xC) — NICHT gelesen, vermutlich trivialer Getter/Setzer.

## Liegt (blockiert)

- **Main-Checkout-Übergabe** .claude/devin/ENDLESS_STATE_5.md: Schreibzugriff
  wiederholt denied → diese Datei hier im Worktree ist die aktuelle Version;
  bei Gelegenheit dorthin kopieren.
- Unit-77-Residuen (oben) und Unit-79-Implementierung: Arbeit, kein Blocker.
- Fetch/check.py funktionieren wieder (waren früher denied).

## Nächste Schritte

1. Unit 79 implementieren: echte XObjDust-Deklaration nach xobjflags.h
   (statt WII_XOBJ_DECL-Makro in xobj.h, Wii-Size 0xA00; Retail-Felder
   _198 flagIndex-Träger, _9AC s32/Handle, _9B8.._9D4 f32, _9FC/_9FE s16).
   VF-Slots klären: vf0A4(Mtx-referenz?), vf108(SetAnim-artig:
   (obj,r4,r5,f1,f2,f3,f4)), vf1B4(SetWorldMtx-artig), vf224(BOOL),
   vf1EC(script-ptr?), vf11C=GetAnimId (in xobj.h schon benannt?).
   PlayWiiSoundEffect-Deklaration suchen (in xobjcoinupdate.cpp genutzt?).
   StartOrQueueEventScript/IsPlayerBlockingEvents/GetPlayerActionParameter/
   GetStageCollectionFlagBase/DrawGsCircle sind GC-bekannte Free-Functions.
   mPart.mSfx: Retail braucht BasicSound**-artiges Handle — Feldtyp prüfen.
   configure.py: Unit nach xobjhboxpart.cpp in Retail-Reihenfolge, Flag-
   Muster von Unit 77 (mw GC/3.0a5, -Cpp_exceptions on -ipa file, ggf.
   -i include/rvl_msl wegen xobjflags.h→animsfx.h→NW4R).
   Splits: .text 0x8011701C..0x801178E8 inkl. extab/extabindex/sdata2/lit.
2. Danach Units 80–98, dann 116–122 laut ENDLESS.md-Aufteilung.
3. Unit-77-Residuen (s.o.) später verbessern.

## Basen/Reports

- Baseline-Vergleichsreports (Post-Merge): /tmp/lane5_baseline/{R24J01,GGTE01,GGTJ01_01,GGTP01}.json
- GC-Baselines von 17.11.: /tmp/gc2run/reports/ bzw. /tmp/codex-prompt1-baseline/
- Commit-Identität: LabDennis <341377186+LabDennis@users.noreply.github.com>
