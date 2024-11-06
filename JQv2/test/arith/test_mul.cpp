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
  BoolIO<NetIO> *bios[threads];
  for (int i = 0; i < threads; ++i)
    bios[i] = new BoolIO<NetIO>(
        new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + threads + 1 + i),
        party == ALICE);
  
  OSTriple<BoolIO<NetIO>> bos(party, threads, bios);
  int chunk = 1000000;

 
  __uint128_t* a = new __uint128_t[chunk];

  auto start = clock_start();
  __uint128_t pro = 1;
  for (int i = 0; i < chunk; ++i) {
    a[i] = rand() % PR;
  }

  if (party == ALICE){
    start = clock_start();
    for (int i = 0; i < (chunk); ++i) { 
        pro = mult_mod(LOW64(a[i]), pro);
    }
    cout << "\tMul Speed: \t" << time_from(start)<< "us \t" << endl;
  }

  block *ab = new block[chunk];
  bos.random_bits_input(ab, chunk);

  if (party == ALICE){
    start = clock_start();
    block tmp;
    gfmul(ab[0], ab[1], &tmp);
    for (int i = 0; i < chunk; ++i) { 
        // cout << LOW64(ab[i]) << endl;
        gfmul(tmp, ab[i], &tmp);
    }
    cout << "__m128i Mul Speed: \t" << (time_from(start)) << "us \t" << endl;
  }


  delete[] a;  
  delete[] ab;  
  return 0;
}
