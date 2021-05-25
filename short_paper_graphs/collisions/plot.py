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
hr_names = {"radix_spline": "radix spline", "rmi": "rmi", "vp_rmi": "vp_rmi",
        "mult_prime64": "mult", "mult_add64": "mult_add", "murmur_finalizer64":
        "murmur finalizer"}
def name(hashfn):
    return hr_names.get(hashfn) or hashfn

def plot_collision_statistic(stat_key, title, expected_fun, ymax=1):
    DATASET_KEY="dataset"
    MACHINE_KEY="machine"
    COMPILER_KEY="compiler"
    REDUCER_KEY="reducer"
    HASH_KEY="hash"
    LOAD_FACTOR_KEY="load_factor"
    SAMPLE_SIZE_KEY="sample_size"

    CLAMP="clamp"
    FASTMOD="fast_modulo"

    csv = pd.read_csv(f"collisions.csv")
    partial_data = csv[csv[DATASET_KEY].isnull()]
    data = csv[csv[DATASET_KEY].notnull()]

    # Filter data
    data = data[
            # Only use g++ results
            ((data[COMPILER_KEY].isnull()) | (data[COMPILER_KEY].str.match(r"g\+\+")))
            # Only use load factor 1 results
            & (data[LOAD_FACTOR_KEY] == 1.0)
            # Only use fast modulo results
            & ((data[REDUCER_KEY] == FASTMOD) | (data[REDUCER_KEY] == CLAMP))
            # Don't use normal dataset results for now (dataset broken)
            & ((data[DATASET_KEY] != "normal_200M_uint64"))
            # Only use certain hash functions
            & (
               # (data[HASH_KEY] == "mult_prime64")
               # | (data[HASH_KEY] == "mult_add64")
                (data[HASH_KEY] == "murmur_finalizer64")
               # | (data[HASH_KEY].str.contains("rmi"))
                | (data[HASH_KEY].str.match("radix_spline"))
               # | (data[HASH_KEY].str.match("pgm"))
                )
            ]

    # Use do_nothing entries to determine order
    tmp_d = data[((data[SAMPLE_SIZE_KEY] == 0.01) | (data[SAMPLE_SIZE_KEY].isnull()))
            & (data[DATASET_KEY] ==
                "wiki_ts_200M_uint64")].sort_values("colliding_slots")
    # dict preserves insertion order since python 3.7
    classical_hashfns = [
            #"mult_prime64", "mult_add64", 
            "murmur_finalizer64"] 
    learned_hashfns = list(dict.fromkeys(tmp_d[tmp_d[REDUCER_KEY] == CLAMP][HASH_KEY])) 
    all_hashfns = learned_hashfns + classical_hashfns

    pallette = list(mcolors.TABLEAU_COLORS.keys())
    colors = {h: pallette[i % len(pallette)] for i, h in enumerate(all_hashfns)}
    datasets = sorted(set(data[DATASET_KEY]))

    # Generate plot
    fig, ax = plt.subplots(figsize=(7.00697/2,2))

    # Aggregate data over multiple datasets
    datasets = sorted(set(data[DATASET_KEY]))

    for i, dataset in enumerate(datasets):
        d = data[data[DATASET_KEY] == dataset]
        hashfns = [hfn for hfn in all_hashfns if hfn in set(d[HASH_KEY])]

        bars = {}
        for hashfn in hashfns:
            bars[hashfn] = list(d[d[HASH_KEY] == hashfn][stat_key])

        # Plt data
        plt_data = [(s.strip(), bars[s]) for s in hashfns]
        empty_space = 0.2
        bar_width = (1 - empty_space) / len(plt_data)
        gap_width = 0 #0.1 / len(plt_data)
        for j, (hash_name, value) in enumerate(plt_data):
            ax.bar(empty_space/2 + i + j * (bar_width+gap_width), value, bar_width, color=colors.get(hash_name) or "purple")

    # Expected value (load factor 1)
    y = expected_fun(1.0)
    ax.plot([0, len(datasets)], [y,y], color="black",
            linestyle="dashed", linewidth=1)

    # Plot style/info
    yticks = np.linspace(0, ymax, 5)
    plt.ylim(0,ymax)
    plt.yticks(yticks, [f"{int(yt*100)}%" for yt in yticks], fontsize=8)

    plt.xticks([i+0.5 for i in range(0, len(datasets))], [d.replace(r"_200M",
        "").replace("_uint64", "").replace("_", " ") for d in datasets],
        rotation=45, ha="right", va="top",
        fontsize=8)
    plt.margins(x=0.01,y=0.2)
    plt.tight_layout(pad=0.1)

    # Legend
    fig.legend(
        handles=[mpatches.Patch(color=colors.get(h), label=name(h)) for h in all_hashfns],
        bbox_to_anchor=(1, 0.98),
        loc="upper right",
        fontsize=6,
        ncol=1)

    plt.savefig(f"out/{stat_key}.pdf")

#plot_expected_colliding_keys()
plot_collision_statistic("colliding_keys", "Colliding keys", lambda load_fac : 1
        - np.exp(-load_fac))
plot_collision_statistic("colliding_slots", "Colliding slots", lambda load_fac :
        1 - (((1 + 1/load_fac) * np.exp(-load_fac)) / (1 / load_fac)), ymax=0.5)
plot_collision_statistic("empty_slots", "Empty slots", lambda load_fac:
        np.exp(-load_fac), ymax=0.75)
plot_collision_statistic("exclusive_slots", "Exclusive slots (exactly one key)", lambda load_fac :
        1 - (1 - (((1 + 1/load_fac) * np.exp(-load_fac)) / (1 / load_fac)) +
            np.exp(-load_fac)))

