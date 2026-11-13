"""Print the R24J01 -> GGTE01 Japanese text index map for two original DOLs."""

import argparse
import hashlib
import json
import struct
from collections import defaultdict
from pathlib import Path


class Dol:
    def __init__(self, path, version):
        self.data = path.read_bytes()
        manifest = Path(__file__).resolve().parents[1] / "config" / version / "build.sha1"
        expected = next(line.split()[0] for line in manifest.read_text().splitlines()
                        if line.split()[-1] == f"build/{version}/main.dol")
        if hashlib.sha1(self.data).hexdigest() != expected:
            raise ValueError(f"{path} is not the original {version} main.dol")
        self.sections = list(zip(*(struct.unpack_from(">18I", self.data, pos)
                                   for pos in (0, 72, 144))))

    def offset(self, address):
        for offset, start, size in self.sections:
            if start <= address < start + size:
                return offset + address - start
        raise ValueError(f"unmapped DOL address {address:#x}")

    def pointers(self, address, count):
        return struct.unpack_from(f">{count}I", self.data, self.offset(address))

    def strings(self, address, count):
        result = []
        for pointer in self.pointers(address, count):
            offset = self.offset(pointer)
            result.append(self.data[offset:self.data.index(0, offset)])
        return result


def match_table(wii, gc, skip=0):
    candidates = defaultdict(list)
    for index, value in enumerate(gc):
        candidates[value].append(index)
    mapped = [None] * len(wii)
    unclear = []
    for index in range(skip, len(wii)):
        hits = candidates[wii[index]]
        if len(hits) == 1:
            mapped[index] = hits[0]
        elif hits:
            unclear.append(index)
    return mapped, unclear


# GC call sites under src/game at ae152594. For multiline card prompts, the
# matched heading, surrounding unique entries and line position identify each
# continuation/padding slot within the same five-slot record.
# (Wii start, Japanese GC start, number of consecutive slots)
INTERFACE_DECISIONS = [
    # stg.cpp:2919-24 adds the menu base. Slots 48..56 are successive
    # placeholders for mode titles (:856-870) and panel fields (:532-659).
    (49, 48, 9),
    (60, 59, 1),  # stg.cpp:1054, contextual action, relative PC label 14.
    (59, 58, 1),  # stg.cpp:1038, PC menu return action (label 13 + base 45).
    (65, 64, 2),  # stg.cpp:10585, yes/no for the scrap transaction.
    (79, 78, 1),  # stg.cpp:10585, second line confirming that transaction.
    (86, 85, 3),  # gameccameraset.cpp:260, DVD states 4/6 insert, 11 unreadable.
    (98, 97, 1),  # gxb.cpp:2619, option-panel help now describes menu controls.
    (102, 101, 2),  # gxb.cpp:2604, load-screen confirmation choices.
    (104, 103, 5),  # stg.cpp:6397, no card present.
    (109, 108, 5),  # stg.cpp:6422, unusable device, retry allowed.
    (114, 113, 5),  # stg.cpp:6307, unusable damaged card.
    (119, 118, 5),  # stg.cpp:6244, damaged data requires formatting.
    (124, 123, 5),  # stg.cpp:6252, formatting erases data.
    (129, 128, 5),  # stg.cpp:6275, formatting in progress.
    (134, 133, 5),  # stg.cpp:6205, format failure.
    (139, 138, 5),  # stg.cpp:6282, format success.
    (144, 143, 5),  # stg.cpp:6292, confirmation to continue without saving.
    (149, 148, 5),  # gxb.cpp:2466, select file for a new game.
    (154, 153, 5),  # stg.cpp:9060-62, capacity caption uses rows 153/154 (one or two by locale).
    (159, 158, 5),  # stg.cpp:9014, corrupt save slot.
    (169, 163, 5),  # stg.cpp:6412, insufficient blocks or directory entries.
    (174, 168, 5),  # stg.cpp:9222, confirm overwrite.
    (179, 173, 5),  # stg.cpp:6168, manage card contents in system menu.
    (184, 178, 1),  # stg.cpp:6237, save in progress. Warning reflows below.
    (187, 181, 2),  # stg.cpp:6232, trailing padding of the write-progress record.
    (189, 183, 5),  # stg.cpp:6199, save failure.
    (194, 188, 5),  # gxb.cpp:2470, choose saved file to load.
    (199, 193, 5),  # gxb.cpp:2485, loading selected file.
    (204, 198, 5),  # stg.cpp:6380, corrupt save data.
    (209, 203, 5),  # stg.cpp:6302, confirm creating replacement save.
    (214, 208, 5),  # stg.cpp:6450, wrong device, saving unavailable.
    (224, 218, 5),  # stg.cpp:6350, broken card, saving unavailable.
    (229, 223, 5),  # stg.cpp:6175, replace card.
    (234, 228, 5),  # stg.cpp:9229, successful save.
    (239, 233, 5),  # stg.cpp:9218, select destination file.
    (244, 238, 5),  # stg.cpp:6222, load failure.
    (249, 243, 5),  # stg.cpp:6390, exhausted card lifetime.
]


def build_map(wii, gc):
    interface_base, = wii.pointers(0x802FF2C8, 1)
    pc_base, = wii.pointers(0x802FF2E4, 1)
    # The requested 325-word window includes the adjacent 24 PC pointers.
    count = (pc_base - interface_base) // 4
    interface, unclear = match_table(wii.strings(interface_base, count),
                                     gc.strings(0x802167E8, 250), skip=6)
    interface.extend([None] * (325 - count))
    for start, target, length in INTERFACE_DECISIONS:
        for delta in range(length):
            interface[start + delta] = target + delta
    # The shortened power warning spans two GC rows.
    unclear.append(185)
    pc, pc_unclear = match_table(wii.strings(pc_base, 24), gc.strings(0x80216BD0, 24))
    # stg.cpp:1139 keeper animation, :222/:1131 rank messages by level,
    # :1152 state 9 and :1163 default ticker. All pass the index unchanged.
    for index in [2, *range(4, 15), 16, 19]:
        pc[index] = index
    # Flat list of Wii slots: PC index n occupies interface window slot 301+n.
    return {"interface": interface, "pc": pc, "unclear": sorted(
        [i for i in unclear if interface[i] is None]
        + [count + i for i in pc_unclear if pc[i] is None])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wii_dol", type=Path)
    parser.add_argument("gc_dol", type=Path)
    args = parser.parse_args()
    print(json.dumps(build_map(Dol(args.wii_dol, "R24J01"), Dol(args.gc_dol, "GGTE01")), indent=2))
