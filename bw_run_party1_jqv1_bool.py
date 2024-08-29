#!/usr/python
import subprocess

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 10Mbit latency 15ms burst 10000k
'''
print(ban)
subprocess.call(["bash", "-c", ban])
JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 1 12345 0
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  1 12345 0
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 20Mbit latency 15ms burst 15000k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 1 12345 0
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  1 12345 0
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 30Mbit latency 15ms burst 20000k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 1 12345 0
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  1 12345 0
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root tbf rate 50Mbit latency 15ms burst 25000k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 1 12345 0
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  1 12345 0
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root tbf rate 80Mbit latency 15ms burst 80000k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 1 12345 0
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# # JQv2_bool = '''
# # ./JQv2/bin/test_bool_ostriple  1 12345 0
# # '''
# # print(JQv2_bool)
# # subprocess.call(["bash", "-c", JQv2_bool])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root tbf rate 100Mbit latency 15ms burst 100000k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 1 12345 0
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# # JQv2_bool = '''
# # ./JQv2/bin/test_bool_ostriple  1 12345 0
# # '''
# # print(JQv2_bool)
# # subprocess.call(["bash", "-c", JQv2_bool])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root tbf rate 200Mbit latency 15ms burst 200000k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 1 12345 0
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  1 12345 0
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])



ban = '''
sudo tc qdisc del dev ens5 root
'''
print(ban)
subprocess.call(["bash", "-c", ban])




