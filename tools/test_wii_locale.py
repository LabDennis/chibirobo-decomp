import json
import tempfile
import unittest
from pathlib import Path
from xml.etree import ElementTree

from tools.wii_locale import disc_image, write_header, write_localization, write_mod


class WiiLocaleTests(unittest.TestCase):
    source = Path(__file__).resolve().parents[1] / "mods/wii-localization/interface.json"

    def test_locales_emit_matching_descriptor_and_encoded_banner(self):
        translations = json.loads(self.source.read_text(encoding="utf-8"))
        with tempfile.TemporaryDirectory() as tmp:
            for locale, strings in translations.items():
                with self.subTest(locale=locale):
                    output = Path(tmp) / locale
                    base = Path(tmp) / "original.rvz"
                    write_localization(self.source, locale, output)
                    write_mod(output, base, f"{locale} localization")
                    descriptor = json.loads((output / "game.json").read_text())
                    self.assertEqual((output / descriptor["base-file"]).resolve(), base.resolve())
                    patch = descriptor["riivolution"]["patches"][0]
                    xml = ElementTree.parse(output / patch["xml"]).getroot()
                    option = xml.find("options/section/option")
                    self.assertEqual(option.get("id"), patch["options"][0]["option-id"])
                    self.assertEqual(option.find("choice").get("name"), f"{locale} localization")
                    self.assertEqual(xml.find("patch/file").get("disc"), "main.dol")
                    header = (output / "include/wii_locale.h").read_text()
                    for line in (1, 2):
                        text = strings[line].encode("utf-16-be").hex()
                        units = [f"0x{text[i:i+4]}" for i in range(0, len(text), 4)]
                        self.assertIn(", ".join(units + ["0"]), header)
                    self.assertIn(strings[3], header)

    def test_wii_slots_and_five_translations(self):
        mapping = json.loads(self.source.with_name("text_map.json").read_text())
        records = json.loads(self.source.with_name("wii_text.json").read_text())
        for record in records:
            self.assertEqual(set(record["translations"]), {"en", "de", "fr", "es", "it"})
            for index in record["indices"]:
                self.assertIsNone(mapping["interface"][index])
        with tempfile.TemporaryDirectory() as tmp:
            write_localization(self.source, "de-DE", Path(tmp))
            header = (Path(tmp) / "include/wii_text.h").read_text()
            for name, length in (("Interface", 301), ("PC", 24)):
                values = header.split(f"s{name}Map[] = {{ ")[1].split(" };")[0]
                self.assertEqual(len(values.split(", ")), length)

    def test_gamecube_pad_mod_without_localization(self):
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "wii-gcpad"
            base = Path(tmp) / "original.rvz"
            write_mod(output, base, "GameCube controller")
            descriptor = json.loads((output / "game.json").read_text())
            self.assertEqual(descriptor["display-name"], "Chibi-Robo! (Wii, GameCube controller)")
            self.assertEqual((output / descriptor["base-file"]).resolve(), base.resolve())
            self.assertFalse((output / "include/wii_locale.h").exists())

    def test_disc_only_or_fully_extracted(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp)
            (folder / "sys").mkdir()
            dol = folder / "sys/main.dol"
            dol.touch()
            with self.assertRaises(ValueError):
                disc_image(folder)
            (folder / "sys/boot.bin").touch()
            (folder / "files").mkdir()
            self.assertEqual(disc_image(folder), dol)
            image = folder / "Chibi-Robo (Japan).RVZ"
            image.touch()
            self.assertEqual(disc_image(folder), image)

    def test_rejects_overlong_banner_and_invalid_game_text(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "locale.json"
            for index, value in ((1, "x" * 32), (2, "😀" * 16), (3, "a\0b"), (4, "é")):
                with self.subTest(index=index):
                    strings = ["test"] * 6
                    strings[index] = value
                    source.write_text(json.dumps({"test": strings}))
                    with self.assertRaises(ValueError):
                        write_header(source, "test", root / "locale.h")
