#ifndef FP_OS_TRIPLE_H__
#define FP_OS_TRIPLE_H__

#include "emp-zk/emp-vole/emp-vole.h"
#include "emp-zk/emp-zk-arith/triple_auth.h"

#define LOW64(x) _mm_extract_epi64((block)x, 0)
#define HIGH64(x) _mm_extract_epi64((block)x, 1)


template<typename IO>
class FpOSTriple {
public:
	int party;
	int threads;
	int triple_n;
	__uint128_t delta;

	int input_cnt = 0, andgate_cnt = 0, check_cnt = 0, mul_sz, double_mul_sz;
	__uint128_t* auth_buffer_andgate = nullptr;
	__uint128_t* andgate_left_buffer = nullptr;
	__uint128_t* andgate_right_buffer = nullptr;
	__uint128_t *vole_val = nullptr, *vole_tmp = nullptr;
	uint64_t *val_pre_pro = nullptr, *val_tmp = nullptr;
	uint64_t *A0_buffer = nullptr, *A1_buffer = nullptr;
	bool change_flag = true;

	IO *io;
	IO **ios;
	PRG prg;
	VoleTriple<IO> *vole = nullptr;
	FpAuthHelper<IO> *auth_helper = nullptr;
	ThreadPool *pool = nullptr;

	uint64_t INPUT_BUFFER_SZ = 1000;
	uint64_t MEM_SZ = -1;
	uint64_t CHECK_DIV_SZ = 8;
	uint64_t BUFFER_SZ = -1;

	FpOSTriple (int party, int threads, IO **ios) {
		this->party = party;
		this->threads = threads;
		io = ios[0];
		this->ios = ios;
		pool = new ThreadPool(threads);
		vole = new VoleTriple<IO>(3-party, threads, ios);
		MEM_SZ = vole->param.n;
		BUFFER_SZ = vole->param.buf_sz()/CHECK_DIV_SZ*CHECK_DIV_SZ;
		auth_buffer_andgate = new __uint128_t[MEM_SZ];
		andgate_left_buffer = new __uint128_t[BUFFER_SZ/CHECK_DIV_SZ];
		andgate_right_buffer = new __uint128_t[BUFFER_SZ/CHECK_DIV_SZ];
		A0_buffer = new uint64_t[BUFFER_SZ/CHECK_DIV_SZ];
		if(party == ALICE) {
			vole->setup();
			A1_buffer = new uint64_t[BUFFER_SZ/CHECK_DIV_SZ];
		} else {
			delta_gen();
			vole->setup(delta);
		}
		vole->extend_inplace(auth_buffer_andgate, MEM_SZ);

		auth_helper = new FpAuthHelper<IO>(party, io);
	}

	~FpOSTriple () {
		if(andgate_buf_not_empty())
			andgate_correctness_check_manage();
		auth_helper->flush();
		delete auth_helper;
		delete vole;
		//delete[] auth_buffer_input;
		delete[] auth_buffer_andgate;
		delete[] andgate_left_buffer;
		delete[] andgate_right_buffer;
		if (vole_val != nullptr) 
			delete[] vole_val;
		if (val_pre_pro != nullptr)
			delete[] val_pre_pro;
		delete[] A0_buffer;
		if (party == ALICE)
			delete[] A1_buffer;
	}

	/* ---------------------Setup----------------------*/

