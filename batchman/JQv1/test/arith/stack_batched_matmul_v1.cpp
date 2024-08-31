#include "emp-zk/emp-zk.h"
#include <iostream>
#include "emp-tool/emp-tool.h"
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

inline uint64_t calculate_hash(PRP &prp, uint64_t x) {
	block bk = makeBlock(0, x);
	prp.permute_block(&bk, 1);
	return LOW64(bk) % PR;
}

void test_circuit_zk(BoolIO<NetIO> *ios[threads], int party, int matrix_sz, int branch_sz, int batch_sz) {
	long long test_n = matrix_sz * matrix_sz;
	long long mul_sz = matrix_sz * matrix_sz * matrix_sz;
    long long test_n_batch_sz = test_n * batch_sz;
    long long mul_sz_batch_sz = mul_sz * batch_sz;
    long long w_length = test_n * 2 + mul_sz * 3 + 1;
    long long w_length_batch_sz = w_length * batch_sz;
    long long w_length_branch_sz = w_length * branch_sz;
    long long check_length = mul_sz * 2 + test_n;

    uint64_t *res = new uint64_t[batch_sz];
    for (int i = 0; i < batch_sz; i++) res[i] = 0;

	setup_zk_arith<BoolIO<NetIO>>(ios, threads, party);
    ZKFpExec::zk_exec->setup_JQv1(matrix_sz, batch_sz);
    auto start = clock_start();

	IntFp_JQv1 *mat_a = new IntFp_JQv1[test_n_batch_sz];
	IntFp_JQv1 *mat_b = new IntFp_JQv1[test_n_batch_sz];
    IntFp_JQv1 *mul_le = new IntFp_JQv1[mul_sz_batch_sz];
	IntFp_JQv1 *mul_ri = new IntFp_JQv1[mul_sz_batch_sz];
	IntFp_JQv1 *mul_ou = new IntFp_JQv1[mul_sz_batch_sz];
    for (int tm = 0; tm < batch_sz; tm++) {
        // Input
        int tm_end = (tm + 1) * test_n;
        for (int i = tm * test_n; i < tm_end; i++)
            mat_a[i] = IntFp_JQv1(1, ALICE);
        for (int i = tm * test_n; i < tm_end; i++)
            mat_b[i] = IntFp_JQv1(1, ALICE);

        // Left, Right, Output of MUL gates
        tm_end = (tm + 1) * mul_sz;
        for (int i = tm * mul_sz; i < tm_end; i++)
            mul_le[i] = IntFp_JQv1(1, ALICE);
        for (int i = tm * mul_sz; i < tm_end; i++)
            mul_ri[i] = IntFp_JQv1(1, ALICE);
        for (int i = tm * mul_sz; i < tm_end; i++)
            mul_ou[i] = mul_le[i] * mul_ri[i];
    }

	ZKFpExec::zk_exec->flush_and_proofs_JQv1();

    // debug information
    std::cout << "ACCEPT Multiplication Proofs!" << std::endl;
	auto ttt0 = time_from(start);
    cout << ttt0 << " us\t" << party << " " << endl;
	std::cout << std::endl;    

    // Bob generates linear challenge r on the left-hand-side by a seed
    block left_r_seed;
    if (party == ALICE) {
		ZKFpExec::zk_exec->recv_data(&left_r_seed, sizeof(block));
    } else {
        PRG().random_block(&left_r_seed, 1);
        ZKFpExec::zk_exec->send_data(&left_r_seed, sizeof(block));
    }
    PRG prg_left_r(&left_r_seed);
    uint64_t *left_r = new uint64_t[check_length];
    for (int i = 0; i < check_length; i++) {
        block tmp;
        prg_left_r.random_block(&tmp, 1);
        left_r[i] = LOW64(tmp) % PR;
    }

    // Alice and Bob calulate the left_vec_a
    uint64_t *left_vec_a = new uint64_t[w_length_branch_sz];
    // for each branch
    for (int bid = 0; bid < branch_sz; bid++) {
        uint64_t head = w_length * bid;
        // columns of inputs mat_a
        for (int i = 0; i < matrix_sz; i++)
            for (int j = 0; j < matrix_sz; j++) {
                left_vec_a[head + i*matrix_sz + j] = 0;
                for (int k = 0; k < matrix_sz; k++) left_vec_a[head + i*matrix_sz + j] = add_mod(left_vec_a[head + i*matrix_sz + j], left_r[i*test_n + j*matrix_sz + k]);
            }
        head = head + test_n;
        // columns of inputs mat_b
        for (int i = 0; i < matrix_sz; i++)
            for (int j = 0; j < matrix_sz; j++) {
                left_vec_a[head + i*matrix_sz + j] = 0;
                for (int k = 0; k < matrix_sz; k++) left_vec_a[head + i*matrix_sz + j] = add_mod(left_vec_a[head + i*matrix_sz + j], left_r[mul_sz + k*test_n + i*matrix_sz + j]);
            }
        head = head + test_n;
        // columns of mul_le and mul_ri
        // le
        for (int i = 0; i < mul_sz; i++) left_vec_a[head + i] = PR - left_r[i];
        head = head + mul_sz;
        // ri
        for (int i = 0; i < mul_sz; i++) left_vec_a[head + i] = PR - left_r[mul_sz + i];
        head = head + mul_sz;
        // columns of mul_ou
        for (int i = 0; i < matrix_sz; i++)
            for (int j = 0; j < matrix_sz; j++)
                for (int k = 0; k < matrix_sz; k++) 
                    left_vec_a[head + i*test_n + j*matrix_sz + k] = PR - left_r[mul_sz*2 + i*matrix_sz + k];
        head = head + mul_sz;
        // last columns of constant 1 offset
        left_vec_a[head] = 0;
        for (int i = 0; i < test_n; i++) left_vec_a[head] = add_mod(left_vec_a[head], mult_mod((bid+1)*matrix_sz, left_r[mul_sz*2 + i]));
    }

    std::cout << "Calculated Left Vectors!" << std::endl;
	auto ttt1 = time_from(start)-ttt0;
    cout << ttt1 << " us\t" << party << " " << endl;
	std::cout << std::endl;

    // Alice chooses active left vector to prove inner_product
    IntFp_JQv1 *left_v = new IntFp_JQv1[w_length_batch_sz];
    for (int bid = 0; bid < batch_sz; bid++) 
        for (int i = 0; i < w_length; i++) 
            left_v[bid*w_length + i] = IntFp_JQv1(left_vec_a[i], ALICE);

    std::cout << "Committed left compressed a!" << std::endl;
	auto ttt2 = time_from(start)-ttt1-ttt0;
    cout << ttt2 << " us\t" << party << " " << endl;
	std::cout << std::endl;

    
    
    // Alice proves the inner products are 0
    IntFp_JQv1::finalize_inner_product(w_length, batch_sz);
	
    std::cout << "ACCEPT Inner-Product Proofs" << std::endl;
	auto ttt3 = time_from(start)-ttt2-ttt1-ttt0;
    cout << ttt3 << " us\t" << party << " " << endl;
	std::cout << std::endl;

    // Generates MACs
    // getting the seeds on the right-hand-side
    block right_s_seed; 
    if (party == ALICE) {
		ZKFpExec::zk_exec->recv_data(&right_s_seed, sizeof(block));
    } else {
        PRG().random_block(&right_s_seed, 1);
        ZKFpExec::zk_exec->send_data(&right_s_seed, sizeof(block));
    }
    PRG prg_right_s(&right_s_seed);
    uint64_t *right_s = new uint64_t[w_length];
    for (int i = 0; i < w_length; i++) {
        block tmp;
        prg_right_s.random_block(&tmp, 1);
        right_s[i] = LOW64(tmp) % PR;
    }    
    // Calculate MACs
    uint64_t *mac = new uint64_t[branch_sz];    
    for (int bid = 0; bid < branch_sz; bid++) {
        mac[bid] = 0;
        for (int i = 0; i < w_length; i++) mac[bid] = add_mod(mac[bid], mult_mod(right_s[i], left_vec_a[bid*w_length + i]));
    }

    // std::cout << "mac Generated" << std::endl;    
    // cout << time_from(start) << " us\t" << party << " " << endl;
	// std::cout << std::endl;

    // Generates [MAC]s
    IntFp *MAC = new IntFp[batch_sz];
    for (int bid = 0; bid < batch_sz; bid++) {
        MAC[bid] = IntFp(0, PUBLIC);
        for (int i = 0; i < w_length; i++) MAC[bid] = MAC[bid] + left_v[bid*w_length + i] * right_s[i];
    }

    // std::cout << "[mac] Generated" << std::endl;    
    // cout << time_from(start) << " us\t" << party << " " << endl;
	// std::cout << std::endl;

    // f([MAC])=0 Proofs
    IntFp *f_mac = new IntFp[batch_sz];
    for (int bid = 0; bid < batch_sz; bid++) {
        f_mac[bid] = IntFp(1, PUBLIC);
        for (int br = 0; br < branch_sz; br++) {
			IntFp term = MAC[bid] + IntFp(mac[br], PUBLIC).negate();
			f_mac[bid] = f_mac[bid] * term;
		}
    }
    batch_reveal_check(f_mac, res, batch_sz);
    std::cout << "ACCEPT Mac Proofs" << std::endl;
	auto ttt4 = time_from(start)-ttt3-ttt2-ttt1-ttt0;
    cout << ttt4 << " us\t" << party << " " << endl;
	std::cout << std::endl;

	finalize_zk_arith<BoolIO<NetIO>>();
	auto timeuse = time_from(start);	
    std::cout << "Total:" << std::endl;    
	cout << matrix_sz << "\t" << timeuse << " us\t" << party << " " << endl;
    cout << mul_sz_batch_sz * branch_sz << "\t" << double(mul_sz_batch_sz * branch_sz)/(timeuse)*1000000 / 1000000  << " M /sec\t" << party << " " << endl;
	std::cout << std::endl;


#if defined(__linux__)
	struct rusage rusage;
	if (!getrusage(RUSAGE_SELF, &rusage))
		std::cout << "[Linux]Peak resident set size: " << (size_t)rusage.ru_maxrss << std::endl;
	else std::cout << "[Linux]Query RSS failed" << std::endl;
#elif defined(__APPLE__)
	struct mach_task_basic_info info;
	mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
		std::cout << "[Mac]Peak resident set size: " << (size_t)info.resident_size_max << std::endl;
	else std::cout << "[Mac]Query RSS failed" << std::endl;
#endif
}

int main(int argc, char** argv) {

	parse_party_and_port(argv, &party, &port);
	BoolIO<NetIO>* ios[threads];
	for(int i = 0; i < threads; ++i)
		ios[i] = new BoolIO<NetIO>(new NetIO(party == ALICE?nullptr:argv[3],port+i), party==ALICE);

	std::cout << std::endl << "------------ circuit zero-knowledge proof test ------------" << std::endl << std::endl;;

	int num = 0;
	int branch = 0;
    int batch = 0;
	if(argc < 3) {
		std::cout << "usage: bin/arith/matrix_mul_arith PARTY PORT DIMENSION" << std::endl;
		return -1;
	} else if (argc == 3) {
		num = 10;
		branch = 10;
        batch = 10;
	} else {
		num = atoi(argv[4]);
		branch = atoi(argv[5]);
        batch = atoi(argv[6]);
	}
	
    
	

	test_circuit_zk(ios, party, num, branch, batch);

	for(int i = 0; i < threads; ++i) {
		delete ios[i]->io;
		delete ios[i];
	}
	return 0;
}
