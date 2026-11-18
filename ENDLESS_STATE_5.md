# ENDLESS_STATE_5 — Übergabe Bahn 5 (devin-5, devin/lane5)

Stand: Lauf vom 18.11. (Unit 79 fertig). Letzter Commit: `31b53af2`
"wii: unit 79 xobjhbox.cpp — XObjHBox overlay/hit/contact + XObjDust state half"
Arbeitsbaum sauber. Baselines NACH dem Merge: /tmp/lane5_baseline/{R24J01,GGTE01}.json
(GGTJ01_01/GGTP01-Baselines inzwischen weg — GC-Units diesmal unverändert).

## Zahlen R24J01 (nach 31b53af2, build/R24J01/report.json)

- All: 60.44 % fuzzy, 50.21 % matched, 39.40 % linked
- Code: 6356/8281 Funktionen (+4 gg. 6352 nach Merge)
- main.dol: OK. GC: GGTE01/GGTJ01_01/GGTP01 je "17 files OK".
- Unit-Report: main/game/wii/xobjhbox = 98.29 % fuzzy, 4/7 matched.
- Config-Diffs nur Unit-79-Blöcke; kein dtk-Rewrite-Müll.

## Unit 79 — src/game/wii/xobjhbox.cpp (0x8011701C–0x801178E4) — COMMITTED

Verdrahtet: configure.py nach xobjhboxpart.cpp (GC/3.0a5, `-Cpp_exceptions on
-ipa file -i include/rvl_msl`), splits.txt extab 0x8000D84C–0x8000D87C /
extabindex 0x80014D5C–0x80014DA4 / .text 0x8011701C–0x801178E4, symbols.txt
OnPlayerContact(HBox)+LoadFromPlacement+Release(Dust). XObjDust-Deklaration
lebt in xobjflags.h (Size 0xA00, STATIC_ASSERT) — generisches WII_XOBJ_DECL
aus xobj.h entfernt. xobjcoinupdate.cpp: zwei redundante extern-"C"-Zeilen
entfernt (stagedata.h liefert sie via xobjflags.h).

Scores (objdiff, NonMatching-Unit):
- DrawScreenOverlay__8XObjHBoxFv 97.05 — NONMATCHING dokumentiert: Retail
  nullt ein zweites 12-Byte-Local (Vec[2]-Schreibweise holt die Stores,
  verschiebt aber vptr-Loads → einzelnes Vec + Kommentar ist besser).
- OnHitFromPosition__8XObjHBoxFRC3Vecf 100 ✓
- OnPlayerContact__8XObjHBoxFll 97.52 — Rest: `li r3,0`-Platzierung am Tail
  (Scheduling); `(u32)value & 3` liefert schon das retail-cmplwi.
- GetFlagIndex__8XObjDustFv 100 ✓
- ResetObjectState__8XObjDustFv 100 ✓ — Lösung: `(f32)rand()` in den
  fmod-Aufrufen (Retail fsubs = f32-Konversion, nicht f64).
- LoadFromPlacement__8XObjDustFRC9CoordInfo 96.93 — Retail inlined PSVECScale
  als psq (wie JMathInlineVEC); `#define PSVECScale JMathInlineVEC::PSVECScale`
  wie in ccameraactionscroll2.cpp brachte die psq-Streams, Rest ist
  Load-Reihenfolge/Registerwahl im asm-Block.
- Release__8XObjDustFv 100 ✓

## Fixes dieses Laufs

- extab-Ende 0x8000D874→0x8000D87C (extabindex von LoadFromPlacement zeigt
  auf Record @0x8000D874, der bis 0x8000D87C läuft) — sonst "Bad extabindex
  relocation @ 0x80014DA0".
- GX_BL_SRCCLR→GX_BL_SRCCOL; OnPlayerContact-Override in XObjHBox
  deklariert; vf1D4-Signatur (const Vec*,f32*,f32*,f32*,s32) in xobj.h;
  IsPlayerBlockingEvents extern "C"; BasicSound::Stop(int)→Stop(0).

## Architekturentscheidungen (stehen so)

- Wii-Klassen XObjSound/XObjFlags/XObjCoin/XObjItem/XObjDust liegen in
  include/game/wii/xobjflags.h, nicht xobj.h (NW4R-snd über animsfx.h).
- xobjflags.h inkludiert game/stagedata.h — Quellfiles dürfen die Prototypen
  NICHT nochmal lokal deklarieren (Overload-Kollision).
- xobj.h MTX34-Shim bleibt: xobjflags.h vor xobj.h inkludieren.
- Wii-Vcall-Muster: `vf1D4(&pos,&x,&y,NULL,0)` projiziert pos→screen.
- JMathInlineVEC (#include "JSystem/JMath/Inline.h") ist die ehrliche
  Wii-Antwort auf retails inline-psq-VEC-Code.

## Liegt (blockiert)

- **clang-format-Gate**: System hat 23.1.4, CI will 18.1.8. Projekt-Venv
  existiert unter `.claude/devin/cf18/bin/clang-format`, aber der Aufruf
  wurde vom User abgelehnt (auch ~/.local/bin) → check.py --source-only
  diesmal NICHT gelaufen. Dateien sind im üblichen Stil geschrieben;
  nächster Lauf sollte CLANG_FORMAT=…cf18… check.py --source-only nachholen.
- **callcheck.py**: existiert nicht unter tools/ — Schritt entfällt.
- **Main-Checkout-Übergabe** .claude/devin/ENDLESS_STATE_5.md: Schreibzugriff
  denied → diese Datei im Worktree ist aktuell; bei Gelegenheit kopieren.

## Nächste Schritte

1. Format-Gate nachholen (s. Liegt).
2. Unit 80: UpdateObject__8XObjDustFUl @0x801178E4 (+ Rest der Dust-Familie,
   OnHitFromPosition 0x80118360, OnItemUsed 0x8011851C — Bereich laut
   wii_units.md). GC-Vorlage: src/game/xobjhbox.cpp / xobjdust.cpp.
3. Danach Units 81–98, dann 116–122.
4. Offene Residuen Units 77/79 später verbessern (Liste in älteren Ständen,
   Unit-77-Details im letzten Handoff unter "Unit 77").

## Basen/Reports

- Baseline-Vergleichsreports (Post-Merge): /tmp/lane5_baseline/{R24J01,GGTE01}.json
- Commit-Identität: LabDennis <341377186+LabDennis@users.noreply.github.com>