	void matrix_mul_pre_processing_setup_for_JQv2(int matrix_sz, int batch_sz) {
		int test_n = matrix_sz * matrix_sz;
		mul_sz = matrix_sz * matrix_sz * matrix_sz;
		double_mul_sz = 2 * mul_sz;
		int test_n_batch_sz = test_n * batch_sz;
		int mul_sz_batch_sz = mul_sz * batch_sz;
		int w_length = test_n * 2 + mul_sz * 3 + 1;
		int w_length_batch_sz = w_length * batch_sz;

		vole_val = new __uint128_t[2 * test_n_batch_sz + 3 * mul_sz_batch_sz + w_length_batch_sz];
		val_pre_pro = new uint64_t[mul_sz_batch_sz];

		//batch multiplication
		for (int tm = 0; tm < batch_sz; tm++) {
			//extend test_n * 2 for committing mat_a and mat_b's inputs
			//extend 2 * mul_sz_batch_sz for inputs of multiplcation triples
			// le_in ri_in
			int loc = tm * (2 * test_n + 3 * mul_sz);
			vole->extend(vole_val + loc, 2 * test_n + 2 * mul_sz);
			loc += 2 * test_n;
			//val_pre_pro for output gates
			for (int i = tm * mul_sz; i < (tm + 1) * mul_sz; i++, loc++) {
				vole_val[loc + double_mul_sz] = party == ALICE ? auth_compute_mul_send(vole_val[loc], vole_val[loc + mul_sz]) : 
													auth_compute_mul_recv(vole_val[loc], vole_val[loc + mul_sz]);
				val_pre_pro[i] = add_mod(LOW64(vole_val[loc + double_mul_sz]), mult_mod(LOW64(vole_val[loc]), LOW64(vole_val[loc + mul_sz])));
			}
		}
		if (check_cnt) {
			andgate_correctness_check_manage();
			check_cnt = 0;
			if(andgate_cnt == BUFFER_SZ) {
				vole->extend_inplace(auth_buffer_andgate, MEM_SZ);
				andgate_cnt = 0;
			}
		}

		vole->extend(vole_val + 2 * test_n_batch_sz + 3 * mul_sz_batch_sz, w_length_batch_sz);
		vole_tmp = vole_val;
		val_tmp = val_pre_pro;
	}

	/* ---------------------inputs----------------------*/


	/*
	 * authenticated bits for inputs of the prover
	 */
	__uint128_t authenticated_val_input_with_setup(uint64_t w, uint64_t &d) {
		__uint128_t mac = *vole_tmp;
		d = add_mod(HIGH64(mac), PR - w);
		io->send_data(&d, sizeof(uint64_t));
		if (change_flag) {
			*vole_tmp = (__uint128_t)makeBlock(w, LOW64(mac));
			return *(vole_tmp++);
		} else {
			vole_tmp++;
			return (__uint128_t)makeBlock(w, LOW64(mac));
		}
	}

	__uint128_t authenticated_val_input_with_setup(uint64_t &d) {
		__uint128_t key = *vole_tmp;
		io->recv_data(&d, sizeof(uint64_t));
		if(change_flag) {
			*vole_tmp = add_mod(key, mult_mod(d, delta));
			return *(vole_tmp++);
		} else {
			vole_tmp++;
			return add_mod(key, mult_mod(d, delta));
		}
	}

	__uint128_t authenticated_val_input(uint64_t w) {
		__uint128_t mac;
		/*if(input_cnt == INPUT_BUFFER_SZ) {
			refill_send(auth_buffer_input, &input_cnt, INPUT_BUFFER_SZ);
		}*/
		//mac = auth_buffer_input[input_cnt++];
		vole->extend(&mac, 1);

		uint64_t lam = PR - w;
		lam = add_mod(HIGH64(mac), lam);
		io->send_data(&lam, sizeof(uint64_t));
		return (__uint128_t)makeBlock(w, LOW64(mac));
	}
	
	void authenticated_val_input(__uint128_t *label, const uint64_t *w, int len) {
		uint64_t *lam = new uint64_t[len];
		vole->extend(label, len);

		for(int i = 0; i < len; ++i) {
			lam[i] = PR - w[i];
			lam[i] = add_mod(HIGH64(label[i]), lam[i]);
			label[i] = (__uint128_t)makeBlock(w[i], LOW64(label[i]));
		}
		io->send_data(lam, len*sizeof(uint64_t));
		delete[] lam;
	}

