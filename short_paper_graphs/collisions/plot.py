import numpy as np
import matplotlib as mpl
import matplotlib.patches as mpatches
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

    datasets = sorted(set(data[DATASET_KEY]))
    compilers = sorted(set(data[COMPILER_KEY]))

    for compiler in compilers:
        d1 = data[data[COMPILER_KEY] == compiler]

        # Extract information
        machine = set(d1[MACHINE_KEY]).pop()
        processor = machine[machine.find("(")+1:machine.rfind(")")]

        # Use do_nothing entries to determine order
        tmp_d = d1[(d1[LOAD_FACTOR_KEY] == 1.0) 
                & ((d1[SAMPLE_SIZE_KEY] == 0.01) | (d1[SAMPLE_SIZE_KEY].isnull()))
                & (d1[DATASET_KEY] ==
                    "wiki_ts_200M_uint64")].sort_values(stat_key)
        # dict preserves insertion order since python 3.7
        classical_hashfns = list(dict.fromkeys(tmp_d[tmp_d[REDUCER_KEY] == FASTMOD][HASH_KEY])) 
        learned_hashfns = list(dict.fromkeys(tmp_d[tmp_d[REDUCER_KEY] == CLAMP][HASH_KEY])) 
        all_hashfns = learned_hashfns + classical_hashfns

        all_reducers = set(d1[d1[REDUCER_KEY] != CLAMP][REDUCER_KEY])
        pallette = list(mcolors.TABLEAU_COLORS.keys())
        classical_colors = {h: pallette[i % len(pallette)] for i, h in enumerate(classical_hashfns)}
        pallette = list(mcolors.BASE_COLORS.keys())
        learned_colors = {h: pallette[i % len(pallette)] for i, h in enumerate(learned_hashfns)}
        colors = classical_colors
        colors.update(learned_colors)

        # Generate plot
        fig, axs = plt.subplots(nrows=len(datasets), ncols=3, sharex=True,
                sharey=True, figsize=(30,20))
        plt.suptitle(f"{title}\n{compiler} @ {processor}\n")

        # Aggregate data over multiple datasets
        for l, reducer in enumerate(all_reducers):
            d2 = d1[(d1[REDUCER_KEY] == reducer) | (d1[REDUCER_KEY] == CLAMP)]

            load_factors = sorted(set(d2[LOAD_FACTOR_KEY]), reverse=True)
            datasets = sorted(set(d2[DATASET_KEY]))

            # Column Header
            axs[0][l].set_title(f"learned: clamp, classic: {reducer}", fontweight="bold")

            for k, dataset in enumerate(datasets):
                d3 = d2[d2[DATASET_KEY] == dataset]

                # Row Header
                ds = dataset.replace(r"_200M", "").replace("_uint64", "")
                axs[k][0].set_ylabel(f"{ds}", fontweight="bold")

                ax = axs[k][l]

                for i, load_factor in enumerate(load_factors):
                    d = d3[d3[LOAD_FACTOR_KEY] == load_factor]
                    hashfns = [hfn for hfn in all_hashfns if hfn in set(d[HASH_KEY])]

                    bars = {}
                    for hashfn in hashfns:
                        bars[hashfn] = list(d[d[HASH_KEY] == hashfn][stat_key])

                    # Plt data
                    plt_data = [(s.strip(), bars[s]) for s in hashfns]
                    bar_width = 0.8 / len(plt_data)
                    gap_width = 0.1 / len(plt_data)
                    for j, (hash_name, value) in enumerate(plt_data):
                        ax.bar(i + (0.01 if hash_name in classical_hashfns else -0.01) + j * (bar_width+gap_width), value, bar_width, color=colors.get(hash_name) or "purple")


                for i, load_fac in enumerate(load_factors):
                    y = expected_fun(load_fac)
                    ax.plot([i, i+0.9], [y,y], color="black",
                            linestyle="dashed", linewidth=1)
                    # TODO
                    #ax.text(i+0.9/2.0, y+0.75, r"$1 - \frac{1}{e^{-" +
                    #        str(load_fac) + "}}$", horizontalalignment='center')


            # Plot style/info
            yticks = np.linspace(0, ymax, 5)
            plt.yticks(yticks, [f"{int(yt*100)}%" for yt in yticks])

            plt.xticks([0.5, 1.5, 2.5, 3.5], load_factors)
            fig.text(0.5, 0.1, 'Load factor', ha='center')
            plt.tight_layout()

            # Fit xlabel, ylabel
            plt.subplots_adjust(bottom=0.14)

            # Legend
            fig.legend(
                handles=[mpatches.Patch(color=colors.get(h), label=h) for h in all_hashfns],
                #bbox_to_anchor=(1.05, 1),
                loc="lower center",
                ncol=5)

        plt.savefig(f"out/{stat_key}_{compiler}.pdf")

plot_expected_colliding_keys()
plot_collision_statistic("colliding_keys", "Colliding keys", lambda load_fac : 1
        - np.exp(-load_fac))
plot_collision_statistic("colliding_slots", "Colliding slots", lambda load_fac :
        1 - (((1 + 1/load_fac) * np.exp(-load_fac)) / (1 / load_fac)), ymax=0.5)
plot_collision_statistic("empty_slots", "Empty slots", lambda load_fac :
        np.exp(-load_fac))
