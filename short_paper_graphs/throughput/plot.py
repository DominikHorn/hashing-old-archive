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
        "aqua": "AquaHash",
        "xxh3": "XXH3",
        "radix_spline": "RadixSpline",
        #"mult_prime64": "mult", "mult_add64": "mult_add",
        "rmi": "RMI"
        }
all_palette = list(mcolors.TABLEAU_COLORS.keys())
palette = all_palette[:-1]
colors = {
        "murmur_finalizer64": palette[1],
        "aqua": palette[3],
        "xxh3": palette[4],
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
        ]

# Create plot
fig, ax = plt.subplots(figsize=(7.00697/2,2))

# Extract data
machine = set(data[data[MACHINE_KEY].notnull()][MACHINE_KEY]).pop()
processor = machine[machine.find("(")+1:machine.rfind(")")]

# Plot data
ymax=50
bar_width = 0.15
group_gap = 0.2
bar_gap = 0.005
next_pos = 0
text_pad = 2
text_off = 0.01
xticks_pos=[]
xticks_text=[]

classic_plt_dat = [(h, median(list(data[data[HASH_KEY] == h][THROUGHPUT_KEY]))) for
    h in dict.fromkeys(data[(data[HASH_KEY] == "aqua") | (data[HASH_KEY] == "murmur_finalizer64") | (data[HASH_KEY] == "xxh3")][HASH_KEY]).keys()]
for i, (hashfn, median_time) in enumerate(classic_plt_dat):
    ax.bar(next_pos, median_time, bar_width, color=color(hashfn))
    ax.text(next_pos+text_off, median_time+text_pad, str(round(median_time, 1)), ha="center",
            color=color(hashfn), fontsize=11, rotation=90)

    if i == int(len(classic_plt_dat)/2):
        xticks_pos.append(next_pos)
        xticks_text.append('n/a')

    next_pos += bar_width + bar_gap

next_pos += group_gap
for i, (hashfn, values, model_cnts) in enumerate([(h, list(data[data[HASH_KEY] == h][THROUGHPUT_KEY]),
    list(data[data[HASH_KEY] == h][MODELCOUNT_KEY])) for
    h in dict.fromkeys(data[(data[HASH_KEY] == "rmi") | (data[HASH_KEY] == "radix_spline")][HASH_KEY]).keys()]):
    def nearest_pow_10_exp(num):
        return int(round(math.log10(num)))

    for j, (label, value) in enumerate(zip([f"$10^{str(nearest_pow_10_exp(d))}$" for d in model_cnts],values)):
        ax.bar(next_pos, value, bar_width, color=color(hashfn))
        if value < ymax:
            ax.text(next_pos+text_off, value+text_pad, str(round(value, 1)), ha="center",
                    color=color(hashfn), fontsize=11, rotation=90)
        else:
            ax.text(next_pos+text_off, ymax-text_pad, str(round(value, 1)), ha="center", va="top",
                    color="white", fontsize=11, rotation=90)

        xticks_pos.append(next_pos)
        xticks_text.append(label)

        next_pos += bar_width + bar_gap
    next_pos += group_gap

# Plot style/info
plt.ylim(0,ymax)
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
