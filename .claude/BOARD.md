
## devin-gc2 (devin/gc2, 0b8af642), 18.11. — memcard SetAngle(Vec&) out-of-line wie Retail
Retail definiert `SetAngle__7CCameraFRC3Vec` global genau in memcard.cpp (0x80087C88);
Header deklariert jetzt bodiless, memcard.cpp trägt die Definition (unqualifizierter
Call -> bctrl _60). 100 % in allen 3 GC-Fassungen, Gate 3xGC 17 files OK + R24J01 dol OK,
0 Regress. Befund fürs Projekt: MWCC legt Vtables in die TU des ersten nicht-inlinen
Members — CVec-Ctors in obja_chr, CCameraMemento-Ctor in evt (erklaert die weak-
Vtable/RTTI-Emission in ~100 Objekten; Umbau dokumentiert in ENDLESS_STATE_2.md).
Naechstes: xobjfree::LoadFromPlacement (90.42), ggf. CVec-Ctor-Umbau.
18.11. Devin2 (Lauf 3): xobjfree-Originaldatei committed (b28bff94) +
Merge all-releases cb115b0c -> d3f3785d. Gate 3xGC "17 files OK" + R24J01
dol OK + --non-matching linkt, 0 Funktionsregress, callcheck nur
Altbekanntes (objb_act/gamestart). HBox::UpdateObject kartiert: Retail
kopiert pos (0xc) + HSD_MtxGetRotation-Ergebnis (0x10, kein Vec) in
Temps vor SetPos/SetRotationY; Quaternion/CVec-Versuche schlechter,
zurueckgenommen. Naechstes: LoadFromPlacement (Registerfall).
