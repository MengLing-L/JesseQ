#!/usr/python
import subprocess

shell = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root handle 1: tbf rate 100Mbit burst 10000 limit 1000
sudo tc qdisc add dev ens5 parent 1:1 handle 10: netem delay 2msec
'''
print(shell)
subprocess.call(["bash", "-c", shell])

shell = '''
./stacking-vole-zk/build/bin/test_arith_stack_batched_matmul_v1 2 12345 172.31.27.223 16 512 1024
'''
print(shell)
subprocess.call(["bash", "-c", shell])

shell = '''
./JQv1/build/bin/test_arith_stack_batched_matmul_v1 2 12345 172.31.27.223 16 512 1024
'''
print(shell)
subprocess.call(["bash", "-c", shell])

shell = '''
./JQv2/build/bin/test_arith_stack_batched_matmul_v1 2 12345 172.31.27.223 16 512 1024
'''
print(shell)
subprocess.call(["bash", "-c", shell])