#include "emp-zk/emp-zk-arith/ostriple.h"
#include "emp-tool/emp-tool.h"
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
const int threads = 1;

void test_compute_and_gate_check_JQv1(NetIO *ios[threads + 1], int party) {
  auto t1 = clock_start();
  int len = 30000000;
  PRG prg;
  FpOSTriple<NetIO> os(party, threads, ios);
  cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;

  
  __uint128_t *a = new __uint128_t[len];
  __uint128_t *b = new __uint128_t[len];
  __uint128_t *c = new __uint128_t[len];
  __uint128_t *ab = new __uint128_t[len];

  auto t2 = clock_start();
  for (int i = 0; i < len; i++) {
    a[i] = os.random_val_input();
    b[i] = os.random_val_input();
    c[i] = os.random_val_input();
      if (party == ALICE) {
      ab[i] = os.authenticated_val_input(0);
    } else {
      ab[i] = os.authenticated_val_input();
    }
  }

  if (party == ALICE) {
    for (int i = 0; i < len; i++) {
      __uint128_t ab_, tmp;
      ab_ = os.auth_compute_mul_send(a[i],b[i]);
      ab_ = PR - LOW64(ab_);
      tmp = mult_mod(LOW64(a[i]), LOW64(b[i]));
      tmp = PR - LOW64(tmp);
      ab[i] = add_mod(ab[i], LOW64(c[i]));
      ab[i] = add_mod(ab[i], LOW64(ab_));
      ab[i] = add_mod(ab[i], LOW64(tmp));
    }
    os.andgate_correctness_check_manage();
    std::cout << "sender time for setup: " << time_from(t2)<<" us" << std::endl;

    auto start = clock_start();
    __uint128_t *ain = new __uint128_t[len];
    __uint128_t *bin = new __uint128_t[len];
    prg.random_block((block *)ain, len);
    prg.random_block((block *)bin, len);

    for (int i = 0; i < len; ++i) {
      ain[i] = ain[i] & (__uint128_t)0xFFFFFFFFFFFFFFFFLL;
      ain[i] = mod(ain[i], pr);
      bin[i] = bin[i] & (__uint128_t)0xFFFFFFFFFFFFFFFFLL;
      bin[i] = mod(bin[i], pr);
      os.auth_compute_mul_send_with_setup(a[i], b[i], c[i], ain[i], bin[i], ab[i]);
    }


    block hash_output = Hash::hash_for_block(ab, len * 16);
    ios[0]->send_data(&hash_output, sizeof(block));

    delete[] ain;
    delete[] bin;

    std::cout << "sender time: " << time_from(start) <<" us" << std::endl;
    std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
  } else {
    for (int i = 0; i < len; i++) {
      __uint128_t ab_, tmp;
      ab_ = os.auth_compute_mul_recv(a[i],b[i]);
      ab_ = PR - ab_;
      tmp = mult_mod(a[i], b[i]);
      tmp = PR - tmp;
      ab[i] = add_mod(ab[i], ab_);
      ab[i] = add_mod(ab[i], tmp);
    }
    os.andgate_correctness_check_manage();
    std::cout << "recver time for setup: " << time_from(t2)<<" us" << std::endl;

    auto start = clock_start();
    for (int i = 0; i < len; ++i) 
      os.auth_compute_mul_recv_with_setup(a[i], b[i], c[i], ab[i]);
    
    block hash_output = Hash::hash_for_block(ab, len * 16), output_recv;
    ios[0]->recv_data(&output_recv, sizeof(block));
    if (HIGH64(hash_output) == HIGH64(output_recv) && LOW64(hash_output) == LOW64(output_recv))
      std::cout<<"JQv1 success!\n";
    else std::cout<<"JQv1 fail!\n";

    std::cout << "recver time: " << time_from(start)<<" us" << std::endl;
    std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
  }
  os.check_cnt = 0;

  delete[] a;
  delete[] b;
  delete[] c;
  delete[] ab;

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
  NetIO *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i);
    // ios[i] = new NetIO(party == ALICE ? "localhost" : "localhost", port + i);

  std::cout << std::endl
            << "------------ triple generation test ------------" << std::endl
            << std::endl;
  ;

  test_compute_and_gate_check_JQv1(ios, party);

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  return 0;
}
