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

void random_circuit(uint64_t*& in, int*& left, int*& right, int len, int len_in) {
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
    random_circuit(ain, left, right, len, len_in);
    clr = new bool[len + len_in];
    a = new __uint128_t[len + len_in];
    val_pre_pro = new uint64_t[len + len_in];
    uint64_t *d = new uint64_t[len + len_in];

    auto t2 = clock_start();
    if (party == ALICE) {
      for (int i = 0; i < len + len_in; i++) {
        if (i < len_in || clr[left[i]] || clr[right[i]]) {
          clr[i] = false;
          a[i] = os->random_val_input();
        } else {
          clr[i] = true;
          a[i] = os->auth_compute_mul_send(a[left[i]], a[right[i]]);
        }
      }
      os->setup_pre_processing(a, left, right, clr, val_pre_pro, len + len_in);
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
      __uint128_t *a_pre = new __uint128_t[len + len_in];
      for (int i = 0; i < len + len_in; i++) {
        if (i < len_in || clr[left[i]] || clr[right[i]]) {
          clr[i] = false;
          a[i] = os->random_val_input();
        } else {
          clr[i] = true;
          a[i] = os->auth_compute_mul_recv(a[left[i]], a[right[i]]);
        }
        a_pre[i] = a[i];
      }
      os->setup_pre_processing(a, left, right, clr, val_pre_pro, len + len_in);
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

void test_ostriple(NetIO *ios[threads + 1], int party) {
  //flag == true  ==> random circuit test
  //flag == false ==> layered circuit test
  bool flag = true;
  auto t1 = clock_start();
  FpOSTriple<NetIO> os(party, threads, ios);
  cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;

  //test_compute_and_gate_check_layer(&os, flag);
  test_compute_and_gate_check_JQv2_layer(&os, flag);
}


int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  NetIO *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i);

  std::cout << std::endl
            << "------------ triple generation test ------------" << std::endl
            << std::endl;
  ;


  test_ostriple(ios, party);

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  return 0;
}