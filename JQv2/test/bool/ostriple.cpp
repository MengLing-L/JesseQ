#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
#include <cstdlib>
using namespace emp;
using namespace std;

int port, party;
const int threads = 1;

void extreme_layered_circuit(bool*& in, int len) {
  if (party == ALICE) {
    in = new bool[1];
    PRG prg;
    prg.random_bool(in, 1);
  }
}

void random_circuit(bool*& in, int*& left, int*& right, int len, int len_in) {
  if (party == ALICE) {
    in = new bool[len_in];
    PRG prg;
    prg.random_bool(in, len_in);
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
    //cout<<party<<" Gate: "<<left[i]<<"<-- "<<i<<" -->"<<right[i]<<"\n";
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
    extreme_layered_circuit(ain, len);
    os->original_setup(len + len_in);
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
    os->original_setup(len + len_in);
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
    extreme_layered_circuit(ain, len);
    os->original_setup(len + len_in);
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
    os->original_setup(len + len_in);
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
    //int p_rep, p_left, p_right;
    //block p_apre;
    os->authenticated_bits_input_with_setup(a, ain, d, len_in);
    //for (int i = len_in; i < len + len_in; ++i) {
    for (int i = len_in; i < len + len_in; ++i, ++p_left, ++p_right, ++p_apre) {
      //p_rep = rep[i];
      //p_left = left[i];
      //p_right = right[i];
      //p_apre = a_pre[i];
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
}


void test_ostriple(BoolIO<NetIO> *ios[threads + 1], int party) {
  bool flag = true;
  auto t1 = clock_start();
  OSTriple<BoolIO<NetIO>> os(party, threads, ios);
  cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;

  //test_compute_and_gate_check_layer(&os, ios[0], flag);
  test_compute_and_gate_check_layer_JQv2(&os, ios[0], flag);
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
