#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
#if defined(__linux__)
#include <sys/time.h>
#include <sys/resource.h>
#elif defined(__APPLE__)
#include <unistd.h>
#include <sys/resource.h>
#include <mach/mach.h>
#endif

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

  __uint128_t *x = new __uint128_t[2 * sz];
  __uint128_t *ab = new __uint128_t[sz];
  uint64_t *d = new uint64_t[2 * sz];

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

  
  IntFp wit;
  for (int i = 0; i < 2 * sz; ++i) {
    wit = IntFp(witness[i], true);
    x[i] = wit.get_u();
    d[i] = wit.get_d();
  }
  auto start = clock_start();
  __uint128_t ab_, tmp;
  if (party == ALICE) {
    for (int i = 0; i < sz; ++i) {
      ab_ = auth_compute_mul(x[i],x[sz + i]);
      ab_ = PR - LOW64(ab_);
      tmp = mult_mod(LOW64(x[i]), LOW64(x[sz + i]));
      tmp = PR - LOW64(tmp);
      ab[i] = add_mod(LOW64(tmp), LOW64(ab_));
    }
  } else {
    for (int i = 0; i < sz; ++i) {
      ab_ = auth_compute_mul(x[i],x[sz + i]);
      ab_ = PR - ab_;
      tmp = mult_mod(x[i], x[sz + i]);
      tmp = PR - tmp;
      ab[i] = add_mod(ab_, tmp);
    }
  }
  double tt_0 = time_from(start);
  cout << "Setup of degree-2 polynomial of length " << sz << endl;
  cout << "time use: " << tt_0 / 1000 << " ms" << endl;


  start = clock_start();
  for (int j = 0; j < repeat; ++j) {
    fp_zkp_inner_prdt<BoolIO<NetIO>>(x, x + sz, d, d + sz, ab, constant, sz);
  }

  finalize_zk_arith<BoolIO<NetIO>>();

  double tt_1 = time_from(start);
  cout << "prove " << repeat << " degree-2 polynomial of length " << sz << endl;
  cout << "time use: " << tt_1 / 1000  << " ms" << endl;
  cout << "average time use: " << tt_1 / 1000 / repeat  << " ms" << endl;

  delete[] witness;
  delete[] x;
  delete[] ab;
  delete[] d;

#if defined(__linux__)
struct rusage rusage;
if (!getrusage(RUSAGE_SELF, &rusage))
  std::cout << "[Linux]Peak resident set size: " << (size_t)rusage.ru_maxrss
            << std::endl;
else
  std::cout << "[Linux]Query RSS failed" << std::endl;
#elif defined(__APPLE__)
struct mach_task_basic_info info;
mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info,
              &count) == KERN_SUCCESS)
  std::cout << "[Mac]Peak resident set size: "
            << (size_t)info.resident_size_max << std::endl;
else
  std::cout << "[Mac]Query RSS failed" << std::endl;
#endif
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
    sz = 1000000;
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
