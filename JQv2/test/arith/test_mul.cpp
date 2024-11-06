#include "emp-zk/emp-zk-arith/ostriple.h"
#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
#include "blake3.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gmp.h>
using namespace emp;
using namespace std;

int port, party;
const int threads = 1;

void test_openssl_multiplication(int chunk, const char *bstr, int bitlen) {
    BIGNUM  *bound, *result;
    BN_CTX * ctx;
    ctx = BN_CTX_new();

    bound = BN_new();
    result = BN_new();

    BN_dec2bn(&bound, bstr);

    std::vector<BIGNUM*> a(chunk);
    // BIGNUM **a = new BIGNUM*[chunk];

    for (int i = 0; i < chunk; ++i) {
        a[i] = BN_new();
        BN_rand_range(a[i], bound);
    }
    
    auto start = clock_start();
    for (int i = 0; i < (chunk); ++i) { 
        BN_mod_mul(result, a[i], a[i], bound, ctx);
    }
    cout << "Openssl Mul Speed: \t\t" << time_from(start)<< " us \t" << endl;

    // size_t total_bytes = 0;
    // for (int i = 0; i < (chunk); ++i) { 
    //     total_bytes += BN_num_bytes(a[i]);
    // }

    // char *binary_data = new char[total_bytes];

    // size_t current = 0;
    // for (int i = 0; i < chunk; ++i) {
    //     int num_bytes = BN_num_bytes(a[i]);
    //     unsigned char *bin_data = new unsigned char[num_bytes];
    //     BN_bn2bin(a[i], bin_data);
    //     std::memcpy(binary_data + current, &bin_data, num_bytes);
    //     current += num_bytes;
    // }

    // start = clock_start();
    // Hash::hash_for_block(binary_data, total_bytes);
    // // cout <<  "Openssl's BN as input Hash Speed: \t" << (time_from(start)) << "us \t"<< " input length:" << length << " bytes" << endl;
    // cout <<  "Openssl as input SHA256 Speed: \t" << (time_from(start)) << " us \t" << endl;

    // blake3_hasher hasher;
    // blake3_hasher_init(&hasher);
    // uint8_t output[BLAKE3_OUT_LEN];

    // start = clock_start();
    // blake3_hasher_update(&hasher, binary_data, total_bytes);
    // blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    
    // cout << "Openssl as input blake3 Speed: \t" << (time_from(start)) << " us \t" << endl;

    for (int i = 0; i < chunk; ++i) {
        BN_free(a[i]);
    }
    BN_free(bound);
    BN_free(result);
}

void test_gmp_multiplication(int chunk, const char *bstr, int bitlen) {

    mpz_t *a = new mpz_t[chunk];

    gmp_randstate_t state;
    gmp_randinit_mt(state); 
    gmp_randseed_ui(state, time(NULL));

    mpz_t bound, res;
    mpz_init(bound);
    mpz_init(res);
    mpz_set_str(bound, bstr, 10);

    for (int i = 0; i < (chunk); ++i) { 
        mpz_init(a[i]);
        mpz_urandomm(a[i], state, bound);
    }

    auto start = clock_start();
    for (int i = 0; i < (chunk); ++i) { 
        mpz_mul(res, a[i], a[i]);
        mpz_mod(res, res, bound);
    }
    cout << "GMP Mul Speed: \t\t\t" << time_from(start)<< " us \t" << endl;

    // size_t total_bytes = 0;
    // for (int i = 0; i < (chunk); ++i) { 
    //     total_bytes += (mpz_sizeinbase(a[i], 2) / 8);
    // }

    // char *binary_data = new char[total_bytes];

    // size_t current = 0;
    // for (int i = 0; i < chunk; ++i) {
    //     int num_bytes = mpz_sizeinbase(a[i], 2) / 8;
    //     mpz_export(binary_data + current, nullptr, 1, sizeof(unsigned char), 0, 0, a[i]);
    //     current += num_bytes;
    // }

    // start = clock_start();
    // Hash::hash_for_block(binary_data, total_bytes);
    // // cout <<  "Openssl's BN as input Hash Speed: \t" << (time_from(start)) << "us \t"<< " input length:" << length << " bytes" << endl;
    // cout <<  "GMP as input SHA256 Speed: \t" << (time_from(start)) << " us \t" << endl;

    // blake3_hasher hasher;
    // blake3_hasher_init(&hasher);
    // uint8_t output[BLAKE3_OUT_LEN];

    // start = clock_start();
    // blake3_hasher_update(&hasher, binary_data, total_bytes);
    // blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    
    // cout << "GMP as input blake3 Speed: \t" << (time_from(start)) << " us \t" << endl;

    for (int i = 0; i < (chunk); ++i) { 
        mpz_clear(a[i]);
    }

    mpz_clear(bound);
    mpz_clear(res);
    gmp_randclear(state);

}

