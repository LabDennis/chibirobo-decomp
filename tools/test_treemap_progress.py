import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import progress
import treemap


class SnapshotTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.snapshot = self.root / "progress" / "data.json"
        self.versions = ["GGTE01", "GGTJ01_01", "GGTP01", "R24J01"]
        override = patch.object(progress, "ROOT", self.root)
        override.start()
        self.addCleanup(override.stop)
        for version in self.versions:
            folder = self.root / "build" / version
            folder.mkdir(parents=True)
            (folder / "main.dol").write_bytes(version.encode())
            config = self.root / "config" / version
            config.mkdir(parents=True)
            sha = hashlib.sha1(version.encode()).hexdigest()
            (config / "build.sha1").write_text(f"{sha}  build/{version}/main.dol")
            (folder / "ok").touch()
            unit = {"name": "main/game/count", "metadata": {"complete": True},
                    "measures": {"total_code": 16, "matched_code": 16, "total_data": 4, "matched_data": 4}}
            other = {"name": "main/auto", "metadata": {"auto_generated": True, "complete": True},
                     "measures": {"total_code": 48, "matched_code": 48}}
            (folder / "report.json").write_text(json.dumps({"units": [unit, other]}))

    def seed_legacy(self):
        old = [progress.read_release(v, "verified build") for v in self.versions[:3]]
        for entry in old:
            entry.pop("source_hash")
            entry["generated"] = 1234
        progress.write_snapshot(self.snapshot, {"schema": 1, "default": "GGTE01", "source_hash": "legacy",
                                                "groups": treemap.GROUP_ORDER, "versions": old})
        return old

    def test_merge_retains_gamecube_evidence_and_hashless_local_report(self):
        old = self.seed_legacy()
        progress.merge(["R24J01"], self.snapshot)
        merged = json.loads(self.snapshot.read_text())
        self.assertNotIn("source_hash", merged)
        self.assertEqual([r["version"] for r in merged["versions"]], self.versions)
        for original, entry in zip(old, merged["versions"]):
            self.assertEqual(entry, {**original, "source_hash": "legacy"})
        wii = merged["versions"][-1]
        self.assertIsNone(wii["source_hash"])
        self.assertEqual(sum(u["code"] for u in wii["units"] if u["complete"]), 16)
        # An auto chunk that happens to contain original instructions is never source-linked.
        self.assertEqual(sum(u["code"] for u in wii["units"]), 64)
        # Give the two platforms different linked fractions to check badge selection.
        merged["versions"][0]["units"][0]["complete"] = False
        progress.write_snapshot(self.snapshot, merged)
        with patch.object(progress, "source_hash", return_value="new-checkout"):
            progress.render(self.root / "site", self.snapshot)
        page = (self.root / "site/index.html").read_text(encoding="utf-8")
        start = page.index("const DATA = ") + len("const DATA = ")
        rendered, _ = json.JSONDecoder().raw_decode(page[start:])
        self.assertEqual([r["platform"] for r in rendered["versions"]], ["GameCube"] * 3 + ["Wii"])
        self.assertTrue(all("earlier" in r["source"] for r in rendered["versions"][:3]))
        self.assertEqual(rendered["versions"][-1]["source"], "locally verified build report")
        self.assertEqual(self.snapshot.read_bytes(), (self.root / "site/data.json").read_bytes())
        badge = (self.root / "site/badges/wii-linked.svg").read_text(encoding="utf-8")
        self.assertIn("Japanese Wii code linked from source: 25.0%", badge)

    def test_render_without_wii_still_writes_the_readme_badge(self):
        self.seed_legacy()
        with patch.object(progress, "source_hash", return_value="legacy"):
            progress.render(self.root / "site", self.snapshot)
        badge = (self.root / "site/badges/wii-linked.svg").read_text(encoding="utf-8")
        self.assertIn("Japanese Wii code linked from source: not built", badge)

    def test_invalid_or_incomplete_build_leaves_snapshot_unchanged(self):
        self.seed_legacy()
        old = self.snapshot.read_bytes()
        folder = self.root / "build/R24J01"
        (folder / "ok").unlink()
        with self.assertRaisesRegex(ValueError, "verified build"):
            progress.merge(["R24J01"], self.snapshot)
        (folder / "ok").touch()
        (folder / "main.dol").write_bytes(b"optional source build")
        with self.assertRaisesRegex(ValueError, "differs"):
            progress.merge(["R24J01"], self.snapshot)
        self.assertEqual(old, self.snapshot.read_bytes())

    def test_full_update_checks_four_releases_and_stamps_each(self):
        results = dict.fromkeys(self.versions, True)
        with patch.object(progress.check_versions, "check", return_value=results) as build:
            with patch.object(progress, "source_hash", return_value="current"):
                progress.update(self.versions, self.snapshot)
        build.assert_called_once_with(self.versions, all_source=True)
        data = json.loads(self.snapshot.read_text())
        self.assertEqual(data["source_hash"], "current")
        self.assertEqual([r["source_hash"] for r in data["versions"]], ["current"] * 4)


if __name__ == "__main__":
    unittest.main()
