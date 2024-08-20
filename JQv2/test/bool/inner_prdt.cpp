#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
using namespace emp;
using namespace std;

int port, party;
int repeat, sz;
const int threads = 1;

void test_inner_product(BoolIO<NetIO> *ios[threads], int party) {
  srand(time(NULL));
  
  bool constant;
  bool *witness = new bool[2 * sz];
  memset(witness, 0, 2 * sz * sizeof(bool));


  setup_zk_bool<BoolIO<NetIO>>(ios, threads, party);
  
  // sync_zk_bool<BoolIO<NetIO>>();

  block *x = new block[2 * sz];
  block *ab = new block[sz];

  if (party == ALICE) {
    bool sum = 0, tmp;
    PRG prg;
    prg.random_bool(witness, 2 * sz);
    for (int i = 0; i < sz; ++i) {
      tmp = witness[i] & witness[sz + i];
      sum = sum ^ tmp;
    }
    constant = sum;
    ios[0]->send_data(&constant, sizeof(bool));
  } else {
    ios[0]->recv_data(&constant, sizeof(bool));
  }
  ios[0]->flush();

  random_bits_input<BoolIO<NetIO>>(party, x, 2 * sz);
  
  for (int i = 0; i < sz; ++i) {
    ab[i] = auth_compute_and<BoolIO<NetIO>>(party, x[i], x[sz + i]);
  }

  auto start = clock_start();
  for (int i = 0; i < sz; ++i) {
    if (party == ALICE) {
      witness[i] =  getLSB(x[i]) ^ witness[i];
      witness[sz + i] =  getLSB(x[sz + i]) ^ witness[sz + i];
      ios[0]->send_bit(witness[i]);
      ios[0]->send_bit(witness[sz + i]);
    } else {
      witness[i] = ios[0]->recv_bit();
      witness[sz + i] = ios[0]->recv_bit();
    }
  }
  
  for (int j = 0; j < repeat; ++j) {
    zkp_inner_prdt<BoolIO<NetIO>>(x, x + sz, witness, witness + sz, ab, constant, sz);
  }


  double tt = time_from(start);
  cout << "prove " << repeat << " degree-2 inner_product of length " << sz
       << endl;
  cout << "time use: " << tt / 1000 << " ms" << endl;
  cout << "average time use: " << tt / 1000 / repeat << " ms" << endl;

  bool cheated = finalize_zk_bool<BoolIO<NetIO>>();
  if (cheated)
    error("cheated\n");

  delete[] witness;
  delete[] x;
}

int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  BoolIO<NetIO> *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new BoolIO<NetIO>(
        new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i),
        party == ALICE);

  std::cout << std::endl << "------------ ";
  std::cout << "ZKP inner_product test";
  std::cout << " ------------" << std::endl << std::endl;
  ;

  if (argc < 3) {
    std::cout << "usage: [binary] PARTY PORT POLY_NUM POLY_DIMENSION"
              << std::endl;
    return -1;
  } else if (argc < 5) {
    repeat = 10240;
    sz = 10000000;
  } else {
    repeat = atoi(argv[3]);
    sz = atoi(argv[4]);
  }

  test_inner_product(ios, party);

  for (int i = 0; i < threads; ++i) {
    delete ios[i]->io;
    delete ios[i];
  }
  return 0;
}
