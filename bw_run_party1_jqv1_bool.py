#!/usr/python
import subprocess

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 10Mbit latency 15ms burst 75k
'''
print(ban)
subprocess.call(["bash", "-c", ban])
JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 1 12345 0
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 20Mbit latency 15ms burst 75k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 1 12345 0
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 30Mbit latency 15ms burst 75k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 1 12345 0
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 50Mbit latency 15ms burst 112.5k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 1 12345 0
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])


ban = '''
sudo tc qdisc del dev ens5 root
'''
print(ban)
subprocess.call(["bash", "-c", ban])




