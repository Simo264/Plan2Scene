import os
import matplotlib.pyplot as plt

def load_points(filename):
    if not os.path.exists(filename):
        print(f"Error: file not found: '{filename}'")
        return []

    points = []
    with open(filename, "r", encoding="utf-8") as f:
        for row in f:
            row = row.strip()
            if not row: continue

            try:
                x, y = map(float, row.split(","))
                points.append((x, y))
            except ValueError:
                print(f"ignored the invalid row: {row}")

    return points

vertices = load_points("walls_vertices.txt")

fig, ax = plt.subplots(figsize=(10, 8))

if vertices:
  X = [p[0] for p in vertices]
  Y = [p[1] for p in vertices]
  ax.scatter(X, Y, color="black", s=15, label="Vertices", zorder=3)

ax.set_aspect("equal")
ax.set_title("Debug: Vertex Snapping Points")
ax.set_xlabel("X")
ax.set_ylabel("Y")
ax.grid(True, linestyle=":", alpha=0.5)
ax.legend(loc="upper left")
plt.show()