	__uint128_t authenticated_val_input() {
		__uint128_t key;
		/*if(input_cnt == INPUT_BUFFER_SZ) {
			refill_recv(auth_buffer_input, &input_cnt, INPUT_BUFFER_SZ);
		}
		key = auth_buffer_input[input_cnt++];*/
		vole->extend(&key, 1);

		uint64_t lam;
		io->recv_data(&lam, sizeof(uint64_t));

		lam = mult_mod(lam, delta);
		key = add_mod(key, lam);

		return key;
	}

	void authenticated_val_input(__uint128_t *label, int len) {
		uint64_t *lam = new uint64_t[len];
		vole->extend(label, len);

		io->recv_data(lam, len*sizeof(uint64_t));

		for(int i = 0; i < len; ++i) {
			lam[i] = mult_mod(lam[i], delta);
			label[i] = add_mod(label[i], lam[i]);
		}

		delete[] lam;
	}

	/*
	 * authenticated bits for computing AND gates
	 */

	__uint128_t auth_compute_mul_send(__uint128_t Ma, __uint128_t Mb) {
		__uint128_t mac;
		if(check_cnt == BUFFER_SZ/CHECK_DIV_SZ) {
			andgate_correctness_check_manage();
			check_cnt = 0;
			if(andgate_cnt == BUFFER_SZ) {
				vole->extend_inplace(auth_buffer_andgate, MEM_SZ);
				andgate_cnt = 0;
			}
		}
		mac = auth_buffer_andgate[andgate_cnt];
		andgate_left_buffer[check_cnt] = Ma;
		andgate_right_buffer[check_cnt] = Mb;
		
		uint64_t d = mult_mod(HIGH64(Ma), HIGH64(Mb));
		uint64_t s = PR - d;
		s = add_mod(HIGH64(mac), s);
		io->send_data(&s, sizeof(uint64_t));

		mac = (__uint128_t)makeBlock(d, LOW64(mac));
		auth_buffer_andgate[andgate_cnt++] = mac;
		check_cnt++;

		return mac;
	}

	__uint128_t auth_compute_mul_recv(__uint128_t Ka, __uint128_t Kb) {
		__uint128_t key;
		if(check_cnt == BUFFER_SZ/CHECK_DIV_SZ) {
			andgate_correctness_check_manage();
			check_cnt = 0;
			if(andgate_cnt == BUFFER_SZ) {
				vole->extend_inplace(auth_buffer_andgate, MEM_SZ);
				andgate_cnt = 0;
			}
		}
		key = auth_buffer_andgate[andgate_cnt];
		andgate_left_buffer[check_cnt] = Ka;
		andgate_right_buffer[check_cnt] = Kb;

		uint64_t d;
		io->recv_data(&d, sizeof(uint64_t));
		d = mult_mod(d, delta);
		key = add_mod(key, d);

		auth_buffer_andgate[andgate_cnt++] = key;
		check_cnt++;
		return key;
	}

	__uint128_t evaluate_mul_mac(const __uint128_t& val1, const __uint128_t& val2, const uint64_t& d1, const uint64_t& d2) {
		//std::cout<<"fuck";
		uint64_t w = mult_mod(HIGH64(val1), HIGH64(val2));
		uint64_t mac1 = add_mod(LOW64(val1), d1), mac2 = add_mod(LOW64(val2), d2);
		mac1 = PR - mult_mod(mac1, mac2);
		mac1 = add_mod(mac1, *(val_tmp++));
		*vole_tmp = (__uint128_t)makeBlock(w, mac1);
		*(vole_tmp - double_mul_sz) = val1;
		*(vole_tmp - mul_sz) = val2;
		return *(vole_tmp++);
	}

	__uint128_t evaluate_mul_key(const __uint128_t& val1, const __uint128_t& val2, const uint64_t& d1, const uint64_t& d2) {
		//std::cout<<"fuck";
		uint64_t key1 = add_mod(*(vole_tmp - double_mul_sz), d1), key2 = add_mod(*(vole_tmp - mul_sz), d2);
		key1 = mult_mod(key1, key2);
		key2 = mult_mod(d1,d2);
		key2 = mult_mod(key2, delta);
		key1 = add_mod(key1, key2);
		*vole_tmp = add_mod(PR - key1, *(val_tmp++));
		*(vole_tmp - double_mul_sz) = val1;
		*(vole_tmp - mul_sz) = val2;
		return *(vole_tmp++);
	}

