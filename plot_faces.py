import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon

FACE_CONFIG = {
    0: {"name": "NONE (Error/Bug)", "color": "#ff00ff", "alpha": 0.4},  # Magenta
    1: {"name": "ROOM",             "color": "#fff2cc", "alpha": 0.6},  # Giallo/Beige
    2: {"name": "WINDOW",           "color": "#9fc5e8", "alpha": 0.8},  # Azzurro
    3: {"name": "DOOR",             "color": "#b6d7a8", "alpha": 0.8},  # Verde
    4: {"name": "WALL",             "color": "#7f7f7f", "alpha": 0.9}   # Grigio scuro
}

def load_faces(filename):
    if not os.path.exists(filename):
        print(f"Error: file not found: '{filename}'")
        return []

    df = pd.read_csv(filename, header=None, names=range(300))
    
    faces = []
    for _, row in df.iterrows():
        valid_data = row.dropna().values
        
        if len(valid_data) < 7: 
            continue 
            
        face_type = int(valid_data[0])
        coords = valid_data[1:]
        
        vertices = [(coords[i], coords[i+1]) for i in range(0, len(coords), 2)]
        faces.append((face_type, vertices))
        
    return faces

faces_list = load_faces("faces.csv")

fig, ax = plt.subplots(figsize=(12, 10))
seen_labels = set()

for face_type, vertices in faces_list:
    config = FACE_CONFIG.get(face_type, {"name": f"UNKNOWN({face_type})", "color": "red", "alpha": 0.5})
    
    label = config["name"] if config["name"] not in seen_labels else None
    if label: 
        seen_labels.add(config["name"])
        
    poly_patch = Polygon(
        vertices,
        closed=True,
        facecolor=config["color"],
        edgecolor="#333333",
        linewidth=1.0,
        alpha=config["alpha"],
        label=label
    )
    ax.add_patch(poly_patch)

if faces_list:
    all_x = [v[0] for _, verts in faces_list for v in verts]
    all_y = [v[1] for _, verts in faces_list for v in verts]
    ax.set_xlim(min(all_x) - 1.0, max(all_x) + 1.0)
    ax.set_ylim(min(all_y) - 1.0, max(all_y) + 1.0)

ax.set_aspect("equal")
ax.set_title("Debug: Facce Estratte e Classificate (CGAL Arrangement + Pandas)")
ax.set_xlabel("X")
ax.set_ylabel("Y")
ax.grid(True, linestyle=":", alpha=0.5)
ax.legend(loc="upper right")

#plt.show()
plt.savefig("faces.png", dpi=500, bbox_inches='tight')
print("Immagine salvata come 'faces.png'")