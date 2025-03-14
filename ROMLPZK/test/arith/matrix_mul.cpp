#include "emp-tool/emp-tool.h"
#include "emp-zk/emp-zk.h"
#include "blake3.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
char *ip;
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
    mat_ab[i] = 0;
  }

  

  for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      for (int k = 0; k < matrix_sz; ++k) {
        __uint128_t ab, tmp;
        if (party == ALICE) {
          ab = ostriple.auth_compute_mul_send(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k]);
          mat_ab[i * matrix_sz + k] = add_mod(LOW64(mat_ab[i * matrix_sz + k]), LOW64(ab));
        } else {
          ab = ostriple.auth_compute_mul_recv(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k]);
          ostriple.mul_delta(tmp, ab);
          // mat_ab[i * matrix_sz + k] = add_mod(mat_ab[i * matrix_sz + k], tmp);
          mat_ab[i * matrix_sz + k] = add_mod(mat_ab[i * matrix_sz + k], tmp);
        }
      }
    }
  }

  // if (party == ALICE) {
  //   ios[0]->send_data(&mat_ab[0], sizeof(__uint128_t));
  // } else {
  //   __uint128_t tmp, tmp1, tmp2;
  //   ios[0]->recv_data(&tmp, sizeof(__uint128_t));
  //   tmp1 = HIGH64(tmp);
  //   ostriple.mul_delta(tmp2, tmp1);
  //   tmp2 = PR - tmp2;
  //   tmp2 = add_mod(LOW64(tmp), tmp2);
  //   // tmp2 = add_mod(cr[0], tmp2);
  //   if (tmp2 != mat_ab[0])
  //       std::cout<<"authen fail!\n";
  // }

  auto start = clock_start();

  for (int i = 0; i < test_n; ++i) {
    if (party == ALICE) {
      
      uint64_t sa = PR - ar[i];
      sa = add_mod(LOW64(mat_a[i]), sa);
      ios[0]->send_data(&sa, sizeof(uint64_t));
      mat_a[i] = (__uint128_t)makeBlock(HIGH64(mat_a[i]), ar[i]);

      uint64_t sb = PR - br[i];
      sb = add_mod(LOW64(mat_b[i]), sb);
      ios[0]->send_data(&sb, sizeof(uint64_t));
      mat_b[i] = (__uint128_t)makeBlock(HIGH64(mat_b[i]), br[i]);
    } else {
      uint64_t sa=0,sb=0;
      ios[0]->recv_data(&sa, sizeof(uint64_t));
      sa = PR - sa;
      mat_a[i] = add_mod(mat_a[i], sa);

      ios[0]->recv_data(&sb, sizeof(uint64_t));
      sb = PR - sb;
      mat_b[i] = add_mod(mat_b[i], sb);
    }
  }

  for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      for (int k = 0; k < matrix_sz; ++k) {
         if (party == ALICE) {
          ostriple.auth_compute_mul_send_with_setup(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k], mat_ab[i * matrix_sz + k]);
         } else {
          ostriple.auth_compute_mul_recv_with_setup(mat_a[i * matrix_sz + j], mat_b[j * matrix_sz + k], mat_ab[i * matrix_sz + k]);
         }
      }
    }
  }

  // if (party == ALICE) {
  //   ios[0]->send_data(&mat_ab[0], sizeof(__uint128_t));
  // } else {
  //   __uint128_t tmp, tmp1, tmp2;
  //   ios[0]->recv_data(&tmp, sizeof(__uint128_t));
  //   tmp1 = LOW64(tmp);
  //   ostriple.mul_delta(tmp2, tmp1);
  //   // tmp2 = PR - tmp2;
  //   // tmp2 = add_mod(LOW64(tmp), tmp2);
  //   tmp2 = add_mod(cr[0], tmp2);
  //   if (tmp2 != mat_ab[0])
  //       std::cout<<"authen fail!\n";
  //       std::cout<< LOW64(tmp2) << endl;
  //       std::cout<< LOW64(mat_ab[0]) << endl;
  // }

  if (party == ALICE) {
    __uint128_t pro;
    pro = mat_ab[0];
    for (int i = 1; i < test_n; i++) {
      pro = mult_mod(pro, mat_ab[i]);
    } 
    ios[0]->send_data(&pro,  sizeof(__uint128_t));
  } else {
    __uint128_t pro, output_recv;
    ios[0]->recv_data(&output_recv, sizeof(__uint128_t));
    uint64_t constant = 0;
    constant = PR - cr[0];
    ostriple.auth_constant(constant, mat_ab[0]);
    pro = mat_ab[0];
    ostriple.mul_delta(output_recv, output_recv);
    for (int i = 1; i < test_n; ++i) {
        uint64_t constant = 0;
        constant = PR - cr[i];
        ostriple.auth_constant(constant, mat_ab[i]);
        pro = mult_mod(pro, mat_ab[i]);
        ostriple.mul_delta(output_recv, output_recv);
    }
    if (HIGH64(pro) != HIGH64(output_recv) || LOW64(pro) != LOW64(output_recv))
        std::cout<<"LPZK fail!\n";
      // else 
      //   std::cout<<"LPZK sucess!\n";
    // __uint128_t pro, output_recv;
    // pro = mat_ab[0];
    // for (int i = 1; i < test_n; i++) {
    //   pro = mult_mod(pro, mat_ab[i]);
    // } 
    // ios[0]->recv_data(&output_recv, sizeof(__uint128_t));
    // if (HIGH64(pro) != HIGH64(output_recv) || LOW64(pro) != LOW64(output_recv))
    //   std::cout<<"JQv1 fail!\n";
  }

  std::cout<< "communication" << test_n * 2 *  sizeof(uint64_t) + sizeof(block) << " bytes." << endl;

  auto timeuse = time_from(start);
  cout << matrix_sz << "\t" << timeuse / 1000000 << " s\t" << party << " " << endl;
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
  party = atoi (argv[1]);
	port = atoi (argv[2]);
  ip = argv[3];
  NetIO *ios[threads];
  for (int i = 0; i < threads; ++i)
    ios[i] = new NetIO(party == ALICE ? nullptr : ip, port + i);

  std::cout << std::endl
            << "------------ circuit zero-knowledge proof test ------------"
            << std::endl
            << std::endl;
  ;

  int num = 0;
  if (argc < 3) {
    std::cout << "usage: bin/arith/matrix_mul_arith PARTY PORT IP"
              << std::endl;
    return -1;
  } 

  num = 1024;

  test_circuit_zk(ios, party, num);

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  return 0;
}
