import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon
from pathlib import Path

FACE_CONFIG = {
    0: {"name": "NONE (Error/Bug)", "color": "#ff00ff", "alpha": 0.4},
    1: {"name": "FLOOR",             "color": "#fff2cc", "alpha": 0.6},
    2: {"name": "WINDOW",           "color": "#9fc5e8", "alpha": 0.8},
    3: {"name": "DOOR",             "color": "#b6d7a8", "alpha": 0.8},
    4: {"name": "WALL",             "color": "#7f7f7f", "alpha": 0.9}
}

def load_faces(filename):
    df = pd.read_csv(filename, header=None, names=range(300))
    faces = []
    for _, row in df.iterrows():
        valid_data = row.dropna().values
        if len(valid_data) < 8: # 1 per tipo, 1 per id, almeno 6 per 3 vertici
            continue 
            
        face_type = int(valid_data[0])
        face_id = int(valid_data[1])     # <-- Leggiamo l'ID univoco
        coords = valid_data[2:]          # <-- Le coordinate ora partono dall'indice 2
        
        vertices = [(coords[i], coords[i+1]) for i in range(0, len(coords), 2)]
        faces.append((face_type, face_id, vertices))
        
    return faces


filename = Path("tmp/faces.csv")
if not filename.is_file():
    print(f"Error: file does not exixt: {filename}")
    sys.exit(0)

faces_list = load_faces(filename)
fig, ax = plt.subplots(figsize=(12, 10))
seen_labels = set()

for face_type, face_id, vertices in faces_list:
    config = FACE_CONFIG.get(face_type, {"name": f"UNKNOWN", "color": "red", "alpha": 0.5})
    label = config["name"] if config["name"] not in seen_labels else None
    if label: seen_labels.add(config["name"])
        
    poly_patch = Polygon(
        vertices, closed=True, facecolor=config["color"],
        edgecolor="#333333", linewidth=1.0, alpha=config["alpha"], label=label
    )
    ax.add_patch(poly_patch)

    # TRUCCO DI DEBUG: Calcoliamo il centro della faccia per stamparci l'ID sopra
    cx = sum(v[0] for v in vertices) / len(vertices)
    cy = sum(v[1] for v in vertices) / len(vertices)
    
    # Stampiamo il testo dell'ID al centro del poligono
    ax.text(cx, cy, f"ID:{face_id}", fontsize=9, color="black", 
            weight="bold", ha="center", va="center",
            bbox=dict(boxstyle="round,pad=0.2", fc="white", ec="gray", alpha=0.6))

if faces_list:
    all_x = [v[0] for _, _, verts in faces_list for v in verts]
    all_y = [v[1] for _, _, verts in faces_list for v in verts]
    ax.set_xlim(min(all_x) - 1.0, max(all_x) + 1.0)
    ax.set_ylim(min(all_y) - 1.0, max(all_y) + 1.0)

ax.set_aspect("equal")
ax.set_title("Debug Facce con ID Univoci")
ax.grid(True, linestyle=":", alpha=0.5)
ax.legend(loc="upper right")

plt.savefig("tmp/faces.png", dpi=300, bbox_inches='tight')
print("Image saved: tmp/faces.png")