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
        "radix_spline": "RadixSpline", 
       # "pgm_hash_eps128": "pgm_hash", 
       # "mult_prime64": "mult", 
       # "mult_add64": "mult_add", 
        "murmur_finalizer64": "Murmur"
        }
def name(hashfn):
    if hashfn.startswith("radix_spline"):
        hashfn = "radix_spline"
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


# Generate plot
fig, axs = plt.subplots(2,2,figsize=(7.00697,4), sharex=True, sharey=True)

letter = [["A", "B"], ["C", "D"]]
expected = [[0.6503, 0.8353], [0.6140, 0.8111]]
for l, load_fac in enumerate(sorted(set(data[LOAD_FACTOR_KEY]))):
    for s, strat in enumerate(sorted(set(data[KICKING_STRAT_KEY]))):
        ax = axs[l][s]
        ax.set_title(f"({letter[l][s]}) {load_fac} load factor, {strat[0:strat.find('_')]} kicking", fontsize=8)

        # Filter data
        d = data[
                # Only use g++ results
                ((data[COMPILER_KEY].isnull()) | (data[COMPILER_KEY].str.match(r"g\+\+")))
                # Only use load factor 95 results (?)
                & (data[LOAD_FACTOR_KEY] == load_fac)
                # Only use fast modulo results
                & ((data[REDUCER_KEY].str.match(FASTMOD)) | (data[REDUCER_KEY].str.match(CLAMP)))
                # Only use 16 byte payload results (must be equal to 64 byte results for
                # primary_key_ratio, otherwise bug!)
                & (data[PAYLOAD_SIZE_KEY] == 16)
                # Only use biased kicking
                & (data[KICKING_STRAT_KEY] == strat)
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


        # dict preserves insertion order since python 3.7
        classical_hashfns = [
               # "mult_prime64", "mult_add64", 
                "murmur_finalizer64"] 
        tmp_d = d[((d[SAMPLE_SIZE_KEY] == 0.01) | (d[SAMPLE_SIZE_KEY].isnull()))]
        x = dict.fromkeys(tmp_d[tmp_d[REDUCER_KEY].str.match(CLAMP)][HASH_KEY])
        learned_hashfns = [x[0:x.find("-")] for x in x]
        all_hashfns = learned_hashfns + classical_hashfns

        colors = {"RadixSpline": "tab:blue", "Murmur": "tab:orange"}
        datasets = sorted(set(d[DATASET_KEY]))

        datasets = [
                # synth
                'seq_200M_uint64', 
                'gap_1%_200M_uint64',
                'gap_10%_200M_uint64',
                #'uniform_dense_200M_uint64',
                # real
                'wiki_200M_uint64', 
                'fb_200M_uint64',
                'osm_200M_uint64']

        for i, dataset in enumerate(datasets):
            d2 = d[d[DATASET_KEY] == dataset]
            ds_hfn_map = {x[0:x.find("-")]: x for x in set(d2[HASH_KEY])}
            seen_reps = dict()

            bars = {}
            for hashfn in all_hashfns:
                if hashfn not in ds_hfn_map:
                    continue

                res_l = list(d2[d2[HASH_KEY] ==
                    ds_hfn_map[hashfn]][PRIMARY_KEY_RATIO_KEY])
                if len(res_l) != 1:
                    continue
                res = res_l[0]

                if name(hashfn) in seen_reps:
                    old_hashfn = seen_reps[name(hashfn)]
                    old_res = bars[name(old_hashfn)]
                    if old_res < res:
                        seen_reps[name(hashfn)] = hashfn
                        bars[name(hashfn)] = res
                else:
                    seen_reps[name(hashfn)] = hashfn
                    bars[name(hashfn)] = res


            # Plt data
            plt_data = [(s.strip(), bars[s]) for s in bars.keys()]
            if len(plt_data) <= 0:
                continue

            empty_space = 0.2
            bar_width = (1 - empty_space) / len(plt_data)
            gap_width = 0 #0.1 / len(plt_data)
            for j, (hash_name, value) in enumerate(plt_data):
                ax.bar(empty_space/2 + i + j * (bar_width+gap_width) +
                        (bar_width+gap_width)/2, value, bar_width,
                        color=colors.get(name(hash_name)) or "purple")


        # Expected value
        y = expected[l][s]
        ax.plot([0, len(datasets)], [y,y], color="black",
                linestyle="dashed", linewidth=1.0)

        # Plot style/info
        yticks = np.linspace(0.5, 1, 5)
        ax.set_ylim(0.5,1)
        ax.set_yticks(yticks)
        ax.set_yticklabels([f"{int(yt*100)}%" for yt in yticks], fontsize=8)
        ax.set_xticks([i+0.5 for i in range(0, len(datasets))])
        ax.set_xticklabels([d.replace(r"_200M", "").replace("_uint64",
            "").replace("_", " ") for d in datasets],
                va="center_baseline",position=(0.5,-0.05), fontsize=8)

        # Legend 
        ax.legend(
            handles=[mpatches.Patch(color=colors.get(name(h)), label=name(h)) for h,_ in
                hr_names.items()],
            #bbox_to_anchor=(1, 0.98),
            loc="upper right",
            fontsize=6)

fig.text(0.5, 0.02, 'dataset', ha='center', fontsize=8)
fig.text(0.01, 0.5, 'primary key ratio', va='center', rotation='vertical',
        fontsize=8)

#plt.margins(x=0.01,y=0.2)
plt.tight_layout()
plt.subplots_adjust(left=0.09, bottom=0.12)
plt.savefig(f"out/primary_key_ratio.pdf")
