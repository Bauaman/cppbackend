import argparse
import subprocess
import time
import random
import shlex
import signal
import os

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    args = parser.parse_args()

    args.server = os.path.expanduser(args.server)
    return args.server


def run(command, output=None, inp=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL, stdin=inp)
    return process


def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


server = run(start_server())
pf = run("perf record -g -o perf.data -p " + str(server.pid))
time.sleep(1)
make_shots()
time.sleep(1)
pf.send_signal(signal.SIGINT)
time.sleep(1)
stop(pf, wait = True)
time.sleep(1)
stop(server)
p1 = run("perf script -i perf.data", output=subprocess.PIPE)
p2 = run("./FlameGraph/stackcollapse-perf.pl", output=subprocess.PIPE, inp=p1.stdout)
fout = open('graph.svg', 'wt')
p3 = run("./FlameGraph/flamegraph.pl", output = fout, inp=p2.stdout)

stop(p1, wait = True)
stop(p2, wait = True)
stop(p3, wait = True)
time.sleep(1)
print('Job done')


# python3 shoot.py "/cppbackend/sprint1/problems/final_task/solution/build/bin/game_server //cppbackend/sprint1/problems/final_task/solution/data/config.json" || true