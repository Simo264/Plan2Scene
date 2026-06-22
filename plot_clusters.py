import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

df = pd.read_csv('clusters.csv', header=None, names=['x', 'y', 'cluster'])

clusters = df.groupby('cluster')

fig, ax = plt.subplots(figsize=(10, 8))

colors = plt.cm.tab10(np.linspace(0, 1, len(clusters) - 1))  # escludiamo -1
color_map = {-1: 'gray'}

for i, (cluster_id, group) in enumerate(clusters):
    if cluster_id == -1:
        ax.scatter(group['x'], group['y'], c='gray', s=30, alpha=0.5, label='Rumore')
    else:
        color = colors[i % len(colors)]
        ax.scatter(group['x'], group['y'], c=[color], s=50, label=f'Cluster {cluster_id}')

ax.set_title('Clustering dei punti medi dei segmenti')
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.legend()
ax.grid(True, linestyle='--', alpha=0.6)

plt.tight_layout()

plt.savefig("clusters.png", dpi=300, bbox_inches='tight')
print("Image saved: clusters.png")