	void finalize_inner_product_JQv2(int w_length, int batch_sz) {
		int w_length_minus_one = w_length - 1;
		__uint128_t *p_va = vole_val, *p_vb = vole_val + batch_sz * w_length_minus_one;
		
		if (party == ALICE) {
			for (int tm = 0; tm < batch_sz; tm++) {
				if (check_cnt == BUFFER_SZ/CHECK_DIV_SZ) {
					inner_product_correctness_check_manage();
					check_cnt = 0;
				}

				uint64_t A0 = 0, A1 = 0;
				for (int i = 0; i < w_length_minus_one; i++, p_va++, p_vb++) {
					A0 = add_mod(A0, mult_mod(LOW64(*p_va), LOW64(*p_vb)));
					uint64_t Ma_a = add_mod(LOW64(*p_va), HIGH64(*p_va));
					uint64_t Mb_b = add_mod(LOW64(*p_vb), HIGH64(*p_vb));
					A1 = add_mod(A1, mult_mod(Ma_a, Mb_b));
				}
				A1 = add_mod(A1, add_mod(LOW64(*p_vb), HIGH64(*p_vb)));
				p_vb++;
				A1_buffer[check_cnt] = add_mod(A1, PR - A0);
				A0_buffer[check_cnt++] = A0;
			}
		} else {
			for (int tm = 0; tm < batch_sz; tm++) {
				if (check_cnt == BUFFER_SZ/CHECK_DIV_SZ) {
					inner_product_correctness_check_manage();
					check_cnt = 0;
				}

				uint64_t A0 = 0;
				for (int i = 0; i < w_length_minus_one; i++, p_va++, p_vb++)
					A0 = add_mod(A0, mult_mod(*p_va, *p_vb));
				A0_buffer[check_cnt++] = add_mod(A0, PR - mult_mod(*(p_vb++), delta));
			}
		}

		if (check_cnt) {
			inner_product_correctness_check_manage();
			check_cnt = 0;
		}
	}

	/* ---------------------check----------------------*/

	void andgate_correctness_check_manage() {
		io->flush();

		vector<future<void>> fut;

		uint64_t U = 0, V = 0, W = 0;
		if(check_cnt < 32) {
			block share_seed;
			share_seed_gen(&share_seed, 1);
			io->flush();

			uint64_t sum[2];
			andgate_correctness_check(sum, 0, 0, check_cnt, &share_seed);
			if(party == ALICE) {
				U = sum[0]; V = sum[1];
			} else W = sum[0];
		} else {
			block *share_seed = new block[threads];
			share_seed_gen(share_seed, threads);
			io->flush();

			uint32_t task_base = check_cnt / threads;
			uint32_t leftover = task_base + (check_cnt % task_base);
			uint32_t start = 0;

			uint64_t *sum = new uint64_t[2*threads];

			for(int i = 0; i < threads - 1; ++i) {
				fut.push_back(pool->enqueue([this, sum, i, start, task_base, share_seed](){
					andgate_correctness_check(sum, i, start, task_base, share_seed);
							}));
				start += task_base;
			}
			andgate_correctness_check(sum, threads - 1, start, leftover, share_seed);

			for(auto &f : fut) f.get();

			delete[] share_seed;
			if(party == ALICE) {
				for(int i = 0; i < threads; ++i) {
					U = add_mod(U, sum[2*i]);
					V = add_mod(V, sum[2*i+1]);
				}
			} else {
				for(int i = 0; i < threads; ++i)
					W = add_mod(W, sum[i]);
			}
		}

		if(party == ALICE) {
			__uint128_t ope_data;
			vole->extend(&ope_data, 1);
			uint64_t A0_star = LOW64(ope_data);
			uint64_t A1_star = HIGH64(ope_data);
			uint64_t check_sum[2];
			check_sum[0] = add_mod(U, A0_star);
			check_sum[1] = add_mod(V, A1_star);
			io->send_data(check_sum, 2*sizeof(uint64_t));
		} else {
			__uint128_t ope_data;
			vole->extend(&ope_data, 1);
			uint64_t B_star = LOW64(ope_data);
			W = add_mod(W, B_star);	
			uint64_t check_sum[2];
			io->recv_data(check_sum, 2*sizeof(uint64_t));
			check_sum[1] = mult_mod(check_sum[1], delta);
			check_sum[1] = add_mod(check_sum[1], W);
			if(check_sum[0] != check_sum[1])
				error("multiplication gates check fails");
		}
		io->flush();
	}

