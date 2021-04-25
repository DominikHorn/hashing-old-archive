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
_csv = pandas.read_csv(f"collisions_learned-{compilers[0]}.csv")
dataset_names = sorted(set(_csv['dataset']))
models = list(
    OrderedDict.fromkeys(list(_csv[_csv['reducer'] != 'do_nothing'].sort_values('model')['model'])))
_colors = list(gen_colors(len(models)))
colors = {method: _colors[i] for i, method in enumerate(models)}

# ============================
#    colliding keys percent
# ============================

for compiler in compilers:
    csv = pandas.read_csv(f'collisions_learned-{compiler}.csv')

    for fig, dataset_name in enumerate(dataset_names):
        dataset = csv[csv['dataset'] == dataset_name]
        load_factors = sorted(set(dataset['load_factor']))
        sample_sizes = sorted(set(dataset['sample_size']))

        fig, subplts = plt.subplots(len(sample_sizes), len(load_factors), sharex=True, sharey=True, figsize=(15, 10))

        for k, load_factor in enumerate(reversed(load_factors)):
            for l, sample_size in enumerate(reversed(sample_sizes)):
                ds = dataset[dataset['sample_size'] == sample_size]
                ds = ds[ds['load_factor'] == load_factor]
                reducers = sorted(list(set(ds['reducer'])))

                subplt = subplts[l, k]

                # order preserving deduplication
                for j, model_name in enumerate([m for m in models if m in set(ds['model'])]):
                    series = list(ds[ds['model'] == model_name].sort_values('reducer')['total_colliding_keys_percent'])
                    width = 0.95 / len(models)

                    for i, reducer in enumerate(reducers):
                        # We only want to set label once as otherwise legend will contain duplicates
                        subplt.bar(i + j * width + j * 0.005, series[i], width,
                                   color=colors.get(model_name) or "white")

                subplt.grid(axis='y', linestyle='--', linewidth=0.5)
                subplt.set_title(f"sample_size {sample_size}, load_factor {load_factor}")

                xticks = np.arange(len(reducers)) + 0.4
                subplt.set_xticks(xticks, minor=False)
                subplt.set_xticklabels(reducers, fontdict=None, minor=False)
                subplt.set_yticks([0.0, 0.2, 0.4, 0.6, 0.8, 1.0], minor=False)
                subplt.axhline(1 - pow(math.e, -load_factor), color="blue", ls="-")

            fig.text(0.5, 0.04, 'reduction algorithm', ha='center', va='center')
            fig.text(0.06, 0.5, 'colliding keys / total keys', ha='center', va='center', rotation='vertical')
            fig.suptitle(
                f"collisions on {dataset_name} using compiler {compiler}")
            fig.legend(
                handles=[mpatches.Patch(color=colors.get(name), label=name) for name in models],
                bbox_to_anchor=(0.3, -0.5), loc="lower left", ncol=3)

        plt.savefig(f"graphs/colliding_keys_percent_{dataset_name}_{compiler}.png", bbox_inches='tight', pad_inches=0.5)
        plt.savefig(f"graphs/colliding_keys_percent_{dataset_name}_{compiler}.pdf", bbox_inches='tight', pad_inches=0.5)
        plt.close()


# ============================
#    nanoseconds per key
# ============================

def plot_timing(key):
    for compiler in compilers:
        csv = pandas.read_csv(f'collisions_learned-{compiler}.csv')

        for fig, dataset_name in enumerate(dataset_names):
            dataset = csv[csv['dataset'] == dataset_name]
            sample_sizes = sorted(set(dataset['sample_size']))

            plt_cnt = int(len(sample_sizes) / 2)
            fig, subplts = plt.subplots(plt_cnt, plt_cnt, sharex=True, sharey=True)

            for l, sample_size in enumerate(reversed(sample_sizes)):
                ds = dataset[dataset['sample_size'] == sample_size]
                reducers = sorted(list(set(ds['reducer'])))

                subplt = subplts[int(l / plt_cnt), l % plt_cnt]

                # order preserving deduplication
                for j, model_name in enumerate([m for m in models if m in set(ds['model'])]):
                    series = list(ds[ds['model'] == model_name].sort_values('reducer')[key])
                    width = 0.95 / len(models)

                    for i, reducer in enumerate(reducers):
                        # We only want to set label once as otherwise legend will contain duplicates
                        subplt.bar(i + j * width + j * 0.005, series[i], width,
                                   color=colors.get(model_name) or "white")

                subplt.grid(axis='y', linestyle='--', linewidth=0.5)
                subplt.set_title(f"sample_size {sample_size}")

                xticks = np.arange(len(reducers)) + 0.4
                subplt.set_xticks(xticks, minor=False)
                subplt.set_xticklabels(reducers, fontdict=None, minor=False)

            fig.text(0.5, 0.04, 'reduction algorithm', ha='center', va='center')
            fig.text(0.06, 0.5, 'nanoseconds per key', ha='center', va='center', rotation='vertical')
            fig.suptitle(f"{key.replace('_', ' ')} on {dataset_name} using {compiler}")
            fig.legend(
                handles=[mpatches.Patch(color=colors.get(name), label=name) for name in models],
                bbox_to_anchor=(0.05, -0.5), loc="lower left", ncol=3)

            plt.savefig(f"graphs/{key}_{dataset_name}_{compiler}.png", bbox_inches='tight', pad_inches=0.5)
            plt.savefig(f"graphs/{key}_{dataset_name}_{compiler}.pdf", bbox_inches='tight', pad_inches=0.5)
            plt.close()


timings = [
    'sample_nanoseconds_per_key', 'prepare_nanoseconds_per_key', 'build_nanoseconds_per_key',
    'hashing_nanoseconds_per_key', 'total_nanoseconds_per_key']
for timing in timings:
    plot_timing(timing)
