# JesseQ: Efficient Zero-Knowledge Proofs for Circuits over Any Field
This is the implementation for the paper [JesseQ: Efficient Zero-Knowledge Proofs for Circuits over Any Field](https://eprint.iacr.org/2025/533) to be presented on IEEE S&P 2025.

## Acknowledgment

We would like to acknowledge that our implementation is based on the QuickSilver repository (https://github.com/emp-toolkit/emp-zk) and the Batchman repository (https://github.com/gconeice/stacking-vole-zk). Specifically, we forked these repositories and built upon them for our development work. We also made some modifications to the EMP libraries.

## Installation
- `git clone git@github.com:MengLing-L/JesseQ.git`
- `cd JesseQ`
- `python3 install.py --deps --tool --ot --zk --JQv1 --JQv2`
- `cd JesseQ/batchman`
- `python3 install.py`

## Testing on Localhost

### JQv1 and JQv2
- `python3 run_party1.py`
- In a new terminal window, execute `python3 run_party2.py`

### Applications
- `python3 app_run_party1.py`
- In a new terminal window, execute `python3 app_run_party2.py`

### Hash and Multiplications
- `python3 mul_hash_party1.py`
- In a new terminal window, execute `python3 mul_hash_party2.py`

### QS-Batchman and JesseQ-Batchman
- `cd batchman`
- `python3 party1.py`
- In a new terminal window, execute `python3 party2.py`

## Testing on two

Update the IP address in these files:
- run_party2.py
- app_run_party2.py
- mul_hash_party2.py
- party2.py
  
Change:
127.0.0.1 → [Party 1's actual IP address]