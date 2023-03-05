import re
import os


def main():
    aa = [
        ('HepPh_sym', 0.02, [2], 20000),
        ('Epinions1_sym', 0.02, [4], 20000),
        ('Slashdot_sym', 0.02, [8], 20000),
        ('DBLP_sym', 0.02, [16], 20000),
        ('Youtube_sym', 0.02, [16], 5000),
        ('com-orkut_sym', 0.02, [16], 2000),
        ('soc-LiveJournal1_sym', 0.02, [16], 2000),
        ('HT_5_sym', 0.2, [16], 20000),
        ('Household.lines_5_sym', 0.2, [16], 20000),
        ('CHEM_5_sym', 0.2, [16], 20000),
        ('GeoLifeNoScale_5_sym', 0.2, [16], 5000),
        # ('grid_1000_10000_sym', 0.2, [16], 20000),
        # ('grid_1000_10000_03_sym', 0.2, [16], 20000),
        # ('twitter_sym', 0.02, [16], 1000),
        # ('Germany_sym', 0.2, [16], 20000),
        # ('RoadUSA_sym', 0.2, [16], 20000),
    ]
    res = {}
    log_path = 'logs_0304'
    ff = open('temp.txt', 'w')
    for graph, w, workers_list, iter in aa:
        for workers in workers_list:
            eps = 0.5
            a = {}
            for n_thread in [192, 96, 48, 24, 12, 8, 4, 2, 1]:
                name = f'{graph}_eps_{eps}_workers_{min(workers, n_thread)}_nthreads_{n_thread}'
                logfile = f'./{log_path}/{name}.txt'
                memfile = f'./{log_path}/{name}_mem.txt'
                resfile = f'./{log_path}/{name}_res.txt'
                if not os.path.exists(resfile):
                    continue
                print('loading:', logfile, memfile, resfile)
                f1 = open(logfile, 'r')
                f2 = open(memfile, 'r')
                f3 = open(resfile, 'r')
                logs = f1.read()
                mems = f2.read()
                infs = f3.read()
                f1.close()
                f2.close()
                
                time_str = re.findall(
                    'IMM total: .* s', logs)[-1]
                time = float(time_str[time_str.find(':') + 1: -2])
                select_time_str = re.findall(
                    'select seeds: .* s', logs)[-1]
                select_time = float(select_time_str[select_time_str.find(':') + 1: -2])
                # print(time_str, time)
                # seeds_str = re.findall('seeds: .*\n', logs)[-1]
                # seeds = seeds_str[7:-1]
                # print(seeds)
                
                mem_str = re.findall(
                    'Maximum resident set size \(kbytes\): .*\n', mems)[-1]
                mem = int(mem_str[mem_str.find(':') + 1: -1])
                # print(mem_str, mem)

                inf = list(filter(lambda x: len(x) > 0, infs.split('\n')))[-1]
                inf = float(inf)
                # print(inf)
                a[n_thread] = (time, select_time, time - select_time, mem, inf)
            if len(a) > 0:
                id = 2
                res = f'{graph}'
                for n_thread in [192, 96, 48, 24, 12, 8, 4, 2, 1]:
                    t = ''
                    if n_thread in a:
                        t = a[n_thread][id]
                    res += ' ' + str(t)
                ff.write(res + '\n')


if __name__ == '__main__':
    main()
