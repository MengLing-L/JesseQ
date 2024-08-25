#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
#include <cstdlib>
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

void random_input_set(bool*& in, int len) {
  if (party == ALICE) {
    in = new bool[len];
    PRG prg;
    prg.random_bool(in, len);
  }
}

void random_circuit(bool*& in, int*& left, int*& right, int len, int len_in) {
  random_input_set(in, len_in);

  left = new int[len + len_in];
  right = new int[len + len_in];
  int *rep = new int[len_in];
  for (int i = 0; i < len_in; i++)
    rep[i] = i;
  srand(time(NULL));
  for (int i = len_in; i < len + len_in; i++) {
    left[i] = rep[rand()%len_in];
    right[i] = rep[rand()%len_in];
    rep[rand()%len_in] = i;
  }
  delete[] rep;
}

void test_compute_and_gate_check_layer(OSTriple<BoolIO<NetIO>> *os,
                                 BoolIO<NetIO> *io, bool flag) {
  int len = 30000000, len_in;
  bool *ain = nullptr;
  int *left = nullptr, *right = nullptr;
  block* a = nullptr;
  auto start = clock_start();
  if (!flag) {
    //extreme_layered_circuit
    len_in = 1;
    random_input_set(ain, len);
    a = new block[len + len_in];
    start = clock_start();
    os->authenticated_bits_input(a, ain, len_in);
    for (int i = len_in; i < len_in + len; ++i) {
      a[i] = os->auth_compute_and(a[i-1], a[i-1]);
    }
  } else {
    //random_circuit
    len_in = 1024;
    random_circuit(ain, left, right, len, len_in);
    a = new block[len_in + len];
    start = clock_start();
    os->authenticated_bits_input(a, ain, len_in);
    int *p_left = left + len_in, *p_right = right + len_in;
    for (int i = len_in; i < len_in + len; ++i, p_left++, p_right++) {
      a[i] = os->auth_compute_and(a[*p_left], a[*p_right]);
    }
  }

  if (os->check_cnt) {
    os->andgate_correctness_check_manage();
    os->check_cnt = 0;
  }

  if (party == ALICE) {
    std::cout << "sender time: " << time_from(start) << std::endl;
    std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
  } else {
    std::cout << "recver time: " << time_from(start) << std::endl;
    std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
  }

  delete[] a;
  delete[] ain;
  if (flag) {
    delete[] left;
    delete[] right;
  }
  io->flush();
}

