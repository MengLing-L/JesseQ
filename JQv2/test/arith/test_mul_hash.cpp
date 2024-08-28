
#include "emp-zk/emp-zk-arith/ostriple.h"
#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
#include "blake3.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
  int len = 1024;
  int chunk = 1024;
  int num_of_chunk  = len / chunk;

  

  auto start = clock_start();
  // pro = mult_mod(a[0], a[1]);
  // for (int j = 0; j < num_of_chunk; ++j) { 
  //   for (int i = 0; i < chunk; ++i) {
  //     a[i] = os.random_val_input();
  //     b[i] = os.random_val_input();
  //   }
  //   start = clock_start();
  //   for (int i = 0; i < chunk ; ++i) { 
  //       mult_mod(b[i], a[i]);
  //   }
  //   cout << party << "\tMul Speed: \t" << (time_from(start) * 7) << "us \t" << endl;
  // }

  uint64_t* a = new uint64_t[chunk];
  uint64_t* b = new uint64_t[chunk];

  for (int i = 0; i < chunk; ++i) {
    a[i] = mod(rand());
    b[i] = mod(rand());
  }
  start = clock_start();
  for (int i = 0; i < chunk ; ++i) { 
      mult_mod(b[i], a[i]);
  }
  cout << party << "\tMul Speed: \t" << (time_from(start)) << "us \t" << endl;
  
 
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
      gfmul(tmp, ab[i], &tmp);
    }
  }

  cout << party << "\t Binary Mul Speed: \t" << (time_from(start) * 7) << "us \t" << endl;

  start = clock_start();
  for (int i = 0; i < num_of_chunk; ++i) { 
    Hash::hash_for_block(a, 8 * chunk);
  }
  cout << party << "\tHash Speed: \t" << (time_from(start)) << "us \t" << endl;


  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  uint8_t output[BLAKE3_OUT_LEN];
  start = clock_start();
  for (int i = 0; i < num_of_chunk; ++i) { 
    blake3_hasher_update(&hasher, a, 8 * chunk);
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
  }
  cout << party << "\tblake3 Hash Speed: \t" << (time_from(start)) << "us \t" << endl;

  

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  delete[] a;
  delete[] b;
  delete[] ab;
  delete[] ab_;
  return 0;
}