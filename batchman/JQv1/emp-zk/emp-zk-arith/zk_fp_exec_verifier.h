#ifndef ZK_FP_EXECUTIION_VERIFIER_H__
#define ZK_FP_EXECUTIION_VERIFIER_H__
#include "emp-zk/emp-zk-arith/zk_fp_exec.h"

template<typename IO>
class ZKFpExecVer : public ZKFpExec {
public:
	FpOSTriple<IO> *ostriple;
	IO* io = nullptr;
	__uint128_t delta;

	__uint128_t get_delta() {
		return delta;
	}

	void send_data(void *buf, const uint32_t size) {
		io->send_data(buf, size);
		io->flush();
	}

	void recv_data(void *buf, const uint32_t size) {
		io->recv_data(buf, size);
	}	

	void flush_and_proofs() {
		if(ostriple->andgate_buf_not_empty())
			ostriple->andgate_correctness_check_manage();
	}

	void flush_and_proofs_JQv1() {
		if(ostriple->check_cnt) {
			ostriple->andgate_correctness_check_manage_JQv1();
			ostriple->check_cnt = 0;
		}
	}

	__uint128_t get_one_role() {
		__uint128_t mac;
		ostriple->vole->extend(&mac, 1);
		return mac;		
	}

	void setup_JQv1(int matrix_sz, int batch_sz) {
		ostriple->matrix_mul_pre_processing_setup_for_JQv1(matrix_sz, batch_sz);
	}

	ZKFpExecVer(IO** ios, int threads) : ZKFpExec() {
		PRG prg(fix_key);
		prg.random_block((block*)&this->pub_mac, 1);
		this->pub_mac = mod(this->pub_mac & (__uint128_t)0xFFFFFFFFFFFFFFFFULL, pr);
		this->io = ios[0];
		this->ostriple = new FpOSTriple<IO>(BOB, threads, ios);
		this->delta = this->ostriple->delta;
	}

	~ZKFpExecVer() {
		delete ostriple;
	}

	/* 
	 * Verifier is the sender in iterative COT
	 * interface: get 1 authenticated bit
	 * authenticated message, KEY
	 */
	void feed(__uint128_t &label, const uint64_t& val) {
		label = this->ostriple->authenticated_val_input();
	}

	void feed(__uint128_t *label, const uint64_t *val, int len) {
		this->ostriple->authenticated_val_input(label, len);
	}

	void feed_JQv1(__uint128_t& label, const uint64_t& value) {
		label = this->ostriple->authenticated_val_input_with_setup();
	}

	void finalize_inner_product_JQv1(int w_length, int batch_sz) {
		this->ostriple->finalize_inner_product_JQv1(w_length, batch_sz);
	}

	/*
	 * check correctness of triples using cut and choose and bucketing
	 * check correctness of the output
	 */
	void reveal(__uint128_t *key, uint64_t *value, int len) {
		this->ostriple->reveal_recv(key, value, len);
	}

	void reveal_check(__uint128_t *key, const uint64_t *value, int len) {
		this->ostriple->reveal_check_recv(key, value, len);
	}

	void reveal_check_zero(__uint128_t *key, int len) {
		this->ostriple->reveal_check_zero(key, len);
	}

	__uint128_t add_gate(const __uint128_t& a, const __uint128_t& b) {
		return mod(a+b, pr);
	}

	__uint128_t mul_gate(const __uint128_t& a, const __uint128_t& b) {
		++this->gid;
		return ostriple->auth_compute_mul_recv(a, b);
	}

	__uint128_t mul_gate_JQv1(const __uint128_t& a, const __uint128_t& b) {
		return ostriple->auth_compute_mul_recv_with_setup();
	}

	__uint128_t mul_const_gate(const __uint128_t& a, const uint64_t& b) {
		return ostriple->auth_key_mul_const(a, b);
	}

	__uint128_t pub_label(const uint64_t& a) {
		uint64_t key = mult_mod(delta, a);
		key = PR - key;
		return add_mod(key, (uint64_t)this->pub_mac);
	}
};
#endif
