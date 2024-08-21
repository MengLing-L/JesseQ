#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
using namespace emp;
using namespace std;

int port, party;
const int threads = 1;

void test_circuit_zk(NetIO *ios[threads + 1], int party,
                     int input_sz_lg) {

  long long test_n = 1 << input_sz_lg;
  test_n = 30000000;
  FpOSTriple<NetIO> ostriple(party, threads, ios);
  
  uint64_t *d = new uint64_t[test_n + 1];
  __uint128_t *ao = new __uint128_t[test_n + 1];
  __uint128_t *ab = new __uint128_t[test_n];
  __uint128_t b_u, b_u_0;
    uint64_t db;
  ao[0] = ostriple.random_val_input();
  b_u = ostriple.random_val_input();
  b_u_0 = b_u;
  
  auto start = clock_start();
  for (int i = 0; i < test_n; ++i) {
    __uint128_t ab_, tmp;
    if (party == ALICE) {
      __uint128_t val = mod((ao[i] >> 64) + (b_u >> 64), pr);
      __uint128_t mac = mod((ao[i] & 0xFFFFFFFFFFFFFFFFULL) + (b_u & 0xFFFFFFFFFFFFFFFFULL), pr);
      b_u = (val << 64) ^ mac;
      ab_ = mult_mod(LOW64(ao[i]), LOW64(b_u));
      ab_ = PR - LOW64(ab_);
      tmp = ostriple.auth_compute_mul_send(ao[i], b_u);
      tmp = PR - LOW64(tmp);
      ab[i] = add_mod(LOW64(tmp), LOW64(ab_));
      ao[i + 1] = ostriple.random_val_input();
      ab[i] = add_mod(ab[i], LOW64(ao[i + 1]));
    } else {
      b_u = mod(ao[i] + b_u, pr);
      ab_ = mult_mod(ao[i], b_u);
      ab_ = PR - ab_;
      tmp = ostriple.auth_compute_mul_recv(ao[i], b_u);
      tmp = PR - tmp;
      ab[i] = add_mod(tmp, ab_);
      ao[i + 1] = ostriple.random_val_input();
    }
  }
  ostriple.andgate_correctness_check_manage();
  ostriple.check_cnt = 0;
  cout << "Setup time: " << time_from(start) / 1000 << "ms " << party
       << " " << endl;
  // __uint128_t ab, tmp;
  // if (party == ALICE) {
  //   ab = mult_mod(LOW64(ao[test_n - 1]), LOW64(b_u));
  //   ab = PR - LOW64(ab);
  //   tmp = ostriple.auth_compute_mul_send(ao[test_n - 1], b_u);
  //   tmp = PR - LOW64(tmp);
  //   ab_o = add_mod(LOW64(tmp), LOW64(ab));
  //   c_u = ostriple.random_val_input();
  //   c_u = mult_mod(LOW64(c_u), LOW64(ao[test_n - 1]));
  // } else {
  //   ab = mult_mod(ao[test_n - 1], b_u);
  //   ab = PR - ab;
  //   tmp = ostriple.auth_compute_mul_recv(ao[test_n - 1], b_u);
  //   tmp = PR - LOW64(tmp);
  //   ab_o = add_mod(tmp, ab);
  //   c_u = ostriple.random_val_input();
  //   c_u = mult_mod(c_u, ao[test_n - 1]);
  // }

  start = clock_start();
  uint64_t ar = 2, br = 3;

  if (party == ALICE) {
    db = PR - br;
    db = add_mod(HIGH64(b_u_0), db);
    ios[0]->send_data(&db, sizeof(uint64_t));
  } else {
    ios[0]->recv_data(&db, sizeof(uint64_t));
  }

  if (party == ALICE) {
    for (int i = 0; i < test_n; ++i) {
      d[i] = PR - ar;
      d[i] = add_mod(HIGH64(ao[i]), d[i]);
      br = add_mod(ar, br);
      ar = mult_mod(ar, br);
    }
    d[test_n] = PR - ar;
    d[test_n] = add_mod(HIGH64(ao[test_n]), d[test_n]);

    ios[0]->send_data(d, sizeof(uint64_t) * (test_n+1));
    ios[0]->send_data(&ar, sizeof(uint64_t));
  } else {
    ios[0]->recv_data(d, sizeof(uint64_t) * (test_n+1));
    ios[0]->recv_data(&ar, sizeof(uint64_t));
  }

  for (int i = 0; i < test_n; ++i) {
    __uint128_t tmp;
    if (party == ALICE) {
      __uint128_t val = mod((ao[i] >> 64) + (b_u_0 >> 64), pr);
      __uint128_t mac = mod((ao[i] & 0xFFFFFFFFFFFFFFFFULL) + (b_u_0 & 0xFFFFFFFFFFFFFFFFULL), pr);
      b_u_0 = (val << 64) ^ mac;
      db = add_mod(d[i], db);
      ostriple.auth_compute_mul_send_with_setup(ao[i], b_u_0, ao[i + 1], d[i], db, ab[i]);
    } else {
      b_u_0 = mod(ao[i] + b_u_0, pr);
      db = add_mod(d[i], db);
      tmp = ao[i + 1];
      ostriple.auth_constant(d[i + 1], tmp);
      ostriple.auth_compute_mul_recv_with_setup(ao[i], b_u_0, tmp, d[i], db, ab[i]);
    }
  }

  if (party == ALICE) {
    block hash_output = Hash::hash_for_block(ab, sizeof(__uint128_t) * (test_n));
    ios[0]->send_data(&hash_output, sizeof(block));
  } else {
    block hash_output = Hash::hash_for_block(ab, sizeof(__uint128_t) * (test_n)), output_recv;
    ios[0]->recv_data(&output_recv, sizeof(block));
    if (HIGH64(hash_output) == HIGH64(output_recv) && LOW64(hash_output) == LOW64(output_recv))
      std::cout<<"JQv1 success!\n";
    else std::cout<<"JQv1 fail!\n";
  }

  if (party == ALICE) { 
    __uint128_t val = ar;
    ao[test_n] = (val << 64) ^ LOW64(ao[test_n]);
    ostriple.reveal_check_send(&(ao[test_n]), &ar, 1);
  } else { 
    ostriple.auth_constant(d[test_n], ao[test_n]);
    ostriple.reveal_check_recv(&(ao[test_n]), &ar, 1);
  }

  auto timeuse = time_from(start);
  cout << test_n << "\t" << (timeuse) << "\t" << party << " " << endl;
  cout << test_n << "\t" << double(test_n)/(timeuse)*1000000 << "\t" << party << " " << endl;
  std::cout << std::endl;

  delete[] ao;
  delete[] ab;
  delete[]  d;

}

int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  NetIO *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i);

  std::cout << std::endl
            << "------------ circuit zero-knowledge proof test ------------"
            << std::endl
            << std::endl;
  ;

  int num = 0;
  if (argc < 3) {
    std::cout << "usage: [binary] PARTY PORT LOG(NUM_GATES)" << std::endl;
    return -1;
  } else if (argc == 3) {
    num = 2;
  } else {
    num = atoi(argv[3]);
  }

  test_circuit_zk(ios, party, num);

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  return 0;
}
