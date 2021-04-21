import colorsys
from collections import OrderedDict

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
hash_methods = list(
    OrderedDict.fromkeys(list(_csv[_csv['reducer'] != 'do_nothing'].sort_values('total_nanoseconds_per_key')['model'])))
_colors = list(gen_colors(len(hash_methods)))
colors = {method: _colors[i] for i, method in enumerate(hash_methods)}

for compiler in compilers:
    csv = pandas.read_csv(f"throughput_learned-{compiler}.csv")

    for dataset_name in dataset_names:
        dataset = csv[csv['dataset'] == dataset_name]
        reducers = sorted(list(set(dataset['reducer'])))

        plt.figure(figsize=(15, 7))

        # order preserving deduplication
        for j, hash_name in enumerate([m for m in hash_methods if m in set(dataset['model'])]):
            series = list(dataset[dataset['model'] == hash_name].sort_values('reducer')['total_nanoseconds_per_key'])
            width = 0.95 / len(hash_methods)

            for i, reducer in enumerate(reducers):
                # We only want to set label once as otherwise legend will contain duplicates
                plt.bar(i + j * width, series[i], width, label=hash_name if i == 0 else None,
                        color=colors.get(hash_name) or "white")

        plt.grid(linestyle='--', linewidth=0.5)
        plt.title(f"throughput on {dataset_name} using {compiler}")
        plt.xticks(np.arange(len(reducers)) + 0.4, reducers)
        plt.yticks(list(plt.yticks()[0]) + [1, 2, 3, 4])
        plt.ylabel('ns per key')
        plt.xlabel('reduction algorithm')
        plt.legend(bbox_to_anchor=(0.5, -0.4), loc="lower center", ncol=7)

        plt.savefig(f"graphs/throughput_learned-{compiler}_{dataset_name}.pdf", bbox_inches='tight', pad_inches=0.5)
        plt.savefig(f"graphs/throughput_learned-{compiler}_{dataset_name}.png", bbox_inches='tight', pad_inches=0.5)
