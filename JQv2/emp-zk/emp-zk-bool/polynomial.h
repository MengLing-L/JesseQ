#ifndef POLY_H__
#define POLY_H__

#include "emp-ot/emp-ot.h"
#include "emp-tool/emp-tool.h"

template <typename IO> class PolyProof {
public:
  int party;
  IO *io;
  block delta;
  int buffer_sz = 1000 * 1000;
  // int buffer_sz = 4096;
  block *buffer = nullptr;
  block *buffer1 = nullptr;
  block ch_tmp[2];
  int num;
  GaloisFieldPacking pack;
  FerretCOT<IO> *ferret = nullptr;

  PolyProof(int party, IO *io, FerretCOT<IO> *ferret) {
    this->party = party;
    this->io = io;
    this->ferret = ferret;
    this->delta = ferret->Delta;
    ch_tmp[0] = zero_block;
    if (party == ALICE) {
      buffer = new block[buffer_sz];
      buffer1 = new block[buffer_sz];
    } else {
      buffer = new block[buffer_sz];
    }
    num = 0;
  }

  ~PolyProof() {
    batch_check();
    if (buffer != nullptr)
      delete[] buffer;
    if (buffer1 != nullptr)
      delete[] buffer1;
  }

  void batch_check() {
    if (num == 0)
      return;
    CheatRecord::put("zk polynomial batch check");
    block seed;
    block *chi = new block[num > 4 ? num : 4];
    block ope_data[128];
    block check_sum[2];
    if (party == ALICE) {
      io->recv_data(&seed, sizeof(block));

      uni_hash_coeff_gen(chi, seed, num > 4 ? num : 4);

      vector_inn_prdt_sum_red(check_sum, chi, buffer, num);
      vector_inn_prdt_sum_red(check_sum + 1, chi, buffer1, num);
      ferret->rcot(ope_data, 128);
      block tmp;
      pack.packing(&tmp, ope_data);
      uint64_t choice_bits[2];
      for (int i = 0; i < 2; ++i) {
        choice_bits[i] = 0;
        for (int j = 63; j >= 0; --j) {
          choice_bits[i] <<= 1;
          if (getLSB(ope_data[i * 64 + j]))
            choice_bits[i] |= 0x1;
        }
      }
      check_sum[0] = check_sum[0] ^ tmp;
      tmp = makeBlock(choice_bits[1], choice_bits[0]);
      check_sum[1] = check_sum[1] ^ tmp;
      io->send_data(check_sum, 2 * sizeof(block));
      io->flush();
    } else {
      PRG prg;
      prg.random_block(&seed, 1);
      io->send_data(&seed, sizeof(block));
      io->flush();

      uni_hash_coeff_gen(chi, seed, num > 4 ? num : 4);
      block B;
      vector_inn_prdt_sum_red(&B, chi, buffer, num);
      ferret->rcot(ope_data, 128);
      block tmp;
      pack.packing(&tmp, ope_data);

      B = B ^ tmp;
      io->recv_data(check_sum, 2 * sizeof(block));

      gfmul(check_sum[1], delta, &tmp);
      check_sum[1] = B ^ tmp;
      if (cmpBlock(check_sum, check_sum + 1, 1) != 1)
        CheatRecord::put("zk polynomial: boolean polynomial zkp fails");
      else 
        CheatRecord::put("zk polynomial: boolean polynomial zkp success!");
    }
    num = 0;
    delete[] chi;
  }

  void batch_check(int flag) {
    if (num == 0)
      return;
    io->flush();
    // if (party == ALICE) {
    //   block hash_output = Hash::hash_for_block(buffer, num * 16);
    //   io->send_data(&hash_output, sizeof(block));
    // } else {
    //   block hash_output = Hash::hash_for_block(buffer, num * 16), output_recv;
    //   io->recv_data(&output_recv, sizeof(block));
    //   if (HIGH64(hash_output) == HIGH64(output_recv) && LOW64(hash_output) == LOW64(output_recv))
    //     std::cout<<"JQv1 success!\n";
    //   else std::cout<<"JQv1 fail!\n";
    // }
    io->flush();
    block seed = io->get_hash_block();
    block share_seed;
    PRG(&seed).random_block(&share_seed, 1);
    block *chi = new block[num];
    uni_hash_coeff_gen(chi, share_seed, num);
    block sum;
    if (party == ALICE) {
      vector_inn_prdt_sum_red(&sum, chi, buffer, num);
      io->send_data(&sum, sizeof(block));
    } else {
      block output_recv;
      vector_inn_prdt_sum_red(&sum, chi, buffer, num);
      io->recv_data(&output_recv, sizeof(block));
      if (HIGH64(sum) != HIGH64(output_recv) || LOW64(sum) != LOW64(output_recv))
        std::cout<<"JQv1 fail!\n";
    }
    num = 0;
  }

  inline void zkp_poly_deg2(block *polyx, block *polyy, bool *coeff, int len) {
    if (num >= buffer_sz)
      batch_check();

    block choice[2];
    choice[0] = zero_block;
    if (party == ALICE) {
      block A0 = zero_block, A1 = zero_block;
      block m0, m1, tmp0, tmp1;
      bool w0, w1;
      for (int i = 0; i < len; ++i) {
        w0 = getLSB(polyx[i]);
        m0 = polyx[i];
        w1 = getLSB(polyy[i]);
        m1 = polyy[i];

        gfmul(m0, m1, &tmp0);
        choice[1] = tmp0;
        tmp0 = choice[coeff[i + 1]];
        A0 = A0 ^ tmp0;

        choice[1] = m0;
        tmp0 = choice[w1];
        choice[1] = m1;
        tmp1 = choice[w0];
        tmp0 = tmp0 ^ tmp1;
        choice[1] = tmp0;
        tmp0 = choice[coeff[i + 1]];
        A1 = A1 ^ tmp0;
      }
      buffer[num] = A0;
      buffer1[num] = A1;
    } else {
      block B = zero_block;
      block tmp;
      for (int i = 0; i < len; ++i) {
        gfmul(polyx[i], polyy[i], &tmp);
        choice[1] = tmp;
        tmp = choice[coeff[i + 1]];
        B = B ^ tmp;
      }
      gfmul(delta, delta, &tmp);
      choice[1] = tmp;
      tmp = choice[coeff[0]];
      B = B ^ tmp;
      buffer[num] = B;
    }
    num++;
  }

  inline void zkp_inner_prdt(block *polyx, block *polyy, bool constant,
                             int len) {
    if (num >= buffer_sz)
      batch_check();
    block choice[2];
    choice[0] = zero_block;
    if (party == ALICE) {
      block A0 = zero_block, A1 = zero_block;
      block m0, m1, tmp0, tmp1;
      bool w0, w1;
      for (int i = 0; i < len; ++i) {
        w0 = getLSB(polyx[i]);
        m0 = polyx[i];
        w1 = getLSB(polyy[i]);
        m1 = polyy[i];

        gfmul(m0, m1, &tmp0);
        A0 = A0 ^ tmp0;

        choice[1] = m0;
        tmp0 = choice[w1];
        choice[1] = m1;
        tmp1 = choice[w0];
        tmp0 = tmp0 ^ tmp1;
        A1 = A1 ^ tmp0;
      }
      buffer[num] = A0;
      buffer1[num] = A1;
    } else {
      block B = zero_block;
      block tmp;
      for (int i = 0; i < len; ++i) {
        gfmul(polyx[i], polyy[i], &tmp);
        B = B ^ tmp;
      }
      gfmul(delta, delta, &tmp);
      choice[1] = tmp;
      tmp = choice[constant];
      B = B ^ tmp;
      buffer[num] = B;
    }
    num++;
  }

  inline void zkp_inner_prdt(block *polyx, block *polyy, bool *x, bool *y, block *ab, bool constant,
                             int len) {
    if (num >= buffer_sz)
      batch_check(1);

    block choice[2];
    choice[0] = zero_block;
    choice[1] = delta;
    if (party == ALICE) {
      block mzero = zero_block;
      for (int i = 0; i < len; ++i) {
        // block Ma_ = polyx[i],  Mb_ = polyy[i];
        // io->send_bit(d[1]);
        ch_tmp[1] = polyx[i];
        mzero = mzero ^ ch_tmp[y[i]];
        ch_tmp[1] = polyy[i];
        mzero = mzero ^ ch_tmp[x[i]];
        mzero = mzero ^ ab[i];
      }
      // cout << party<< "mzero ";
      buffer[num] = mzero;
    } else {
      block kzero = zero_block;
      for (int i = 0; i < len; ++i) {
        // block Ka_ = polyx[i],  Kb_ = polyy[i];
        ch_tmp[1] = polyx[i];
        kzero = kzero ^ ch_tmp[y[i]];
        ch_tmp[1] = polyy[i];
        kzero = kzero ^ ch_tmp[x[i]];
        kzero = kzero ^ choice[(x[i] & y[i])];
        kzero = kzero ^ ab[i];
      }
      kzero = kzero ^ choice[constant];
      buffer[num] = kzero;
    }
    num++;
  }

  inline void zkp_inner_prdt_eq(block *polyx, block *polyy, block *r, block *s,
                                int len, int len2) {
    if (num >= buffer_sz)
      batch_check();

    block choice[2];
    choice[0] = zero_block;
    if (party == ALICE) {
      block A0 = zero_block, A1 = zero_block;
      block m0, m1, tmp0, tmp1;
      bool w0, w1;
      for (int i = 0; i < len; ++i) {
        w0 = getLSB(polyx[i]);
        m0 = polyx[i];
        w1 = getLSB(polyy[i]);
        m1 = polyy[i];

        gfmul(m0, m1, &tmp0);
        A0 = A0 ^ tmp0;

        choice[1] = m0;
        tmp0 = choice[w1];
        choice[1] = m1;
        tmp1 = choice[w0];
        tmp0 = tmp0 ^ tmp1;
        A1 = A1 ^ tmp0;
      }
      for (int i = 0; i < len2; ++i) {
        w0 = getLSB(r[i]);
        m0 = r[i];
        w1 = getLSB(s[i]);
        m1 = s[i];

        gfmul(m0, m1, &tmp0);
        A0 = A0 ^ tmp0;

        choice[1] = m0;
        tmp0 = choice[w1];
        choice[1] = m1;
        tmp1 = choice[w0];
        tmp0 = tmp0 ^ tmp1;
        A1 = A1 ^ tmp0;
      }

      buffer[num] = A0;
      buffer1[num] = A1;
    } else {
      block B = zero_block;
      block tmp;
      for (int i = 0; i < len; ++i) {
        gfmul(polyx[i], polyy[i], &tmp);
        B = B ^ tmp;
      }
      for (int i = 0; i < len2; ++i) {
        gfmul(r[i], s[i], &tmp);
        B = B ^ tmp;
      }

      gfmul(delta, delta, &tmp);
      choice[1] = tmp;
      tmp = choice[0];
      B = B ^ tmp;
      buffer[num] = B;
    }
    num++;
  }

  inline void zkp_inner_prdt_eq(block *polyx, block *polyy, block *r, block *s,
                                block *rr, block *ss, int len, int len2) {
    if (num >= buffer_sz)
      batch_check();

    block choice[2];
    choice[0] = zero_block;
    if (party == ALICE) {
      block A0 = zero_block, A1 = zero_block;
      block m0, m1, tmp0, tmp1;
      bool w0, w1;
      for (int i = 0; i < len; ++i) {
        w0 = getLSB(polyx[i]);
        m0 = polyx[i];
        w1 = getLSB(polyy[i]);
        m1 = polyy[i];

        gfmul(m0, m1, &tmp0);
        A0 = A0 ^ tmp0;

        choice[1] = m0;
        tmp0 = choice[w1];
        choice[1] = m1;
        tmp1 = choice[w0];
        tmp0 = tmp0 ^ tmp1;
        A1 = A1 ^ tmp0;
      }
      for (int i = 0; i < len2; ++i) {
        w0 = getLSB(r[i]);
        m0 = r[i];
        w1 = getLSB(s[i]);
        m1 = s[i];

        gfmul(m0, m1, &tmp0);
        A0 = A0 ^ tmp0;

        choice[1] = m0;
        tmp0 = choice[w1];
        choice[1] = m1;
        tmp1 = choice[w0];
        tmp0 = tmp0 ^ tmp1;
        A1 = A1 ^ tmp0;
      }
      {
        w0 = getLSB(*rr);
        m0 = *rr;
        w1 = getLSB(*ss);
        m1 = *ss;

        gfmul(m0, m1, &tmp0);
        A0 = A0 ^ tmp0;

        choice[1] = m0;
        tmp0 = choice[w1];
        choice[1] = m1;
        tmp1 = choice[w0];
        tmp0 = tmp0 ^ tmp1;
        A1 = A1 ^ tmp0;
      }

      buffer[num] = A0;
      buffer1[num] = A1;
    } else {
      block B = zero_block;
      block tmp;
      for (int i = 0; i < len; ++i) {
        gfmul(polyx[i], polyy[i], &tmp);
        B = B ^ tmp;
      }
      for (int i = 0; i < len2; ++i) {
        gfmul(r[i], s[i], &tmp);
        B = B ^ tmp;
      }
      {
        gfmul(*rr, *ss, &tmp);
        B = B ^ tmp;
      }

      gfmul(delta, delta, &tmp);
      choice[1] = tmp;
      tmp = choice[0];
      B = B ^ tmp;
      buffer[num] = B;
    }
    num++;
  }

  inline void zkp_inner_prdt_multi(Integer *polyx, Integer *polyy, Bit *r,
                                   Bit *s, int len, int in_width) {
    for (int width = 0; width < in_width; ++width) {
      if (num >= buffer_sz)
        batch_check();

      block choice[2];
      choice[0] = zero_block;
      if (party == ALICE) {
        block A0 = zero_block, A1 = zero_block;
        block m0, m1, tmp0, tmp1;
        bool w0, w1;
        for (int i = 0; i < len; ++i) {
          w0 = getLSB(polyx[i][width].bit);
          m0 = polyx[i][width].bit;
          w1 = getLSB(polyy[0][i].bit);
          m1 = polyy[0][i].bit;

          gfmul(m0, m1, &tmp0);
          A0 = A0 ^ tmp0;

          choice[1] = m0;
          tmp0 = choice[w1];
          choice[1] = m1;
          tmp1 = choice[w0];
          tmp0 = tmp0 ^ tmp1;
          A1 = A1 ^ tmp0;
        }
        {
          w0 = getLSB(r[width].bit);
          m0 = r[width].bit;
          w1 = getLSB(s->bit);
          m1 = s->bit;

          gfmul(m0, m1, &tmp0);
          A0 = A0 ^ tmp0;

          choice[1] = m0;
          tmp0 = choice[w1];
          choice[1] = m1;
          tmp1 = choice[w0];
          tmp0 = tmp0 ^ tmp1;
          A1 = A1 ^ tmp0;
        }

        buffer[num] = A0;
        buffer1[num] = A1;
      } else {
        block B = zero_block;
        block tmp;
        for (int i = 0; i < len; ++i) {
          gfmul(polyx[i][width].bit, polyy[0][i].bit, &tmp);
          B = B ^ tmp;
        }
        {
          gfmul(r[width].bit, s->bit, &tmp);
          B = B ^ tmp;
        }
        gfmul(delta, delta, &tmp);
        choice[1] = tmp;
        tmp = choice[0];
        B = B ^ tmp;
        buffer[num] = B;
      }
      num++;
    }
  }
};
#endif
