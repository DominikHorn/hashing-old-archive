import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
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
hr_names = {"mult_prime64": "mult", "mult_add64": "mult_add", "murmur_finalizer64":
        "murmur_fin", "rmi": "rmi", "vp_rmi": "vp_rmi"}
all_palette = list(mcolors.TABLEAU_COLORS.keys())
palette = all_palette[:-1]
colors = {h: palette[i % len(palette)] for i,h in enumerate(hr_names.keys())}
def color(hashfn):
    bracket_ind = hashfn.find("(")
    h = hashfn[0: bracket_ind if bracket_ind > 0 else len(hashfn)].strip()
    return colors.get(h) or all_palette[-1]

# Read data
DATASET_KEY="dataset"
MACHINE_KEY="machine"
COMPILER_KEY="compiler"
REDUCER_KEY="reducer"
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
# Only use certain hash functions
data = data[(data[HASH_KEY] == "mult_prime64") | (data[HASH_KEY] ==
    "mult_add64") | (data[HASH_KEY] == "murmur_finalizer64") |
    (data[HASH_KEY].str.contains("rmi"))] #| (data[HASH_KEY].str.match("radix_spline"))]


# Create plot
fig, ax = plt.subplots(figsize=(7.00697/2,3))

# Extract data
machine = set(data[data[DATASET_KEY].notnull()][MACHINE_KEY]).pop()
processor = machine[machine.find("(")+1:machine.rfind(")")]

all_hashfns = list(dict.fromkeys(data[(data[REDUCER_KEY] == FAST_MODULO) |
    (data[REDUCER_KEY] == CLAMP)].sort_values(HASH_KEY).sort_values(THROUGHPUT_KEY)[HASH_KEY])) # preserves order since python 3.7
hashfns = [hfn for hfn in all_hashfns if hfn in set(data[HASH_KEY])]

labels = [hr_names.get(h) or h for h in hashfns]
values = [median(list(data[data[HASH_KEY] == h][THROUGHPUT_KEY])) for h in hashfns] 

# Plot data
bar_width = 1.0 / len(hashfns)
ax.bar(labels, values, color=[color(h) for h in hashfns])

for i,v in enumerate(values):
    ax.text(i, v + 5, str(round(v, 1)), ha="center", color=color(hashfns[i]),
            fontsize=8, rotation=90)

# Plot style/info
plt.xticks(range(0,len(labels)), labels, rotation=45, ha="right", fontsize=8)
#plt.xlabel("hash function")
plt.ylabel("ns per key")
plt.margins(x=0.01,y=0.25)
plt.tight_layout()

plt.savefig(f"out/median_throughput.pdf")
plt.savefig(f"out/median_throughput.pgf")
