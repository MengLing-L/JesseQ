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
        new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + threads + 1 + i),
        party == ALICE);

  FpOSTriple<NetIO> ostriple(party, threads, ios);
  OSTriple<BoolIO<NetIO>> bos(party, threads, bios);
  int len = 1000000;
  int chunk = 1000000;
  int num_of_chunk  = len / chunk;
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  uint8_t output[BLAKE3_OUT_LEN];

  __uint128_t* a = new __uint128_t[chunk];
  __uint128_t* b = new __uint128_t[chunk];

  auto start = clock_start();
  auto start1 = 0;
  __uint128_t pro = 1;
  for (int i = 0; i < chunk; ++i) {
    a[i] = ostriple.random_val_input();
    // b[i] = ostriple.random_val_input();
    // a[i] = rand() % PR;
    //b[i] = rand() % PR;
  }
  if (party == ALICE){
    start = clock_start();
    for (int j = 0; j < num_of_chunk; ++j) { 
      for (int i = 0; i < (chunk); ++i) { 
          pro = mult_mod(LOW64(a[i]), pro);
          // mult_mod(LOW64(a[i]), LOW64(b[i]));
          // pro = LOW64(a[i]) * pro % PR;
      }
      start1 += time_from(start);
    }
    cout << "\tMul Speed: \t" << start1<< "us \t" << endl;
    start = clock_start();
    for (int i = 0; i < num_of_chunk; ++i) { 
      Hash::hash_for_block(a, 8 * chunk);
    }
    cout << "\tHash Speed: \t" << (time_from(start)) << "us \t" << endl;
    
    start = clock_start();
    for (int i = 0; i < num_of_chunk; ++i) { 
      blake3_hasher_update(&hasher, a, 8 * chunk);
      blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    }
    cout << "\tblake3 Hash Speed: \t" << (time_from(start)) << "us \t" << endl;
  }
  
  // uint64_t* a = new uint64_t[chunk];
  // uint64_t* b = new uint64_t[chunk];

  // for (int i = 0; i < chunk; ++i) {
  //   a[i] = rand() % PR;
  //   b[i] = rand() % PR;
  // }
  // auto start = clock_start();
  // for (int i = 0; i < chunk ; ++i) { 
  //     mult_mod(b[i], a[i]);
  // }
  // cout << party << "\tMul Speed: \t" << (time_from(start)) << "us \t" << endl;
  

  
 
  block *ab = new block[chunk];
  block *ab_ = new block[chunk];
  bos.random_bits_input(ab, chunk);
  
  
  if (party == ALICE){
    start = clock_start();
    block tmp;
    gfmul(ab[0], ab[1], &tmp);
    for (int j = 0; j < num_of_chunk; ++j) { 
      for(int i = 0; i < chunk; i++) {
        gfmul(tmp, ab[i], &tmp);
      }
    }

    cout << "\t Binary Mul Speed: \t" << (time_from(start)) << "us \t" << endl;

    start = clock_start();
    for (int i = 0; i < num_of_chunk; ++i) { 
      Hash::hash_for_block(ab, sizeof(block) * chunk);
    }
    cout << "\tHash Speed: \t" << (time_from(start)) << "us \t" << endl;


    start = clock_start();
    for (int i = 0; i < num_of_chunk; ++i) { 
      blake3_hasher_update(&hasher, ab, sizeof(block) * chunk);
      blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    }
    cout << "\tblake3 Hash Speed: \t" << (time_from(start)) << "us \t" << endl;
  }
  

  for (int i = 0; i < threads; ++i) {
    delete bios[i];
  }
  delete[] a;
  delete[] b;
  delete[] ab;
  // delete[] ab_;
  return 0;
}
