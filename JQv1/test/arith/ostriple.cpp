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

void test_compute_and_gate_check(FpOSTriple<NetIO> *os) {
  PRG prg;
  int len = 30000000;
  __uint128_t *a = new __uint128_t[len];
  __uint128_t *b = new __uint128_t[len];
  __uint128_t *c = new __uint128_t[len];
  auto start = clock_start();
  if (party == ALICE) {
    __uint128_t *ain = new __uint128_t[len];
    __uint128_t *bin = new __uint128_t[len];
    PRG prg;
    prg.random_block((block *)ain, len);
    prg.random_block((block *)bin, len);
    for (int i = 0; i < len; ++i) {
      ain[i] = ain[i] & (__uint128_t)0xFFFFFFFFFFFFFFFFLL;
      ain[i] = mod(ain[i], pr);
      a[i] = os->authenticated_val_input(ain[i]);
      bin[i] = bin[i] & (__uint128_t)0xFFFFFFFFFFFFFFFFLL;
      bin[i] = mod(bin[i], pr);
      b[i] = os->authenticated_val_input(bin[i]);
      c[i] = os->auth_compute_mul_send(a[i], b[i]);
    }
    os->andgate_correctness_check_manage();
    delete[] ain;
    delete[] bin;
    std::cout << "sender time: " << time_from(start) << std::endl;
    std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    os->check_compute_mul(a, b, c, len);
  } else {
    for (int i = 0; i < len; ++i) {
      a[i] = os->authenticated_val_input();
      b[i] = os->authenticated_val_input();
      c[i] = os->auth_compute_mul_recv(a[i], b[i]);
    }
    os->andgate_correctness_check_manage();
    std::cout << "recver time: " << time_from(start) << std::endl;
    std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    os->check_compute_mul(a, b, c, len);
  }

  delete[] a;
  delete[] b;
  delete[] c;
}


void test_ostriple(NetIO *ios[threads + 1], int party) {
  auto t1 = clock_start();
  FpOSTriple<NetIO> os(party, threads, ios);
  cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;

  //test_auth_bit_input(&os);
  //std::cout << "check for authenticated bit input\n";

  test_compute_and_gate_check(&os);
  std::cout << "check for multiplication\n";

  std::cout << std::endl;
}

void test_compute_and_gate_check_JQv1(NetIO *ios[threads + 1], int party) {
  auto t1 = clock_start();
  int len = 30000000;
  PRG prg;
  FpOSTriple<NetIO> os(party, threads, ios);
  cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;

  auto t2 = clock_start();
  __uint128_t *a = new __uint128_t[len];
  __uint128_t *b = new __uint128_t[len];
  __uint128_t *c = new __uint128_t[len];
  __uint128_t *ab = new __uint128_t[len];
  __uint128_t *ab_y = new __uint128_t[len];
  uint64_t *hash_input = new uint64_t[len];
  // uint64_t (*d)[3] = new uint64_t[len][3];
  if (party == ALICE) {
    for (int i = 0; i < len; i++) {
      a[i] = os.random_val_input();
      b[i] = os.random_val_input();
      c[i] = os.random_val_input();
      ab[i] = os.auth_compute_mul_send(a[i],b[i]);
      ab[i] = PR - LOW64(ab[i]);
      ab[i] = add_mod(ab[i], LOW64(c[i]));
      ab_y[i] = mult_mod(LOW64(a[i]), LOW64(b[i]));
      ab_y[i] = PR - LOW64(ab_y[i]);
      ab_y[i] = add_mod(ab[i], LOW64(ab_y[i]));
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
      hash_input[i] = os.auth_compute_mul_send_with_setup(a[i], b[i], c[i], ab[i], ain[i], bin[i], ab_y[i]);
    }

    // ios[0]->send_data(d, len * 3 * sizeof(uint64_t));

    block hash_output = Hash::hash_for_block(hash_input, len * 8);
    ios[0]->send_data(&hash_output, sizeof(block));

    delete[] ain;
    delete[] bin;

    std::cout << "sender time: " << time_from(start) <<" us" << std::endl;
    std::cout << "proof of sender for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    os.check_compute_mul(a, b, c, len);
  } else {
    for (int i = 0; i < len; i++) {
      a[i] = os.random_val_input();
      b[i] = os.random_val_input();
      c[i] = os.random_val_input();
      ab[i] = os.auth_compute_mul_recv(a[i],b[i]);
      ab[i] = PR - ab[i];
      ab_y[i] = mult_mod(a[i], b[i]);
      ab_y[i] = PR - ab_y[i];
      ab_y[i] = add_mod(ab[i], ab_y[i]);
    }
    os.andgate_correctness_check_manage();
    std::cout << "recver time for setup: " << time_from(t2)<<" us" << std::endl;

    auto start = clock_start();
    // ios[0]->recv_data(d, len * 3 * sizeof(uint64_t));
    for (int i = 0; i < len; ++i) 
      hash_input[i] = os.auth_compute_mul_recv_with_setup(a[i], b[i], c[i], ab[i], ab_y[i]);
    
    block hash_output = Hash::hash_for_block(hash_input, len * 8), output_recv;
    ios[0]->recv_data(&output_recv, sizeof(block));
    if (HIGH64(hash_output) == HIGH64(output_recv) && LOW64(hash_output) == LOW64(output_recv))
      std::cout<<"JQv1 success!\n";
    else std::cout<<"JQv1 fail!\n";

    std::cout << "recver time: " << time_from(start)<<" us" << std::endl;
    std::cout << "proof of recver for 1s: " << double(len)/time_from(start)*1000000 << std::endl;
    os.check_compute_mul(a, b, c, len);
  }

  //for (int i=0; i<7; i++)
  //    std::cout<<party<<": "<<i<<" -> "<<hash_input[i]<<"\n";

  delete[] a;
  delete[] b;
  delete[] c;
  delete[] ab;
  delete[] hash_input;
  delete[] ab_y;
  
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


  // test_ostriple(ios, party);
  test_compute_and_gate_check_JQv1(ios, party);

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  return 0;
}
