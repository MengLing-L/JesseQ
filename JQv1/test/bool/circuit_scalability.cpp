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

int port, party;
char *ip;
const int threads = 2;

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

void test_compute_and_gate_check_JQv1(OSTriple<BoolIO<NetIO>> *os,
                                 BoolIO<NetIO> *io) {
  PRG prg;
  // long long len = 300000000;
  // int chunk = 30000000;
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
  long long len = 1024 * 1024 * 10 * 10 * 3;
  int chunk = 1024 * 1024 * 10;
  int num_of_chunk = len / chunk;
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  uint8_t output[BLAKE3_OUT_LEN], output_recv[BLAKE3_OUT_LEN];
  bool *d = new bool[chunk + 1];
  block *a = new block[chunk + 1];
  block *ab = new block[chunk];
  block a_u, b_u, b_u_0;
  bool db;
  
  
  // if (party == ALICE) {
  //   prg.random_bool(ain, 2 * chunk);
  // }

  auto start= clock_start();
  auto setup= 0;
  auto prove= 0;
  auto circuit_independe_setup=0;
  auto start_circuit_in =clock_start();
  start_circuit_in = clock_start();
  os->random_bits_input(a, chunk + 1);
  os->random_bits_input(&b_u, 1);
  a_u = a[0];
  b_u_0 = b_u;
  circuit_independe_setup += time_from(start_circuit_in);

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
      if (cpu_flag) {
        block hash_output = Hash::hash_for_block(ab, sizeof(uint64_t) * (chunk));
        io[0].send_data(&hash_output, sizeof(block));
      } else {
        blake3_hasher_update(&hasher, ab, sizeof(block) * (chunk));
        blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
        io[0].send_data(&output, BLAKE3_OUT_LEN);
      }
    } else {
      if (cpu_flag) {
        block hash_output = Hash::hash_for_block(ab, sizeof(uint64_t) * (chunk));
        io[0].recv_data(&output_recv, sizeof(block));
        if (memcmp(&hash_output, &output_recv, sizeof(block)) != 0)
          std::cout<<"JQv1 fail!\n";
      } else {
        blake3_hasher_update(&hasher, ab, sizeof(block) * (chunk));
        blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
        io[0].recv_data(&output_recv, BLAKE3_OUT_LEN);
        if (memcmp(output, output_recv, BLAKE3_OUT_LEN) != 0)
          std::cout<<"JQv1 fail!\n";
      }
    }
    prove += time_from(start);
  }

  cout << "Total Setup time: " << (setup + circuit_independe_setup) / 1000 << " ms " << party
        << " " << endl;
  cout << "Circui-independ Setup time: " << circuit_independe_setup / 1000 << " ms " << party
        << " " << endl;
  cout << "Circui-depend Setup time: " << (setup) / 1000 << " ms " << party
        << " " << endl;

  // cout << len << "\t" << (prove) << "\t" << party << " " << endl;
  cout << len << "\t Prove time: " << (prove)/ 1000 << " ms " << "\t" << party << " " << endl;

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
  party = atoi (argv[1]);
	port = atoi (argv[2]);
  ip = argv[3];
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
