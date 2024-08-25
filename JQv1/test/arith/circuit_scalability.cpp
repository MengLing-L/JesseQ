#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
using namespace emp;
using namespace std;

int port, party;
const int threads = 1;

void test_circuit_zk(NetIO *ios[threads + 1], int party,
                     int input_sz_lg) {

  // long long chunk = 1 << input_sz_lg;
  // long long test_n = 300000000;
  // int chunk = 30000000;
  long long test_n = 1024 * 1024 * 10 * 10 * 3;
  int chunk = 1024 * 100;
  int num_of_chunk = test_n / chunk;
  FpOSTriple<NetIO> ostriple(party, threads, ios);
  
  uint64_t *d = new uint64_t[chunk + 1];
  __uint128_t *ao = new __uint128_t[chunk + 1];
  __uint128_t *ab = new __uint128_t[chunk];
  __uint128_t a_u, b_u, b_u_0;
  uint64_t db;
  // ao[0] = ostriple.random_val_input();
  a_u = ostriple.random_val_input();
  b_u = ostriple.random_val_input();
  b_u_0 = b_u;
  
  auto start= clock_start();
  auto setup= 0;
  auto prove= 0;

  uint64_t ar = 2, br = 3;
  for (int j = 0; j < num_of_chunk; ++j) {
    start = clock_start();
    for (int i = 0; i < chunk; ++i) {
      __uint128_t ab_, tmp;
      if (i == 0) {
        ao[i] = a_u;
      }
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
    a_u = ao[chunk];
    ostriple.andgate_correctness_check_manage();
    ostriple.check_cnt = 0;
    setup += time_from(start);

    start = clock_start();
    if (party == ALICE) {
      db = PR - br;
      db = add_mod(HIGH64(b_u_0), db);
      ios[0]->send_data(&db, sizeof(uint64_t));
    } else {
      ios[0]->recv_data(&db, sizeof(uint64_t));
    }

    if (party == ALICE) {
      for (int i = 0; i < chunk; ++i) {
        d[i] = PR - ar;
        d[i] = add_mod(HIGH64(ao[i]), d[i]);
        br = add_mod(ar, br);
        ar = mult_mod(ar, br);
      }
      d[chunk] = PR - ar;
      d[chunk] = add_mod(HIGH64(ao[chunk]), d[chunk]);

      ios[0]->send_data(d, sizeof(uint64_t) * (chunk+1));
      ios[0]->send_data(&ar, sizeof(uint64_t));
    } else {
      ios[0]->recv_data(d, sizeof(uint64_t) * (chunk+1));
      ios[0]->recv_data(&ar, sizeof(uint64_t));
    }

    for (int i = 0; i < chunk; ++i) {
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
      block hash_output = Hash::hash_for_block(ab, sizeof(__uint128_t) * (chunk));
      ios[0]->send_data(&hash_output, sizeof(block));
    } else {
      block hash_output = Hash::hash_for_block(ab, sizeof(__uint128_t) * (chunk)), output_recv;
      ios[0]->recv_data(&output_recv, sizeof(block));
      if (HIGH64(hash_output) != HIGH64(output_recv) || LOW64(hash_output) != LOW64(output_recv))
        std::cout<<"JQv1 fail!\n";
    }
    prove += time_from(start);
  }

  if (party == ALICE) { 
    __uint128_t val = ar;
    ao[chunk] = (val << 64) ^ LOW64(ao[chunk]);
    ostriple.reveal_check_send(&(ao[chunk]), &ar, 1);
  } else { 
    ostriple.auth_constant(d[chunk], ao[chunk]);
    ostriple.reveal_check_recv(&(ao[chunk]), &ar, 1);
  }

  cout << "Setup time: " << setup / 1000 << "ms " << party
        << " " << endl;

  cout << test_n << "\t" << (prove) << "\t" << party << " " << endl;
  cout << test_n << "\t" << double(test_n)/(prove)*1000000 << "\t" << party << " " << endl;
  std::cout << std::endl;

  delete[] ao;
  delete[] ab;
  delete[]  d;

}

int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  NetIO *ios[threads];
  for (int i = 0; i < threads; ++i)
    // ios[i] = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i);
    ios[i] = new NetIO(party == ALICE ? nullptr : "172.31.53.21", port + i);
    // ios[i] = new NetIO(party == ALICE ? "172.31.59.161" : "172.31.59.161", port + i);

  std::cout << std::endl
            << "------------ circuit zero-knowledge proof test ------------"
            << std::endl;

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
