#!/usr/python
import subprocess

ban = '''
sudo tc qdisc add dev ens5 root tbf rate 500Mbit latency 15ms burst 937.5k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

inner = '''
./ROMLPZK/bin/test_arith_inner_prdt 2 12345 172.31.24.125 10240 1000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

inner = '''
./ROMLPZK/bin/test_arith_inner_prdt 2 12345 172.31.24.125 1024 10000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

inner = '''
./ROMLPZK/bin/test_arith_inner_prdt 2 12345 172.31.24.125 1 100000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

mat = '''
 ./ROMLPZK/bin/test_arith_matrix_mul 2 12345 172.31.24.125
'''
print(mat)
subprocess.call(["bash", "-c", mat])

sis = '''
 ./ROMLPZK/bin/test_arith_sis 2 12345 172.31.24.125
'''
print(sis)
subprocess.call(["bash", "-c", sis])

ban = '''
sudo tc qdisc del dev ens5 root
'''
print(ban)
subprocess.call(["bash", "-c", ban])