void test_compute_and_gate_check_layer_JQv2(OSTriple<BoolIO<NetIO>> *os, BoolIO<NetIO> *io, bool flag) {
  int len = 30000000, len_in;
  bool *ain = nullptr, *clr = nullptr, *d = nullptr;
  int *left = nullptr, *right = nullptr;
  block *a = nullptr, *a_pre = nullptr;
  auto t1 = clock_start(), start = clock_start();
  if (!flag) {
    //extreme_layered_circuit
    len_in = 1;
    random_input_set(ain, len);
    t1 = clock_start();
    a = new block[len + len_in];
    for (int i = 0; i < len + len_in; i++) {
      if (i&1) 
        a[i] = os->auth_compute_and(a[i-1], a[i-1]);
      else 
        a[i] = os->random_val_input();
    }
    os->andgate_correctness_check_manage();
    os->check_cnt = 0;
    if (party == ALICE)
      std::cout << "sender time for setup: " << time_from(t1)<<" us" << std::endl;
    else
      std::cout << "recver time for setup: " << time_from(t1)<<" us" << std::endl;

    start = clock_start();
    bool d_tmp;
    block atmp = a[0];
    os->authenticated_bits_input_with_setup(a, ain, &d_tmp, len_in);
    for (int i = len_in; i < len + len_in; ++i) {
      if (i&1) {
        a[i] = os->evaluate_MAC(atmp, atmp, d_tmp, d_tmp, a[i]);
      }
      else {
        atmp = a[i];
        a[i] = os->auth_compute_and_with_setup(a[i-1], a[i-1], a[i], d_tmp);
      }
    }
  } else {
    //random_circuit
    len_in = 1024;
    random_circuit(ain, left, right, len, len_in);
    t1 = clock_start();
    a = new block[len_in + len];
    d = new bool[len_in + len];
    clr = new bool[len_in + len];
    a_pre = new block[len + len_in];
    for (int i = 0; i < len + len_in; i++) {
      if (i < len_in || clr[left[i]] || clr[right[i]]) {
        clr[i] = false;
        a[i] = os->random_val_input();
      } else {
        clr[i] = true;
        a[i] = os->auth_compute_and(a[left[i]], a[right[i]]);
      }
      a_pre[i] = a[i];
    }
    os->andgate_correctness_check_manage();
    os->check_cnt = 0;
    if (party == ALICE)
      std::cout << "sender time for setup: " << time_from(t1)<<" us" << std::endl;
    else
      std::cout << "recver time for setup: " << time_from(t1)<<" us" << std::endl;

    start = clock_start();
    int  *p_left = left + len_in, *p_right = right + len_in;
    block *p_apre = a_pre + len_in;

    os->authenticated_bits_input_with_setup(a, ain, d, len_in);

    for (int i = len_in; i < len + len_in; ++i, ++p_left, ++p_right, ++p_apre) {
      if (!clr[i]) {
        a[i] = os->auth_compute_and_with_setup(a[*p_left], a[*p_right], *p_apre, d[i]);
      } else {
        a[i] = os->evaluate_MAC(a_pre[*p_left], a_pre[*p_right], d[*p_left], d[*p_right], *p_apre);
      }
    }
  }
  
  if (os->buffer_cnt) {
    os->andgate_correctness_check_manage_JQv2();
    os->buffer_cnt = 0;
  }
  
  if (party == ALICE) {
    std::cout << "sender time: " << time_from(start) <<" us" << std::endl;
    std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
  } else {
    std::cout << "recver time: " << time_from(start)<<" us" << std::endl;
    std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
  }

  delete[] a;
  delete[] ain;
  if (flag) {
    delete[] clr;
    delete[] left;
    delete[] right;
    delete[] a_pre;
    delete[] d;
  }
  io->flush();

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


void random_circuit_cutted(block*& a, block*& a_pre, int*& left, int*& right, bool*& clr, bool*& d, int len, int len_in, int type_phase) {
  static int *rep = new int[len_in];
  static block *bk_tmp = new block[len_in];
  static bool *bl_tmp = new bool[len_in];
  if (!type_phase) {
    srand(time(NULL));
    a = new block[len + len_in];
    a_pre = new block[len + len_in];
    left = new int[len + len_in];
    right = new int[len + len_in];
    clr = new bool[len + len_in];
    d = new bool[len + len_in];
    rep = new int[len_in];
    bk_tmp = new block[len_in];
    bl_tmp = new bool[len_in];
  } else {
    for (int i = 0; i < len_in; i++) {
      bk_tmp[i] = a[rep[i]];
      bl_tmp[i] = clr[rep[i]];
    }
    memcpy(a, bk_tmp, len_in * sizeof(block));
    memcpy(clr, bl_tmp, len_in * sizeof(bool));
    for (int i = 0; i < len_in; i++) {
      bk_tmp[i] = a_pre[rep[i]];
      bl_tmp[i] = d[rep[i]];
    }
    memcpy(a_pre, bk_tmp, len_in * sizeof(block));
    memcpy(d, bl_tmp, len_in * sizeof(bool));
  }

  for (int i = 0; i < len_in; i++)
    rep[i] = i;

  for (int i = len_in; i < len + len_in; i++) {
    left[i] = rep[rand()%len_in];
    right[i] = rep[rand()%len_in];
    rep[rand()%len_in] = i;
  }

  if (type_phase == 2) {
    delete[] rep;
    delete[] bk_tmp;
    delete[] bl_tmp;
  }
}




void test_compute_and_gate_check_huge_random_circuit_JQv2(OSTriple<BoolIO<NetIO>> *os, BoolIO<NetIO> *io) {
  int len = (int)1e6, len_in = 1024, repeat = 300;
  bool *ain = nullptr, *clr = nullptr, *d = nullptr;
  int *left = nullptr, *right = nullptr;
  block* a = nullptr, *a_pre = nullptr;

  int timeuse = 0, timesetup = 0;

  for (int tm = 0; tm < repeat; tm++) {
    if (!tm) {
      random_input_set(ain, len_in);
      random_circuit_cutted(a, a_pre, left, right, clr, d, len, len_in, 0);
      auto start = clock_start();
      for (int i = 0; i < len_in; i++) {
        clr[i] = false;
        a[i] = a_pre[i] = os->random_val_input();
      }
      timesetup += time_from(start);
    }
    else random_circuit_cutted(a, a_pre, left, right, clr, d, len, len_in, 1 + bool(tm == repeat - 1));
    
    auto start = clock_start();
    for (int i = len_in; i < len + len_in; i++) {
      if (clr[left[i]] || clr[right[i]]) {
        clr[i] = false;
        a_pre[i] = os->random_val_input();
      } else {
        clr[i] = true;
        a_pre[i] = os->auth_compute_and(a_pre[left[i]], a_pre[right[i]]);
      }
    }
    timesetup += time_from(start);

    start = clock_start();
    if (!tm) os->authenticated_bits_input_with_setup(a, ain, d, len_in);

    /*int *p_left = left + len_in, *p_right = right + len_in;
    block *p_apre = a_pre + len_in;
    for (int i = len_in; i < len + len_in; ++i, ++p_left, ++p_right, ++p_apre) {
      if (!clr[i]) {
        a[i] = os->auth_compute_and_with_setup(a[*p_left], a[*p_right], *p_apre, d[i]);
      } else {
        a[i] = os->evaluate_MAC(a_pre[*p_left], a_pre[*p_right], d[*p_left], d[*p_right], *p_apre);
      }
    }*/
    for (int i = len_in; i < len + len_in; ++i) {
      if (!clr[i]) {
        a[i] = os->auth_compute_and_with_setup(a[left[i]], a[right[i]], a_pre[i], d[i]);
      } else {
        a[i] = os->evaluate_MAC(a_pre[left[i]], a_pre[right[i]], d[left[i]], d[right[i]], a_pre[i]);
      }
    }

    timeuse += time_from(start);
  }

  auto start = clock_start();
  if (os->check_cnt) {
    os->andgate_correctness_check_manage();
    os->check_cnt = 0;
  }
  timesetup += time_from(start);

  start = clock_start();
  if (os->buffer_cnt) {
    os->andgate_correctness_check_manage_JQv2();
    os->buffer_cnt = 0;
  }
  timeuse += time_from(start);

  if (party == ALICE) {
    std::cout << "prover setup time: " << timesetup <<" us" << std::endl;
    std::cout << "Prove time: " << timeuse <<" us" << std::endl;
    std::cout << "proof of sender for 1s: " << double(len * repeat) / timeuse * 1000000 << std::endl;
  } else {
    std::cout << "recver setup time: " << timesetup <<" us" << std::endl;
    std::cout << "Verify time: " << timeuse<<" us" << std::endl;
    std::cout << "proof of recver for 1s: " << double(len * repeat) / timeuse * 1000000 << std::endl;
  }

  delete[] a;
  if (party == ALICE) delete[] ain;
  delete[] clr;
  delete[] left;
  delete[] right;
  delete[] a_pre;
  delete[] d;
}

void test_ostriple(BoolIO<NetIO> *ios[threads + 1], int party) {
  //bool flag = true;
  auto t1 = clock_start();
  OSTriple<BoolIO<NetIO>> os(party, threads, ios);
  cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;

  //test_compute_and_gate_check_layer(&os, ios[0], flag);
  //test_compute_and_gate_check_layer_JQv2(&os, ios[0], flag);
  test_compute_and_gate_check_huge_random_circuit_JQv2(&os, ios[0]);
}

int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  BoolIO<NetIO> *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new BoolIO<NetIO>(
        new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i),
        // new NetIO(party == ALICE ? nullptr : "172.31.37.146", port + i),
        // new NetIO(party == ALICE ? "172.31.43.140" : "172.31.43.140", port + i),
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
