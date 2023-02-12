import os
import re
import subprocess


def run_ripples(graph, eps=0.1, workers=2):
    print(f'Runing ripples on {graph}, eps={eps}, workers={workers}')
    p = 0.02
    k = 100
    command = f'./build/release/tools/imm -i ./edgelists/{graph}-sorted-edge-list.txt -u -w --distribution normal --mean {p} --variance 0.0 --disable-renumbering -k {k} -d IC -e {eps} -o output.txt -p --seed-select-max-workers {workers}'
    name = f'ripples_{graph}_eps_{eps}_workers_{workers}'
    logfile = f'./logs/{name}.txt'
    memfile = f'./logs/{name}_mem.txt'
    subprocess.call(
        f'/usr/bin/time -v numactl -i all {command} 1>> {logfile} 2>> {memfile}', shell=True)


if __name__ == '__main__':
    subprocess.call('mkdir -p logs', shell=True)

    aa = [
        # ('HepPh_sym', [1,2,4,8,16,32,64]),
        ('DBLP_sym', [32]),
        # ('Epinions1_sym', [1,2,4,8,16,32,64]),
        # ('Slashdot_sym', [2,4,8,16,32,64]),
        # ('DBLP_sym', [4,8,16,32,64,128]),
        # ('Youtube_sym', [8,16,32,64,128,192]),
        # ('Household.lines_5_sym', [16,32,64,128,192]),
    ]
    eps = 0.1

    for graph, workers_list in aa:
        for workers in workers_list:
            run_ripples(graph, eps=eps, workers=workers)
