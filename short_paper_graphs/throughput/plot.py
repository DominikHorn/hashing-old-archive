import numpy as np
import math as math
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import matplotlib.patches as mpatches
import pandas as pd
from statistics import median

# Latex figure export
mpl.use("pgf")
mpl.rcParams.update({
    "pgf.texsystem": "pdflatex",
    "font.family": "serif",
    "text.usetex": True,
    "pgf.rcfonts": False
})

# Style
hr_names = {
        "murmur_finalizer64": "Murmur",
        "radix_spline": "RadixSpline",
        #"mult_prime64": "mult", "mult_add64": "mult_add",
        "rmi": "Rmi"
        }
all_palette = list(mcolors.TABLEAU_COLORS.keys())
palette = all_palette[:-1]
colors = {
        "murmur_finalizer64": palette[1],
        "radix_spline": palette[0],
        "rmi": palette[2]
        }

def hr_name(h):
    return hr_names.get(h) or h

def color(h):
    return colors.get(h) or all_palette[-1]

# Read data
MACHINE_KEY="machine"
COMPILER_KEY="compiler"
REDUCER_KEY="reducer"
HASH_KEY="hash"
THROUGHPUT_KEY="throughput"
MODELCOUNT_KEY="model_size"

FAST_MODULO="fast_modulo"
DO_NOTHING="do_nothing"
CLAMP="clamp"

data = pd.read_csv(f"throughput.csv")

# Filter data
data = data[
        # Only use g++ results
        ((data[COMPILER_KEY].isnull()) |
            (data[COMPILER_KEY].str.match(r"g\+\+")))
        # Only use fast modulo results
        & ((data[REDUCER_KEY] == FAST_MODULO) | (data[REDUCER_KEY] == CLAMP))
        # Only plot certain functions
        & (
            (data[HASH_KEY] == "murmur_finalizer64") |
            (data[HASH_KEY] == "rmi") |
            (data[HASH_KEY] == "radix_spline")
            )
        ]

# Create plot
fig, ax = plt.subplots(figsize=(7.00697/2,3))

# Extract data
machine = set(data[data[MACHINE_KEY].notnull()][MACHINE_KEY]).pop()
processor = machine[machine.find("(")+1:machine.rfind(")")]

plt_dat = [(h, list(data[data[HASH_KEY] == h][THROUGHPUT_KEY]),
    list(data[data[HASH_KEY] == h][MODELCOUNT_KEY])) for
    h in dict.fromkeys(data[HASH_KEY]).keys()]

# Plot data
bar_width = 0.15
next_pos = 0
xticks_pos=[]
xticks_text=[]
for i, group in enumerate(plt_dat):
    def nearest_pow_10_exp(num):
        return int(round(math.log10(num)))

    hashfn = group[0]
    labels = [f"$10^{str(nearest_pow_10_exp(d))}$" if not math.isnan(d) else
            "n/a" for d in group[2]]
    values = group[1]

    for j, (label, value) in enumerate(zip(labels,values)):
        ax.bar(next_pos, value, bar_width, color=color(hashfn))
        ax.text(next_pos, value+5, str(round(value, 1)), ha="center",
                color=color(hashfn), fontsize=11, rotation=90)
        xticks_pos.append(next_pos)
        xticks_text.append(label)

        next_pos += bar_width + 0.005
    next_pos += 0.2

# Plot style/info
plt.yticks(fontsize=8)
plt.xticks(xticks_pos, xticks_text, fontsize=8)

plt.xlabel("Model count", fontsize=8)
plt.ylabel("Nanoseconds per key", fontsize=8)
plt.margins(x=0.01,y=0.2)
plt.tight_layout(pad=0.1)

# Legend
#plt.subplots_adjust(bottom=0.15)
fig.legend(
    handles=[mpatches.Patch(color=color, label=hr_name(h)) for h,color in colors.items()],
    bbox_to_anchor=(0.125, 0.985),
    loc="upper left",
    fontsize=6,
    ncol=1)

plt.savefig(f"out/median_throughput.pdf")
