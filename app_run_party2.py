#!/usr/python
import subprocess

ban = '''
sudo tc qdisc add dev ens5 root tbf rate 500Mbit latency 15ms burst 937.5k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

inner = '''
./JQv2/bin/test_arith_inner_prdt 2 12345 172.31.2.131 10240 1000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

inner = '''
./JQv2/bin/test_arith_inner_prdt 2 12345 172.31.2.131 1024 10000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

inner = '''
./JQv2/bin/test_arith_inner_prdt 2 12345 172.31.2.131 1 100000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

mat = '''
 ./JQv2/bin/test_arith_matrix_mul 2 12345 172.31.2.131
'''
print(mat)
subprocess.call(["bash", "-c", mat])

sis = '''
 ./JQv2/bin/test_arith_sis 2 12345 172.31.2.131
'''
print(sis)
subprocess.call(["bash", "-c", sis])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 20Mbit latency 15ms burst 37.5k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

inner = '''
./JQv2/bin/test_bool_inner_prdt 2 12345 172.31.2.131 10240 1000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

inner = '''
./JQv2/bin/test_bool_inner_prdt 2 12345 172.31.2.131 1024 10000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

inner = '''
./JQv2/bin/test_bool_inner_prdt 2 12345 172.31.2.131 1 100000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

ban = '''
sudo tc qdisc del dev ens5 root
'''
print(ban)
subprocess.call(["bash", "-c", ban])




