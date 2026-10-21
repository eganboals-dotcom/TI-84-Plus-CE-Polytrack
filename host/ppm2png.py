"""Convert every build/shots/*.ppm (or the dir given) to PNG next to it."""
import sys, pathlib
from PIL import Image
d = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "build/shots")
for p in sorted(d.glob("*.ppm")):
    Image.open(p).save(p.with_suffix(".png"))
    print(p.with_suffix(".png"))
