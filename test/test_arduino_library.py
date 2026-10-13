"""Compile the portable Arduino library; no firmware image is required."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ArduinoLibraryTests(unittest.TestCase):
    def test_protocol_display_and_events(self):
        cxx = shutil.which("clang++")
        self.assertTrue(cxx, "A C++ compiler is required")
        env = os.environ.copy()
        sdk_flags = []
        if Path("/Applications/Xcode.app/Contents/Developer").exists():
            env["DEVELOPER_DIR"] = "/Applications/Xcode.app/Contents/Developer"
            env["SDKROOT"] = subprocess.check_output(
                ["xcrun", "--sdk", "macosx", "--show-sdk-path"], env=env, text=True).strip()
            cxx = subprocess.check_output(["xcrun", "--find", "clang++"], env=env, text=True).strip()
            sdk_flags = ["-isysroot", env["SDKROOT"]]
        with tempfile.TemporaryDirectory(prefix="cdj800spi-test-") as directory:
            sources = sorted((ROOT / "lib/CDJ800SPI/src").glob("*.cpp"))
            executable = str(Path(directory) / "library-tests")
            subprocess.run([cxx, *sdk_flags, "-std=c++11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                            "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                            "-I" + str(ROOT / "lib/CDJ800SPI/src"),
                            *map(str, sources), str(ROOT / "test/arduino_library_test.cpp"),
                            "-o", executable], check=True, env=env)
            subprocess.run([executable], check=True, env=env)


if __name__ == "__main__":
    unittest.main()
