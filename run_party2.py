#!/usr/python
import subprocess

JQv1_arith = '''
./JQv1/bin/test_arith_circuit_scalability 2 12345 172.31.63.48
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

JQv1_bool = '''
./JQv1/bin/test_bool_circuit_scalability 2 12345 172.31.63.48
'''
print(JQv1_bool)
subprocess.call(["bash", "-c", JQv1_bool])

JQv2_arith = '''
./JQv2/bin/test_arith_ostriple 2 12345 172.31.63.48
'''
print(JQv2_arith)
subprocess.call(["bash", "-c", JQv2_arith])

JQv2_bool = '''
./JQv2/bin/test_bool_ostriple  2 12345 172.31.63.48
'''
print(JQv2_bool)
subprocess.call(["bash", "-c", JQv2_bool])


