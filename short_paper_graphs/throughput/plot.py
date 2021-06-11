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
        "rmi": "RMI",
        #"pgm": "PGM"
        }
all_palette = list(mcolors.TABLEAU_COLORS.keys())
palette = all_palette[:-1]
colors = {
        "aqua": palette[3],
        "murmur_finalizer64": palette[1],
        "xxh3": palette[5],
        "radix_spline": palette[0],
        "rmi": "tab:cyan",
        #"pgm": palette[4]
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
fig, ax = plt.subplots(figsize=(7.00697/2,2.5))

# Extract data
machine = set(data[data[MACHINE_KEY].notnull()][MACHINE_KEY]).pop()
processor = machine[machine.find("(")+1:machine.rfind(")")]

# Plot data
bar_width = 0.2
group_gap = 0.15
bar_gap = 0.005
next_pos = 0
text_pad = 2
text_off = 0.01
xticks_pos=[]
xticks_text=[]

classic_plt_dat = [(h, median(list(data[data[HASH_KEY] == h][THROUGHPUT_KEY]))) for
    h in dict.fromkeys(data[(data[HASH_KEY] == "aqua") | (data[HASH_KEY] == "murmur_finalizer64") | (data[HASH_KEY] == "xxh3")][HASH_KEY]).keys()]
for i, (hashfn, median_time) in enumerate(classic_plt_dat):
    keys_per_second = 1 * (10**9) / median_time
    ax.bar(next_pos, round(keys_per_second), bar_width, color=color(hashfn))
    #ax.text(next_pos+text_off, median_time+text_pad, str(round(median_time, 1)), ha="center",
    #        color=color(hashfn), fontsize=11, rotation=90)

    if i == int(len(classic_plt_dat)/2):
        xticks_pos.append(next_pos)
        xticks_text.append('n/a')

    next_pos += bar_width + bar_gap

next_pos += group_gap
for i, (hashfn, median_times, model_cnts) in enumerate([(h, list(data[data[HASH_KEY] == h][THROUGHPUT_KEY]),
    list(data[data[HASH_KEY] == h][MODELCOUNT_KEY])) for
    h in dict.fromkeys(data[(data[HASH_KEY] == "rmi") | (data[HASH_KEY] == "radix_spline")][HASH_KEY]).keys()]):
    def nearest_pow_10_exp(num):
        return int(round(math.log10(num)))

    for j, (label, median_time) in enumerate(zip([f"$10^{str(nearest_pow_10_exp(d))}$" for d in model_cnts],median_times)):
        keys_per_second = 1 * (10**9) / median_time
        ax.bar(next_pos, round(keys_per_second), bar_width, color=color(hashfn))
        #    ax.text(next_pos+text_off, median_time+text_pad, str(round(median_time, 1)), ha="center",
        #            color=color(hashfn), fontsize=11, rotation=90)

        xticks_pos.append(next_pos)
        xticks_text.append(label)

        next_pos += bar_width + bar_gap
    next_pos += group_gap

# Plot style/info
plt.yticks([0, 2.5 * 10**8, 5 * 10**8], fontsize=15)
ax.yaxis.get_offset_text().set_fontsize(15)
plt.xticks(xticks_pos, xticks_text, fontsize=15, rotation=35)

plt.xlabel("Model count", fontsize=15, labelpad=0)
plt.ylabel("Keys per second", fontsize=15)
plt.margins(x=0.01,y=0.2)
plt.tight_layout()

# Legend
plt.subplots_adjust(left=0.2, bottom=0.43, top=0.9)
l = fig.legend(
    handles=[mpatches.Patch(color=color, label=hr_name(h)) for h,color in colors.items()],
    loc="lower center",
    fontsize=11,
    ncol=3,
    borderpad=0.2,
    labelspacing=0.15,
    columnspacing=0.15)
for r in l.legendHandles:
    r.set_width(10.0)
for t in l.get_texts():
    t.set_position((-20,0))

plt.savefig(f"out/median_throughput.pdf")
