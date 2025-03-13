#ifndef FP_OS_TRIPLE_H__
#define FP_OS_TRIPLE_H__

#include "emp-zk/emp-vole/emp-vole.h"
#include "emp-zk/emp-zk-arith/triple_auth.h"

#define LOW64(x) _mm_extract_epi64((block)x, 0)
#define HIGH64(x) _mm_extract_epi64((block)x, 1)

template <typename IO> class FpOSTriple {
public:
  int party;
  int threads;
  int triple_n;
  __uint128_t delta;

  int check_cnt = 0, buffer_cnt = 0;
  __uint128_t *andgate_out_buffer = nullptr;
  __uint128_t *andgate_left_buffer = nullptr;
  __uint128_t *andgate_right_buffer = nullptr;
  uint64_t *A0_buffer = nullptr;
  uint64_t *A1_buffer = nullptr;

  IO *io;
  IO **ios;
  PRG prg;
  VoleTriple<IO> *vole = nullptr;
  FpAuthHelper<IO> *auth_helper = nullptr;
  ThreadPool *pool = nullptr;

  uint64_t CHECK_SZ = 1024 * 1024;


  FpOSTriple(int party, int threads, IO **ios) {
    this->party = party;
    this->threads = threads;
    io = ios[0];
    this->ios = ios;
    pool = new ThreadPool(threads);
    vole = new VoleTriple<IO>(3 - party, threads, ios);

    andgate_out_buffer = new __uint128_t[CHECK_SZ];
    andgate_left_buffer = new __uint128_t[CHECK_SZ];
    andgate_right_buffer = new __uint128_t[CHECK_SZ];
    A0_buffer = new uint64_t[CHECK_SZ];
    if (party == ALICE) {
      A1_buffer = new uint64_t[CHECK_SZ];
      vole->setup();
    } else {
      delta_gen();
      vole->setup(delta);
    }
    __uint128_t tmp;
    vole->extend(&tmp, 1);

    auth_helper = new FpAuthHelper<IO>(party, io);
  }

  ~FpOSTriple() {
    if (check_cnt != 0)
      andgate_correctness_check_manage();
    if (buffer_cnt !=0)
      andgate_correctness_check_manage_JQv2();
    auth_helper->flush();
    delete auth_helper;
    delete vole;
    delete[] andgate_out_buffer;
    delete[] andgate_left_buffer;
    delete[] andgate_right_buffer;
    delete[] A0_buffer;
    if (party == ALICE) delete[] A1_buffer;
  }
  /* ---------------------inputs----------------------*/

  /*
   * random bits for inputs and authentications for JQv1
   */

  __uint128_t random_val_input() {
    __uint128_t val;
    vole->extend(&val, 1);
    return val;
  }

  __uint128_t authenticated_val_input(uint64_t w, uint64_t &d) {
    __uint128_t mac;
    vole->extend(&mac, 1);

    d = PR - w;
    d = add_mod(HIGH64(mac), d);
    io->send_data(&d, sizeof(uint64_t));
    return mac;
  }

  __uint128_t authenticated_val_input(uint64_t &d, int flag) {
    __uint128_t key;
    vole->extend(&key, 1);

    io->recv_data(&d, sizeof(uint64_t));
    return key;
  }


  void auth_compute_mul_send_with_setup(const __uint128_t Ma,const __uint128_t Mb, uint64_t da, uint64_t db, __uint128_t &H1) {
  
    uint64_t M1 = add_mod(db, LOW64(Mb)), M2 = add_mod(da, LOW64(Ma));
    M1 = mult_mod(M1,M2);
    H1 = add_mod(M1,H1);
  }

  void auth_scal_recv_with_setup( const uint64_t A, uint64_t d, __uint128_t &H1) {
    uint64_t d_;
    d_ = mult_mod(d, A);
    uint64_t K1;
    K1 = mult_mod(d_, delta);
    H1 = add_mod(K1, H1);
  }

  void auth_add_recv_with_setup(uint64_t d, __uint128_t &H1) {
    uint64_t K1;
    K1 = mult_mod(d, delta);
    H1 = add_mod(K1, H1);
  }

  void auth_compute_mul_recv_with_setup(const __uint128_t Ka,const __uint128_t Kb, uint64_t da, uint64_t db, __uint128_t &H1) {

    uint64_t K1 = add_mod(db, Kb), K2 = add_mod(da, Ka);
    K1 = mult_mod(K1, K2);
    K2 = mult_mod(da, db);
    K2 = mult_mod(K2, delta);
    K1 = add_mod(K1, K2);
    H1 = add_mod(K1, H1);
  }

  void auth_constant(const uint64_t con, __uint128_t &H1) {
    uint64_t tmp;
    tmp = mult_mod(con, delta);
    H1 = add_mod(tmp, H1);
  }

  uint64_t auth_compute_mul_send_with_setup(__uint128_t &Ma, __uint128_t &Mb, __uint128_t &Mc,
                                               uint64_t Mabc, uint64_t wa, uint64_t wb) {
    uint64_t wc = mult_mod(wa,wb);
    uint64_t sa = PR - wa, sb = PR - wb, sc = PR - wc;
    sa = add_mod(HIGH64(Ma), sa);
    sb = add_mod(HIGH64(Mb), sb);
    sc = add_mod(HIGH64(Mc), sc);

    uint64_t M1 = mult_mod(sa, LOW64(Mb)), M2 = mult_mod(sb, LOW64(Ma));
    M1 = add_mod(M1,M2);
    M1 = add_mod(M1,Mabc);

    io->send_data(&sa, sizeof(uint64_t));
    io->send_data(&sb, sizeof(uint64_t));
    io->send_data(&sc, sizeof(uint64_t));
    Ma = (__uint128_t)makeBlock(wa, LOW64(Ma));
    Mb = (__uint128_t)makeBlock(wb, LOW64(Mb));
    Mc = (__uint128_t)makeBlock(wc, LOW64(Mc));

    return M1;
  }

  uint64_t auth_compute_mul_recv_with_setup(__uint128_t &Ka, __uint128_t &Kb, __uint128_t &Kc, __uint128_t Kab) {
    uint64_t da, db, dc;
    io->recv_data(&da, sizeof(uint64_t));
    io->recv_data(&db, sizeof(uint64_t));
    io->recv_data(&dc, sizeof(uint64_t));

    dc = mult_mod(dc, delta);
    Kc = add_mod(Kc, dc);

    uint64_t K1 = mult_mod(da, Kb), K2 = mult_mod(db, Ka);
    K1 = add_mod(K1, K2);
    K1 = add_mod(K1, Kc);
    K2 = mult_mod(da, db);
    K2 = mult_mod(K2, delta);
    K1 = add_mod(K1, K2);
    K1 = add_mod(K1, Kab);

    da = mult_mod(da, delta);
    db = mult_mod(db, delta);
    Ka = add_mod(Ka, da);
    Kb = add_mod(Kb, db);

    return K1;
  }

  /*
   * random bits for inputs and authentications for JQv2
   */

  void debug_A0_A1() {
    if (party == ALICE) {
      for (int i = 0; i < buffer_cnt; i++) {
        //io->send_data(&A0_buffer[i], sizeof(uint64_t));
        //io->send_data(&A1_buffer[i], sizeof(uint64_t));
        //io->flush();
        std::cout<<party<<"->"<<A0_buffer[i]<<" "<<A1_buffer[i]<<"\n";
      }
    } else {
      //uint64_t a0, a1;
      for (int i = 0; i < buffer_cnt; i++) {
        //io->recv_data(&a0, sizeof(uint64_t));
        //io->recv_data(&a1, sizeof(uint64_t));
        std::cout<<party<<"->"<<A0_buffer[i]<<"\n";
        //if (add_mod(A0_buffer[i], mult_mod(a1, delta)) != a0) {
        //}
      }
    }
  }

  void authenticated_val_input_with_setup(__uint128_t &mac, uint64_t w, uint64_t &lam) {
    lam = PR - w;
    lam = add_mod(HIGH64(mac), lam);
    io->send_data(&lam, sizeof(uint64_t));
    mac = (__uint128_t)makeBlock(w, LOW64(mac));
  }

  void authenticated_val_input_with_setup(__uint128_t &key, uint64_t &lam) {
    io->recv_data(&lam, sizeof(uint64_t));
    key = add_mod(key, mult_mod(lam, delta));
  }

  void authenticated_val_input_with_setup(__uint128_t *mac, uint64_t *w, uint64_t *lam, int len) {
    for (int i = 0; i < len; i++) {
      lam[i] = PR - w[i];
      lam[i] = add_mod(HIGH64(mac[i]), lam[i]);
      mac[i] = (__uint128_t)makeBlock(w[i], LOW64(mac[i]));
    }

    io->send_data(lam, len * sizeof(uint64_t));
  }

  void authenticated_val_input_with_setup(__uint128_t *key, uint64_t *lam, int len) {
    io->recv_data(lam, len * sizeof(uint64_t));
    
    for (int i = 0; i < len; i++) {
      key[i] = add_mod(key[i], mult_mod(lam[i], delta));
    }
  }

  void setup_pre_processing(__uint128_t *val1,int *left, int *right, bool *clr, uint64_t *val_pre_pro, int len) {
    for (int i = 0; i < len; i++) 
    if (clr[i]) {
      val_pre_pro[i] = mult_mod(LOW64(val1[left[i]]), LOW64(val1[right[i]]));
      val_pre_pro[i] = add_mod(val_pre_pro[i], LOW64(val1[i]));
    }
  }

  void setup_pre_processing(__uint128_t *val1,int *left, int *right, bool *clr, uint64_t *val_pre_pro, int len, int len_in) {
    for (int i = len_in; i < len + len_in; i++) 
    if (clr[i]) {
      val_pre_pro[i] = mult_mod(LOW64(val1[left[i]]), LOW64(val1[right[i]]));
      val_pre_pro[i] = add_mod(val_pre_pro[i], LOW64(val1[i]));
    }
  }

  void evaluate_MAC(__uint128_t val1, __uint128_t val2, uint64_t d1, uint64_t d2, uint64_t val_pre_pro, __uint128_t &val) {
    if (party == ALICE) {
      uint64_t w = mult_mod(HIGH64(val1), HIGH64(val2));
      uint64_t mac1 = add_mod(LOW64(val1), d1), mac2 = add_mod(LOW64(val2), d2);
      mac1 = mult_mod(mac1, mac2);
      mac1 = pr - mac1;
      mac1 = add_mod(mac1, val_pre_pro);
      val = (__uint128_t)makeBlock(w, mac1);
    } else {
      //!!! val1 and val2 is generated in the preprocessing phase instead of the function auth_compute_mul_with_setup()
      uint64_t key1 = add_mod(val1, d1), key2 = add_mod(val2, d2);
      key1 = mult_mod(key1, key2);
      key2 = mult_mod(d1,d2);
      key2 = mult_mod(key2, delta);
      key1 = add_mod(key1, key2);
      key1 = pr - key1;
      val = add_mod(key1, val_pre_pro);
    }
  }

  void auth_compute_mul_with_setup(__uint128_t Vala, __uint128_t Valb, __uint128_t &Valc, uint64_t &d) {
    if (buffer_cnt == CHECK_SZ) {
      andgate_correctness_check_manage_JQv2();
      buffer_cnt = 0;
    }

    if (party == ALICE) {
      uint64_t m_wa = pr - HIGH64(Vala), m_wb = pr - HIGH64(Valb);
      uint64_t wc = mult_mod(m_wa, m_wb);
      d = PR - wc;
      d = add_mod(HIGH64(Valc), d);
      io->send_data(&d, sizeof(uint64_t));
      Valc = (__uint128_t)makeBlock(wc, LOW64(Valc));

      A0_buffer[buffer_cnt] = mult_mod(LOW64(Vala), LOW64(Valb));
      m_wa = add_mod(m_wa, LOW64(Vala)); m_wb = add_mod(m_wb, LOW64(Valb));
      m_wa = mult_mod(m_wa, m_wb);
      m_wa = add_mod(m_wa, LOW64(Valc));
      m_wa = pr - m_wa;
      m_wa = add_mod(m_wa, wc);
      A1_buffer[buffer_cnt] = add_mod(m_wa, A0_buffer[buffer_cnt]);
    } else {
      io->recv_data(&d, sizeof(uint64_t));
      uint64_t dc = mult_mod(d, delta);
      Valc = add_mod(Valc, dc);

      dc = mult_mod(LOW64(Vala), LOW64(Valb));
      A0_buffer[buffer_cnt] = mult_mod(Valc, delta);
      A0_buffer[buffer_cnt] = add_mod(A0_buffer[buffer_cnt], dc);
    }

    buffer_cnt++;
  }

  /*
   * JQv2 check
   */

  void andgate_correctness_check_manage_JQv2() {
    io->flush();

    vector<future<void>> fut;

    uint64_t U = 0, V = 0, W = 0;
    if (buffer_cnt < 32) {
      block share_seed;
      share_seed_gen(&share_seed, 1);
      io->flush();

      uint64_t sum[2];
      andgate_correctness_check_JQv2(sum, 0, 0, buffer_cnt, &share_seed);
      if (party == ALICE) {
        U = sum[0];
        V = sum[1];
      } else
        W = sum[0];
    } else {
      block *share_seed = new block[threads];
      share_seed_gen(share_seed, threads);
      io->flush();

      uint32_t task_base = buffer_cnt / threads;
      uint32_t leftover = task_base + (buffer_cnt % task_base);
      uint32_t start = 0;

      uint64_t *sum = new uint64_t[2 * threads];

      for (int i = 0; i < threads - 1; ++i) {
        fut.push_back(
            pool->enqueue([this, sum, i, start, task_base, share_seed]() {
              andgate_correctness_check_JQv2(sum, i, start, task_base, share_seed);
            }));
        start += task_base;
      }
      andgate_correctness_check_JQv2(sum, threads - 1, start, leftover, share_seed);

      for (auto &f : fut)
        f.get();

      delete[] share_seed;
      if (party == ALICE) {
        for (int i = 0; i < threads; ++i) {
          U = add_mod(U, sum[2 * i]);
          V = add_mod(V, sum[2 * i + 1]);
        }
      } else {
        for (int i = 0; i < threads; ++i)
          W = add_mod(W, sum[i]);
      }
    }

    if (party == ALICE) {
      __uint128_t ope_data;
      vole->extend(&ope_data, 1);
      uint64_t A0_star = LOW64(ope_data);
      uint64_t A1_star = HIGH64(ope_data);
      uint64_t check_sum[2];
      check_sum[0] = add_mod(U, A0_star);
      check_sum[1] = add_mod(V, A1_star);
      io->send_data(check_sum, 2 * sizeof(uint64_t));
    } else {
      __uint128_t ope_data;
      vole->extend(&ope_data, 1);
      uint64_t B_star = LOW64(ope_data);
      W = add_mod(W, B_star);
      uint64_t check_sum[2];
      io->recv_data(check_sum, 2 * sizeof(uint64_t));
      check_sum[1] = mult_mod(check_sum[1], delta);
      check_sum[1] = add_mod(check_sum[1], W);
      if (check_sum[0] != check_sum[1])
        error("JQv2 check fails");
    }
    io->flush();
  }

  void andgate_correctness_check_JQv2(uint64_t *ret, int thr_idx, uint32_t start,
                                 uint32_t task_n, block *chi_seed) {
    if (task_n == 0)
      return;

    uint64_t *chi = new uint64_t[task_n];
    uint64_t seed = mod(LOW64(chi_seed[thr_idx]));
    uni_hash_coeff_gen(chi, seed, task_n);
    if (party == ALICE) {
      uint64_t U = 0, V = 0;
      for (uint32_t i = start, k = 0; i < start + task_n; ++i, ++k) {
        U = add_mod(U, mult_mod(A0_buffer[i], chi[k]));
        V = add_mod(V, mult_mod(A1_buffer[i], chi[k]));
      }
      ret[2 * thr_idx] = U;
      ret[2 * thr_idx + 1] = V;
    } else {
      uint64_t W = 0;
      for (uint32_t i = start, k = 0; i < start + task_n; ++i, ++k) {
        W = add_mod(W, mult_mod(A0_buffer[i], chi[k]));
      }
      ret[thr_idx] = W;
    }

    delete[] chi;
  }

  /*
   * authenticated bits for inputs of the prover
   */

  __uint128_t authenticated_val_input(uint64_t w) {
    __uint128_t mac;
    vole->extend(&mac, 1);

    uint64_t lam = PR - w;
    lam = add_mod(HIGH64(mac), lam);
    io->send_data(&lam, sizeof(uint64_t));
    return (__uint128_t)makeBlock(w, LOW64(mac));
  }

  void authenticated_val_input(__uint128_t *label, const uint64_t *w, int len) {
    uint64_t *lam = new uint64_t[len];
    vole->extend(label, len);

    for (int i = 0; i < len; ++i) {
      lam[i] = PR - w[i];
      lam[i] = add_mod(HIGH64(label[i]), lam[i]);
      label[i] = (__uint128_t)makeBlock(w[i], LOW64(label[i]));
    }
    io->send_data(lam, len * sizeof(uint64_t));
    delete[] lam;
  }

  __uint128_t authenticated_val_input() {
    __uint128_t key;
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

    io->recv_data(lam, len * sizeof(uint64_t));

    for (int i = 0; i < len; ++i) {
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
    if (check_cnt == CHECK_SZ) {
      andgate_correctness_check_manage();
      check_cnt = 0;
    }
    vole->extend(&mac, 1);
    andgate_left_buffer[check_cnt] = Ma;
    andgate_right_buffer[check_cnt] = Mb;

    uint64_t d = mult_mod(HIGH64(Ma), HIGH64(Mb));
    uint64_t s = PR - d;
    s = add_mod(HIGH64(mac), s);
    io->send_data(&s, sizeof(uint64_t));

    mac = (__uint128_t)makeBlock(d, LOW64(mac));
    andgate_out_buffer[check_cnt] = mac;
    check_cnt++;

    return mac;
  }

  __uint128_t auth_compute_mul_recv(__uint128_t Ka, __uint128_t Kb) {
    __uint128_t key;
    if (check_cnt == CHECK_SZ) {
      andgate_correctness_check_manage();
      check_cnt = 0;
    }
    vole->extend(&key, 1);
    andgate_left_buffer[check_cnt] = Ka;
    andgate_right_buffer[check_cnt] = Kb;

    uint64_t d;
    io->recv_data(&d, sizeof(uint64_t));
    d = mult_mod(d, delta);
    key = add_mod(key, d);

    andgate_out_buffer[check_cnt] = key;
    check_cnt++;
    return key;
  }

  /* ---------------------check----------------------*/

  void andgate_correctness_check_manage() {
    io->flush();

    vector<future<void>> fut;

    uint64_t U = 0, V = 0, W = 0;
    if (check_cnt < 32) {
      block share_seed;
      share_seed_gen(&share_seed, 1);
      io->flush();

      uint64_t sum[2];
      andgate_correctness_check(sum, 0, 0, check_cnt, &share_seed);
      if (party == ALICE) {
        U = sum[0];
        V = sum[1];
      } else
        W = sum[0];
    } else {
      block *share_seed = new block[threads];
      share_seed_gen(share_seed, threads);
      io->flush();

      uint32_t task_base = check_cnt / threads;
      uint32_t leftover = task_base + (check_cnt % task_base);
      uint32_t start = 0;

      uint64_t *sum = new uint64_t[2 * threads];

      for (int i = 0; i < threads - 1; ++i) {
        fut.push_back(
            pool->enqueue([this, sum, i, start, task_base, share_seed]() {
              andgate_correctness_check(sum, i, start, task_base, share_seed);
            }));
        start += task_base;
      }
      andgate_correctness_check(sum, threads - 1, start, leftover, share_seed);

      for (auto &f : fut)
        f.get();

      delete[] share_seed;
      if (party == ALICE) {
        for (int i = 0; i < threads; ++i) {
          U = add_mod(U, sum[2 * i]);
          V = add_mod(V, sum[2 * i + 1]);
        }
      } else {
        for (int i = 0; i < threads; ++i)
          W = add_mod(W, sum[i]);
      }
    }

    if (party == ALICE) {
      __uint128_t ope_data;
      vole->extend(&ope_data, 1);
      uint64_t A0_star = LOW64(ope_data);
      uint64_t A1_star = HIGH64(ope_data);
      uint64_t check_sum[2];
      check_sum[0] = add_mod(U, A0_star);
      check_sum[1] = add_mod(V, A1_star);
      io->send_data(check_sum, 2 * sizeof(uint64_t));
    } else {
      __uint128_t ope_data;
      vole->extend(&ope_data, 1);
      uint64_t B_star = LOW64(ope_data);
      W = add_mod(W, B_star);
      uint64_t check_sum[2];
      io->recv_data(check_sum, 2 * sizeof(uint64_t));
      check_sum[1] = mult_mod(check_sum[1], delta);
      check_sum[1] = add_mod(check_sum[1], W);
      if (check_sum[0] != check_sum[1])
        error("multiplication gates check fails");
    }
    io->flush();
  }

  void andgate_correctness_check(uint64_t *ret, int thr_idx, uint32_t start,
                                 uint32_t task_n, block *chi_seed) {
    if (task_n == 0)
      return;
    __uint128_t *left = andgate_left_buffer;
    __uint128_t *right = andgate_right_buffer;
    __uint128_t *gateout = andgate_out_buffer;

    uint64_t *chi = new uint64_t[task_n];
    uint64_t seed = mod(LOW64(chi_seed[thr_idx]));
    uni_hash_coeff_gen(chi, seed, task_n);
    if (party == ALICE) {
      uint64_t A0, A1;
      uint64_t U = 0, V = 0;
      uint64_t a, b, ma, mb, mc;
      for (uint32_t i = start, k = 0; i < start + task_n; ++i, ++k) {
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
      ret[2 * thr_idx] = U;
      ret[2 * thr_idx + 1] = V;
    } else {
      uint64_t B;
      uint64_t W = 0;
      uint64_t ka, kb, kc;
      for (uint32_t i = start, k = 0; i < start + task_n; ++i, ++k) {
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

  /*
   * verify the output
   * open and check if the value equals 1
   */
  void reveal_send(const __uint128_t *output, uint64_t *value, int len) {
    for (int i = 0; i < len; ++i) {
      value[i] = HIGH64(output[i]);
      uint64_t mac = LOW64(output[i]);
      auth_helper->store(mac); // TODO
    }
    io->send_data(value, len * sizeof(uint64_t));
  }

  void reveal_recv(const __uint128_t *output, uint64_t *value, int len) {
    io->recv_data(value, len * sizeof(uint64_t));
    for (int i = 0; i < len; ++i) {
      uint64_t mac = mult_mod(value[i], LOW64(delta));
      mac = add_mod(mac, LOW64(output[i]));
      auth_helper->store(mac); // TODO
    }
  }

  void reveal_check_send(const __uint128_t *output, const uint64_t *value,
                         int len) {
    uint64_t *val_real = new uint64_t[len];
    reveal_send(output, val_real, len);
    delete[] val_real;
  }

  void reveal_check_recv(const __uint128_t *output, const uint64_t *val_exp,
                         int len) {
    uint64_t *val_real = new uint64_t[len];
    reveal_recv(output, val_real, len);
    bool res = memcmp(val_exp, val_real, len * sizeof(uint64_t));
    delete[] val_real;
    if (res != 0)
      error("arithmetic reveal value not expected");
  }

  void reveal_check_zero(const __uint128_t *output, int len) {
    for (int i = 0; i < len; ++i) {
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
    if (party == ALICE) {
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

  void compute_mu_prv(__uint128_t &ret, __uint128_t z1, __uint128_t *triple,
                      __uint128_t epsilon, __uint128_t sigma) {
    __uint128_t tmp1 = auth_mac_subtract(triple[2], z1);
    __uint128_t tmp2 = auth_mac_mul_const(triple[0], sigma >> 64);
    __uint128_t tmp3 = auth_mac_mul_const(triple[1], epsilon >> 64);
    __uint128_t tmp4 = mod((epsilon >> 64) * (sigma >> 64), pr);
    tmp1 = auth_mac_add(tmp1, tmp2);
    tmp1 = auth_mac_add(tmp1, tmp3);
    ret = auth_mac_add_const(tmp1, tmp4);
  }
  void compute_mu_vrf(__uint128_t &ret, __uint128_t z1, __uint128_t *triple,
                      __uint128_t epsilon, __uint128_t sigma) {
    __uint128_t tmp1 = auth_key_subtract(triple[2], z1);
    __uint128_t tmp2 = auth_key_mul_const(triple[0], sigma);
    __uint128_t tmp3 = auth_key_mul_const(triple[1], epsilon);
    __uint128_t tmp4 = mod(epsilon * sigma, pr);
    tmp1 = auth_key_add(tmp1, tmp2);
    tmp1 = auth_key_add(tmp1, tmp3);
    ret = auth_key_add_const(tmp1, tmp4);
  }

  __uint128_t compute_mu_prv_opt(__uint128_t la, __uint128_t lb,
                                 __uint128_t eta_wr, __uint128_t *triple) {
    __uint128_t tmp1 = auth_mac_subtract(triple[2], eta_wr);
    __uint128_t tmp2 = auth_mac_mul_const(triple[0], HIGH64(lb));
    __uint128_t tmp3 = auth_mac_mul_const(triple[1], HIGH64(la));
    __uint128_t tmp4 = mult_mod(HIGH64(la), HIGH64(lb));
    tmp1 = auth_mac_add(tmp1, tmp2);
    tmp1 = auth_mac_add(tmp1, tmp3);
    return auth_mac_add_const(tmp1, tmp4);
  }

  __uint128_t compute_mu_vrf_opt(__uint128_t la, __uint128_t lb,
                                 __uint128_t eta_wr, __uint128_t *triple) {
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
    for (int i = 0; i < threads; ++i)
      res += ios[i]->counter;
    return res;
  }

  /* ---------------------debug functions----------------------*/

  void check_auth_mac(__uint128_t *auth, int len) {
    if (party == ALICE) {
      io->send_data(auth, len * sizeof(__uint128_t));
    } else {
      __uint128_t *auth_recv = new __uint128_t[len];
      io->recv_data(auth_recv, len * sizeof(__uint128_t));
      for (int i = 0; i < len; ++i) {
        __uint128_t mac = mod((auth_recv[i] >> 64) * delta, pr);
        mac = mod(mac + auth[i], pr);
        if ((auth_recv[i] & (__uint128_t)0xFFFFFFFFFFFFFFFFLL) != mac) {
          std::cout << "authenticated mac error at: " << i << std::endl;
          abort();
        }
      }
      delete[] auth_recv;
    }
  }

  void check_compute_mul(__uint128_t *a, __uint128_t *b, __uint128_t *c,
                         int len) {
    if (party == ALICE) {
      io->send_data(a, len * sizeof(__uint128_t));
      io->send_data(b, len * sizeof(__uint128_t));
      io->send_data(c, len * sizeof(__uint128_t));
    } else {
      __uint128_t *ar = new __uint128_t[len];
      __uint128_t *br = new __uint128_t[len];
      __uint128_t *cr = new __uint128_t[len];
      io->recv_data(ar, len * sizeof(__uint128_t));
      io->recv_data(br, len * sizeof(__uint128_t));
      io->recv_data(cr, len * sizeof(__uint128_t));
      for (int i = 0; i < len; ++i) {
        __uint128_t product = mod((ar[i] >> 64) * (br[i] >> 64), pr);
        if (product != (cr[i] >> 64))
          error("wrong product");
        __uint128_t mac = mod(product * delta, pr);
        mac = mod(mac + c[i], pr);
        if (mac != (cr[i] & (__uint128_t)0xFFFFFFFFFFFFFFFFLL))
          error("wrong mac");
      }
      delete[] ar;
      delete[] br;
      delete[] cr;
    }
  }
};
#endif