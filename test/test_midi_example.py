"""Check actual MIDI wire messages, congestion and panel disconnect/reconnect."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
EXAMPLE = ROOT / "lib/CDJ800SPI/examples/MidiController"


class MidiExampleTests(unittest.TestCase):
    def compile_and_run(self, configured=False):
        cxx = shutil.which("clang++")
        self.assertTrue(cxx, "A C++ compiler is required")
        env, sdk_flags = os.environ.copy(), []
        if Path("/Applications/Xcode.app/Contents/Developer").exists():
            env["DEVELOPER_DIR"] = "/Applications/Xcode.app/Contents/Developer"
            env["SDKROOT"] = subprocess.check_output(
                ["xcrun", "--sdk", "macosx", "--show-sdk-path"], env=env, text=True).strip()
            cxx = subprocess.check_output(["xcrun", "--find", "clang++"], env=env, text=True).strip()
            sdk_flags = ["-isysroot", env["SDKROOT"]]
        with tempfile.TemporaryDirectory(prefix="cdj800spi-midi-") as directory:
            work = Path(directory)
            for source in EXAMPLE.iterdir():
                if source.suffix in (".h", ".ino"):
                    shutil.copy(source, work)
            if configured:
                config = work / "MidiConfig.h"
                value = config.read_text()
                for before, after in (
                    ("CHANNEL = 1;", "CHANNEL = 16;"),
                    ("FIRST_BUTTON_NOTE = 36;", "FIRST_BUTTON_NOTE = 70;"),
                    ("PITCH_RAW_MIN = 0;", "PITCH_RAW_MIN = 1000;"),
                    ("PITCH_RAW_MAX = 65535;", "PITCH_RAW_MAX = 5000;"),
                    ("INVERT_PITCH = false;", "INVERT_PITCH = true;"),
                    ("INVERT_JOG = false;", "INVERT_JOG = true;"),
                    ("JOG_PULSES_PER_STEP = 1;", "JOG_PULSES_PER_STEP = 4;"),
                    ("SEND_TOUCH = false;", "SEND_TOUCH = true;"),
                    ("SEND_JOG_PRESSURE = false;", "SEND_JOG_PRESSURE = true;"),
                ):
                    self.assertIn(before, value)
                    value = value.replace(before, after)
                config.write_text(value)
            (work / "Arduino.h").write_text("""
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cassert>
#include <vector>
class HardwareSerial {
public:
    unsigned long baud = 0;
    int room = 0;
    std::vector<uint8_t> sent;
    void begin(unsigned long value) { baud = value; }
    int availableForWrite() const { return room; }
    size_t write(const uint8_t *data, size_t count) {
        assert(count <= static_cast<size_t>(room));
        sent.insert(sent.end(), data, data + count);
        return count;
    }
};
extern HardwareSerial Serial;
unsigned long millis();
""")
            executable = str(work / "midi-tests")
            sources = sorted((ROOT / "lib/CDJ800SPI/src").glob("*.cpp"))
            flags = ["-DCDJ800_MIDI_BAUD=115200"] if configured else []
            subprocess.run([cxx, *sdk_flags, "-std=c++11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                            "-fsanitize=address,undefined", "-fno-omit-frame-pointer", *flags,
                            "-I" + str(work), "-I" + str(ROOT / "lib/CDJ800SPI/src"),
                            *map(str, sources), str(ROOT / "test/midi_example_test.cpp"),
                            "-o", executable], check=True, env=env)
            subprocess.run([executable], check=True, env=env)

    def test_default_din_controller(self):
        self.compile_and_run()

    def test_calibrated_inverted_controller_with_optional_inputs(self):
        self.compile_and_run(configured=True)


if __name__ == "__main__":
    unittest.main()
