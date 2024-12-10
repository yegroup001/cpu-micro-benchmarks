import subprocess
from matplotlib import pyplot as plt
import numpy as np
import csv

# Reproduce Table 2 of Half&Half
# Test target toggles
for prefix in [""]:
    x_data = []
    y_data = []
    z_data = []

    with open(f"{prefix}phr_target_bits_location.csv", newline="") as f:
        r = csv.DictReader(f)
        for row in r:
            x_data.append(int(row["toggle"]))
            y_data.append(int(row["dummy"]))
            z_data.append(float(row["avg"]))
        z_data = np.array(z_data)

    x_data = list(sorted(set(x_data)))
    y_data = list(sorted(set(y_data)))
    z_data = z_data.reshape((len(x_data), len(y_data)))

    plt.figure(figsize=(len(y_data) / 4, len(x_data) / 4))
    plt.imshow(z_data)
    plt.xlabel("Dummy branches")
    plt.xticks(range(len(y_data)), y_data, rotation=90)
    plt.ylabel("Target toggle bit")
    plt.yticks(range(len(x_data)), x_data)
    bar = plt.colorbar(shrink=0.8)
    bar.ax.set_ylabel("Misprediction rate", fontsize=8, rotation=270, labelpad=9.0)
    if np.max(z_data) > 0.7:
        # missing conditional branch misses
        plt.clim(0.5, 1.0)
    else:
        plt.clim(0, 0.5)
    plt.grid()
    plt.savefig(f"plot_{prefix}phr_target_bits_location.png")
    plt.savefig(f"plot_{prefix}phr_target_bits_location.pdf", bbox_inches="tight")
    plt.cla()
