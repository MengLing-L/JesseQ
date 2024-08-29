#!/usr/python
import subprocess

run = '''
python3 run_party1.py
'''
print(run)
subprocess.call(["bash", "-c", run])

run = '''
python3 bw_run_party1_jqv1_bool.py
'''
print(run)
subprocess.call(["bash", "-c", run])

run = '''
python3 bw_run_party1.py
'''
print(run)
subprocess.call(["bash", "-c", run])