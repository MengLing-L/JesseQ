#!/usr/python
import subprocess


ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 800Mbit latency 15ms burst 1500k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 1000Mbit latency 15ms burst 1875k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 2000Mbit latency 15ms burst 3750k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 5000Mbit latency 15ms burst 9375k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])


ban = '''
sudo tc qdisc del dev ens5 root
'''
print(ban)
subprocess.call(["bash", "-c", ban])


