#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
#include "blake3.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
using namespace emp;
using namespace std;

int port, party;
char *ip;
const int threads = 1;

std::string getCPUVendor() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo) {
        // std::cerr << "Error: Unable to open /proc/cpuinfo\n";
        return "Unknown";
    }

    std::string line, vendor = "Unknown";
    while (std::getline(cpuinfo, line)) {
        if (line.find("vendor_id") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                vendor = line.substr(pos + 2);
                break;
            }
        }
    }
    return vendor;
}

void test_circuit_zk(NetIO *ios[threads + 1], int party,
                     int input_sz_lg) {

  // long long chunk = 1 << input_sz_lg;
  long long test_n = 300000000;
  int chunk = 10000000;
  bool cpu_flag = false;
  std::string vendor = getCPUVendor();
  if (vendor == "GenuineIntel") {
      std::cout << "This is an Intel CPU.\n";
  } else if (vendor == "AuthenticAMD") {
      std::cout << "This is an AMD CPU.\n";
      cpu_flag = true;
  } else {
      std::cout << "Unknown CPU manufacturer.\n";
  }
  // long long test_n = 1024 * 1024 * 10 * 10 * 3;
  // int chunk = 1024 * 10;
  int num_of_chunk = test_n / chunk;
  FpOSTriple<NetIO> ostriple(party, threads, ios);
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  uint8_t output[BLAKE3_OUT_LEN], output_recv[BLAKE3_OUT_LEN];
  
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

  start= clock_start();
  uint64_t ar = 2, br = 3;
  for (int i = 0; i < test_n; ++i) {
    br = add_mod(ar, br);
    ar = mult_mod(ar, br);
  }
  cout << test_n << "Plant test eval\t" << double(test_n)/(time_from(start))*1000000 << "\t" << party << " " << endl;

  ar = 2, br = 3;
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

    cout << "SetUp\t" << endl;
    
    if (party == ALICE) {
      start = clock_start();
      db = PR - br;
      db = add_mod(HIGH64(b_u_0), db);

      d[0] = PR - ar;
      d[0] = add_mod(HIGH64(ao[0]), d[0]);
      ios[0]->send_data(&db, sizeof(uint64_t));
      ios[0]->send_data(&d[0], sizeof(uint64_t));
      
    } else {
      start = clock_start();
      ios[0]->recv_data(&db, sizeof(uint64_t));
      ios[0]->recv_data(&d[0], sizeof(uint64_t));
    }

    for (int i = 0; i < chunk; ++i) {
      
      __uint128_t tmp;
      if (party == ALICE) {
        br = add_mod(ar, br);
        ar = mult_mod(ar, br);
        d[i + 1] = PR - ar;
        d[i + 1] = add_mod(HIGH64(ao[i + 1]), d[i + 1]);
        ios[0]->send_data(d + i + 1, sizeof(uint64_t));
        __uint128_t val = mod((ao[i] >> 64) + (b_u_0 >> 64), pr);
        __uint128_t mac = mod((ao[i] & 0xFFFFFFFFFFFFFFFFULL) + (b_u_0 & 0xFFFFFFFFFFFFFFFFULL), pr);
        b_u_0 = (val << 64) ^ mac;
        db = add_mod(d[i], db);
        ostriple.auth_compute_mul_send_with_setup(ao[i], b_u_0, ao[i + 1], d[i], db, ab[i]);
      } else {
        ios[0]->recv_data(d + i + 1, sizeof(uint64_t));
        b_u_0 = mod(ao[i] + b_u_0, pr);
        db = add_mod(d[i], db);
        tmp = ao[i + 1];
        ostriple.auth_constant(d[i + 1], tmp);
        ostriple.auth_compute_mul_recv_with_setup(ao[i], b_u_0, tmp, d[i], db, ab[i]);
      }
    }

    if (party == ALICE) {
      auto multime = clock_start();
      __uint128_t pro;
      pro = ab[0];
      for (int i = 1; i < chunk; i++) {
        pro = mult_mod(pro, LOW64(ab[i]));
      } 
      cout << chunk << "mul time \t" << time_from(multime) << "\t" << party << " " << endl;
      
      if (cpu_flag) {
        auto hashtime = clock_start();
        block hash_output = Hash::hash_for_block(ab, sizeof(uint64_t) * (chunk));
        cout << chunk << "hash time \t" << time_from(hashtime) << "\t" << party << " " << endl;
        ios[0]->send_data(&hash_output, sizeof(block));
      } else {
        auto hashtime = clock_start();
        blake3_hasher_update(&hasher, ab, sizeof(uint64_t) * (chunk));
        blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
        cout << chunk << "hash time \t" << time_from(hashtime) << "\t" << party << " " << endl;
        ios[0]->send_data(&output, BLAKE3_OUT_LEN);
      }
      
      
    } else {
      if (cpu_flag) {
        block hash_output = Hash::hash_for_block(ab, sizeof(uint64_t) * (chunk));
        ios[0]->recv_data(&output_recv, sizeof(block));
        if (memcmp(&hash_output, &output_recv, sizeof(block)) != 0)
          std::cout<<"JQv1 fail!\n";
      } else {
        blake3_hasher_update(&hasher, ab, sizeof(uint64_t) * (chunk));
        blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
        ios[0]->recv_data(&output_recv, BLAKE3_OUT_LEN);
        if (memcmp(output, output_recv, BLAKE3_OUT_LEN) != 0)
          std::cout<<"JQv1 fail!\n";
      }
    }
    prove += time_from(start);
  }

  if (party == ALICE) { 
    ios[0]->send_data(&ar, sizeof(uint64_t));
    __uint128_t val = ar;
    ao[chunk] = (val << 64) ^ LOW64(ao[chunk]);
    ostriple.reveal_check_send(&(ao[chunk]), &ar, 1);
  } else { 
    ios[0]->recv_data(&ar, sizeof(uint64_t));
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
  party = atoi (argv[1]);
	port = atoi (argv[2]);
  ip = argv[3];
  NetIO *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new NetIO(party == ALICE ? nullptr : ip, port + i);

  std::cout << std::endl
            << "------------ circuit zero-knowledge proof test ------------"
            << std::endl;

  int num = 0;
  if (argc < 3) {
    std::cout << "usage: [binary] PARTY PORT IP" << std::endl;
    return -1;
  }
  num = 2;

  test_circuit_zk(ios, party, num);

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  return 0;
}
