#!/usr/bin/env python3
"""Build the Arduino IDE installable library ZIP, excluding build artifacts."""
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

ROOT = Path(__file__).resolve().parents[1]
LIBRARY = ROOT / "lib/CDJ800SPI"
destination = ROOT / "output/CDJ800SPI-1.0.0.zip"
destination.parent.mkdir(exist_ok=True)
with ZipFile(destination, "w", ZIP_DEFLATED) as archive:
    for source in sorted(LIBRARY.rglob("*")):
        if source.is_file() and source.name != ".DS_Store":
            archive.write(source, Path("CDJ800SPI") / source.relative_to(LIBRARY))
print(destination)
