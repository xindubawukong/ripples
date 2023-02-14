import re
import os


def main():
    res = {}
    for graph in ['HepPh_sym', 'Epinions1_sym', 'Slashdot_sym', 'DBLP_sym', 'Youtube_sym', 'com-orkut_sym', 'soc-LiveJournal1_sym',
                  'HT_5_sym', 'Household.lines_5_sym', 'CHEM_5_sym']:
        for eps in [0.13, 0.2, 0.3, 0.4, 0.5]:
            a = []
            for workers in [1, 2, 4, 8, 16, 32, 64, 128]:
                log_file = f'./logs_1000/ripples_{graph}_eps_{eps}_workers_{workers}.txt'
                mem_file = log_file[:-4] + '_mem.txt'
                if not os.path.exists(log_file):
                    continue
                # print(log_file, mem_file)
                try:
                    log_file = open(log_file)
                    mem_file = open(mem_file)
                    logs = log_file.read()
                    mems = mem_file.read()
                    time_str = re.findall(
                        'IMM Parallel Real Total : .*ms', logs)[-1]
                    time = float(time_str[time_str.find(':') + 1: -2]) / 1000
                    # print(time_str, time)
                    seeds_str = re.findall('seeds: .*\n', logs)[-1]
                    seeds = seeds_str[7:-1]
                    # print(seeds)
                    mem_str = re.findall(
                        'Maximum resident set size \(kbytes\): .*\n', mems)[-1]
                    mem = int(mem_str[mem_str.find(':') + 1: -1])
                    # print(mem_str, mem)
                    a.append((time, workers, mem, seeds))
                    log_file.close()
                    mem_file.close()
                except:
                    pass
            a = sorted(a)
            if len(a) > 0:
                a = a[0]
                print('')
                print(graph, eps)
                print(f'time: {a[0]}')
                print(f'workers: {a[1]}')
                print(f'mem: {a[2]}')
                print(f'seeds: {a[3]}')


if __name__ == '__main__':
    main()
