#include "emp-tool/emp-tool.h"
#include "emp-zk/emp-zk.h"
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

void test_circuit_zk(NetIO *ios[threads + 1], int party, int matrix_sz) {
  long long test_n = matrix_sz * matrix_sz;

  FpOSTriple<NetIO> ostriple(party, threads, ios);

  uint64_t *ar, *br, *cr;
  ar = new uint64_t[test_n];
  br = new uint64_t[test_n];
  cr = new uint64_t[test_n];
  for (int i = 0; i < test_n; ++i) {
    ar[i] = i;
    br[i] = (test_n - i);
    cr[i] = 0;
  }
  for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      for (int k = 0; k < matrix_sz; ++k) {
        uint64_t tmp = (ar[i * matrix_sz + j] * br[j * matrix_sz + k]) % pr;
        cr[i * matrix_sz + k] = (cr[i * matrix_sz + k] + tmp) % pr;
      }
    }
  }

  __uint128_t *mat_a = new __uint128_t[test_n];
  __uint128_t *mat_b = new __uint128_t[test_n];
  __uint128_t *mat_ab = new __uint128_t[test_n];
  // IntFp *mat_c = new IntFp[test_n];

  for (int i = 0; i < test_n; ++i) {
    mat_a[i] = ostriple.random_val_input();
    mat_b[i] = ostriple.random_val_input();
    // mat_c[i] = IntFp();
    if (party == ALICE) {
      mat_ab[i] = ostriple.authenticated_val_input(0);
    } else {
      mat_ab[i] = ostriple.authenticated_val_input();
    }
  }

  for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      for (int k = 0; k < matrix_sz; ++k) {
        __uint128_t ab, tmp;
        if (party == ALICE) {
          tmp = ostriple.auth_compute_mul_send(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k]);
          tmp = PR - LOW64(tmp);
          ab = mult_mod(LOW64(mat_a[i * matrix_sz + j]), LOW64(mat_b[j * matrix_sz + k]));
          ab = PR - LOW64(ab);
          mat_ab[i * matrix_sz + k] = add_mod(LOW64(mat_ab[i * matrix_sz + k]), LOW64(tmp));
          mat_ab[i * matrix_sz + k] = add_mod(LOW64(mat_ab[i * matrix_sz + k]), LOW64(ab));
        } else {
          tmp = ostriple.auth_compute_mul_recv(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k]);
          tmp = PR - tmp;
          ab = mult_mod(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k]);
          ab = PR - ab;
          mat_ab[i * matrix_sz + k] = add_mod(mat_ab[i * matrix_sz + k], tmp);
          mat_ab[i * matrix_sz + k] = add_mod(mat_ab[i * matrix_sz + k], ab);
        }
      }
    }
  }

  auto start = clock_start();

  for (int i = 0; i < test_n; ++i) {
    if (party == ALICE) {
      uint64_t sa = PR - ar[i];
      ar[i] = add_mod(HIGH64(mat_a[i]), sa);
      uint64_t sb = PR - br[i];
      br[i] = add_mod(HIGH64(mat_b[i]), sb);
    }
  }

  if (party == ALICE) { 
    ios[0]->send_data(ar, sizeof(uint64_t) * test_n);
    ios[0]->send_data(br, sizeof(uint64_t) * test_n);
  } else {
    ios[0]->recv_data(ar, sizeof(uint64_t) * test_n);
    ios[0]->recv_data(br, sizeof(uint64_t) * test_n);
  }

  for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      for (int k = 0; k < matrix_sz; ++k) {
         if (party == ALICE) {
          ostriple.auth_compute_mul_send_with_setup(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k], ar[i * matrix_sz + j], br[j * matrix_sz + k], mat_ab[i * matrix_sz + k]);
         } else {
          ostriple.auth_compute_mul_recv_with_setup(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k], ar[i * matrix_sz + j], br[j * matrix_sz + k], mat_ab[i * matrix_sz + k]);
         }
      }
    }
  }

  if (party == ALICE) {
    block hash_output = Hash::hash_for_block(mat_ab, test_n * 8);
    ios[0]->send_data(&hash_output, sizeof(block));
  } else {
    for (int i = 0; i < test_n; ++i) {
      uint64_t constant = 0;
      constant = PR - cr[i];
      ostriple.auth_constant(constant, mat_ab[i]);
    }
    block hash_output = Hash::hash_for_block(mat_ab, test_n * 8), output_recv;
    ios[0]->recv_data(&output_recv, sizeof(block));
    if (HIGH64(hash_output) == HIGH64(output_recv) && LOW64(hash_output) == LOW64(output_recv))
      std::cout<<"JQv2 success!\n";
    else std::cout<<"JQv2 fail!\n";
  }

  std::cout<< "communication" << test_n * 2 *  sizeof(uint64_t) + sizeof(block) << " bytes." << endl;

  auto timeuse = time_from(start);
  cout << matrix_sz << "\t" << timeuse << " us\t" << party << " " << endl;
  std::cout << std::endl;

  delete[] ar;
  delete[] br;
  delete[] cr;
  delete[] mat_a;
  delete[] mat_b;
  delete[] mat_ab;

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

int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  NetIO *ios[threads];
  for (int i = 0; i < threads; ++i)
    // ios[i] = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i);
    ios[i] = new NetIO(party == ALICE ? nullptr : "172.31.2.203", port + i);
  // ios[i] = new NetIO(party == ALICE ? "172.31.5.65" : "172.31.5.65", port + i),

  std::cout << std::endl
            << "------------ circuit zero-knowledge proof test ------------"
            << std::endl
            << std::endl;
  ;

  int num = 0;
  if (argc < 3) {
    std::cout << "usage: bin/arith/matrix_mul_arith PARTY PORT DIMENSION"
              << std::endl;
    return -1;
  } else if (argc == 3) {
    num = 1024;
  } else {
    num = atoi(argv[3]);
  }

  test_circuit_zk(ios, party, num);

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  return 0;
}
