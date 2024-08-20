#include "emp-zk/emp-zk-arith/ostriple.h"
#include "emp-tool/emp-tool.h"
#include <iostream>
using namespace emp;
using namespace std;

int port, party;
const int threads = 1;

void test_auth_bit_input(FpOSTriple<NetIO> *os) {
  PRG prg;
  int len = 1024;
  __uint128_t *auth = new __uint128_t[len];
  if (party == ALICE) {
    __uint128_t *in = new __uint128_t[len];
    PRG prg;
    prg.random_block((block *)in, len);
    for (int i = 0; i < len; ++i) {
      in[i] = in[i] & (__uint128_t)0xFFFFFFFFFFFFFFFFLL;
      in[i] = mod(in[i], pr);
      auth[i] = os->authenticated_val_input(in[i]);
    }
    os->check_auth_mac(auth, len);
    delete[] in;
  } else {
    for (int i = 0; i < len; ++i)
      auth[i] = os->authenticated_val_input();
    os->check_auth_mac(auth, len);
  }
  delete[] auth;
}

void test_compute_and_gate_check_layer(FpOSTriple<NetIO> *os) {
  PRG prg;
  int len = 30000000;
  __uint128_t *a = new __uint128_t[len+1];
  auto start = clock_start();
  if (party == ALICE) {
    block a_block;
    PRG prg;
    prg.random_block(&a_block, 1);
    __uint128_t ain = (__uint128_t)a_block;
    ain = ain & (__uint128_t)0xFFFFFFFFFFFFFFFFLL;
    ain = mod(ain, pr);
    a[0] = os->authenticated_val_input(ain);
    for (int i = 1; i <= len; ++i) {
      a[i] = os->auth_compute_mul_send(a[i-1], a[i-1]);
    }
    os->andgate_correctness_check_manage();
    os->check_cnt = 0;
    std::cout << "sender time: " << time_from(start) << std::endl;
    std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    //os->check_compute_mul(a, b, c, len);
  } else {
    a[0] = os->authenticated_val_input();
    for (int i = 1; i <= len; ++i) {
      a[i] = os->auth_compute_mul_recv(a[i-1], a[i-1]);
    }
    os->andgate_correctness_check_manage();
    os->check_cnt = 0;
    std::cout << "recver time: " << time_from(start) << std::endl;
    std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    //os->check_compute_mul(a, b, c, len);
  }

  delete[] a;
}

void test_compute_and_gate_check_JQv2_layer(FpOSTriple<NetIO> *os) {
  int len = 30000000;

  auto t2 = clock_start();
  __uint128_t *a = new __uint128_t[len+1];
  uint64_t *val_pre_pro = new uint64_t[len+1];
  int *left = new int[len+1];
  int *right = new int[len+1];
  bool *clr = new bool[len+1];
  if (party == ALICE) {
    for (int i = 0; i <= len; i++) {
      if (i&1) {
        a[i] = os->auth_compute_mul_send(a[i-1], a[i-1]);
        clr[i] = true;
        left[i] = right[i] = i-1;
      }
      else {
        a[i] = os->random_val_input();
        clr[i] = false;
      }
    }
    os->setup_pre_processing(a, left, right, clr, val_pre_pro, len + 1);
    os->andgate_correctness_check_manage();
    os->check_cnt = 0;
    std::cout << "sender time for setup: " << time_from(t2)<<" us" << std::endl;

    auto start = clock_start();
    block a_block;
    PRG prg;
    prg.random_block(&a_block, 1);
    __uint128_t ain = (__uint128_t)a_block;
    ain = ain & (__uint128_t)0xFFFFFFFFFFFFFFFFLL;
    ain = mod(ain, pr);
    uint64_t d_1, d_2;
    os->authenticated_val_input_with_setup(a[0], ain, d_1);
    //os->authenticated_val_input_with_setup(a[0], ain, d_2);
    d_2 = d_1;
    for (int i = 1; i <= len; ++i) {
      if (i&1)
        os->evaluate_MAC(a[i-1], a[i-1], d_1, d_2, val_pre_pro[i], a[i]);
      else{
        os->auth_compute_mul_with_setup(a[i-1], a[i-1], a[i], d_1);
        d_2 = d_1;
      }
    }
    os->andgate_correctness_check_manage_JQv2();
    os->buffer_cnt = 0;

    std::cout << "sender time: " << time_from(start) <<" us" << std::endl;
    std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
  } else {
    for (int i = 0; i <= len; i++) {
      if (i&1) {
        a[i] = os->auth_compute_mul_recv(a[i-1], a[i-1]);
        clr[i] = 1;
        left[i] = right[i] = i-1;
      }
      else {
        a[i] = os->random_val_input();
        clr[i] = 0;
      }
    }
    os->setup_pre_processing(a, left, right, clr, val_pre_pro, len + 1);
    os->andgate_correctness_check_manage();
    os->check_cnt = 0;
    std::cout << "recver time for setup: " << time_from(t2)<<" us" << std::endl;

    auto start = clock_start();

    uint64_t d_1, d_2;
    __uint128_t key1 = a[0], key2 = a[0];
    os->authenticated_val_input_with_setup(a[0], d_1);
    //os->authenticated_val_input_with_setup(a[0],  d_2);
    d_2 = d_1;

    for (int i = 1; i <= len; ++i) {
      if (i&1) 
        os->evaluate_MAC(key1, key2, d_1, d_2, val_pre_pro[i], a[i]);
      else {
        key1 = key2 = a[i];
        os->auth_compute_mul_with_setup(a[i-1], a[i-1], a[i], d_1);
        d_2 = d_1;
      }
    }
    os->andgate_correctness_check_manage_JQv2();
    os->buffer_cnt = 0;

    std::cout << "recver time: " << time_from(start)<<" us" << std::endl;
    std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
  }

  delete[] a;
  delete[] val_pre_pro;
  delete[] left;
  delete[] right;
  delete[] clr;
}

void test_ostriple(NetIO *ios[threads + 1], int party) {
  auto t1 = clock_start();
  FpOSTriple<NetIO> os(party, threads, ios);
  cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;

  //test_auth_bit_input(&os);
  //std::cout << "check for authenticated bit input\n";

  //test_compute_and_gate_check(&os);
  //test_compute_and_gate_check_layer(&os);
  test_compute_and_gate_check_JQv2_layer(&os);
  std::cout << "check for multiplication\n";

  std::cout << std::endl;
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
