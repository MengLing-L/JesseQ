#include "emp-tool/emp-tool.h"
#include <emp-zk/emp-zk.h>
#include <iostream>
#include <cstdio>
#include <random>
using namespace emp;
using namespace std;


int main(int argc, char **argv) { 
    

    int len = 30000000;
    int chunk = 30000000;
    __uint128_t *ab = new __uint128_t[chunk];
    __uint128_t *ao = new __uint128_t[chunk];
    // __uint128_t *_ab = new __uint128_t[len];

    std::random_device rd; 
    std::mt19937 eng(rd()); 
    std::uniform_int_distribution<__uint128_t> distr; 
    
    FILE* fp = fopen("example.bin", "wb+"); 
    if (!fp) {
        perror("fopen");
        return 1;
    }


    auto start = clock_start();
    fseek(fp, 0, SEEK_SET); 
    for (int i = 0; i < len / chunk; ++i) { 
        for (int j = 0; j < chunk; ++j) { 
            ab[j] = distr(eng);
            ao[j] = distr(eng);
            // _ab[j + i * chunk] = ab[j];
        }
        fwrite(ab, sizeof(__uint128_t), chunk, fp);
        fwrite(ao, sizeof(__uint128_t), chunk, fp);
    }

    cout << "Write time: " << (time_from(start)) << " us "  << endl;
    cout << "Write time: " << double(len)/(time_from(start))*1000000 << " M/sec "  << endl;

    __uint128_t *ab_ = new __uint128_t[chunk];
    __uint128_t *ao_ = new __uint128_t[chunk];

    start = clock_start();
    fseek(fp, 0, SEEK_SET); 
    for (int i = 0; i < len / chunk; ++i) {
        fread(ab_, sizeof(__uint128_t), chunk, fp);
        fread(ao_, sizeof(__uint128_t), chunk, fp);
        // for (int j = 0; j < chunk; ++j) { 
        //     if (_ab[j + i * chunk] != ab_[j]){
        //         cout << "ab[i]" << (uint64_t)(_ab[j + i * chunk] >> 64) << "\n";
        //         cout << "ab[i]" << (uint64_t)_ab[j + i * chunk] << "\n";
        //         cout << "ab_[i]" << (uint64_t)(ab_[j] >> 64) << "\n";
        //         cout << "ab_[i]" << (uint64_t)ab_[j] << "\n";
        //         cout << "\n" << "faild" << "\n";
        //     }
        // }
    }
    cout << "Read time: " << (time_from(start)) << " us "  << endl;
    cout << "Read time: " << double(len)/(time_from(start))*1000000 << " M/sec "  << endl;



    // Write to the file in chunks


    fclose(fp);
    return 0;

}