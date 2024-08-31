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

	virtual void feed(__uint128_t& label, const uint64_t& value) = 0;

	virtual void feed(__uint128_t *label, const uint64_t *value, int len) = 0;

	virtual void feed_JQv2(__uint128_t& label, const uint64_t& value, uint64_t& d) = 0;

	virtual void reveal(__uint128_t *label, uint64_t *value, int len) = 0;

	virtual void reveal_check(__uint128_t *label, const uint64_t *value, int len) = 0;

	virtual void reveal_check_zero(__uint128_t *label, int len) = 0;

	virtual __uint128_t add_gate(const __uint128_t& a, const __uint128_t& b) = 0;

	virtual __uint128_t mul_gate(const __uint128_t& a, const __uint128_t& b) = 0;

	virtual __uint128_t mul_gate_JQv2(const __uint128_t& a, const __uint128_t& b, const uint64_t& d1, const uint64_t& d2) = 0;

	virtual __uint128_t mul_const_gate(const __uint128_t& a, const uint64_t& b) = 0;

	virtual __uint128_t pub_label(const uint64_t&a) = 0;

	virtual __uint128_t get_delta() = 0;

	virtual void send_data(void *buf, const uint32_t size) = 0;

	virtual void recv_data(void *buf, const uint32_t size) = 0;

	virtual void flush_and_proofs() = 0;

	virtual __uint128_t get_one_role() = 0;

	virtual void setup_JQv2(int matrix_sz, int batch_sz) = 0;

	virtual void reverse_flag() = 0;

	virtual void finalize_inner_product_JQv2(int w_length, int batch_sz) = 0;
};

// ZKFpExec * ZKFpExec::zk_exec = nullptr;
#endif
