#ifndef ZK_FP_EXECUTION_H__
#define ZK_FP_EXECUTION_H__

#include "emp-zk/emp-vole/emp-vole.h"
#include "emp-zk/emp-zk-arith/ostriple.h"
#include "emp-zk/edabit/edabits.h"

class ZKFpExec {
public:
  int64_t gid = 0;
  __uint128_t pub_mac;
  int B, c;

  static ZKFpExec *zk_exec;

  ZKFpExec() {}
  virtual ~ZKFpExec() {}

  virtual void feed(__uint128_t &label, const uint64_t &val, uint64_t &d) = 0;

  virtual void feed(__uint128_t &label, const uint64_t &value) = 0;

  virtual void feed(__uint128_t *label, const uint64_t *value, int len) = 0;

  virtual void reveal(__uint128_t *label, uint64_t *value, int len) = 0;

  virtual void reveal_check(__uint128_t *label, const uint64_t *value,
                            int len) = 0;

  virtual void reveal_check_zero(__uint128_t *label, int len) = 0;

  virtual __uint128_t add_gate(const __uint128_t &a, const __uint128_t &b) = 0;

  virtual __uint128_t mul_gate(const __uint128_t &a, const __uint128_t &b) = 0;

  virtual __uint128_t mul_const_gate(const __uint128_t &a,
                                     const uint64_t &b) = 0;

  virtual __uint128_t pub_label(const uint64_t &a) = 0;

  virtual void random_val_input(__uint128_t &label) = 0;

  virtual __uint128_t auth_compute_mul(__uint128_t &a, __uint128_t &b) = 0;
};

// ZKFpExec * ZKFpExec::zk_exec = nullptr;
#endif
