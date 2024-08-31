#!/usr/python
import subprocess

shell = '''
cd JQv1
sudo bash setup.sh
bash install2.sh
cd /home/ubuntu/JesseQ/batchman
'''
print(shell)
subprocess.call(["bash", "-c", shell])

shell = '''
cd JQv2
sudo bash setup.sh
bash install2.sh
cd /home/ubuntu/JesseQ/batchman
'''
print(shell)
subprocess.call(["bash", "-c", shell])

shell = '''
cd stacking-vole-zk
sudo bash setup.sh
bash install2.sh
cd /home/ubuntu/JesseQ/batchman
'''
print(shell)
subprocess.call(["bash", "-c", shell])