import numpy as np
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
hr_names = {"radix_spline": "radix_spline", "rmi": "rmi", "vp_rmi": "vp_rmi", "mult_prime64": "mult", "mult_add64": "mult_add", "murmur_finalizer64": "murmur_fin"}
all_palette = list(mcolors.TABLEAU_COLORS.keys())
palette = all_palette[:-1]
colors = {h: palette[i % len(palette)] for i,h in enumerate(hr_names.keys())}

# Helper
def name(hashfn):
    return hr_names.get(hashfn) or hashfn

def color(hashfn):
    bracket_ind = hashfn.find("(")
    h = hashfn[0: bracket_ind if bracket_ind > 0 else len(hashfn)].strip()
    return colors.get(h) or all_palette[-1]

def models(h):
    bracket_ind = h.find("(")
    return h[bracket_ind+1:h.find(")")] if bracket_ind > 0 else ""

# Read data
DATASET_KEY="dataset"
MACHINE_KEY="machine"
COMPILER_KEY="compiler"
REDUCER_KEY="reducer"
SAMPLE_SIZE_KEY="sample_size"
HASH_KEY="hash"
THROUGHPUT_KEY="throughput"

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
data = data[(data[HASH_KEY] == "mult_prime64") | (data[HASH_KEY] ==
    "mult_add64") | (data[HASH_KEY] == "murmur_finalizer64") |
    (data[HASH_KEY].str.contains("rmi")) |
    ((data[HASH_KEY].str.contains("radix_spline")) & (data[HASH_KEY] !=
        "radix_spline (32:18)"))]

# Create plot
fig, ax = plt.subplots(figsize=(7.00697/2,3))

# Extract data
machine = set(data[data[DATASET_KEY].notnull()][MACHINE_KEY]).pop()
processor = machine[machine.find("(")+1:machine.rfind(")")]

plt_dat = sorted([(h, median(list(data[data[HASH_KEY] == h][THROUGHPUT_KEY])))for
    h in set(data[HASH_KEY])], key=lambda x: x[1])

labels = [name(pltd[0]) for pltd in plt_dat]
values = [pltd[1] for pltd in plt_dat]

# Plot data
bar_width = 1.0 / len(plt_dat)
ax.bar(labels, values, color=[color(pltd[0]) for pltd in plt_dat])

# Number above bar
for i,v in enumerate(values):
    ax.text(i, v + 5, str(round(v, 1)), ha="center", color=color(plt_dat[i][0]),
            fontsize=8, rotation=90)

# Plot style/info
plt.yticks(fontsize=8)
plt.xticks(range(0,len(labels)), [l for l in labels], rotation=45, ha="right",
        va="top", fontsize=6)

#plt.xlabel("hash function")
plt.ylabel("ns per key")
plt.margins(x=0.01,y=0.25)
plt.tight_layout(pad=0.1)

# Legend
#plt.subplots_adjust(bottom=0.15)
fig.legend(
    handles=[mpatches.Patch(color=color, label=name(h)) for h,color in colors.items()],
    loc="upper center",
    fontsize=6,
    ncol=3)

plt.savefig(f"out/median_throughput.pdf")
