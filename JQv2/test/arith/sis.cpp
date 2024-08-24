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

void test_sis_proof(NetIO *ios[threads + 1], int party, int n, int m) {

  srand(time(NULL));

  FpOSTriple<NetIO> ostriple(party, threads, ios);

  int mat_size = n * m;
  uint64_t *A, *s, *t;
  A = new uint64_t[mat_size];
  s = new uint64_t[m];
  t = new uint64_t[n + m];
  for (int i = 0; i < mat_size; ++i)
    A[i] = mod(rand());
  for (int i = 0; i < m; ++i)
    s[i] = rand() % 2;
  for (int i = 0; i < (m + n); ++i) {
    t[i] = (uint64_t)0;
  }
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      t[i] = add_mod(t[i], A[i * m + j] * s[j]);
    }
  }

  // allocation
  __uint128_t *vec_s = new __uint128_t[m];
  __uint128_t *vec_t = new __uint128_t[n + m];
  __uint128_t *vec_r = new __uint128_t[m];


  // init
  for (int i = 0; i < m; ++i)
    vec_s[i] = ostriple.random_val_input();

  for (int i = 0; i < n + m; ++i) {
    if (party == ALICE) {
      vec_t[i] = ostriple.authenticated_val_input(0);
    } else {
      vec_t[i] = ostriple.authenticated_val_input();
    }
  }
  // r[i] = s[i]^2
  for (int i = 0; i < m; ++i) {
    __uint128_t ab, tmp;
    if (party == ALICE) {
      tmp = ostriple.auth_compute_mul_send(vec_s[i], vec_s[i]);
      tmp = PR - LOW64(tmp);
      ab = mult_mod(LOW64(vec_s[i]), LOW64(vec_s[i]));
      ab = PR - LOW64(ab);
      vec_r[i] = add_mod(LOW64(tmp), LOW64(ab));
    } else {
      tmp = ostriple.auth_compute_mul_recv(vec_s[i], vec_s[i]);
      tmp = PR - tmp;
      ab = mult_mod(vec_s[i], vec_s[i]);
      ab = PR - ab;
      vec_r[i] = add_mod(tmp, ab);
    }
  }

  // y[i] = sum{A[j][i]*s[i]}
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      __uint128_t tmp;
      if (party == ALICE) {
        tmp = mult_mod(LOW64(vec_s[j]), A[i * m + j]);
        vec_t[i] = add_mod(LOW64(vec_t[i]), tmp);
      } else {
        tmp = mult_mod(vec_s[j], A[i * m + j]);
        vec_t[i] = add_mod(vec_t[i], tmp);
      }
    }
  }

  // y[i+n] = r[i] - s[i]
  for (int i = 0; i < m; ++i) {
    if (party == ALICE) {
      vec_t[i + n] = add_mod(LOW64(vec_r[i]), LOW64(vec_s[i]));
    } else {
      vec_t[i + n] = add_mod(vec_r[i], vec_s[i]);
    }
  }

  auto start = clock_start();

  for (int i = 0; i < m; ++i) {
    if (party == ALICE) {
      uint64_t sa = PR - s[i];
      s[i] = add_mod(HIGH64(vec_s[i]), sa);
    }
  }

  if (party == ALICE) { 
    ios[0]->send_data(s, sizeof(uint64_t) * m);
  } else {
    ios[0]->recv_data(s, sizeof(uint64_t) * m);
  }

  // r[i] = s[i]^2
  for (int i = 0; i < m; ++i) {
    if (party == ALICE) {
      ostriple.auth_compute_mul_send_with_setup(vec_s[i], vec_s[i], s[i], s[i], vec_t[i + n]);
    } else {
      ostriple.auth_compute_mul_recv_with_setup(vec_s[i], vec_s[i], s[i], s[i], vec_t[i + n]);
    }
  }

  // y[i] = sum{A[j][i]*s[i]}
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      if (party != ALICE) {
        ostriple.auth_scal_recv_with_setup(A[i * m + j], s[j], vec_t[i]);
      }
    }
  }

  // y[i+n] = r[i] - s[i]
  for (int i = 0; i < m; ++i) {
    if (party != ALICE) {
      ostriple.auth_add_recv_with_setup(s[i], vec_t[i + n]);
    }
  }

  if (party == ALICE) {
    block hash_output = Hash::hash_for_block(vec_t, (n+m) * 16);
    ios[0]->send_data(&hash_output, sizeof(block));
  } else {
    for (int i = 0; i < (n+m); ++i) {
      uint64_t constant = 0;
      constant = t[i];
      ostriple.auth_constant(constant, vec_t[i]);
    }
    block hash_output = Hash::hash_for_block(vec_t,  (n+m) * 16), output_recv;
    ios[0]->recv_data(&output_recv, sizeof(block));
    if (HIGH64(hash_output) == HIGH64(output_recv) && LOW64(hash_output) == LOW64(output_recv))
      std::cout<<"JQv2 success!\n";
    else std::cout<<"JQv2 fail!\n";
  }

  auto timeuse = time_from(start);
  cout << n << "\t" << m << "\t" << timeuse  << " us\t" << party << " "
       << endl;

  delete[] A;
  delete[] s;
  delete[] t;
  delete[] vec_s;
  delete[] vec_t;
  delete[] vec_r;

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

  // test_sis_proof(ios, party, 2, 2);
  test_sis_proof(ios, party, 1024, 1024);

  for (int i = 0; i < threads; ++i) {
    delete ios[i];
  }
  return 0;
}
