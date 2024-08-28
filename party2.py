#!/usr/python
import subprocess

run = '''
python3 run_party2.py
'''
print(run)
subprocess.call(["bash", "-c", run])

run = '''
python3 app_run_party2.py
'''
print(run)
subprocess.call(["bash", "-c", run])

run = '''
python3 bw_run_party2.py
'''
print(run)
subprocess.call(["bash", "-c", run])