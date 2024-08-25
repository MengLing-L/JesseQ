#include "emp-zk/emp-zk-arith/ostriple.h"
#include "emp-tool/emp-tool.h"
#include <iostream>
using namespace emp;
using namespace std;

int port, party;
const int threads = 1;

void extreme_layered_circuit(uint64_t &in) {
  if (party == ALICE) {
    block a_block;
    PRG prg;
    prg.random_block(&a_block, 1);
    __uint128_t ain = (__uint128_t)a_block;
    ain = ain & (__uint128_t)0xFFFFFFFFFFFFFFFFLL;
    ain = mod(ain, pr);
    in = LOW64(ain);
  }
}

void random_input_set(uint64_t*& in, int len_in) {
  if (party == ALICE) {
    in = new uint64_t[len_in];
    block *a_block = new block[len_in];
    PRG prg;
    prg.random_block(a_block, len_in);
    for (int i = 0; i < len_in; i++) {
      __uint128_t ain = (__uint128_t)a_block[i];
      ain = ain & (__uint128_t)0xFFFFFFFFFFFFFFFFLL;
      ain = mod(ain, pr);
      in[i] = LOW64(ain);
    }
    delete[] a_block;
  }
}

void random_circuit(uint64_t*& in, int*& left, int*& right, int len, int len_in) {
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

void test_compute_and_gate_check_layer(FpOSTriple<NetIO> *os, bool flag) {
  int len = 30000000, len_in;
  __uint128_t *a = nullptr;
  if (!flag) {
    len_in = 1;
    a = new __uint128_t[len + len_in];
    if (party == ALICE) {
      uint64_t ain;
      extreme_layered_circuit(ain);
      auto start = clock_start();
      a[0] = os->authenticated_val_input(ain);
      for (int i = len_in; i < len + len_in; i++)
        a[i] = os->auth_compute_mul_send(a[i-1], a[i-1]);
      os->andgate_correctness_check_manage();
      os->check_cnt = 0;
      std::cout << "sender time: " << time_from(start) << std::endl;
      std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    } else {
      auto start = clock_start();
      a[0] = os->authenticated_val_input();
      for (int i = 1; i <= len; ++i) {
        a[i] = os->auth_compute_mul_recv(a[i-1], a[i-1]);
      }
      os->andgate_correctness_check_manage();
      os->check_cnt = 0;
      std::cout << "recver time: " << time_from(start) << std::endl;
      std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    }
  } else {
    len_in = 1024;
    a = new __uint128_t[len + len_in];
    uint64_t *ain = nullptr;
    int *left = nullptr, *right = nullptr;
    random_circuit(ain, left, right, len, len_in);
    auto start = clock_start();
    if (party == ALICE) {
      os->authenticated_val_input(a, ain, len_in);
      int *p_left = left + len_in, *p_right = right + len_in;
      for (int i = len_in; i < len + len_in; i++, p_left++, p_right++)
        a[i] = os->auth_compute_mul_send(a[*p_left], a[*p_right]);
      os->andgate_correctness_check_manage();
      os->check_cnt = 0;
      std::cout << "sender time: " << time_from(start) << std::endl;
      std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;

      delete[] ain;
    } else {
      os->authenticated_val_input(a, len_in);
      int *p_left = left + len_in, *p_right = right + len_in;
      for (int i = len_in; i < len + len_in; i++, p_left++, p_right++)
        a[i] = os->auth_compute_mul_recv(a[*p_left], a[*p_right]);
      os->andgate_correctness_check_manage();
      os->check_cnt = 0;
      std::cout << "recver time: " << time_from(start) << std::endl;
      std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    }
    delete[] left;
    delete[] right;
  }

  delete[] a;
}

void random_circuit_cutted(__uint128_t*& a, __uint128_t*& a_pre, int*& left, int*& right, bool*& clr, uint64_t*& d, int len, int len_in, int type_phase) {
  static int *rep = nullptr;
  static __uint128_t *bk_tmp = nullptr;
  static bool *bl_tmp = nullptr;
  static uint64_t *int_tmp = nullptr;
  if (!type_phase) {
    srand(time(NULL));
    a = new __uint128_t[len + len_in];
    a_pre = new __uint128_t[len + len_in];
    left = new int[len + len_in];
    right = new int[len + len_in];
    clr = new bool[len + len_in];
    d = new uint64_t[len + len_in];
    rep = new int[len_in];
    bk_tmp = new __uint128_t[len_in];
    bl_tmp = new bool[len_in];
    int_tmp = new uint64_t[len_in];
  } else {
    for (int i = 0; i < len_in; i++) {
      bk_tmp[i] = a[rep[i]];
      bl_tmp[i] = clr[rep[i]];
      int_tmp[i] = d[rep[i]];
    }
    memcpy(a, bk_tmp, len_in * sizeof(__uint128_t));
    memcpy(clr, bl_tmp, len_in * sizeof(bool));
    memcpy(d, int_tmp, len_in * sizeof(uint64_t));
    for (int i = 0; i < len_in; i++) {
      bk_tmp[i] = a_pre[rep[i]];
    }
    memcpy(a_pre, bk_tmp, len_in * sizeof(__uint128_t));
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
    delete[] int_tmp;
  }
}

void test_compute_and_gate_check_JQv2_layer(FpOSTriple<NetIO> *os, bool flag) {
  int len = 30000000, len_in;
  __uint128_t *a = nullptr;
  uint64_t *val_pre_pro = nullptr;
  int *left = nullptr, *right = nullptr;
  bool *clr = nullptr;
  if (!flag) {
    len_in = 1;
    left = new int[len + len_in];
    right = new int[len + len_in];
    clr = new bool[len + len_in];
    for (int i = 0; i < len + len_in; i++) {
      left[i] = right[i] = i - 1;
      clr[i] = ((i & 1) == 1);
    }
    a = new __uint128_t[len + len_in];
    val_pre_pro = new uint64_t[len + len_in];
    uint64_t ain;
    extreme_layered_circuit(ain);
    auto t2 = clock_start();
    if (party == ALICE) {
      for (int i = 0; i < len + len_in; i++) {
        if (clr[i])
          a[i] = os->auth_compute_mul_send(a[i - 1], a[i - 1]);
        else
          a[i] = os->random_val_input();
      }
      os->setup_pre_processing(a, left, right, clr, val_pre_pro, len + 1);
      os->andgate_correctness_check_manage();
      os->check_cnt = 0;
      std::cout << "sender time for setup: " << time_from(t2)<<" us" << std::endl;

      uint64_t d;
      auto start = clock_start();
      os->authenticated_val_input_with_setup(a[0], ain, d);
      for (int i = len_in; i < len + len_in; ++i) {
        if (i&1)
          os->evaluate_MAC(a[i-1], a[i-1], d, d, val_pre_pro[i], a[i]);
        else{
          os->auth_compute_mul_with_setup(a[i-1], a[i-1], a[i], d);
        }
      }
      os->andgate_correctness_check_manage_JQv2();
      os->buffer_cnt = 0;

      std::cout << "sender time: " << time_from(start) <<" us" << std::endl;
      std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    } else {
      for (int i = 0; i < len + len_in; i++) {
        if (clr[i])
          a[i] = os->auth_compute_mul_recv(a[i-1], a[i-1]);
        else
          a[i] = os->random_val_input();
      }
      os->setup_pre_processing(a, left, right, clr, val_pre_pro, len + len_in);
      os->andgate_correctness_check_manage();
      os->check_cnt = 0;
      std::cout << "recver time for setup: " << time_from(t2)<<" us" << std::endl;

      auto start = clock_start();
      uint64_t d;
      __uint128_t a_tmp = a[0];
      os->authenticated_val_input_with_setup(a[0], d);

      for (int i = len_in; i < len + len_in; ++i) {
        if (i&1) 
          os->evaluate_MAC(a_tmp, a_tmp, d, d, val_pre_pro[i], a[i]);
        else {
          a_tmp = a[i];
          os->auth_compute_mul_with_setup(a[i-1], a[i-1], a[i], d);
        }
      }
      os->andgate_correctness_check_manage_JQv2();
      os->buffer_cnt = 0;

      std::cout << "recver time: " << time_from(start)<<" us" << std::endl;
      std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    }
  } else {
    len_in = 1024;
    uint64_t *ain = nullptr;
    val_pre_pro = new uint64_t[len + len_in];
    /*random_circuit(ain, left, right, len, len_in);
    clr = new bool[len + len_in];
    a = new __uint128_t[len + len_in];
    uint64_t *d = new uint64_t[len + len_in];
    __uint128_t *a_pre = new __uint128_t[len + len_in];*/
    uint64_t *d = nullptr;
    __uint128_t *a_pre = nullptr;
    random_input_set(ain, len_in);
    random_circuit_cutted(a, a_pre, left, right, clr, d, len, len_in, 0);

    auto t2 = clock_start();
    for (int i = 0; i < len_in; i++) {
      clr[i] = false;
      a[i] = os->random_val_input();
    }
    if (party == BOB) {
      for (int i = 0; i < len_in; i++)
        a_pre[i] = a[i];
    }


    if (party == ALICE) {
      for (int i = len_in; i < len + len_in; i++) {
        if (clr[left[i]] || clr[right[i]]) {
          clr[i] = false;
          a[i] = os->random_val_input();
        } else {
          clr[i] = true;
          a[i] = os->auth_compute_mul_send(a[left[i]], a[right[i]]);
        }
      }
      os->setup_pre_processing(a, left, right, clr, val_pre_pro, len, len_in);
      os->andgate_correctness_check_manage();
      os->check_cnt = 0;
      std::cout << "sender time for setup: " << time_from(t2)<<" us" << std::endl;

      auto start = clock_start();
      os->authenticated_val_input_with_setup(a, ain, d, len_in);

      int  *p_left = left + len_in, *p_right = right + len_in;
      uint64_t *p_val_pre_pro = val_pre_pro + len_in;
      for (int i = len_in; i < len + len_in; ++i, ++p_left, ++p_right, ++p_val_pre_pro) {
        if (clr[i])
          os->evaluate_MAC(a[*p_left], a[*p_right], d[*p_left], d[*p_right], *p_val_pre_pro, a[i]);
        else
          os->auth_compute_mul_with_setup(a[*p_left], a[*p_right], a[i], d[i]);
      }

      os->andgate_correctness_check_manage_JQv2();
      os->buffer_cnt = 0;

      std::cout << "sender time: " << time_from(start) <<" us" << std::endl;
      std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
      delete[] ain;
    } else {
      for (int i = len_in; i < len + len_in; i++) {
        if (clr[left[i]] || clr[right[i]]) {
          clr[i] = false;
          a[i] = os->random_val_input();
        } else {
          clr[i] = true;
          a[i] = os->auth_compute_mul_recv(a_pre[left[i]], a_pre[right[i]]);
        }
        a_pre[i] = a[i];
      }
      os->setup_pre_processing(a_pre, left, right, clr, val_pre_pro, len, len_in);
      os->andgate_correctness_check_manage();
      os->check_cnt = 0;
      std::cout << "sender time for setup: " << time_from(t2)<<" us" << std::endl;

      auto start = clock_start();
      os->authenticated_val_input_with_setup(a, d, len_in);

      int  *p_left = left + len_in, *p_right = right + len_in;
      uint64_t *p_val_pre_pro = val_pre_pro + len_in;
      for (int i = len_in; i < len + len_in; ++i, ++p_left, ++p_right, ++p_val_pre_pro) {
        if (clr[i])
          os->evaluate_MAC(a_pre[*p_left], a_pre[*p_right], d[*p_left], d[*p_right], *p_val_pre_pro, a[i]);
        else
          os->auth_compute_mul_with_setup(a[*p_left], a[*p_right], a[i], d[i]);
      }

      os->andgate_correctness_check_manage_JQv2();
      os->buffer_cnt = 0;

      std::cout << "recver time: " << time_from(start) <<" us" << std::endl;
      std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
      delete[] a_pre;
    }

    delete[] d;
  }

  delete[] a;
  delete[] val_pre_pro;
  delete[] left;
  delete[] right;
  delete[] clr;
}



void test_compute_and_gate_check_huge_random_circuit_JQv2(FpOSTriple<NetIO> *os) {
  int len = (int)1e6, len_in = 1024, repeat = 300;
  uint64_t *ain = nullptr, *d = nullptr;
  bool *clr = nullptr;
  int *left = nullptr, *right = nullptr;
  __uint128_t* a = nullptr, *a_pre = nullptr;
  uint64_t *val_pre_pro = new uint64_t[len + len_in];

  int timeuse = 0, timesetup = 0;

  for (int tm = 0; tm < repeat; tm++) {
    if (!tm) {
      random_input_set(ain, len_in);
      random_circuit_cutted(a, a_pre, left, right, clr, d, len, len_in, 0);
      auto start = clock_start();
      for (int i = 0; i < len_in; i++) {
        clr[i] = false;
        a_pre[i] = a[i] = os->random_val_input();
      }
      timesetup += time_from(start);
    }
    else random_circuit_cutted(a, a_pre, left, right, clr, d, len, len_in, 1 + bool(tm == repeat - 1));
    
    if (party == ALICE) {
      auto start = clock_start();
      for (int i = len_in; i < len + len_in; i++) {
        if (clr[left[i]] || clr[right[i]]) {
          clr[i] = false;
          a[i] = os->random_val_input();
        } else {
          clr[i] = true;
          a[i] = os->auth_compute_mul_send(a_pre[left[i]], a_pre[right[i]]);
        }
        a_pre[i] = a[i];
      }
      os->setup_pre_processing(a_pre, left, right, clr, val_pre_pro, len, len_in);
      timesetup += time_from(start);

      start = clock_start();
      if (!tm) os->authenticated_val_input_with_setup(a, ain, d, len_in);

      int *p_left = left + len_in, *p_right = right + len_in;
      uint64_t *p_val_pre_pro = val_pre_pro + len_in;
      for (int i = len_in; i < len + len_in; ++i, ++p_left, ++p_right, ++p_val_pre_pro) {
        if (clr[i])
          os->evaluate_MAC(a[*p_left], a[*p_right], d[*p_left], d[*p_right], *p_val_pre_pro, a[i]);
        else
          os->auth_compute_mul_with_setup(a[*p_left], a[*p_right], a[i], d[i]);
      }

      timeuse += time_from(start);
    } else {
      auto start = clock_start();
      for (int i = len_in; i < len + len_in; i++) {
        if (clr[left[i]] || clr[right[i]]) {
          clr[i] = false;
          a[i] = os->random_val_input();
        } else {
          clr[i] = true;
          a[i] = os->auth_compute_mul_recv(a_pre[left[i]], a_pre[right[i]]);
        }
        a_pre[i] = a[i];
      }
      os->setup_pre_processing(a_pre, left, right, clr, val_pre_pro, len, len_in);
      timesetup += time_from(start);

      start = clock_start();
      if (!tm) os->authenticated_val_input_with_setup(a, d, len_in);

      int  *p_left = left + len_in, *p_right = right + len_in;
      uint64_t *p_val_pre_pro = val_pre_pro + len_in;
      for (int i = len_in; i < len + len_in; ++i, ++p_left, ++p_right, ++p_val_pre_pro) {
        if (clr[i])
          os->evaluate_MAC(a_pre[*p_left], a_pre[*p_right], d[*p_left], d[*p_right], *p_val_pre_pro, a[i]);
        else
          os->auth_compute_mul_with_setup(a[*p_left], a[*p_right], a[i], d[i]);
      }

      timeuse += time_from(start);
    }
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
  delete[] val_pre_pro;
  delete[] d;
  delete[] a_pre;
}


void test_ostriple(NetIO *ios[threads + 1], int party) {
  //flag == true  ==> random circuit test
  //flag == false ==> layered circuit test
  //bool flag = true;
  auto t1 = clock_start();
  FpOSTriple<NetIO> os(party, threads, ios);
  cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;

  //test_compute_and_gate_check_layer(&os, flag);
  //test_compute_and_gate_check_JQv2_layer(&os, flag);
  test_compute_and_gate_check_huge_random_circuit_JQv2(&os);
}


int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  NetIO *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i);
    // ios[i] = new NetIO(party == ALICE ? nullptr : "172.31.37.146", port + i);
  // ios[i] = new NetIO(party == ALICE ? "172.31.43.140" : "172.31.43.140", port + i),

  std::cout << std::endl
            << "------------ triple generation test ------------" << std::endl
            << std::endl;


  test_ostriple(ios, party);

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  return 0;
}