	void andgate_correctness_check(uint64_t *ret, int thr_idx, uint32_t start, uint32_t task_n, block *chi_seed) {
		if(task_n == 0) return;
		__uint128_t *left = andgate_left_buffer;
		__uint128_t *right = andgate_right_buffer;
		__uint128_t *gateout = auth_buffer_andgate + andgate_cnt - check_cnt;

		uint64_t *chi = new uint64_t[task_n];
		uint64_t seed = mod(LOW64(chi_seed[thr_idx]));
		uni_hash_coeff_gen(chi, seed, task_n);
		if(party == ALICE) {
			uint64_t A0, A1;
			uint64_t U = 0, V = 0;
			uint64_t a, b, ma, mb, mc;
			for(uint32_t i = start, k = 0; i < start + task_n; ++i, ++k) {
				a = HIGH64(left[i]);
				ma = LOW64(left[i]);
				b = HIGH64(right[i]);
				mb = LOW64(right[i]);
				mc = LOW64(gateout[i]);
				A0 = mult_mod(ma, mb);
				A1 = add_mod(mult_mod(a, mb), mult_mod(b, ma));
				uint64_t tmp = PR - mc;
				A1 = add_mod(A1, tmp);
				U = add_mod(U, mult_mod(A0, chi[k]));
				V = add_mod(V, mult_mod(A1, chi[k]));
			}
			ret[2*thr_idx] = U;
			ret[2*thr_idx+1] = V;
		} else {
			uint64_t B;
			uint64_t W = 0;
			uint64_t ka, kb, kc;
			for(uint32_t i = start, k = 0; i < start + task_n; ++i, ++k) {
				ka = LOW64(left[i]);
				kb = LOW64(right[i]);
				kc = LOW64(gateout[i]);
				B = add_mod(mult_mod(ka, kb), mult_mod(kc, delta));
				W = add_mod(W, mult_mod(B, chi[k]));
			}
			ret[thr_idx] = W;
		}

		delete[] chi;
	}

	void inner_product_correctness_check_manage() {
		io->flush();

		vector<future<void>> fut;

		uint64_t U = 0, V = 0, W = 0;
		if(check_cnt < 32) {
			block share_seed;
			share_seed_gen(&share_seed, 1);
			io->flush();

			uint64_t sum[2];
			inner_product_correctness_check(sum, 0, 0, check_cnt, &share_seed);
			if(party == ALICE) {
				U = sum[0]; V = sum[1];
			} else W = sum[0];
		} else {
			block *share_seed = new block[threads];
			share_seed_gen(share_seed, threads);
			io->flush();

			uint32_t task_base = check_cnt / threads;
			uint32_t leftover = task_base + (check_cnt % task_base);
			uint32_t start = 0;

			uint64_t *sum = new uint64_t[2*threads];

			for(int i = 0; i < threads - 1; ++i) {
				fut.push_back(pool->enqueue([this, sum, i, start, task_base, share_seed](){
					inner_product_correctness_check(sum, i, start, task_base, share_seed);
							}));
				start += task_base;
			}
			inner_product_correctness_check(sum, threads - 1, start, leftover, share_seed);

			for(auto &f : fut) f.get();

			delete[] share_seed;
			if(party == ALICE) {
				for(int i = 0; i < threads; ++i) {
					U = add_mod(U, sum[2*i]);
					V = add_mod(V, sum[2*i+1]);
				}
			} else {
				for(int i = 0; i < threads; ++i)
					W = add_mod(W, sum[i]);
			}
		}

		if(party == ALICE) {
			__uint128_t ope_data;
			vole->extend(&ope_data, 1);
			uint64_t A0_star = LOW64(ope_data);
			uint64_t A1_star = HIGH64(ope_data);
			uint64_t check_sum[2];
			check_sum[0] = add_mod(U, A0_star);
			check_sum[1] = add_mod(V, A1_star);
			io->send_data(check_sum, 2*sizeof(uint64_t));
		} else {
			__uint128_t ope_data;
			vole->extend(&ope_data, 1);
			uint64_t B_star = LOW64(ope_data);
			W = add_mod(W, B_star);	
			uint64_t check_sum[2];
			io->recv_data(check_sum, 2*sizeof(uint64_t));
			check_sum[1] = mult_mod(check_sum[1], delta);
			check_sum[1] = add_mod(check_sum[1], W);
			if(check_sum[0] != check_sum[1]) {
				std::cout<<"Wrong on inner products!";
				error("inner products check fails");
			}
		}
		io->flush();
	}

