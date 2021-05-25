import numpy as np
import matplotlib as mpl
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import pandas as pd
import math as math

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
        "radix_spline": "radix_spline", 
       # "pgm_hash_eps128": "pgm_hash", 
       # "mult_prime64": "mult", 
       # "mult_add64": "mult_add", 
        "murmur_finalizer64": "murmur finalizer"
        }
def name(hashfn):
    if hashfn.startswith("radix_spline"):
        return "radix spline"
    return hr_names.get(hashfn) or hashfn

DATASET_KEY="dataset"
MACHINE_KEY="machine"
COMPILER_KEY="compiler"
REDUCER_KEY="reducer"
HASH_KEY="hash"
KICKING_STRAT_KEY="kicking_strategy"
PAYLOAD_SIZE_KEY="payload"
LOAD_FACTOR_KEY="load_factor"
SAMPLE_SIZE_KEY="sample_size"
PRIMARY_KEY_RATIO_KEY="primary_key_ratio"

CLAMP="clamp"
FASTMOD="fast_modulo"

csv = pd.read_csv(f"cuckoo.csv")
partial_data = csv[csv[DATASET_KEY].isnull()]
data = csv[csv[DATASET_KEY].notnull()]

# Filter data
data = data[
        # Only use g++ results
        ((data[COMPILER_KEY].isnull()) | (data[COMPILER_KEY].str.match(r"g\+\+")))
        # Only use load factor 95 results (?)
        & (data[LOAD_FACTOR_KEY] == 0.95)
        # Only use fast modulo results
        & ((data[REDUCER_KEY].str.match(FASTMOD)) | (data[REDUCER_KEY].str.match(CLAMP)))
        # Only use 16 byte payload results (must be equal to 64 byte results for
        # primary_key_ratio, otherwise bug!)
        & (data[PAYLOAD_SIZE_KEY] == 16)
        # Only use biased kicking
        & (data[KICKING_STRAT_KEY] == "biased_kicking_10")
        # Don't use normal dataset results for now (dataset broken)
        & ((data[DATASET_KEY] != "normal_200M_uint64"))
        # Only use certain hash functions
        & (
           # (data[HASH_KEY].str.match("mult_prime64")) |
           # (data[HASH_KEY].str.match("mult_add64")) |
            (data[HASH_KEY].str.match("murmur_finalizer64")) |
           # (data[HASH_KEY].str.contains("rmi")) |
            (data[HASH_KEY].str.match("radix_spline")) #|
           # (data[HASH_KEY].str.match("pgm"))
           )
        ]


# Use do_nothing entries to determine order
tmp_d = data[((data[SAMPLE_SIZE_KEY] == 0.01) | (data[SAMPLE_SIZE_KEY].isnull()))
        & (data[DATASET_KEY] ==
            "wiki_ts_200M_uint64")].sort_values(PRIMARY_KEY_RATIO_KEY)
# dict preserves insertion order since python 3.7
classical_hashfns = [
       # "mult_prime64", "mult_add64", 
        "murmur_finalizer64"] 
learned_hashfns = [x[0:x.find("-")] for x in
    dict.fromkeys(tmp_d[tmp_d[REDUCER_KEY].str.match(CLAMP)][HASH_KEY])]
all_hashfns = learned_hashfns + classical_hashfns

pallette = list(mcolors.TABLEAU_COLORS.keys())
colors = {h: pallette[i % len(pallette)] for i, h in enumerate(all_hashfns)}
datasets = sorted(set(data[DATASET_KEY]))

# Generate plot
fig, ax = plt.subplots(figsize=(7.00697/2,2))

datasets = [
        # synth
        'consecutive_200M_uint64', 
        'gapped_1%_200M_uint64',
        'gapped_10%_200M_uint64',
        #'uniform_dense_200M_uint64',
        # real
        'wiki_ts_200M_uint64', 
        'fb_200M_uint64',
        'osm_cellids_200M_uint64']

for i, dataset in enumerate(datasets):
    d = data[data[DATASET_KEY] == dataset]
    ds_hfn_map = {x[0:x.find("-")]: x for x in set(d[HASH_KEY])}

    bars = {}
    for hashfn in all_hashfns:
        if hashfn not in ds_hfn_map:
            continue
        bars[hashfn] = list(d[d[HASH_KEY] == ds_hfn_map[hashfn]][PRIMARY_KEY_RATIO_KEY])

    # Plt data
    plt_data = [(s.strip(), bars[s]) for s in bars.keys()]
    empty_space = 0.2
    bar_width = (1 - empty_space) / len(plt_data)
    gap_width = 0 #0.1 / len(plt_data)
    for j, (hash_name, value) in enumerate(plt_data):
        ax.bar(empty_space/2 + i + j * (bar_width+gap_width), value, bar_width, color=colors.get(hash_name) or "purple")

# Plot style/info
yticks = np.linspace(0.75, 1, 5)
plt.ylim(0.75,1)
plt.yticks(yticks, [f"{int(yt*100)}%" for yt in yticks], fontsize=8)

plt.xticks([i+0.5 for i in range(0, len(datasets))], [d.replace(r"_200M",
    "").replace("_uint64", "").replace("_", " ") for d in datasets], 
    rotation=45,
    ha="right",
    va="top",
    fontsize=8)
plt.margins(x=0.01,y=0.2)
plt.tight_layout(pad=0.1)

# Legend
fig.legend(
    handles=[mpatches.Patch(color=colors.get(h), label=name(h)) for h in all_hashfns],
    bbox_to_anchor=(1, 0.98),
    loc="upper right",
    fontsize=6)

plt.savefig(f"out/primary_key_ratio.pdf")