void test_61mul_hash(int chunk){
    __uint128_t* a = new __uint128_t[chunk];
    auto start = clock_start();
    __uint128_t pro = 1;
    for (int i = 0; i < chunk; ++i) {
      a[i] = rand() % PR;
    }
    start = clock_start();
    for (int i = 0; i < (chunk); ++i) { 
        pro = mult_mod(LOW64(a[i]), pro);
    }
    cout << "Mul Speed: \t\t\t" << time_from(start)<< " us \t" << endl;
    size_t total_bytes = chunk * sizeof(uint64_t);

    char *binary_data = new char[total_bytes];

    for (int i = 0; i < chunk; ++i) {
        std::memcpy(binary_data + i * sizeof(uint64_t), &a[i], sizeof(uint64_t));
    }

    delete[] a;  

    start = clock_start();
    Hash::hash_for_block(binary_data, total_bytes);
    // cout <<  "Openssl's BN as input Hash Speed: \t" << (time_from(start)) << "us \t"<< " input length:" << length << " bytes" << endl;
    cout <<  "SHA256 Speed: \t\t\t" << (time_from(start)) << " us \t" << endl;

    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    uint8_t output[BLAKE3_OUT_LEN];

    start = clock_start();
    blake3_hasher_update(&hasher, binary_data, total_bytes);
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    
    cout << "blake3 Speed: \t\t\t" << time_from(start) << " us \t" << endl;
    delete[] a;
}

int main(int argc, char **argv) {
  parse_party_and_port(argv, &party, &port);
  BoolIO<NetIO> *bios[threads];
  for (int i = 0; i < threads; ++i)
    bios[i] = new BoolIO<NetIO>(
        new NetIO(party == ALICE ? nullptr : "127.0.0.1", port + threads + 1 + i),
        party == ALICE);
  
  OSTriple<BoolIO<NetIO>> bos(party, threads, bios);
  int chunk = 1000000;

 
  // __uint128_t* a = new __uint128_t[chunk];

  // auto start = clock_start();
  // __uint128_t pro = 1;
  // for (int i = 0; i < chunk; ++i) {
  //   a[i] = rand() % PR;
  // }

  if (party == ALICE){
    const char *str = "2305843009213693951";

    cout << " ---------------- " << chunk << " " << 61 << "-bit field multiplications" << " ---------------- " << endl;

    // start = clock_start();
    // for (int i = 0; i < (chunk); ++i) { 
    //     pro = mult_mod(LOW64(a[i]), pro);
    // }
    // cout << "Mul Speed: \t\t\t" << time_from(start)<< " us \t" << endl;
    test_openssl_multiplication(chunk, str, 61);

    test_gmp_multiplication(chunk, str, 61);
    // size_t total_bytes = chunk * sizeof(uint64_t);

    // char *binary_data = new char[total_bytes];

    // for (int i = 0; i < chunk; ++i) {
    //     std::memcpy(binary_data + i * sizeof(uint64_t), &a[i], sizeof(uint64_t));
    // }

    // delete[] a;  

    // start = clock_start();
    // Hash::hash_for_block(binary_data, total_bytes);
    // // cout <<  "Openssl's BN as input Hash Speed: \t" << (time_from(start)) << "us \t"<< " input length:" << length << " bytes" << endl;
    // cout <<  "SHA256 Speed: \t\t\t" << (time_from(start)) << " us \t" << endl;

    // blake3_hasher hasher;
    // blake3_hasher_init(&hasher);
    // uint8_t output[BLAKE3_OUT_LEN];

    // start = clock_start();
    // blake3_hasher_update(&hasher, binary_data, total_bytes);
    // blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    
    // cout << "blake3 Speed: \t\t\t" << time_from(start) << " us \t" << endl;

    test_61mul_hash(chunk);
  }

  block *ab = new block[chunk];
  bos.random_bits_input(ab, chunk);

  if (party == ALICE){
    const char *str128 = "340282366920938463463374607431768211459";

    cout << " ---------------- " << chunk << " " << 128 << "-bit field multiplications" << " ---------------- " << endl;
    auto start = clock_start();
    block tmp;
    gfmul(ab[0], ab[1], &tmp);
    for (int i = 0; i < chunk; ++i) { 
        // cout << LOW64(ab[i]) << endl;
        gfmul(tmp, ab[i], &tmp);
    }
    cout << "__m128i Mul Speed: \t\t" << (time_from(start)) << "us \t" << endl;

     size_t total_bytes = chunk * sizeof(block);
    char *binary_data = new char[total_bytes];

    // 将 __m128i 数组转换为 char * 数组
    for (size_t i = 0; i < chunk; ++i) {
        std::memcpy(binary_data + i * sizeof(block), &ab[i], sizeof(block));
    }
    delete[] ab;  
    test_openssl_multiplication(chunk, str128, 128);

    test_gmp_multiplication(chunk, str128, 128);

    start = clock_start();
    Hash::hash_for_block(binary_data, total_bytes);
    // cout <<  "Openssl's BN as input Hash Speed: \t" << (time_from(start)) << "us \t"<< " input length:" << length << " bytes" << endl;
    cout <<  "SHA256 Speed: \t\t\t" << (time_from(start)) << "us \t" << endl;

    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    uint8_t output[BLAKE3_OUT_LEN];

    start = clock_start();
    blake3_hasher_update(&hasher, binary_data, total_bytes);
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    
    cout << "blake3 Speed: \t\t\t" << (time_from(start)) << "us \t" << endl;

    
  }


  return 0;
}
