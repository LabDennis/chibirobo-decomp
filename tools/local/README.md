# Local decomp tooling

Helper scripts for the matching work. They operate on the build tree of the
configured version (`CHIBI_VERSION`, default `GGTJ01_01`) and are plain local
tools — nothing here is part of the dtk build.

- `worklist.py` — prioritized list of incomplete units from `report.json`,
  smallest remaining diff first. Units flagged `DATA` have all functions
  matched and only need pool/data work.
- `diffunit.py <unit>` — mismatched functions of one unit with match percent
  and section table differences.
- `diffshow.py <unit> [function]` — side-by-side instruction diff (target
  left, build right); without a function name it shows every mismatched
  function of the unit.
- `flipcheck.py <source.cpp>` — flips the `Object(NonMatching, ...)` line for
  the file to `Matching`, reconfigures, builds, and reports whether the dol
  still matches. Reverts automatically on failure. Apply any needed
  `splits.txt` data-section attribution before running it.

All scripts run from anywhere with the repo's own Python: `python tools/local/worklist.py`.
