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

def cmd(command)
    p = subprocess.Popen(command.split(' '))
    p.wait()

def qmake():
    cmd('qmake -r CONFIG+=debug_and_release')

def clean():
    cmd('make clean')

def init():
    cmd('git clean -ffxd')

def make(dbg):
    cmd('make -j8{0}'.format(' debug' if dbg else ''))

def run(spec, rom):
    os.chdir(os.path.join('public', os.path.join(spec, 'bin')))

    binary = 'gbc.exe' if platform.system() == 'Windows' else './gbc'
    cmd('{0} {1}'.format(binary, rom))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='General build script')
    parser.add_argument('-p', dest='path', action='store_true')
    parser.add_argument('-q', dest='qmake', action='store_true')
    parser.add_argument('-c', dest='clean', action='store_true')
    parser.add_argument('-i', dest='init', action='store_true')
    parser.add_argument('-b', dest='target', nargs=1, choices=['debug', 'release'])
    parser.add_argument('-r', dest='run', nargs=2)

    base = get_base()
    os.chdir(base)

    args = parser.parse_args()
    if args.path:
        print (os.getcwd())
    elif args.qmake:
        qmake()
    elif args.clean:
        clean()
    elif args.init:
        init()
    elif not args.target is None:
        os.chdir('src')
        make((args.target[0].lower() == 'debug'))
    elif not args.run is None:
        run(*args.run)
