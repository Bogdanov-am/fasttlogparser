"""
Performance comparison script for two telemetry log parsers: fasttlogparser and pymavlog.MavTLog.
Parses a telemetry log file using both methods, measures execution time and memory usage,
and prints a summary of the results.
"""

import timeit

import pandas as pd
from pymavlog import MavTLog
import fasttlogparser


FILE = "dev/bigtlog.tlog"


def parse_1():
    """
    Parse the telemetry log using fasttlogparser.parseTLog.
    Returns the total memory usage of all parsed message DataFrames (in bytes).
    """
    log, _ = fasttlogparser.parseTLog(FILE)
    sum_memory = 0
    dfs: dict[str, pd.DataFrame] = {}
    for msg in log:
        df = pd.DataFrame(log[msg])
        dfs[msg] = df
        sum_memory += df.memory_usage(index=False).sum()
    del log
    return sum_memory


def parse_2():
    """
    Parse the telemetry log using pymavlog.MavTLog.
    Returns the total memory usage of all parsed message DataFrames (in bytes).
    """
    tlog = MavTLog(FILE)
    tlog.parse()
    dfs: dict[str, pd.DataFrame] = {}
    sum_memory = 0
    for field_type in tlog.types:
        af = {}
        log = tlog.get(field_type)
        if not log:
            continue
        for column in log.columns:
            af[column] = log.raw_fields[column]
        df = pd.DataFrame(af)
        dfs[field_type] = df
        sum_memory += df.memory_usage(index=False).sum()
    return sum_memory


COUNT = 50
result1 = timeit.timeit(parse_1, number=COUNT)
result2 = timeit.timeit(parse_2, number=COUNT)
result1_mem = parse_1()
result2_mem = parse_2()

print(f"MavTLog - {result2 / COUNT:.5f}s / {result2_mem / 1024 / 1024:.2f}KB")
print(f"fasttlogparser - {result1 / COUNT:.5f}s / {result1_mem / 1024 / 1024:.2f}KB")
print(f"Time coeff - {result2 / result1:.1f}")
print(f"Memory coeff - {result2_mem / result1_mem:.1f}")
