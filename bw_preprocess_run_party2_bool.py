#!/usr/python
import subprocess


ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root handle 1: tbf rate 20Mbit latency 5ms burst 2000k 
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_arith = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])


JQv2_arith = '''
./JQv2/bin/test_bool_ostriple  2 12345 172.31.2.131
'''
print(JQv2_arith)
subprocess.call(["bash", "-c", JQv2_arith])

# QS_arith = '''
# ./emp-zk/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
# '''
# print(QS_arith)
# subprocess.call(["bash", "-c", QS_arith])



ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root handle 1: tbf rate 30Mbit latency 5ms burst 3750k 
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_arith = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

# QS_arith = '''
# ./emp-zk/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
# '''
# print(QS_arith)
# subprocess.call(["bash", "-c", QS_arith])

JQv2_arith = '''
./JQv2/bin/test_bool_ostriple  2 12345 172.31.2.131
'''
print(JQv2_arith)
subprocess.call(["bash", "-c", JQv2_arith])


ban = '''
sudo tc qdisc del dev ens5 root
sudo tc qdisc add dev ens5 root handle 1: tbf rate 50Mbit latency 5ms burst 6250k 
'''
print(ban)
subprocess.call(["bash", "-c", ban])

JQv1_arith = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

# QS_arith = '''
# ./emp-zk/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
# '''
# print(QS_arith)
# subprocess.call(["bash", "-c", QS_arith])

JQv2_arith = '''
./JQv2/bin/test_bool_ostriple  2 12345 172.31.2.131
'''
print(JQv2_arith)
subprocess.call(["bash", "-c", JQv2_arith])

# ban = '''
# sudo tc qdisc del dev ens5 root
# sudo tc qdisc add dev ens5 root tbf rate 20Mbit latency 15ms burst 37.5k
# '''
# print(ban)
# subprocess.call(["bash", "-c", ban])

# JQv1_bool = '''
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  2 12345 172.31.2.131
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
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  2 12345 172.31.2.131
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
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  2 12345 172.31.2.131
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
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  2 12345 172.31.2.131
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
# ./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.2.131
# '''
# print(JQv1_bool)
# subprocess.call(["bash", "-c", JQv1_bool])

# JQv2_bool = '''
# ./JQv2/bin/test_bool_ostriple  2 12345 172.31.2.131
# '''
# print(JQv2_bool)
# subprocess.call(["bash", "-c", JQv2_bool])

ban = '''
sudo tc qdisc del dev ens5 root
'''
print(ban)
subprocess.call(["bash", "-c", ban])


