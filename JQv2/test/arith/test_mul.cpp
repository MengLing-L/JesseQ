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
using namespace emp;
using namespace std;

int port, party;
const int threads = 1;

int main(int argc, char **argv) {
  int chunk = 1000000;

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
  cout << "\tMul Speed: \t" << time_from(start)<< "us \t" << endl;


  delete[] a;  
  return 0;
}