	void inner_product_correctness_check(uint64_t *ret, int thr_idx, uint32_t start, uint32_t task_n, block *chi_seed) {
		if(task_n == 0) return;
		uint64_t *chi = new uint64_t[task_n];
		uint64_t seed = mod(LOW64(chi_seed[thr_idx]));
		uni_hash_coeff_gen(chi, seed, task_n);
		if(party == ALICE) {
			uint64_t U = 0, V = 0;
			for(uint32_t i = start, k = 0; i < start + task_n; ++i, ++k) {
				U = add_mod(U, mult_mod(A0_buffer[i], chi[k]));
				V = add_mod(V, mult_mod(A1_buffer[i], chi[k]));
			}
			ret[2*thr_idx] = U;
			ret[2*thr_idx+1] = V;
		} else {
			uint64_t W = 0;
			for(uint32_t i = start, k = 0; i < start + task_n; ++i, ++k) {
				W = add_mod(W, mult_mod(A0_buffer[i], chi[k]));
			}
			ret[thr_idx] = W;
		}

		delete[] chi;
	}
	
	/*
	 * verify the output
	 * open and check if the value equals 1
	 */
	void reveal_send(const __uint128_t *output, uint64_t *value, int len) {
		for(int i = 0; i < len; ++i) {
			value[i] = HIGH64(output[i]);
			uint64_t mac = LOW64(output[i]);
			auth_helper->store(mac); // TODO
		}
		io->send_data(value, len*sizeof(uint64_t));
	}
	
	void reveal_recv(const __uint128_t *output, uint64_t *value, int len) {
		io->recv_data(value, len*sizeof(uint64_t));
		for(int i = 0; i < len; ++i) {
			uint64_t mac = mult_mod(value[i], LOW64(delta));
			mac = add_mod(mac, LOW64(output[i]));
			auth_helper->store(mac); // TODO
		}
	}

	void reveal_check_send(const __uint128_t *output, const uint64_t *value, int len) {
		uint64_t *val_real = new uint64_t[len];
		reveal_send(output, val_real, len);
		delete[] val_real;
	}

	void reveal_check_recv(const __uint128_t *output, const uint64_t *val_exp, int len) {
		uint64_t *val_real = new uint64_t[len];
		reveal_recv(output, val_real, len);
		bool res = memcmp(val_exp, val_real, len*sizeof(uint64_t));
		delete[] val_real;
		if(res != 0)
			error("arithmetic reveal value not expected");
	}

	void reveal_check_zero(const __uint128_t *output, int len) {
		for(int i = 0; i < len; ++i) {
			uint64_t mac = LOW64(output[i]);
			auth_helper->store(mac);
		}
	}


	/* ---------------------helper functions----------------------*/

