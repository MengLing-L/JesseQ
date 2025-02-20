#!/usr/python
import subprocess


ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root handle 1: tbf rate 2000Mbit latency 5ms burst 200000k 
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_arith = '''
./JQv1/bin/test_arith_circuit_scalability_incre 1 12345 0 1000000 1000000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

JQv1_arith = '''
./JQv1/bin/test_arith_circuit_scalability_incre 1 12345 0 10000000 10000000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

JQv1_arith = '''
./JQv1/bin/test_arith_circuit_scalability_incre 1 12345 0 100000000 100000000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])



JQv2_arith = '''
./JQv2/bin/test_arith_ostriple_incre  1 12345 0  1000000 1
'''
print(JQv2_arith)
subprocess.call(["bash", "-c", JQv2_arith])

JQv2_arith = '''
./JQv2/bin/test_arith_ostriple_incre  1 12345 0  1000000 10
'''
print(JQv2_arith)
subprocess.call(["bash", "-c", JQv2_arith])

JQv2_arith = '''
./JQv2/bin/test_arith_ostriple_incre  1 12345 0  1000000 100
'''
print(JQv2_arith)
subprocess.call(["bash", "-c", JQv2_arith])


EMP_arith = '''
./emp-zk/bin/test_arith_circuit_scalability  1 12345 10000
'''
print(EMP_arith)
subprocess.call(["bash", "-c", EMP_arith])

EMP_arith = '''
./emp-zk/bin/test_arith_circuit_scalability  1 12345 100000
'''
print(EMP_arith)
subprocess.call(["bash", "-c", EMP_arith])

EMP_arith = '''
./emp-zk/bin/test_arith_circuit_scalability  1 12345 1000000
'''
print(EMP_arith)
subprocess.call(["bash", "-c", EMP_arith])


ban = '''
sudo tc qdisc del dev ens5 root
'''
print(ban)
subprocess.call(["bash", "-c", ban])


