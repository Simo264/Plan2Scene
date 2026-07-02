import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

def load_points(filename):
    df = pd.read_csv(filename, header=None, dtype=float)
    points = [tuple(row) for row in df.values]
    return points

filename = Path("tmp/vertices.csv")
if not filename.is_file():
    print(f"Error: file does not exixt: {filename}")
    sys.exit(0)

vertices = load_points(filename)

fig, ax = plt.subplots(figsize=(10, 8))
if vertices:
  X = [p[0] for p in vertices]
  Y = [p[1] for p in vertices]
  ax.scatter(X, Y, color="black", s=15, label="Vertices", zorder=3)

ax.set_aspect("equal")
ax.set_title("Debug: Vertices")
ax.set_xlabel("X")
ax.set_ylabel("Y")
ax.grid(True, linestyle=":", alpha=0.5)
ax.legend(loc="upper left")

plt.savefig("tmp/points.png", dpi=300, bbox_inches='tight')
print("Image saved: tmp/points.png")