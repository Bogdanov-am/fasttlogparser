import fasttlogparser
import pandas as pd
from pymavlog import MavTLog
import timeit

file = "logs/2024-07-15 07-49-51.tlog"

def parse_1():
    log, ids = fasttlogparser.parseTLog(file, blacklist=["AUTOPILOT_VERSION","FILE_TRANSFER_PROTOCOL","HOME_POSITION"])
    sum = 0
    dfs: dict[str, pd.DataFrame] = {}
    for msg in log:
        df = pd.DataFrame(log[msg])
        dfs[msg] = df
        sum += df.memory_usage(index=False).sum()
    del log
    return sum

def parse_2():
    tlog = MavTLog(file, ["AUTOPILOT_VERSION","FILE_TRANSFER_PROTOCOL","HOME_POSITION"])
    print('parse_2')
    tlog.parse()
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
result1_mem = parse_1()
result2_mem = parse_2()

print("MavTLog - {:.5f}s / {:.2f}KB".format(result2,result2_mem/1024/1024))
print("fasttlogparser - {:.5f}s / {:.2f}KB".format(result1,result1_mem/1024/1024))
print("Time coeff - {:.1f}".format(result2/result1))
print("Memory coeff - {:.1f}".format(result2_mem/result1_mem))
