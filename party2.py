#!/usr/python
import subprocess

run = '''
python3 run_party2.py
'''
print(run)
subprocess.call(["bash", "-c", run])

run = '''
python3 bw_run_party2_jqv1_bool.py
'''
print(run)
subprocess.call(["bash", "-c", run])

run = '''
python3 bw_run_party2.py
'''
print(run)
subprocess.call(["bash", "-c", run])