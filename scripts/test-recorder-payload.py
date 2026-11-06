import unittest
from recorder_payload import UNKNOWN, NSIS, nsis_payload


class PayloadContract(unittest.TestCase):
    def test_only_the_single_documented_bundle_marker_may_differ(self):
        compiled = b"MZ\x00compiled-code\xff" + UNKNOWN + b"unchanged-resources"
        self.assertEqual(nsis_payload(compiled), compiled.replace(UNKNOWN, NSIS))
        for invalid in (b"MZ-no-marker", b"MZ" + UNKNOWN * 2,
                        b"MZ" + NSIS, b"not-a-Windows-executable" + UNKNOWN):
            with self.assertRaises(ValueError):
                nsis_payload(invalid)


if __name__ == "__main__":
    unittest.main()
