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


void test_circuit_zk_Freivalds(NetIO *ios[threads + 1], int party, int matrix_sz) {
  long long test_n = matrix_sz * matrix_sz;

  FpOSTriple<NetIO> ostriple(party, threads, ios);
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  uint8_t output[BLAKE3_OUT_LEN], output_recv[BLAKE3_OUT_LEN];

  uint64_t *ar, *br, *br_copy, *cr, *yr, *zr, *rr, *check_br;
  ar = new uint64_t[test_n];
  br = new uint64_t[test_n];
  br_copy = new uint64_t[test_n];
  cr = new uint64_t[test_n];
  yr = new uint64_t[matrix_sz];
  zr = new uint64_t[matrix_sz];
  rr = new uint64_t[matrix_sz];
  check_br = new uint64_t[matrix_sz];
  for (int i = 0; i < test_n; ++i) {
    ar[i] = i;
    br_copy[i] = br[i] = (test_n - i);
  }
  for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      for (int k = 0; k < matrix_sz; ++k) {
        uint64_t tmp = (ar[i * matrix_sz + j] * br[j * matrix_sz + k]) % pr;
        cr[i * matrix_sz + k] = (cr[i * matrix_sz + k] + tmp) % pr;
      }
    }
  }
  
  for (int i = 0; i < matrix_sz; ++i) {
    yr[i] = 0;
    zr[i] = 0;
  }
  

  

  __uint128_t *mat_a = new __uint128_t[test_n];
  __uint128_t *mat_b = new __uint128_t[test_n];
  __uint128_t *vec_y = new __uint128_t[matrix_sz];
  __uint128_t *vec_ay = new __uint128_t[matrix_sz];

  for (int i = 0; i < test_n; ++i) {
    mat_a[i] = ostriple.random_val_input();
    mat_b[i] = ostriple.random_val_input();
  }
  for (int i = 0; i < matrix_sz; ++i) {
    vec_y[i] = ostriple.random_val_input();
    if (party == ALICE) {
      vec_ay[i] = __uint128_t(0);
    } else {
      vec_ay[i] = __uint128_t(0);
    }
  }

  for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      __uint128_t ab, tmp;
      if (party == ALICE) {
        tmp = ostriple.auth_compute_mul_send(mat_a[i * matrix_sz + j], vec_y[j]);
        tmp = PR - LOW64(tmp);
        ab = mult_mod(LOW64(mat_a[i * matrix_sz + j]), LOW64(vec_y[j]));
        ab = PR - LOW64(ab);
        vec_ay[i] = add_mod(LOW64(vec_ay[i]), LOW64(tmp));
        vec_ay[i] = add_mod(LOW64(vec_ay[i]), LOW64(ab));
      } else {
        tmp = ostriple.auth_compute_mul_recv(mat_a[i * matrix_sz + j], vec_y[j]);
        tmp = PR - tmp;
        ab = mult_mod(mat_a[i * matrix_sz + j], vec_y[j]);
        ab = PR - ab;
        vec_ay[i] = add_mod(vec_ay[i], tmp);
        vec_ay[i] = add_mod(vec_ay[i], ab);
      }
    }
  }

  auto start = clock_start();

  if (party == ALICE) { 
    for (int i = 0; i < test_n; ++i) {
      uint64_t sa = PR - ar[i];
      ar[i] = add_mod(HIGH64(mat_a[i]), sa);
      uint64_t sb = PR - br[i];
      br[i] = add_mod(HIGH64(mat_b[i]), sb);
    }
    ios[0]->send_data(ar, sizeof(uint64_t) * test_n);
    ios[0]->send_data(br, sizeof(uint64_t) * test_n);
    ios[0]->recv_data(rr, sizeof(uint64_t) * matrix_sz);
    for (int i = 0; i < matrix_sz; ++i) {
      for (int j = 0; j < matrix_sz; ++j) {
        uint64_t tmp = (br_copy[i * matrix_sz + j] * rr[j]) % pr;
        yr[i] = (yr[i] + tmp) % pr;
      }
      uint64_t sb = PR - yr[i];
      yr[i] = add_mod(HIGH64(vec_y[i]), sb);
    }
    ios[0]->send_data(yr, sizeof(uint64_t) * matrix_sz);
  } else {
    ios[0]->recv_data(ar, sizeof(uint64_t) * test_n);
    ios[0]->recv_data(br, sizeof(uint64_t) * test_n);
    for (int i = 0; i < matrix_sz; ++i) {
      rr[i] = i+1;// modify it to be randomized!!!
    }
    ios[0]->send_data(rr, sizeof(uint64_t) * matrix_sz);
    ios[0]->recv_data(yr, sizeof(uint64_t) * matrix_sz);
  }
  for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      uint64_t tmp = (cr[i * matrix_sz + j] * rr[j]) % pr;
      zr[i] = (zr[i] + tmp) % pr;
    }
  }

  for (int i = 0; i < matrix_sz; ++i) {
    for (int j = 0; j < matrix_sz; ++j) {
      if (party == ALICE) {
        ostriple.auth_compute_mul_send_with_setup(mat_a[i * matrix_sz + j], vec_y[j], ar[i * matrix_sz + j], yr[j], vec_ay[i]);
      } else {
        ostriple.auth_compute_mul_recv_with_setup(mat_a[i * matrix_sz + j], vec_y[j], ar[i * matrix_sz + j], yr[j], vec_ay[i]);
      }
    }
  }

  if (party != ALICE) {
    for (int i = 0; i < matrix_sz; ++i) {
      ostriple.auth_add_recv_with_setup(yr[i], vec_y[i]);
      for (int j = 0; j < matrix_sz; ++j) {
        ostriple.auth_add_recv_with_setup(br[i * matrix_sz + j], mat_b[i * matrix_sz + j]);
      }
    }
  }

  for (int i = 0; i < matrix_sz; ++i) {
    check_br[i] = PR - LOW64(vec_y[i]);
    for (int j = 0; j < matrix_sz; ++j) {
      uint64_t tmp = mult_mod(LOW64(mat_b[i * matrix_sz + j]), rr[j]);
      check_br[i] = add_mod(tmp, check_br[i]);
    }
  }


  if (party == ALICE) {
    blake3_hasher_update(&hasher, vec_ay, sizeof(__uint128_t) * (matrix_sz));
    blake3_hasher_update(&hasher, check_br, sizeof(uint64_t) * (matrix_sz));
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    ios[0]->send_data(&output, BLAKE3_OUT_LEN);
  } else {
    for (int i = 0; i < matrix_sz; ++i) {
      uint64_t constant = 0;
      constant = PR - zr[i];
      ostriple.auth_constant(constant, vec_ay[i]);
    }
    blake3_hasher_update(&hasher, vec_ay, sizeof(__uint128_t) * (matrix_sz));
    blake3_hasher_update(&hasher, check_br, sizeof(uint64_t) * (matrix_sz));

    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    ios[0]->recv_data(&output_recv, BLAKE3_OUT_LEN);
    if (memcmp(output, output_recv, BLAKE3_OUT_LEN) != 0)
      std::cout<<"JQv2 fail!\n";
    else
      std::cout<<"JQv2 Success!\n";
  }

  std::cout<< "communication" << (test_n + matrix_sz) * 2 *  sizeof(uint64_t) + sizeof(block) << " bytes." << endl;

  auto timeuse = time_from(start);
  cout << matrix_sz << "\t" << timeuse / 1000000 << " s\t" << party << " " << endl;
  std::cout << std::endl;

  delete[] ar;
  delete[] br;
  delete[] cr;
  delete[] yr;
  delete[] zr;
  delete[] rr;
  delete[] check_br;
  delete[] mat_a;
  delete[] mat_b;
  delete[] vec_y;
  delete[] vec_ay;

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

