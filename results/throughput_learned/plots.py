import colorsys
import math
from collections import OrderedDict

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


compilers = ["g++"]  # , "clang++", "g++-10"]
_csv = pandas.read_csv(f"throughput_learned-{compilers[0]}.csv")
dataset_names = sorted(set(_csv['dataset']))
models = list(
    OrderedDict.fromkeys(list(_csv[_csv['reducer'] != 'do_nothing'].sort_values('model')['model'])))
_colors = list(gen_colors(len(models)))
colors = {method: _colors[i] for i, method in enumerate(models)}

for compiler in compilers:
    csv = pandas.read_csv(f"throughput_learned-{compiler}.csv")

    for dataset_name in dataset_names:
        dataset = csv[csv['dataset'] == dataset_name]
        reducers = sorted(list(set(dataset['reducer'])))
        sample_sizes = list(reversed(sorted(list(set(dataset['sample_size'])))))
        plot_keys = ["sample_nanoseconds_per_key", "prepare_nanoseconds_per_key", "build_nanoseconds_per_key",
                     "hashing_nanoseconds_per_key"]

        fig, subplts = plt.subplots(len(sample_sizes), len(plot_keys), sharey=True, sharex=True, figsize=(20, 15))

        for k, plot_key in enumerate(plot_keys):
            for l, sample_size in enumerate(sample_sizes):
                ds = dataset[dataset['sample_size'] == sample_size]

                subplt = subplts[l, k]

                # order preserving deduplication
                for j, model_name in enumerate([m for m in models if m in set(ds['model'])]):
                    series = list(
                        ds[ds['model'] == model_name].sort_values('reducer')[plot_key])
                    width = 0.95 / len(models)

                    for i, reducer in enumerate(reducers):
                        # We only want to set label once as otherwise legend will contain duplicates
                        subplt.bar(i + j * width, series[i], width,
                                   color=colors.get(model_name) or "white")

                        subplt.text(i + (j - 0.5) * width + (j - 0.5) * 0.01, series[i] + 10,
                                    str(math.ceil(series[i])),
                                    color=colors.get(model_name),
                                    fontsize='xx-small')

                subplt.grid(linestyle='--', linewidth=0.5)
                subplt.set_title(f"{plot_key.replace('_', ' ')} ({sample_size} sample)")

                xticks = np.arange(len(reducers)) + 0.4
                subplt.set_xticks(xticks, minor=False)
                subplt.set_xticklabels(reducers, fontdict=None, minor=False)

        fig.text(0.5, 0.04, 'reduction algorithm', ha='center', va='center')
        fig.text(0.06, 0.5, 'nanoseconds per key', ha='center', va='center', rotation='vertical')
        fig.suptitle(f"throughput on {dataset_name} using {compiler}")
        fig.legend(
            handles=[mpatches.Patch(color=colors.get(name), label=name) for name in models],
            bbox_to_anchor=(0.5, -0.1), loc="lower center", ncol=7)

        plt.savefig(f"graphs/throughput_learned-{compiler}_{dataset_name}.pdf", bbox_inches='tight', pad_inches=0.5)
        plt.savefig(f"graphs/throughput_learned-{compiler}_{dataset_name}.png", bbox_inches='tight', pad_inches=0.5)
        plt.close()
