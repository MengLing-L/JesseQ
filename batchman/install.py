#!/usr/python
import subprocess

shell = '''
cd JQv1
bash setup.sh
bash install2.sh
cd ..
'''
print(shell)
subprocess.call(["bash", "-c", shell])

shell = '''
cd JQv2
bash setup.sh
bash install2.sh
cd ..
'''
print(shell)
subprocess.call(["bash", "-c", shell])

shell = '''
cd stacking-vole-zk
bash setup.sh
bash install2.sh
cd ..
'''
print(shell)
subprocess.call(["bash", "-c", shell])