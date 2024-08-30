#!/usr/python
import subprocess

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root handle 1: tbf rate 10Mbit latency 5ms burst 1000k limit 1000k
'''
print(ban)
subprocess.call(["bash", "-c", ban])
JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

JQv2_bool = '''
./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
'''
print(JQv2_bool)
subprocess.call(["bash", "-c", JQv2_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root handle 1: tbf rate 15Mbit latency 5ms burst 1500k limit 1500k
'''
print(ban)
subprocess.call(["bash", "-c", ban])
JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

JQv2_bool = '''
./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
'''
print(JQv2_bool)
subprocess.call(["bash", "-c", JQv2_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root handle 1: tbf rate 20Mbit latency 5ms burst 2000k limit 2000k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

JQv2_bool = '''
./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
'''
print(JQv2_bool)
subprocess.call(["bash", "-c", JQv2_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root handle 1: tbf rate 30Mbit latency 5ms burst 3750k limit 3750k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

JQv2_bool = '''
./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
'''
print(JQv2_bool)
subprocess.call(["bash", "-c", JQv2_bool])

ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root handle 1: tbf rate 50Mbit latency 5ms burst 6250k limit 6250k
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

JQv2_bool = '''
./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
'''
print(JQv2_bool)
subprocess.call(["bash", "-c", JQv2_bool])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root handle 1: tbf rate 80Mbit latency 15ms burst 80000k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# # JQv2_bool = '''
# # ./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
# # '''
# # print(JQv2_bool)
# # subprocess.call(["bash", "-c", JQv2_bool])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root handle 1: tbf rate 100Mbit latency 15ms burst 100000k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.28.201
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# # JQv2_bool = '''
# # ./JQv2/bin/test_bool_ostriple  2 12345 172.31.28.201
# # '''
# # print(JQv2_bool)
# # subprocess.call(["bash", "-c", JQv2_bool])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root handle 1: tbf rate 200Mbit latency 15ms burst 200000k
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




