#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
#include "blake3.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

int port, party, chunk;
long long len;
char *ip;
const int threads = 1;


void test_compute_and_gate_check_JQv1(OSTriple<BoolIO<NetIO>> *os,
                                 BoolIO<NetIO> *io) {
  PRG prg;
  int num_of_chunk = len / chunk;
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  uint8_t output[BLAKE3_OUT_LEN], output_recv[BLAKE3_OUT_LEN];
  bool *d = new bool[chunk + 1];
  block *a = new block[chunk + 1];
  block *ab = new block[chunk];
  block a_u, b_u, b_u_0;
  bool db;
  os->random_bits_input(a, chunk + 1);
  os->random_bits_input(&b_u, 1);
  a_u = a[0];
  b_u_0 = b_u;
  
  // if (party == ALICE) {
  //   prg.random_bool(ain, 2 * chunk);
  // }

  auto start= clock_start();

  // start= clock_start();
  // bool ar = true, br = false;
  // for (int i = 0; i < len; ++i) {
  //   br = ar ^ br;
  //   ar = ar & br;
  // }
  // cout << len << "Plant test eval\t" << double(len)/(time_from(start))*1000000 << "\t" << party << " " << endl;

  bool ar = true, br = false;
  for (int j = 0; j < num_of_chunk; ++j) {
    start = clock_start();
    for (int i = 0; i < chunk; ++i) {
      if (i == 0) {
        a[i] = a_u;
      }
      b_u = a[i] ^ b_u;
      ab[i] = os->auth_compute_and(a[i], b_u);
    }
    a_u = a[chunk];
    os->andgate_correctness_check_manage();
    os->check_cnt = 0;


    start = clock_start();
    if (party == ALICE) {
      db = getLSB(b_u_0) ^ br;
      io[0].send_bit(db);
    } else {
      db = io[0].recv_bit();
    }

    if (party == ALICE) {
        for (int i = 0; i < chunk; ++i) {
          d[i] = getLSB(a[i]) ^ ar;
          br = ar ^ br;
          ar = ar & br;
        }
        d[chunk] = getLSB(a[chunk]) ^ ar;

        // io[0].send_data_internal(d, chunk + 1);
        io[0].send_bit(ar);
        io[0].send_bit(d[0]);
      } else {
        // io[0].recv_data_internal(d, chunk + 1);
        ar = io[0].recv_bit();
        d[0] = io[0].recv_bit();
      }


    for (int i = 0; i < chunk; ++i) {
      block tmp;
      b_u_0 = a[i] ^ b_u_0;
      if (party == ALICE) {
        io[0].send_bit(d[i + 1]);
        db = db ^ d[i];
        os->auth_compute_and_send_with_setup(a[i], b_u_0, a[i + 1], d[i], db, d[i + 1] ^  getLSB(a[i + 1]), ab[i]);
      } else {
        d[i + 1] = io[0].recv_bit();
        db = db ^ d[i];
        tmp = a[i + 1];
        os->adjust_kc(tmp, d[i + 1]);
        os->auth_compute_and_recv_with_setup(a[i], b_u_0, tmp, d[i], db, ab[i]);
      }
    }

    
    if (party == ALICE) {
      auto multime = clock_start();
      block tmp = ab[0];
      // gfmul(ab[0], ab[1], &tmp);
      for(int i = 1; i < chunk; i++) {
        gfmul(tmp, ab[i], &tmp);
      }
      cout << chunk << " MUL time \t\t" << time_from(multime) << " us \t" << endl;
      io[0].send_data(&tmp, sizeof(block));
      
      auto hashtime = clock_start();
      block hash_output = Hash::hash_for_block(ab, sizeof(block) * (chunk));
      cout << chunk << " SHA256 hash time \t" << time_from(hashtime) << " us \t" << endl;
      io[0].send_data(&hash_output, sizeof(block));
    
      hashtime = clock_start();
      blake3_hasher_update(&hasher, ab, sizeof(block) * (chunk));
      blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
      cout << chunk << " BLAKE3 hash time \t" << time_from(hashtime) << " us \t" << endl;
      io[0].send_data(&output, BLAKE3_OUT_LEN);
      
    } else {
      block tmp = ab[0], output_pro;
      for(int i = 1; i < chunk; i++) {
        gfmul(tmp, ab[i], &tmp);
      }
      io[0].recv_data(&output_pro, sizeof(block));
      if (cmpBlock(&output_pro, &tmp, 1) != 1)
            std::cout<<"JQv1 fail!\n";

      block hash_output = Hash::hash_for_block(ab, sizeof(block) * (chunk));
      io[0].recv_data(&output_recv, sizeof(block));
      if (memcmp(&hash_output, &output_recv, sizeof(block)) != 0)
        std::cout<<"JQv1 fail!\n";
        
      blake3_hasher_update(&hasher, ab, sizeof(block) * (chunk));
      blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
      io[0].recv_data(&output_recv, BLAKE3_OUT_LEN);
      if (memcmp(output, output_recv, BLAKE3_OUT_LEN) != 0)
        std::cout<<"JQv1 fail!\n";
      
    }
  }

  // cout << "Setup time: " << setup / 1000 << "ms " << party
  //       << " " << endl;

  // cout << len << "\t" << (prove) << "\t" << party << " " << endl;
  // cout << len << "\t" << double(len)/(prove)*1000000 << "\t" << party << " " << endl;

  delete[] a;
  delete[] ab;
  delete[]  d;
  
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

void test_circuit_zk(BoolIO<NetIO> *ios[threads + 1], int party) {
  auto t1 = clock_start();
  OSTriple<BoolIO<NetIO>> os(party, threads, ios);
  cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;


  test_compute_and_gate_check_JQv1(&os, ios[0]);
}

int main(int argc, char **argv) {
  party = atoi (argv[1]);
	port = atoi (argv[2]);
  ip = argv[3];
  len = atoi (argv[4]);
  chunk = atoi (argv[5]);
  BoolIO<NetIO> *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new BoolIO<NetIO>(
        new NetIO(party == ALICE ? nullptr : ip, port + i),
        party == ALICE);

  std::cout << std::endl
            << "------------ triple generation test ------------" << std::endl
            << std::endl;
  ;

  test_circuit_zk(ios, party);
  for (int i = 0; i < threads; ++i) {
    delete ios[i]->io;
    delete ios[i];
  }
  return 0;
}
