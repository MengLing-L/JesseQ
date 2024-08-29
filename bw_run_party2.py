#!/usr/python
import subprocess

ban = '''
sudo tc qdisc add dev ens5 root tbf rate 100Mbit latency 15ms burst 375k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_arith = '''
./JQv1/bin/test_arith_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 500Mbit latency 15ms burst 1125k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_arith = '''
./JQv1/bin/test_arith_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])


ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 1000Mbit latency 15ms burst 1875k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_arith = '''
./JQv1/bin/test_arith_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])


ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 2000Mbit latency 15ms burst 20000k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_arith = '''
./JQv1/bin/test_arith_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root tbf rate 20Mbit latency 15ms burst 37.5k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root tbf rate 40Mbit latency 15ms burst 75k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root tbf rate 60Mbit latency 15ms burst 112.5k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root tbf rate 80Mbit latency 15ms burst 150k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root tbf rate 100Mbit latency 15ms burst 187.5k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])

ban = '''
sudo tc qdisc del dev ens5 root
'''
print(ban)
subprocess.call(["bash", "-c", ban])


