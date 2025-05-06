#!/usr/python
import subprocess



shell = '''
./stacking-vole-zk/build/bin/test_arith_stack_batched_matmul_v1 1 12345 0 16 512 1024
'''
print(shell)
subprocess.call(["bash", "-c", shell])

shell = '''
./JQv1/build/bin/test_arith_stack_batched_matmul_v1 1 12345 0 16 512 1024
'''
print(shell)
subprocess.call(["bash", "-c", shell])

shell = '''
./JQv2/build/bin/test_arith_stack_batched_matmul_v1 1 12345 0 16 512 1024
'''
print(shell)
subprocess.call(["bash", "-c", shell])