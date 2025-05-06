#!/usr/python
import subprocess

# keygen = '''
# ssh-keygen
# cat ~/.ssh/*.pub
# '''
# print(keygen)
# subprocess.call(["bash", "-c", keygen])


install_packages = '''
if [ "$(uname)" == "Darwin" ]; then
	brew list openssl || brew install openssl
	brew list blake3 || brew install blake3
 	brew list pkg-config || brew install pkg-config
 	brew list cmake || brew install cmake
	brew list gmp || brew install gmp
else
    if command -v apt-get >/dev/null; then
        sudo apt-get install -y software-properties-common
        sudo apt-get update
        sudo apt-get install -y cmake git build-essential libssl-dev libgmp3-dev
		git clone https://github.com/BLAKE3-team/BLAKE3.git
		cd BLAKE3/c/
		cmake -DBUILD_SHARED_LIBS=ON .
		make -j4
		sudo make install
    elif command -v yum >/dev/null; then
        sudo yum install -y python3 gcc make git cmake gcc-c++ openssl-devel epel-release blake3
    else
        echo "System not supported yet!"
    fi
fi
'''

install_template = '''
git clone https://github.com/emp-toolkit/X.git --branch Y
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
args = parser.parse_args()

if vars(args)['install'] or vars(args)['deps']:
	subprocess.call(["bash", "-c", install_packages])

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

for k in ['JQv1','JQv2']:
	if vars(args)[k]:
		template = JQ_install_template.replace("X", k)
		print(template)
		subprocess.call(["bash", "-c", template])

