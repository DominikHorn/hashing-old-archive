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

# Helper
def base_name(hashfn):
    bracket_ind = hashfn.find("(")
    return  hashfn[0:bracket_ind if bracket_ind > 0 else len(hashfn)].strip()

def hr_name(hashfn):
    h = base_name(hashfn)
    return hr_names.get(h) or h

def color(hashfn):
    return colors.get(base_name(hashfn)) or all_palette[-1]

# Read data
DATASET_KEY="dataset"
MACHINE_KEY="machine"
COMPILER_KEY="compiler"
REDUCER_KEY="reducer"
SAMPLE_SIZE_KEY="sample_size"
HASH_KEY="hash"
THROUGHPUT_KEY="throughput"
MODELCOUNT_KEY="model_size"

FAST_MODULO="fast_modulo"
DO_NOTHING="do_nothing"
CLAMP="clamp"

csv = pd.read_csv(f"throughput.csv")
partial_data = csv[csv[DATASET_KEY].isnull()]
data = csv # csv[csv[DATASET_KEY].notnull()]

# Filter data
# Only use g++ results
data = data[(data[COMPILER_KEY].isnull()) |
        (data[COMPILER_KEY].str.match(r"g\+\+"))]
# Only use fast modulo results
data = data[(data[REDUCER_KEY] == FAST_MODULO) | (data[REDUCER_KEY] == CLAMP)]
# Only use sample size NaN or 1%
data = data[(data[SAMPLE_SIZE_KEY] == 0.01) | (data[SAMPLE_SIZE_KEY].isnull())]
# Only use certain hash functions
data = data[
        # (data[HASH_KEY] == "mult_prime64") | (data[HASH_KEY] == "mult_add64") | 
    (data[HASH_KEY] == "murmur_finalizer64") |
    (data[HASH_KEY].str.match("rmi")) |
    ((data[HASH_KEY].str.contains("radix_spline")) & (data[HASH_KEY] != "radix_spline (32:18)"))]

# Create plot
fig, ax = plt.subplots(figsize=(7.00697/2,3))

# Extract data
machine = set(data[data[DATASET_KEY].notnull()][MACHINE_KEY]).pop()
processor = machine[machine.find("(")+1:machine.rfind(")")]

plt_dat = sorted([(h, median(list(data[data[HASH_KEY] == h][THROUGHPUT_KEY])),
    set(data[data[HASH_KEY] == h][MODELCOUNT_KEY]).pop()) for
    h in set(data[HASH_KEY])], key=lambda x: x[1])

# Plot data
groups = list({base_name(d[0]):0 for d in plt_dat}.keys())
bar_width = 0.15
next_pos = 0
xticks_pos=[]
xticks_text=[]
for i, group in enumerate(groups):
    def nearest_pow_10_exp(num):
        return int(round(math.log10(num)))
     
    labels = [(f"$10^{str(nearest_pow_10_exp(pltd[2]))}$", pltd[0]) if not
            math.isnan(pltd[2]) else ("", pltd[0]) for pltd in plt_dat if
            pltd[0].startswith(group)]
    values = [pltd[1] for pltd in plt_dat if pltd[0].startswith(group)]

    for j, ((label, hashfn), value) in enumerate(zip(labels,values)):
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
    bbox_to_anchor=(0.14, 1),
    loc="upper left",
    fontsize=6,
    ncol=1)

plt.savefig(f"out/median_throughput.pdf")
