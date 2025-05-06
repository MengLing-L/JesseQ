#!/usr/python
import subprocess


inner = '''
./JQv2/bin/test_arith_inner_prdt 2 12345 127.0.0.1 10240 1000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

inner = '''
./JQv2/bin/test_arith_inner_prdt 2 12345 127.0.0.1 1024 10000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

inner = '''
./JQv2/bin/test_arith_inner_prdt 2 12345 127.0.0.1 1 100000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

mat = '''
 ./JQv2/bin/test_arith_matrix_mul 2 12345 127.0.0.1
'''
print(mat)
subprocess.call(["bash", "-c", mat])

sis = '''
 ./JQv2/bin/test_arith_sis 2 12345 127.0.0.1
'''
print(sis)
subprocess.call(["bash", "-c", sis])


inner = '''
./JQv2/bin/test_bool_inner_prdt 2 12345 127.0.0.1 10240 1000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

inner = '''
./JQv2/bin/test_bool_inner_prdt 2 12345 127.0.0.1 1024 10000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])

inner = '''
./JQv2/bin/test_bool_inner_prdt 2 12345 127.0.0.1 1 100000000
'''
print(inner)
subprocess.call(["bash", "-c", inner])





