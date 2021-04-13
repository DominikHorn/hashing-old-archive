import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
from pylab import cm
import pandas as pandas

csv = pandas.read_csv('throughput.csv')

# TODO: tmp
data = csv[csv['load_factor'] == 1.0] \
              [csv['dataset'] == "books_200M_uint32.ds"] \
              [csv['reducer'] != "do_nothing"]
rows = data.shape[0]
columns = data.shape[1]

reducers = list(set(data['reducer']))

# loosely taken from https://benalexkeen.com/bar-charts-in-matplotlib/
for i, reducer in enumerate(reducers):
    methods = data[data['reducer'] == reducer]
    width = 0.9 / methods.shape[0]
    for j, (method, ns) in enumerate(zip(list(methods['hash']), list(methods['nanoseconds_per_key']))):
        plt.bar(i + j * width, ns, width, label=method)

plt.ylabel('nanoseconds per key')
plt.title('Nanoseconds per key using different reducers')

plt.xticks(np.arange(len(reducers)) + 0.5, reducers)
plt.legend(loc="best")
plt.show()
