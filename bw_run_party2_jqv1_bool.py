#!/usr/python
import subprocess

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 20Mbit latency 15ms burst 37.5k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.24.125
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])


ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 40Mbit latency 15ms burst 75k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.24.125
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])


ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 60Mbit latency 15ms burst 112.5k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.24.125
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 80Mbit latency 15ms burst 150k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.24.125
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])


ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 100Mbit latency 15ms burst 187.5k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.24.125
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 200Mbit latency 15ms burst 375k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.24.125
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 400Mbit latency 15ms burst 750k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.24.125
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 600Mbit latency 15ms burst 1125k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.24.125
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])


ban = '''
sudo tc qdisc del dev ens5 root
'''
print(ban)
subprocess.call(["bash", "-c", ban])


