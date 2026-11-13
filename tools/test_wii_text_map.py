import unittest

from tools.wii_text_map import match_table


class TextMapTests(unittest.TestCase):
    def test_reordered_repeated_missing_and_skipped(self):
        gc = [b"red", b"blue", b"red", b""]
        wii = [b"blue", b"", b"red", b"blue", b"Blue", b"blue"]
        self.assertEqual(match_table(wii, gc, skip=1),
                         ([None, 3, None, 1, None, 1], [2]))

    def test_duplicate_empty_and_significant_trailing_space(self):
        self.assertEqual(match_table([b"", b"x ", b"x"], [b"", b"x", b""]),
                         ([None, None, 1], [0]))