	void delta_gen() {
		PRG prg;
		prg.random_data(&delta, sizeof(__uint128_t));
		extract_fp(delta);
	}

	void share_seed_gen(block *seed, uint32_t num) {
		block seed0;
		if(party == ALICE) {
			io->recv_data(&seed0, sizeof(block));
			PRG(&seed0).random_block(seed, num);
		} else {
			prg.random_block(&seed0, 1);
			io->send_data(&seed0, sizeof(block));
			PRG(&seed0).random_block(seed, num);
		}
	}	

	// sender
	void refill_send(__uint128_t *yz, int *cnt, int sz) {
		vole->extend(yz, sz);
		*cnt = 0;
	}

	// recver
	void refill_recv(__uint128_t *yz, int *cnt, int sz) {
		vole->extend(yz, sz);
		*cnt = 0;
	}

	void compute_mu_prv(__uint128_t& ret, __uint128_t z1, __uint128_t *triple, __uint128_t epsilon, __uint128_t sigma) {
		__uint128_t tmp1 = auth_mac_subtract(triple[2], z1);
		__uint128_t tmp2 = auth_mac_mul_const(triple[0], sigma>>64);
		__uint128_t tmp3 = auth_mac_mul_const(triple[1], epsilon>>64);
		__uint128_t tmp4 = mod((epsilon>>64)*(sigma>>64), pr);
		tmp1 = auth_mac_add(tmp1, tmp2);
		tmp1 = auth_mac_add(tmp1, tmp3);
		ret = auth_mac_add_const(tmp1, tmp4);
	}
	void compute_mu_vrf(__uint128_t& ret, __uint128_t z1, __uint128_t *triple, __uint128_t epsilon, __uint128_t sigma) {
		__uint128_t tmp1 = auth_key_subtract(triple[2], z1);
		__uint128_t tmp2 = auth_key_mul_const(triple[0], sigma);
		__uint128_t tmp3 = auth_key_mul_const(triple[1], epsilon);
		__uint128_t tmp4 = mod(epsilon*sigma, pr);
		tmp1 = auth_key_add(tmp1, tmp2);
		tmp1 = auth_key_add(tmp1, tmp3);
		ret = auth_key_add_const(tmp1, tmp4);
	}

	__uint128_t compute_mu_prv_opt(__uint128_t la, __uint128_t lb, __uint128_t eta_wr, __uint128_t* triple) {
		__uint128_t tmp1 = auth_mac_subtract(triple[2], eta_wr);
		__uint128_t tmp2 = auth_mac_mul_const(triple[0], HIGH64(lb));
		__uint128_t tmp3 = auth_mac_mul_const(triple[1], HIGH64(la));
		__uint128_t tmp4 = mult_mod(HIGH64(la), HIGH64(lb));
		tmp1 = auth_mac_add(tmp1, tmp2);
		tmp1 = auth_mac_add(tmp1, tmp3);
		return auth_mac_add_const(tmp1, tmp4);
	}

	__uint128_t compute_mu_vrf_opt(__uint128_t la, __uint128_t lb, __uint128_t eta_wr, __uint128_t* triple) {
		__uint128_t tmp1 = auth_key_subtract(triple[2], eta_wr);
		__uint128_t tmp2 = auth_key_mul_const(triple[0], lb);
		__uint128_t tmp3 = auth_key_mul_const(triple[1], la);
		__uint128_t tmp4 = mult_mod((uint64_t)la, (uint64_t)lb);
		tmp1 = auth_key_add(tmp1, tmp2);
		tmp1 = auth_key_add(tmp1, tmp3);
		return auth_key_add_const(tmp1, tmp4);
	}

	// prover: add 2 IT-MACs
	// return: [a] + [b]
	__uint128_t auth_mac_add(__uint128_t a, __uint128_t b) {
		block res = _mm_add_epi64((block)a, (block)b);
		return (__uint128_t)vec_mod(res);
	}

