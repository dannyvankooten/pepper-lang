#!/usr/bin/python
import pandas as pd
import matplotlib as mlp 
df = pd.read_csv("benchmarks.csv")
ax = df['Fib 35'].plot(grid=True, figsize=(12, 5), title="Benchmark (fib 35, recursive function)", marker='o')
ax.set_xlabel("Commit")
ax.set_ylabel("Runtime (s)")
mlp.pyplot.savefig('benchmarks.jpg')