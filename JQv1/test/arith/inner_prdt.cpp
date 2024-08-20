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
  uint64_t constant = 0;
  uint64_t *witness = new uint64_t[2 * sz];
  memset(witness, 0, 2 * sz * sizeof(uint64_t));

  setup_zk_arith<BoolIO<NetIO>>(ios, threads, party);

  IntFp *wit = new IntFp[2 * sz];
  __uint128_t *x = new __uint128_t[2 * sz];
  uint64_t *d = new uint64_t[2 * sz];
  __uint128_t *ab = new __uint128_t[sz];
  __uint128_t *ab_y = new __uint128_t[sz];

  if (party == ALICE) {
    uint64_t sum = 0, tmp;
    for (int i = 0; i < sz; ++i) {
      witness[i] = rand() % PR;
      witness[sz + i] = rand() % PR;
    }
    for (int i = 0; i < sz; ++i) {
      tmp = mult_mod(witness[i], witness[sz + i]);
      sum = add_mod(sum, tmp);
    }
    constant = PR - sum;
    ios[0]->send_data(&constant, sizeof(uint64_t));
  } else {
    ios[0]->recv_data(&constant, sizeof(uint64_t));
  }

  auto start = clock_start();
  for (int i = 0; i < 2 * sz; ++i) {
    wit[i] = IntFp(witness[i], ALICE, 1);
    x[i] = wit[i].get_u();
    d[i] = wit[i].get_d();
  }
  double tt_0 = time_from(start);

  cout << "commit witness of degree-2 polynomial of length " << sz << endl;
  cout << "time use: " << tt_0 / 1000 << " ms" << endl;
  
  if (party == ALICE) {
    for (int i = 0; i < sz; ++i) {
      ab[i] = auth_compute_mul(x[i],x[sz + i]);
      ab[i] = PR - LOW64(ab[i]);
      ab_y[i] = mult_mod(LOW64(x[i]), LOW64(x[sz + i]));
      ab_y[i] = PR - LOW64(ab_y[i]);
      ab_y[i] = add_mod(ab[i], LOW64(ab_y[i]));
    }
  } else {
    for (int i = 0; i < sz; ++i) {
      // c[i] = IntFp().get_u();
      ab[i] = auth_compute_mul(x[i],x[sz + i]);
      ab[i] = PR - ab[i];
      ab_y[i] = mult_mod(x[i], x[sz + i]);
      ab_y[i] = PR - ab_y[i];
      ab_y[i] = add_mod(ab[i], ab_y[i]);
    }
  }


  start = clock_start();
  for (int j = 0; j < repeat; ++j) {
    fp_zkp_inner_prdt<BoolIO<NetIO>>(x, x + sz, d, d + sz, ab_y, constant, sz);
  }

  finalize_zk_arith<BoolIO<NetIO>>();

  double tt_1 = time_from(start);
  cout << "prove " << repeat << " degree-2 polynomial of length " << sz << endl;
  cout << "time use: " << tt_1 / 1000  << " ms" << endl;
  cout << "average time use: " << tt_1 / 1000 / repeat  << " ms" << endl;

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
  std::cout << "ZKP inner product test";
  std::cout << " ------------" << std::endl << std::endl;
  ;

  if (argc < 3) {
    std::cout << "usage: [binary] PARTY PORT POLY_NUM POLY_DIMENSION"
              << std::endl;
    return -1;
  } else if (argc < 5) {
    repeat = 10000;
    sz = 100000;
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
