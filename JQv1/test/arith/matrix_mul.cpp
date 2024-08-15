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
  // cout << party << "\tconstructor\t" << time_from(t1) << " us" << endl;

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

  auto start = clock_start();

  __uint128_t *mat_a = new __uint128_t[test_n];
  __uint128_t *mat_b = new __uint128_t[test_n];
  __uint128_t *mat_c = new __uint128_t[test_n];
  __uint128_t *mat_ab = new __uint128_t[test_n * matrix_sz];
  __uint128_t *ab = new __uint128_t[test_n * matrix_sz];
  __uint128_t *ab_y = new __uint128_t[test_n * matrix_sz];
  uint64_t *hash_input = new uint64_t[test_n * matrix_sz];

  for (int i = 0; i < test_n; ++i) {
    mat_a[i] = ostriple.random_val_input();
    mat_b[i] = ostriple.random_val_input();
  }
  int z = 0;
   for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      for (int k = 0; k < matrix_sz; ++k) {
        mat_ab[z] = ostriple.random_val_input();
        if (party == ALICE) {
          ab[z] = ostriple.auth_compute_mul_send(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k]);
          ab[z] = PR - LOW64(ab[z]);
          ab[z] = add_mod(ab[z], LOW64(mat_ab[z]));
          ab_y[z] = mult_mod(LOW64(mat_a[i * matrix_sz + j]), LOW64(mat_b[j * matrix_sz + k]));
          ab_y[z] = PR - LOW64(ab_y[z]);
          ab_y[z] = add_mod(ab[z], LOW64(ab_y[z]));
        } else {
          ab[z] = ostriple.auth_compute_mul_recv(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k]);
          ab[z] = PR - ab[z];
          ab_y[z] = mult_mod(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k]);
          ab_y[z] = PR - ab_y[z];
          ab_y[z] = add_mod(ab[z], ab_y[z]);
        }
        z++;
      }
    }
  }
  std::cout << "Time for setup: " << time_from(start)<<" us" << std::endl;

  start = clock_start();
  z = 0;
  for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      for (int k = 0; k < matrix_sz; ++k) {
        if (party == ALICE) {
          hash_input[z] = ostriple.auth_compute_mul_send_with_setup(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k], mat_ab[z], mat_ab[z], ar[i * matrix_sz + j], br[j * matrix_sz + k], ab_y[z]);
        } else {
          hash_input[z] = ostriple.auth_compute_mul_recv_with_setup(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k], mat_ab[z], mat_ab[z], ab_y[z]);
        }
        z++;
      }
    }
  }
  if (party == ALICE) {
    block hash_output = Hash::hash_for_block(hash_input, test_n * matrix_sz * 8);
    ios[0]->send_data(&hash_output, sizeof(block));
  } else {
    block hash_output = Hash::hash_for_block(hash_input, test_n * matrix_sz * 8), output_recv;
    ios[0]->recv_data(&output_recv, sizeof(block));
    if (HIGH64(hash_output) == HIGH64(output_recv) && LOW64(hash_output) == LOW64(output_recv))
      std::cout<<"JQv1 matrix success!\n";
    else std::cout<<"JQv1 matrix fail!\n";
  }

  // batch_reveal_check(mat_c, cr, test_n);
  auto timeuse = time_from(start);
  // finalize_zk_arith<BoolIO<NetIO>>();
  cout << matrix_sz << "\t" << timeuse << " us\t" << party << " " << endl;
  std::cout << std::endl;

  delete[] ar;
  delete[] br;
  delete[] cr;
  delete[] mat_a;
  delete[] mat_b;
  delete[] mat_c;


delete[] ab; // delete outermost dimension
delete[] ab_y; // delete outermost dimension
delete[] hash_input; // delete outermost dimension
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
    ios[i] = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + i);

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
