import colorsys
from collections import OrderedDict

import math
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import numpy as np
import pandas as pandas


def gen_colors(max=40.0, s_sawtooth_min=0.7, s_sawtooth_max=0.9, s_sawtooth_step=0.1, v_sawtooth_min=0.5,
               v_sawtooth_max=1.0, v_sawtooth_step=0.15):
    s_next = s_sawtooth_min
    v_next = v_sawtooth_min
    for i in range(max):
        h = i / max
        s = s_next
        v = v_next
        yield colorsys.hsv_to_rgb(h, s, v)

        s_next += s_sawtooth_step
        if s_next > s_sawtooth_max:
            s_next = s_sawtooth_min

        v_next += v_sawtooth_step
        if v_next > v_sawtooth_max:
            v_next = v_sawtooth_min


compilers = ["g++"]  # ["clang++", "g++"]
_csv = pandas.read_csv(f"hashtable_learned-{compilers[0]}.csv")
dataset_names = sorted(set(_csv['dataset']))
models = list(
    OrderedDict.fromkeys(list(_csv.sort_values('lookup_nanoseconds_total')['model'])))
_colors = list(gen_colors(len(models)))
colors = {method: _colors[i] for i, method in enumerate(models)}


def gen_plot(key):
    for compiler in compilers:
        csv = pandas.read_csv(f'hashtable_learned-{compiler}.csv')

        for fig, dataset_name in enumerate(dataset_names):
            dataset = csv[csv['dataset'] == dataset_name]
            load_factors = sorted(set(dataset['load_factor']))
            bucket_sizes = sorted(set(dataset['bucket_size']))

            fig, subplts = plt.subplots(len(bucket_sizes), len(load_factors), sharex=True, sharey=True,
                                        figsize=(20, 10))

            for l, load_factor in enumerate(reversed(load_factors)):
                for k, bucket_size in enumerate(reversed(bucket_sizes)):
                    ds = dataset[dataset['bucket_size'] == bucket_size]
                    ds = ds[ds['load_factor'] == load_factor]
                    reducers = sorted(list(set(ds['reducer'])))

                    subplt = subplts[k, l]

                    # order preserving deduplication
                    for j, model_name in enumerate([m for m in models if m in set(ds['model'])]):
                        series = list(
                            ds[ds['model'] == model_name].sort_values('reducer')[f'{key}_nanoseconds_per_key'])
                        width = 0.95 / len(models)

                        for i, reducer in enumerate(reducers):
                            # We only want to set label once as otherwise legend will contain duplicates
                            subplt.bar(i + j * width + j * 0.005, series[i], width,
                                       color=colors.get(model_name) or "white")
                            subplt.text(i + (j - 0.5) * width + (j - 0.5) * 0.01, series[i] + 5,
                                        str(math.ceil(series[i])),
                                        color=colors.get(model_name),
                                        fontsize='xx-small')

                    subplt.grid(axis='y', linestyle='--', linewidth=0.5)
                    subplt.set_title(f"bucket_size {bucket_size}, load_factor {load_factor} (1% sample)")

                    xticks = np.arange(len(reducers)) + 0.25
                    subplt.set_xticks(xticks, minor=False)
                    subplt.set_xticklabels(reducers, fontdict=None, minor=False)
                    subplt.axhline(1 - pow(math.e, -load_factor), color="blue", ls="-")

                fig.text(0.5, 0.04, 'reduction algorithm', ha='center', va='center')
                fig.text(0.06, 0.5, 'nanoseconds per key', ha='center', va='center', rotation='vertical')
                fig.suptitle(
                    f"hashtable {key} on {dataset_name} using compiler {compiler}")
                fig.legend(
                    handles=[mpatches.Patch(color=colors.get(name), label=name) for name in models],
                    bbox_to_anchor=(0.3, 0), loc="lower left", ncol=3)

            plt.savefig(f"graphs/{key}_{dataset_name}_{compiler}.png", bbox_inches='tight', pad_inches=0.5)
            plt.savefig(f"graphs/{key}_{dataset_name}_{compiler}.pdf", bbox_inches='tight', pad_inches=0.5)
            plt.close()


gen_plot('lookup')
gen_plot('insert')
