#ifndef INT_FP_H__
#define INT_FP_H__

#include "emp-tool/emp-tool.h"
#include "emp-zk/emp-zk-arith/polynomial.h"
#include "emp-zk/emp-zk-arith/zk_fp_exec.h"
#include "emp-zk/emp-zk-arith/zk_fp_exec_prover.h"
#include "emp-zk/emp-zk-arith/zk_fp_exec_verifier.h"
#include "emp-zk/emp-zk-bool/emp-zk-bool.h"

class IntFp {
public:
  __uint128_t value;
  uint64_t d;

  IntFp() {
    ZKFpExec::zk_exec->random_val_input(value);
  }

  IntFp(IntFp *obj) { this->value = obj->value; }

  IntFp(uint64_t input, int party = PUBLIC) {
    if (party == PUBLIC) {
      value = ZKFpExec::zk_exec->pub_label(input);
    } else {
      ZKFpExec::zk_exec->feed(value, input);
    }
  }

  IntFp(uint64_t input, int party = PUBLIC, int flag=1) {
      ZKFpExec::zk_exec->feed(value, input, d);
  }

  __uint128_t get_u(){
    return this->value;
  }

  uint64_t get_d(){
    return this->d;
  }

  uint64_t reveal() {
    uint64_t out;
    ZKFpExec::zk_exec->reveal(&(this->value), &out, 1);
    return out;
  }

  bool reveal(uint64_t expect) {
    ZKFpExec::zk_exec->reveal_check(&(this->value), &expect, 1);
    return true;
  }

  void reveal_zero() {
    ZKFpExec::zk_exec->reveal_check_zero(&(this->value), 1);
  }

  IntFp operator+(const IntFp &rhs) const {
    IntFp res(*this);
    res.value = ZKFpExec::zk_exec->add_gate(this->value, rhs.value);
    return res;
  }

  IntFp operator*(const IntFp &rhs) const {
    IntFp res(*this);
    res.value = ZKFpExec::zk_exec->mul_gate(this->value, rhs.value);
    return res;
  }

  IntFp operator*(const uint64_t &rhs) const {
    IntFp res(*this);
    res.value = ZKFpExec::zk_exec->mul_const_gate(this->value, rhs);
    return res;
  }

  IntFp negate() {
    IntFp res(*this);
    res.value = ZKFpExec::zk_exec->mul_const_gate(this->value, p - 1);
    return res;
  }
};

// static inline __uint128_t random_val_input() {
//   return ZKFpExec::zk_exec->random_val_input();
// }

static inline __uint128_t auth_compute_mul(__uint128_t &a, __uint128_t &b){
  return ZKFpExec::zk_exec->auth_compute_mul(a, b);
}

static inline void batch_feed(IntFp *obj, uint64_t *value, int len) {
  ZKFpExec::zk_exec->feed((__uint128_t *)obj, value, len);
}

static inline void batch_reveal(IntFp *obj, uint64_t *value, int len) {
  ZKFpExec::zk_exec->reveal((__uint128_t *)obj, value, len);
}

static inline bool batch_reveal_check(IntFp *obj, uint64_t *expected, int len) {
  ZKFpExec::zk_exec->reveal_check((__uint128_t *)obj, expected, len);
  return true;
}

static inline bool batch_reveal_check_zero(IntFp *obj, int len) {
  ZKFpExec::zk_exec->reveal_check_zero((__uint128_t *)obj, len);
  return true;
}

template <typename IO>
inline void fp_zkp_poly_deg2(IntFp *x, IntFp *y, uint64_t *coeff, int len) {
  FpPolyProof<IO>::fppolyproof->zkp_poly_deg2((__uint128_t *)x,
                                              (__uint128_t *)y, coeff, len);
}

template <typename IO>
inline void fp_zkp_inner_prdt(IntFp *x, IntFp *y, uint64_t constant, int len) {
  FpPolyProof<IO>::fppolyproof->zkp_inner_prdt((__uint128_t *)x,
                                               (__uint128_t *)y, constant, len);
}

template <typename IO>
inline void fp_zkp_inner_prdt(__uint128_t *au, __uint128_t *bu, uint64_t *da, uint64_t *db, __uint128_t *mamb_my, uint64_t constant, int len) {
  FpPolyProof<IO>::fppolyproof->zkp_inner_prdt(au, bu, da, db, mamb_my, constant, len);
}
#endif
