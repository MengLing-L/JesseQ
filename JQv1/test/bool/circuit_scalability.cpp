#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
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

void test_compute_and_gate_check_JQv1(OSTriple<BoolIO<NetIO>> *os,
                                 BoolIO<NetIO> *io) {
  PRG prg;
  // long long len = 300000000;
  // int chunk = 30000000;
  long long len = 1024 * 1024 * 10 * 10 * 3;
  int chunk = 1024 ;
  int num_of_chunk = len / chunk;
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
  auto setup= 0;
  auto prove= 0;

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

    setup += time_from(start);

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
          // io[0].send_bit(d[i]);
          br = ar ^ br;
          ar = ar & br;
        }
        d[chunk] = getLSB(a[chunk]) ^ ar;

        io[0].send_data_internal(d, chunk + 1);
        io[0].send_bit(ar);
      } else {
        io[0].recv_data_internal(d, chunk + 1);
        ar = io[0].recv_bit();
      }


    for (int i = 0; i < chunk; ++i) {
      block tmp;
      b_u_0 = a[i] ^ b_u_0;
      if (party == ALICE) {
        db = db ^ d[i];
        os->auth_compute_and_send_with_setup(a[i], b_u_0, a[i + 1], d[i], db, d[i + 1] ^  getLSB(a[i + 1]), ab[i]);
      } else {
        db = db ^ d[i];
        tmp = a[i + 1];
        os->adjust_kc(tmp, d[i + 1]);
        os->auth_compute_and_recv_with_setup(a[i], b_u_0, tmp, d[i], db, ab[i]);
      }
    }

    if (party == ALICE) {
      block hash_output = Hash::hash_for_block(ab, sizeof(block) * (chunk));
      io[0].send_data(&hash_output, sizeof(block));
    } else {
      block hash_output = Hash::hash_for_block(ab, sizeof(block) * (chunk)), output_recv;
      io[0].recv_data(&output_recv, sizeof(block));
      if (HIGH64(hash_output) != HIGH64(output_recv) || LOW64(hash_output) != LOW64(output_recv))
        std::cout<<"JQv1 fail!\n";
    }
    // io[0].flush();
    // block seed = io[0].get_hash_block();
    // block share_seed;
    // PRG(&seed).random_block(&share_seed, 1);
    // block *chi = new block[chunk];
    // uni_hash_coeff_gen(chi, share_seed, chunk);
    // block sum;
    // gfmul(ab[0], ab[1], &sum);
    // if (party == ALICE) {
    //   vector_inn_prdt_sum_red(&sum, chi, ab, chunk);
    //   io[0].send_data(&sum, sizeof(block));
    // } else {
    //   for(int i = 0; i < chunk; i++) {
    //     block tmp;
    //     gfmul(chi[i], ab[i], &tmp);
    //     sum = tmp;
    //   }
    //   block output_recv;
    //   vector_inn_prdt_sum_red(&sum, chi, ab, chunk);
    //   io[0].recv_data(&output_recv, sizeof(block));
    //   if (HIGH64(sum) != HIGH64(output_recv) || LOW64(sum) != LOW64(output_recv))
    //     std::cout<<"JQv1 fail!\n";
    // }
    prove += time_from(start);
  }

  cout << "Setup time: " << setup / 1000 << "ms " << party
        << " " << endl;

  cout << len << "\t" << (prove) << "\t" << party << " " << endl;
  cout << len << "\t" << double(len)/(prove)*1000000 << "\t" << party << " " << endl;

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

  test_circuit_zk(ios, party);
  for (int i = 0; i < threads; ++i) {
    delete ios[i]->io;
    delete ios[i];
  }
  return 0;
}