void test_circuit_zk(NetIO *ios[threads + 1], int party, int matrix_sz) {
  long long test_n = matrix_sz * matrix_sz;

  FpOSTriple<NetIO> ostriple(party, threads, ios);
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  uint8_t output[BLAKE3_OUT_LEN], output_recv[BLAKE3_OUT_LEN];

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
    // __uint128_t pro;
    // pro = mat_ab[0];
    // for (int i = 1; i < test_n; i++) {
    //   pro = mult_mod(pro, mat_ab[i]);
    // } 
    // ios[0]->send_data(&pro, sizeof(__uint128_t));
    blake3_hasher_update(&hasher, mat_ab, sizeof(__uint128_t) * (test_n));
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    ios[0]->send_data(&output, BLAKE3_OUT_LEN);
  } else {
    for (int i = 0; i < test_n; ++i) {
      uint64_t constant = 0;
      constant = PR - cr[i];
      ostriple.auth_constant(constant, mat_ab[i]);
    }
    blake3_hasher_update(&hasher, mat_ab, sizeof(__uint128_t) * (test_n));
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    ios[0]->recv_data(&output_recv, BLAKE3_OUT_LEN);
    if (memcmp(output, output_recv, BLAKE3_OUT_LEN) != 0)
      std::cout<<"JQv2 fail!\n";
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

  //test_circuit_zk(ios, party, num);
  test_circuit_zk_Freivalds(ios, party, num);

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  return 0;
}