	// prover: add a IT-MAC with a constant
	// return: [a] + c
	__uint128_t auth_mac_add_const(__uint128_t a, __uint128_t c) {
		block cc = makeBlock(c, 0);
		cc = _mm_add_epi64((block)a, cc);
		return (__uint128_t)vec_mod(cc);
	}

	// prover: subtract 2 IT-MACs
	// return: [a] - [b]
	__uint128_t auth_mac_subtract(__uint128_t a, __uint128_t b) {
		block res = _mm_sub_epi64(PRs, (block)b);
		res = _mm_add_epi64((block)a, res);
		return (__uint128_t)vec_mod(res);
	}

	// prover: multiplies IT-MAC with a constatnt
	// return: c*[a]
	__uint128_t auth_mac_mul_const(__uint128_t a, uint64_t c) {
		return (__uint128_t)mult_mod((block)a, c);
	}

	// verifier: add 2 IT-MACs
	// return: [a] + [b]
	__uint128_t auth_key_add(__uint128_t a, __uint128_t b) {
		return add_mod(a, b);
	}

	// verifier: add a IT-MACs with a constant
	// return: [a] + [b]
	__uint128_t auth_key_add_const(__uint128_t a, __uint128_t c) {
		__uint128_t tmp = mult_mod(c, delta);
		tmp = pr - tmp;
		return add_mod(a, tmp);
	}

	// verifier: subtract 2 Keys
	// return: [a] - [b]
	__uint128_t auth_key_subtract(__uint128_t a, __uint128_t b) {
		__uint128_t key = pr - b;
		return add_mod(key, a);
	}

	// verifier: multiplies Key with a constant
	// return: c*[a]
	__uint128_t auth_key_mul_const(__uint128_t a, __uint128_t c) {
		return mult_mod(a, c);
	}

	uint64_t communication() {
		uint64_t res = 0;
		for(int i = 0; i < threads; ++i)
			res += ios[i]->counter;
		return res;
	}

	bool andgate_buf_not_empty() {
		if(andgate_cnt == 0) return false;
		else return true;
	}

	/* ---------------------debug functions----------------------*/

	void check_auth_mac(__uint128_t* auth, int len) {
		if(party == ALICE) {
			io->send_data(auth, len*sizeof(__uint128_t));
		} else {
			__uint128_t* auth_recv = new __uint128_t[len];
			io->recv_data(auth_recv, len*sizeof(__uint128_t));
			for(int i = 0; i < len; ++i) {
				__uint128_t mac = mod((auth_recv[i]>>64)*delta, pr);
				mac = mod(mac+auth[i], pr);
				if((auth_recv[i]&(__uint128_t)0xFFFFFFFFFFFFFFFFLL) != mac) {
					std::cout << "authenticated mac error at: " << i << std::endl;
					abort();
				} 
			}
			delete[] auth_recv;
		}
	}
	
	void check_compute_mul(__uint128_t* a, __uint128_t *b, __uint128_t *c, int len) {
		if(party == ALICE) {
			io->send_data(a, len*sizeof(__uint128_t));
			io->send_data(b, len*sizeof(__uint128_t));
			io->send_data(c, len*sizeof(__uint128_t));
		} else {
			__uint128_t* ar = new __uint128_t[len];
			__uint128_t* br = new __uint128_t[len];
			__uint128_t* cr = new __uint128_t[len];
			io->recv_data(ar, len*sizeof(__uint128_t));
			io->recv_data(br, len*sizeof(__uint128_t));
			io->recv_data(cr, len*sizeof(__uint128_t));
			for(int i = 0; i < len; ++i) {
				__uint128_t product = mod((ar[i]>>64)*(br[i]>>64), pr);
				if(product != (cr[i]>>64)) error("wrong product");
				__uint128_t mac = mod(product*delta, pr);
				mac = mod(mac+c[i], pr);
				if(mac != (cr[i]&(__uint128_t)0xFFFFFFFFFFFFFFFFLL)) error("wrong mac");
			}
			delete[] ar;
			delete[] br;
			delete[] cr;
		}
	}

};
#endif
