
#include "emp-zk/emp-zk-arith/ostriple.h"
#include "emp-tool/emp-tool.h"
#include <iostream>
using namespace emp;
using namespace std;

int port, party;
const int threads = 1;

int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  NetIO *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i);

  FpOSTriple<NetIO> os(party, threads, ios);
  int len = 1024 * 1024 * 10 * 10 * 3;

  __uint128_t* a = new __uint128_t[len + 1];

  for (int i = 0; i < len; ++i) {
    a[i] = os.random_val_input();
  }
  a[len] = os.random_val_input();

  auto start = clock_start();
    for (int i = 0; i < len; ++i) { 
        mult_mod(a[i], a[i + 1]);
    }
  
  cout << party << "\tMul Speed: \t" << (time_from(start) * 7)/1000000 << "ms \t" << endl;

  int chunk = 1024 ;
  int num_of_chunk  = len / chunk;

  start = clock_start();
//   for (int i = 0; i < num_of_chunk; ++i) { 
    block hash_output = Hash::hash_for_block(a, 16);
//   }
  cout << party << "\tHash Speed: \t" << (time_from(start)) << "us \t" << endl;

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  delete[] a;
  return 0;
}