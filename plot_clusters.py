import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

df = pd.read_csv('clusters.csv', header=None, names=['x', 'y', 'cluster'])

clusters = df.groupby('cluster')

fig, ax = plt.subplots(figsize=(10, 8))

# Ottieni i cluster che non sono rumore (cluster != -1)
non_noise_clusters = [cid for cid in clusters.groups.keys() if cid != -1]
num_clusters = len(non_noise_clusters)

# Crea una mappa colore per i cluster non rumore, usando tab10 (10 colori)
# Se hai più di 10 cluster, puoi usare un'altra mappa o ciclare
cmap = plt.cm.tab10
colors = [cmap(i % 10) for i in range(num_clusters)]  # lista di colori

# Assegna il grigio al rumore
noise_color = 'gray'

# Iteriamo sui gruppi
# Per tenere traccia dell'indice del colore per i cluster non rumore
color_idx = 0
for cluster_id, group in clusters:
    if cluster_id == -1:
        ax.scatter(group['x'], group['y'], c=noise_color, s=30, alpha=0.5, label='Rumore')
    else:
        # Prendi il colore corrispondente all'indice corrente (in base all'ordine di iterazione)
        # Nota: cluster_id potrebbe non essere ordinato, ma usiamo l'indice per mantenere coerenza
        # Se vuoi usare cluster_id come indice, puoi creare un dizionario, ma è più semplice
        color = colors[color_idx % len(colors)] if num_clusters > 0 else 'blue'  # fallback
        ax.scatter(group['x'], group['y'], c=[color], s=50, label=f'Cluster {cluster_id}')
        color_idx += 1

ax.set_title('Clustering dei punti medi dei segmenti')
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.legend()
ax.grid(True, linestyle='--', alpha=0.6)

plt.tight_layout()

plt.savefig("clusters.png", dpi=300, bbox_inches='tight')
print("Image saved: clusters.png")