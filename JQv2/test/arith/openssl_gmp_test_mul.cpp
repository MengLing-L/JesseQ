#include "emp-tool/emp-tool.h"
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <emp-zk/emp-zk.h>
#include <iostream>
#include "blake3.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
using namespace emp;
using namespace std;

std::string bignum_to_string(const BIGNUM *bn) {
    // char *bn_str = BN_bn2hex(bn);
    int num_bytes = BN_num_bytes(bn);

    // 使用 BN_bn2bin
    unsigned char *bin_data = new unsigned char[num_bytes];
    BN_bn2bin(bn, bin_data);
    std::string result(reinterpret_cast<const char*>(bin_data), num_bytes);
    // OPENSSL_free(bn_str);
    return result;
}

char *bignums_to_char_ptr(const std::vector<BIGNUM *> &bignums) {
    std::string result;

    for (const auto &bn : bignums) {
        result += bignum_to_string(bn);
    }

    char *char_ptr = new char[result.size() + 1];
    strcpy(char_ptr, result.c_str());

    return char_ptr;
}


void test_openssl_multiplication(int chunk, const char *bstr, int bitlen) {
    BIGNUM  *bound, *result;
    BN_CTX * ctx;
    ctx = BN_CTX_new();

    bound = BN_new();
    result = BN_new();

    BN_dec2bn(&bound, bstr);

    std::vector<BIGNUM*> a(chunk);
    // BIGNUM **a = new BIGNUM*[chunk];
    std::vector<BIGNUM*> b(chunk);

    for (int i = 0; i < chunk; ++i) {
        a[i] = BN_new();
        BN_rand_range(a[i], bound);
        b[i] = BN_new();
        BN_rand_range(b[i], bound);
    }
    
    auto start = clock_start();
    for (int i = 0; i < (chunk); ++i) { 
        BN_mod_mul(result, a[i], b[i], bound, ctx);
    }
    cout << "Openssl Mul Speed: \t" << time_from(start)<< "us \t" << endl;

    char *chars = bignums_to_char_ptr(a);
    size_t length = strlen(chars);
    start = clock_start();
    Hash::hash_for_block(chars, length);
    // cout <<  "Openssl's BN as input Hash Speed: \t" << (time_from(start)) << "us \t"<< " input length:" << length << " bytes" << endl;
    cout <<  "Openssl's BN as input SHA256 Speed: \t" << (time_from(start)) << "us \t" << endl;

    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    uint8_t output[BLAKE3_OUT_LEN];

    start = clock_start();
    blake3_hasher_update(&hasher, chars, length);
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    
    cout << "Openssl's BN as input blake3 Speed: \t" << (time_from(start)) << "us \t" << endl;

    for (int i = 0; i < chunk; ++i) {
        BN_free(a[i]);
        BN_free(b[i]);
    }
    BN_free(bound);
    BN_free(result);
}


void test_u64_multiplication(int chunk, int bitlen) {
    uint64_t* aa = new uint64_t[chunk];
    uint64_t* bb = new uint64_t[chunk];

    for (int i = 0; i < chunk; ++i) {
        aa[i] = rand() % PR;
        bb[i] = rand() % PR;
    }
    auto start = clock_start();
    for (int i = 0; i < chunk ; ++i) { 
        mult_mod(bb[i], aa[i]);
        // bb[i] * aa[i];
    }
    cout << "uint64_t Mul Speed: \t" << (time_from(start)) << "us \t" << endl;

    start = clock_start();
    Hash::hash_for_block(aa, 8 * chunk);
    
    // cout <<  "Openssl's BN as input Hash Speed: \t" << (time_from(start)) << "us \t"<< " input length:" << length << " bytes" << endl;
    cout <<  "uint64_t as input SHA256 Speed: \t" << (time_from(start)) << "us \t" << endl;

    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    uint8_t output[BLAKE3_OUT_LEN];

    start = clock_start();
    blake3_hasher_update(&hasher, aa, 8 * chunk);
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    
    cout << "uint64_t as input blake3 Speed: \t" << (time_from(start)) << "us \t" << endl;
}

void test_128_multiplication(int chunk, int bitlen) {

    block *ab = new block[chunk];
    PRG prg;
    prg.random_block(ab, chunk);

    auto start = clock_start();
    block tmp;
    gfmul(ab[0], ab[1], &tmp);
    for (int i = 0; i < chunk; ++i) { 
        // cout << LOW64(ab[i]) << endl;
        gfmul(tmp, ab[i], &tmp);
    }
    cout << "__m128i Mul Speed: \t" << (time_from(start)) << "us \t" << endl;
}

void test_gmp_multiplication(int chunk, const char *bstr, int bitlen) {

    mpz_t *a = new mpz_t[chunk];
    mpz_t *b = new mpz_t[chunk];

    gmp_randstate_t state;
    gmp_randinit_mt(state); 
    gmp_randseed_ui(state, time(NULL));

    mpz_t bound, res;
    mpz_init(bound);
    mpz_init(res);
    mpz_set_str(bound, bstr, 10);

    for (int i = 0; i < (chunk); ++i) { 
        mpz_init(a[i]);
        mpz_init(b[i]);
        mpz_urandomm(a[i], state, bound);
        mpz_urandomm(b[i], state, bound);
    }

    auto start = clock_start();
    for (int i = 0; i < (chunk); ++i) { 
        mpz_mul(res, a[i], b[i]);
        mpz_mod(res, res, bound);
    }
    cout << "GMP Mul Speed: \t\t" << time_from(start)<< "us \t" << endl;

    for (int i = 0; i < (chunk); ++i) { 
        mpz_clear(a[i]);
        mpz_clear(b[i]);
    }

    mpz_clear(bound);
    mpz_clear(res);
    gmp_randclear(state);

}


int main(int argc, char **argv) {
    
    int chunk = 100000;

    const char *str = "2305843009213693951";

    cout << " ---------------- " << chunk << " " << 61 << "-bit field multiplications" << " ---------------- " << endl;

    test_u64_multiplication(chunk, 61);

    test_openssl_multiplication(chunk, str, 61);

    test_gmp_multiplication(chunk, str, 61);

    const char *str128 = "340282366920938463463374607431768211459";

    cout << " ---------------- " << chunk << " " << 128 << "-bit field multiplications" << " ---------------- " << endl;

    test_128_multiplication(chunk, 128);

    test_openssl_multiplication(chunk, str128, 128);

    test_gmp_multiplication(chunk, str128, 128);

    return 0;
}