import copy
import json
import unittest
import xml.etree.ElementTree as ET
from unittest.mock import patch

import progress
import treemap
import treemap_layout


def payload():
    return {
        "default": "GGTJ01_01",
        "groups": ["game", "SDK"],
        "versions": [
            {"version": "GGTJ01_01", "generated": 1, "units": [
                {"full": "main/Japan", "group": "game", "code": 999,
                 "status": "linked", "fuzzy": 100, "funcs": []},
            ]},
            {"version": "GGTE01", "generated": 2, "units": [
                {"full": "main/empty", "group": "game", "code": 0,
                 "status": "named", "fuzzy": 0, "funcs": []},
                {"full": 'main/unsafe<&"</script><script>alert(1)</script>', "group": "game", "code": 3,
                 "status": "named", "fuzzy": 0, "funcs": [["fn", 4, 3, 0]]},
                {"full": "main/linked", "group": "SDK", "code": 3,
                 "status": "linked", "fuzzy": 100, "funcs": [["done", 8, 3, 100]]},
                {"full": "main/fuzzy", "group": "game", "code": 10,
                 "status": "fuzzy", "fuzzy": 25, "funcs": [["partial", 12, 10, 25]]},
            ]},
        ],
    }


def embedded_data(page):
    start = page.index("const DATA = ") + len("const DATA = ")
    data, _ = json.JSONDecoder().raw_decode(page[start:])
    return data


class RenderTests(unittest.TestCase):
    def test_readme_rectangles_match_embedded_usa_map(self):
        source = payload()
        page = treemap.render_page(copy.deepcopy(source))
        data = embedded_data(page)
        self.assertEqual(data["default"], "GGTJ01_01")
        self.assertEqual(data["versions"][0]["version"], "GGTJ01_01")
        usa = next(v for v in data["versions"] if v["version"] == "GGTE01")
        svg = ET.fromstring(progress.svg(source))
        namespace = {"svg": "http://www.w3.org/2000/svg"}
        rectangles = svg.findall("svg:rect", namespace)
        self.assertEqual(len(rectangles), 3)
        self.assertEqual(len(rectangles), len(usa["layouts"]["all"]))
        for name, value in data["map_style"]["svg"].items():
            self.assertEqual(svg.attrib[name], value)
        for rect, (index, *coordinates) in zip(rectangles, usa["layouts"]["all"]):
            self.assertGreater(usa["units"][index]["code"], 0)
            self.assertEqual([float(rect.attrib[key]) for key in ("x", "y", "width", "height")], coordinates)
            self.assertEqual(rect.attrib["fill"], usa["units"][index]["color"])
            self.assertEqual(rect.find("svg:title", namespace).text, usa["units"][index]["full"])
            for name, value in data["map_style"]["rect"].items():
                self.assertEqual(rect.attrib[name], value)

    def test_unsafe_names_are_escaped_and_round_trip(self):
        source = payload()
        name = source["versions"][1]["units"][1]["full"]
        page = treemap.render_page(source)
        self.assertNotIn(name, page)
        self.assertNotIn("</script><script>alert(1)</script>", page)
        self.assertEqual(embedded_data(page)["versions"][1]["units"][1]["full"], name)
        svg = progress.svg(source)
        self.assertNotIn("</script>", svg)
        self.assertIn("&lt;/script&gt;", svg)
        self.assertIn("&amp;", svg)

    def test_platforms_and_publication_url_in_page_metadata(self):
        site = "https://example.test/project/"
        page = treemap.render_page(payload(), site_url=site)
        self.assertIn(f'<link rel="canonical" href="{site}">', page)
        self.assertIn(f'<meta property="og:url" content="{site}">', page)
        schema = json.loads(page.split('<script type="application/ld+json">', 1)[1].split("</script>", 1)[0])
        self.assertEqual(schema["url"], site)
        self.assertIn("GameCube and Wii", schema["description"])
        self.assertIn("GameCube and Wii", page.split("<h1>", 1)[1].split("</h1>", 1)[0])
        self.assertIn('<link rel="canonical" href="./">', treemap.render_page(payload()))

    def test_local_stamp_ignores_generated_timestamps(self):
        before = payload()
        after = copy.deepcopy(before)
        for version in after["versions"]:
            version["generated"] += 1000
        first_page = treemap.render_page(before)
        second_page = treemap.render_page(after)
        self.assertEqual(before["stamp"], after["stamp"])
        self.assertNotEqual(first_page, second_page)
        self.assertEqual(embedded_data(first_page)["stamp"], before["stamp"])

    def test_local_stamp_tracks_layout_changes(self):
        before = payload()
        after = copy.deepcopy(before)
        treemap.render_page(before)
        original_layout = treemap_layout.layout

        def changed_layout(weights):
            rectangles = original_layout(weights)
            if rectangles:
                rectangles[0][1] += 0.001
            return rectangles

        with patch.object(treemap_layout, "layout", side_effect=changed_layout):
            treemap.render_page(after)
        self.assertNotEqual(before["stamp"], after["stamp"])

    def test_local_stamp_tracks_function_changes(self):
        before = payload()
        after = copy.deepcopy(before)
        after["versions"][1]["units"][1]["funcs"][0][3] = 50
        treemap.render_page(before)
        treemap.render_page(after)
        self.assertNotEqual(before["stamp"], after["stamp"])
        stamp = after["stamp"]
        treemap.render_page(after)
        self.assertEqual(after["stamp"], stamp)


if __name__ == "__main__":
    unittest.main()
