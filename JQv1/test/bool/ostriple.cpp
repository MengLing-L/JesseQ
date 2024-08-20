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
const int threads = 1;

void test_compute_and_gate_check_JQv1(OSTriple<BoolIO<NetIO>> *os,
                                 BoolIO<NetIO> *io) {
  PRG prg;
  int len = 30000000;
  block *a = new block[2 * len];
  block *ab = new block[len];
  block *c = new block[len];
  bool *ain = new bool[3 * len];
  os->random_bits_input(a, 2 * len);
  os->random_bits_input(c, len);
  if (party == ALICE) {
    prg.random_bool(ain, 2 * len);
  }

  auto tt = clock_start();
  for (int i = 0; i < len; ++i) {
    ab[i] = os->auth_compute_and(a[i], a[len + i]);
    if (party == ALICE) {
      ain[2 * len + i] = ain[i] & ain[len + i]; 
    }
  }

  std::cout << "setu up time of party " << party << ": " << time_from(tt) << " us" << std::endl;

  auto start = clock_start();

  for (int i = 0; i < len; ++i) {
    if (party == ALICE) {
      os->auth_compute_and_send_with_setup(a[i], a[len + i], c[i], ain[i], ain[len + i], ain[2 * len + i], ab[i]);
    } else {
      os->auth_compute_and_recv_with_setup(a[i], a[len + i], c[i], ab[i]);
    }
  }

  if (party == ALICE) {
    block hash_output = Hash::hash_for_block(ab, len * 16);
    io[0].send_data(&hash_output, sizeof(block));
  } else {
    block hash_output = Hash::hash_for_block(ab, len * 16), output_recv;
    io[0].recv_data(&output_recv, sizeof(block));
    if (HIGH64(hash_output) == HIGH64(output_recv) && LOW64(hash_output) == LOW64(output_recv))
      std::cout<<"JQv1 success!\n";
    else std::cout<<"JQv1 fail!\n";
  }
  auto timeuse = time_from(start);
  cout << len << "\t" << timeuse << " us\t" << party << " " << endl;
  std::cout << std::endl;
  
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

void test_ostriple(BoolIO<NetIO> *ios[threads + 1], int party) {
  auto t1 = clock_start();
  OSTriple<BoolIO<NetIO>> os(party, threads, ios);
  cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;


  test_compute_and_gate_check_JQv1(&os, ios[0]);
}

int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  BoolIO<NetIO> *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new BoolIO<NetIO>(
        new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i),
        party == ALICE);

  std::cout << std::endl
            << "------------ triple generation test ------------" << std::endl
            << std::endl;
  ;

  test_ostriple(ios, party);
  for (int i = 0; i < threads; ++i) {
    delete ios[i]->io;
    delete ios[i];
  }
  return 0;
}
