#!/usr/bin/python
import pandas as pd
import matplotlib as mlp 
import os

benchmark_dir = os.path.dirname(os.path.realpath(__file__)) 

df = pd.read_csv(benchmark_dir + "/data.csv", index_col=0)
df['Label'] = df.index + ' \n(' + df['Commit'] + ')'
df.index = df['Label']
ax = df[['Fib 35']].plot(grid=True, figsize=(12, 5), title="Benchmark (fib 35, recursive function)", marker='o')
ax.set_ylabel("Runtime (s)")
ax.set_xlabel("")
mlp.pyplot.xticks(rotation=8, ha='right')
mlp.pyplot.savefig(benchmark_dir + '/chart.jpg')