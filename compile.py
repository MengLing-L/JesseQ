#!/usr/python
import subprocess

install_template = '''
cd X
cmake .
make -j4
sudo make install
cd ..
'''

import argparse
parser = argparse.ArgumentParser()
parser.add_argument('-install', '--install', action='store_true')
parser.add_argument('-deps', '--deps', action='store_true')
parser.add_argument('--tool', nargs='?', const='master')
parser.add_argument('--ot', nargs='?', const='master')
parser.add_argument('--sh2pc', nargs='?', const='master')
parser.add_argument('--ag2pc', nargs='?', const='master')
parser.add_argument('--agmpc', nargs='?', const='master')
parser.add_argument('--zk', nargs='?', const='master')
parser.add_argument('--JQv1', nargs='?', const='master')
parser.add_argument('--JQv2', nargs='?', const='master')
parser.add_argument('--ROMLPZK', nargs='?', const='master')
args = parser.parse_args()


for k in ['tool', 'ot', 'zk', 'sh2pc', 'ag2pc', 'agmpc']:
	if vars(args)[k]:
		template = install_template.replace("X", "emp-"+k).replace("Y", vars(args)[k])
		print(template)
		subprocess.call(["bash", "-c", template])

JQ_install_template = '''
cd X
cmake .
make -j4
sudo make install
cd ..
'''

for k in ['JQv1','JQv2','ROMLPZK']:
	if vars(args)[k]:
		template = JQ_install_template.replace("X", k)
		print(template)
		subprocess.call(["bash", "-c", template])
