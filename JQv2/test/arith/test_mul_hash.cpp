
#include "emp-zk/emp-zk-arith/ostriple.h"
#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
using namespace emp;
using namespace std;

int port, party;
const int threads = 1;

int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  NetIO *ios[threads];
  BoolIO<NetIO> *bios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i);

  for (int i = 0; i < threads; ++i)
    bios[i] = new BoolIO<NetIO>(
        new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i),
        party == ALICE);

  FpOSTriple<NetIO> os(party, threads, ios);
  OSTriple<BoolIO<NetIO>> bos(party, threads, bios);
  int len = 1024 * 1024 * 10 * 10 * 3;
  int chunk = 1024 * 100;
  int num_of_chunk  = len / chunk;

  __uint128_t* a = new __uint128_t[chunk];


  for (int i = 0; i < chunk; ++i) {
    a[i] = os.random_val_input();
  }

  auto start = clock_start();
  for (int i = 0; i < num_of_chunk; ++i) { 
    for (int i = 0; i < chunk; ++i) { 
        mult_mod(a[i], a[i]);
    }
  }
  
  cout << party << "\tMul Speed: \t" << (time_from(start) * 7)/1000 << "ms \t" << endl;


  block *ab = new block[chunk];
  block *ab_ = new block[chunk];
  bos.random_bits_input(ab, chunk);
  bos.random_bits_input(ab_, chunk);

  bios[0]->flush();
  block seed = bios[0]->get_hash_block();
  block share_seed;
  PRG(&seed).random_block(&share_seed, 1);
  block *chi = new block[chunk];
  // cout << "send" << share_seed << endl;
  uni_hash_coeff_gen(chi, share_seed, chunk);

  start = clock_start();
  block tmp;
  gfmul(ab[0], ab[1], &tmp);
  for (int j = 0; j < num_of_chunk; ++j) { 
    for(int i = 0; i < chunk; i++) {
      gfmul(ab_[i], ab[i], &tmp);
    }
  }

  cout << party << "\t Binary Mul Speed: \t" << (time_from(start) * 7)/1000 << "ms \t" << endl;

  start = clock_start();
  for (int i = 0; i < num_of_chunk; ++i) { 
    block hash_output = Hash::hash_for_block(a, 16 * chunk);
  }
  cout << party << "\tHash Speed: \t" << (time_from(start))/1000 << "ms \t" << endl;

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  delete[] a;
  return 0;
}