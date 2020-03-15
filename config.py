#!/usr/bin/python

import sys
import os
import argparse
import subprocess
import platform

def get_base():
    for i in range(10):
        if os.path.exists('.git'):
            return os.getcwd()
        os.chdir('..')

    raise Exception('Failed to find project root directory')

def binary():
    return 'gbc.exe' if platform.system() == 'Windows' else './gbc'

def cmd(command):
    p = subprocess.Popen(command.split(' '))
    p.wait()

    return p.returncode

def qmake():
    sys.exit(cmd('qmake -r CONFIG+=debug_and_release'))

def clean():
    sys.exit(cmd('make clean'))

def init():
    sys.exit(cmd('git clean -ffxd'))

def make(dbg):
    sys.exit(cmd('make -j8{0}'.format(' debug' if dbg else '')))

def test(spec):
    os.chdir(os.path.join('public', spec, 'bin'))
    sys.exit(cmd('./unittest'))

def gdb():
    os.chdir(os.path.join('public', 'debug', 'bin'))

    sys.exit(cmd('gdb -tui {0}'.format(binary())))

def valgrind(rom):
    os.chdir(os.path.join('public', 'release', 'bin'))

    sys.exit(cmd('valgrind --tool=callgrind {0} {1}'.format(binary(), rom)))

def run(spec, rom=''):
    os.chdir(os.path.join('public', spec, 'bin'))

    command = binary()
    if '' != rom:
        command += ' ' + rom
    sys.exit(cmd(command))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='General build script')
    parser.add_argument('-p', dest='path', action='store_true')
    parser.add_argument('-q', dest='qmake', action='store_true')
    parser.add_argument('-c', dest='clean', action='store_true')
    parser.add_argument('-i', dest='init', action='store_true')
    parser.add_argument('-b', dest='target', nargs=1, choices=['debug', 'release'])
    parser.add_argument('-t', dest='test', nargs=1, choices=['debug', 'release'])
    parser.add_argument('-d', dest='debug', action='store_true')
    parser.add_argument('-v', dest='valgrind', nargs=1)
    parser.add_argument('-r', dest='run', nargs='*')

    os.environ['LD_PRELOAD'] = '/usr/lib/gcc/x86_64-linux-gnu/7/libasan.so'

    base = get_base()
    os.chdir(base)

    args = parser.parse_args()
    if args.path:
        print (os.getcwd())
    elif args.qmake:
        os.chdir('src')
        qmake()
    elif args.clean:
        os.chdir('src')
        clean()
    elif args.init:
        init()
    elif args.debug:
        gdb()
    elif not args.valgrind is None:
        valgrind(args.valgrind[0])
    elif not args.test is None:
        test(args.test[0].lower())
    elif not args.target is None:
        os.chdir('src')
        make((args.target[0].lower() == 'debug'))
    elif not args.run is None:
        run(*args.run)
