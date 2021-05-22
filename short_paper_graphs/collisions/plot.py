import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import pandas as pd
import math as math

# Plot expected colliding keys based on load_factor
def plot_expected_colliding_keys():
    fix, ax2 = plt.subplots()
    ax1 = ax2.twinx()

    # E[X] : X collision probability per key
    color1 = 'tab:blue'
    x = np.linspace(0, 5, 100)
    y = 1 - np.exp(-x)
    yticks = np.linspace(0,1,5)
    ax1.set_yticks(yticks)
    ax1.set_yticklabels([f"{round(100*y, 2)}%" for y in yticks])
    ax1.set_ylabel("collision chance per key (percent)")
    ax1.tick_params(axis='y', labelcolor=color1)
    ax1.set_ylim(min(yticks),max(yticks))
    ax1.plot(x, y, color=color1)

    # Hashtable size
    color2 = 'tab:orange'
    x = np.linspace(0, 5, 100)
    y = [1 / x if x > 0 else 4294967296 for x in x]
    yticks = np.linspace(0, 5, 11)
    ax2.set_yticks(yticks)
    ax2.set_yticklabels([f"{y}x" for y in yticks])
    ax2.set_ylabel("hashtable over allocation")
    ax2.tick_params(axis='y', labelcolor=color2)
    ax2.set_ylim(min(yticks),max(yticks))
    ax2.plot(x, y, color=color2)

    # Annotated special points of interest
    x = [0.25, 0.5, 0.75, 1.0, 1.25]
    y = [1 - math.exp(-x) for x in x]
    ax1.scatter(x,y, color=color1)
    for i, yi in enumerate(y):
        percent = round(yi*100,2)
        ax1.annotate(f"{percent}%", (x[i]+0.1, y[i]-0.05), color=color1)
    x = [0.25, 0.5, 0.75, 1.0, 1.25]
    y = [1 / x for x in x]
    ax2.scatter([x for x in x if x != 0.5], [y for y in y if y != 2], color=color2)
    for i, yi in enumerate(y):
        ax2.annotate(f"{round(yi, 2)}x", (x[i]+0.1, y[i]), color=color2)

    # customize x ticks
    plt.xticks(np.linspace(0, 5, 11))
    ax2.set_xlabel("load factor")

    plt.savefig(f"out/expected_colliding_keys.pdf", bbox_inches='tight')

plot_expected_colliding_keys()
