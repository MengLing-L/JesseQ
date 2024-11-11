#!/usr/python
import subprocess

JQv1_arith = '''
./JQv1/bin/test_arith_mul_hash_performance 2 12345 127.0.0.1 300000 100000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

JQv1_arith = '''
./JQv1/bin/test_arith_mul_hash_performance 2 12345 127.0.0.1 3000000 1000000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

JQv1_arith = '''
./JQv1/bin/test_arith_mul_hash_performance 2 12345 127.0.0.1 30000000 10000000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

JQv1_arith = '''
./JQv1/bin/test_arith_mul_hash_performance 2 12345 127.0.0.1 300000000 100000000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

JQv1_arith = '''
./JQv1/bin/test_bool_mul_hash_performance 2 12345 127.0.0.1 300000 100000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

JQv1_arith = '''
./JQv1/bin/test_bool_mul_hash_performance 2 12345 127.0.0.1 3000000 1000000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

JQv1_arith = '''
./JQv1/bin/test_bool_mul_hash_performance 2 12345 127.0.0.1 30000000 10000000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])

JQv1_arith = '''
./JQv1/bin/test_bool_mul_hash_performance 2 12345 127.0.0.1 300000000 100000000
'''
print(JQv1_arith)
subprocess.call(["bash", "-c", JQv1_arith])


