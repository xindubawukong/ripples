import os
import re
import subprocess


def run_ripples(log_path, graph, eps, workers, w, iter):
    n_threads = [192, 96, 48, 24, 12, 8, 4, 2, 1]
    cores = ['0-191', '0-95', '0-95:2', '0-95:4', '0-47:4', '0-31:4', '0-15:4', '0-7:4', '0-3:4']
    for i in range(len(n_threads)):
        n_thread = n_threads[i]
        core = cores[i]
        numa = 'numactl -i all' if n_thread > 1 else ''
        workers = min(workers, n_thread)
        p = w
        k = 100
        graph_path = f'/data0/graphs/links/{graph}.bin'
        if not os.path.exists(graph_path):
            graph_path = f'/data0/lwang323/graph/bin/{graph}.bin'
        # graph_path = f'./edgelists2/{graph}-sorted-edge-list.txt'
        command = f'./build/release/tools/imm -i {graph_path} -u --distribution normal --mean {p} --variance 0.0 --disable-renumbering -k {k} -d IC -e {eps} -o output.txt -p --seed-select-max-workers {workers}'
        name = f'{graph}_eps_{eps}_workers_{workers}_nthreads_{n_thread}'
        logfile = f'./{log_path}/{name}.txt'
        memfile = f'./{log_path}/{name}_mem.txt'
        resfile = f'./{log_path}/{name}_res.txt'
        if os.path.exists(resfile):
            continue
        print(f'Runing ripples on {graph}, eps={eps}, workers={workers}, w={w}, n_thread={n_thread}, core={core}')
        subprocess.call(
            f'OMP_NUM_THREADS={n_thread} /usr/bin/time -v taskset -c {core} {numa} {command} 1>> {logfile} 2>> {memfile}', shell=True)
        print(f'evaluating seeds influence on {graph}')
        subprocess.call(
            f'/home/xding9001/IM/general_cascade {graph_path} {logfile} -i {iter} -w {w} >> {resfile}', shell=True)


if __name__ == '__main__':
    path = 'logs_temp'
    subprocess.call(f'mkdir -p {path}', shell=True)

    aa = [
        ('HepPh_sym', 0.02, [2], 20000),
        # ('Epinions1_sym', 0.02, [4], 20000),
        # ('Slashdot_sym', 0.02, [8], 20000),
        # ('DBLP_sym', 0.02, [16], 20000),
        # ('Youtube_sym', 0.02, [16], 5000),
        # ('com-orkut_sym', 0.02, [16], 2000),
        # ('soc-LiveJournal1_sym', 0.02, [16], 2000),
        # ('HT_5_sym', 0.2, [16], 20000),
        # ('Household.lines_5_sym', 0.2, [16], 20000),
        # ('CHEM_5_sym', 0.2, [16], 20000),
        # ('GeoLifeNoScale_5_sym', 0.2, [16], 5000),
        # ('grid_1000_10000_sym', 0.2, [16], 20000),
        # ('grid_1000_10000_03_sym', 0.2, [16], 20000),
        # ('twitter_sym', 0.02, [16], 1000),
        # ('Germany_sym', 0.2, [16], 20000),
        # ('RoadUSA_sym', 0.2, [16], 20000),
    ]

    for eps in [0.5]:
        for graph, w, workers_list, iter in aa:
            for workers in workers_list:
                run_ripples(log_path=path, graph=graph,
                            eps=eps, workers=workers, w=w, iter=iter)
