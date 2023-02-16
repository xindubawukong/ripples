import os
import re
import subprocess


def run_ripples(log_path, graph, eps, workers, w):
    print(f'Runing ripples on {graph}, eps={eps}, workers={workers}, w={w}')
    p = w
    k = 100
    graph_path = f'/data0/graphs/links/{graph}.bin'
    # graph_path = f'./edgelists2/{graph}-sorted-edge-list.txt'
    command = f'./build/release/tools/imm -i {graph_path} -u --distribution normal --mean {p} --variance 0.0 --disable-renumbering -k {k} -d IC -e {eps} -o output.txt -p --seed-select-max-workers {workers}'
    name = f'ripples_{graph}_eps_{eps}_workers_{workers}'
    logfile = f'./{log_path}/{name}.txt'
    memfile = f'./{log_path}/{name}_mem.txt'
    subprocess.call(
        f'/usr/bin/time -v numactl -i all {command} 1>> {logfile} 2>> {memfile}', shell=True)


if __name__ == '__main__':
    path = 'logs_1000_more'
    subprocess.call(f'mkdir -p {path}', shell=True)

    aa = [
        # ('HepPh_sym', 0.02, [2]),
        # ('Epinions1_sym', 0.02, [4]),
        # ('Slashdot_sym', 0.02, [8]),
        # ('DBLP_sym', 0.02, [16]),
        # ('Youtube_sym', 0.02, [16]),
        # ('com-orkut_sym', 0.02, [16]),
        # ('soc-LiveJournal1_sym', 0.02, [16]),
        ('twitter_sym', 0.02, [16]),
        # ('RoadUSA_sym', 0.2, [16,32]),
        # ('HT_5_sym', 0.2, [16]),
        # ('Household.lines_5_sym', 0.2, [16]),
        # ('CHEM_5_sym', 0.2, [16]),
        # ('GeoLifeNoScale_5_sym', 0.2, [16]),
        # ('grid_1000_10000_sym', 0.2, [16]),
        # ('grid_1000_10000_03_sym', 0.2, [16]),
    ]

    for eps in [0.5]:
        for graph, w, workers_list in aa:
            for workers in workers_list:
                run_ripples(log_path=path, graph=graph,
                            eps=eps, workers=workers, w=w)
