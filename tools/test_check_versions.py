import json
from pathlib import Path
from subprocess import CompletedProcess
import tempfile
import unittest
from unittest.mock import patch

from tools import check_versions


class CheckVersionsTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.objdiff = self.root / "objdiff.json"
        for module in (check_versions, check_versions.ver):
            root = patch.object(module, "ROOT", self.root)
            root.start()
            self.addCleanup(root.stop)

    def test_verification_restores_arguments_and_local_mappings(self):
        arguments = ["--non-matching", "--dtk", "local tools/dtk", "--version", "GGTP01"]
        manifest = self.root / "build.ninja"
        manifest.write_text(f"{check_versions.ver.CONFIGURE_ARGS_PREFIX}{json.dumps(arguments)}\n")
        mappings = b'{"units":[{"name":"unit","symbol_mappings":{"old":"local"}}]}'
        self.objdiff.write_bytes(mappings)

        def build(version, all_source):
            self.assertEqual(version, "GGTE01")
            self.assertTrue(all_source)
            self.assertEqual(self.objdiff.read_bytes(), mappings)
            self.objdiff.write_bytes(b'{"units":[]}')
            return True, ""

        def restore(restored_arguments):
            self.assertEqual(restored_arguments, arguments)
            self.assertEqual(self.objdiff.read_bytes(), mappings)
            return CompletedProcess([], 0)

        with patch.object(check_versions, "build", side_effect=build), patch.object(
            check_versions, "configure", side_effect=restore
        ) as configure:
            results = check_versions.check(["GGTE01"], all_source=True)
        self.assertEqual(results, {"GGTE01": True})
        configure.assert_called_once()
        self.assertEqual(self.objdiff.read_bytes(), mappings)

    def test_restore_failure_is_reported_for_a_legacy_manifest(self):
        (self.root / "build.ninja").write_text(
            "configure_args = -v GGTP01 --non-matching $\n"
            "    --no-progress\n"
        )
        with patch.object(check_versions, "build", return_value=(True, "")), patch.object(
            check_versions, "configure", return_value=CompletedProcess([], 1, "configure failed")
        ) as configure:
            with self.assertRaisesRegex(ValueError, "could not restore the previous build configuration"):
                check_versions.check(["GGTE01"])
        configure.assert_called_once_with(["-v", "GGTP01", "--non-matching", "--no-progress"])
