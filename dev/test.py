import fasttlogparser
import pandas as pd
from pymavlog import MavTLog
import timeit

file = "../logs/2024-09-27 10-58-56.tlog"

def parse_1():
    log = fasttlogparser.parseTLog(file, whitelist=["HEARTBEAT"])
    
    sum = 0
    dfs: dict[str, pd.DataFrame] = {}
    for msg in log:
        df = pd.DataFrame(log[msg])
        dfs[msg] = df
        sum += df.memory_usage(index=False).sum()
    del log
    return sum

def parse_2():
    tlog = MavTLog(file, [],
                ["HEARTBEAT"],
                map_columns={'flags': 'flags_ekf'})

    tlog.parse([])
    dfs: dict[str, pd.DataFrame] = {}
    sum = 0
    for type in tlog.types:
        af = {}
        log = tlog.get(type)
        if not log:
            continue
        for column in log.columns:
            af[column] = log.raw_fields[column]
        df = pd.DataFrame(af)
        dfs[type] = df
        sum += df.memory_usage(index=False).sum()
    return sum

result1 = timeit.timeit(parse_1, number=5)
result2 = timeit.timeit(parse_2, number=5)

print("Time coeff:", result2/result1)

result1_mem = parse_1()
result2_mem = parse_2()

print("Memory coeff:", result2_mem/result1_mem)
