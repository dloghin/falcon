/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by wuyuncheng on 30/10/20.
//

#include "falcon/operator/phe/djcs_t_aux.h"

#include <cstdlib>
#include <stdio.h>

#include <falcon/utils/logger/logger.h>
#include <glog/logging.h>

#include "omp.h"

void djcs_t_aux_encrypt(djcs_t_public_key *pk, hcs_random *hr,
                        EncodedNumber &res, const EncodedNumber &plain) {
  if (plain.getter_type() != Plaintext) {
    log_error("The given value is not Plaintext, and should not be encrypted.");
    exit(EXIT_FAILURE);
  }

  mpz_t t1, t2, t3;
  mpz_init(t1);
  mpz_init(t2);
  mpz_init(t3);
  plain.getter_value(t1);
  plain.getter_n(t2);

  djcs_t_encrypt(pk, hr, t3, t1);

  res.setter_n(t2);
  res.setter_value(t3);
  res.setter_exponent(plain.getter_exponent());
  res.setter_type(Ciphertext);

  mpz_clear(t1);
  mpz_clear(t2);
  mpz_clear(t3);
}

void djcs_t_aux_partial_decrypt(djcs_t_public_key *pk, djcs_t_auth_server *au,
                                EncodedNumber &res,
                                const EncodedNumber &cipher) {
  if (cipher.getter_type() != Ciphertext) {
    log_error("The value is not ciphertext and cannot be decrypted.");
    exit(EXIT_FAILURE);
  }

  mpz_t t1, t2, t3;
  mpz_init(t1);
  mpz_init(t2);
  mpz_init(t3);
  cipher.getter_n(t1);
  cipher.getter_value(t2);

  res.setter_n(t1);
  res.setter_exponent(cipher.getter_exponent());
  res.setter_type(Plaintext);
  djcs_t_share_decrypt(pk, au, t3, t2);
  res.setter_value(t3);

  mpz_clear(t1);
  mpz_clear(t2);
  mpz_clear(t3);
}

void djcs_t_aux_share_combine(djcs_t_public_key *pk, EncodedNumber &res,
                              EncodedNumber *shares, int size) {
  auto *shares_value = (mpz_t *)malloc(size * sizeof(mpz_t));
  for (int i = 0; i < size; i++) {
    mpz_init(shares_value[i]);
    shares[i].getter_value(shares_value[i]);
  }

  mpz_t t1, t2;
  mpz_init(t1);
  mpz_init(t2);
  djcs_t_share_combine(pk, t1, shares_value);
  shares[0].getter_n(t2);
  res.setter_value(t1);
  res.setter_n(t2);
  res.setter_type(Plaintext);
  res.setter_exponent(shares[0].getter_exponent());

  for (int i = 0; i < size; i++) {
    mpz_clear(shares_value[i]);
  }
  free(shares_value);
  mpz_clear(t1);
  mpz_clear(t2);
}

void djcs_t_aux_ee_add(djcs_t_public_key *pk, EncodedNumber &res,
                       const EncodedNumber &cipher1,
                       const EncodedNumber &cipher2) {
  if (cipher1.getter_type() != Ciphertext) {
    log_error("The first input need be ciphertexts for homomorphic addition.");
    exit(EXIT_FAILURE);
  }
  if (cipher2.getter_type() != Ciphertext) {
    log_error("The second input need be ciphertexts for homomorphic addition.");
    exit(EXIT_FAILURE);
  }

  check_ee_add_exponent(cipher1, cipher2);
  check_encoded_public_key(cipher1, cipher2);

  mpz_t t1;
  mpz_init(t1);
  cipher1.getter_n(t1);

  mpz_t t2, t3, sum;
  mpz_init(t2);
  mpz_init(t3);
  mpz_init(sum);

  cipher1.getter_value(t2);
  cipher2.getter_value(t3);
  djcs_t_ee_add(pk, sum, t2, t3);
  res.setter_n(t1);
  res.setter_value(sum);
  res.setter_type(Ciphertext);
  res.setter_exponent(cipher1.getter_exponent());

  mpz_clear(t1);
  mpz_clear(t2);
  mpz_clear(t3);
  mpz_clear(sum);
}

void djcs_t_aux_ee_add_a_vector(djcs_t_public_key *pk, hcs_random *hr,
                                EncodedNumber &res, int size,
                                EncodedNumber *cipher_vector) {

  // get the precision of cipher
  int cipher_vector_precision = std::abs(cipher_vector[0].getter_exponent());
  log_info("[djcs_t_aux_ee_add_a_vector]: vector's precision =" +
           std::to_string(cipher_vector_precision));

  if (size == 0) {
    log_error("The input vector is empty");
    exit(EXIT_FAILURE);
  } else if (size == 1) {
    res = cipher_vector[0];
  } else if (size == 2) {
    djcs_t_aux_ee_add(pk, res, cipher_vector[0], cipher_vector[1]);
  } else {
    // init the tmp result
    EncodedNumber tmp_res;
    tmp_res.set_double(pk->n[0], 0, cipher_vector_precision);
    djcs_t_aux_encrypt(pk, hr, tmp_res, tmp_res);
    // add all values in the vector
    for (int i = 0; i < size; i++) {
      log_info("[djcs_t_aux_ee_add_a_vector]: add i-th element, i = " +
               std::to_string(i));
      djcs_t_aux_ee_add(pk, tmp_res, cipher_vector[i], tmp_res);
    }
    // assign to result
    res = tmp_res;
  }
}

void djcs_t_aux_ee_add_ext(djcs_t_public_key *pk, EncodedNumber &res,
                           EncodedNumber cipher1, EncodedNumber cipher2) {
  if (cipher1.getter_type() != Ciphertext ||
      cipher2.getter_type() != Ciphertext) {
    log_error("The two inputs need be ciphertexts for homomorphic addition.");
    exit(EXIT_FAILURE);
  }
  int prec1 = std::abs(cipher1.getter_exponent());
  int prec2 = std::abs(cipher2.getter_exponent());
  if (prec1 < prec2) {
    // increase cipher1's precision
    djcs_t_aux_increase_prec(pk, cipher1, prec2, cipher1);
  }
  if (prec1 > prec2) {
    // increase cipher2's precision
    djcs_t_aux_increase_prec(pk, cipher2, prec1, cipher2);
  }
  djcs_t_aux_ee_add(pk, res, cipher1, cipher2);
}

void djcs_t_aux_ep_mul(djcs_t_public_key *pk, EncodedNumber &res,
                       const EncodedNumber &cipher,
                       const EncodedNumber &plain) {
  if (cipher.getter_type() != Ciphertext || plain.getter_type() != Plaintext) {
    log_error("The input types do not match ciphertext or plaintext.");
    exit(EXIT_FAILURE);
  }

  check_encoded_public_key(cipher, plain);

  mpz_t t1;
  mpz_init(t1);
  cipher.getter_n(t1);

  mpz_t t2, t3, mult;
  mpz_init(t2);
  mpz_init(t3);
  mpz_init(mult);
  cipher.getter_value(t2);
  plain.getter_value(t3);
  djcs_t_ep_mul(pk, mult, t2, t3);

  res.setter_n(t1);
  res.setter_value(mult);
  res.setter_exponent(cipher.getter_exponent() + plain.getter_exponent());
  res.setter_type(Ciphertext);

  mpz_clear(t1);
  mpz_clear(t2);
  mpz_clear(t3);
  mpz_clear(mult);
}

void djcs_t_aux_increase_prec(djcs_t_public_key *pk, EncodedNumber &res,
                              int target_precision, EncodedNumber cipher) {
  if (cipher.getter_type() != Ciphertext) {
    log_error("The input type does not match ciphertext.");
    exit(EXIT_FAILURE);
  }
  int cur_precision = std::abs(cipher.getter_exponent());
  if (target_precision < cur_precision) {
    log_error("The target precision is less than current precision, cannot "
              "increase.");
    exit(EXIT_FAILURE);
  } else if (target_precision == cur_precision) {
    //    log_info("The precision is correct, no need to increase.");
    return;
  } else {
    // init an encoded number plaintext of 1.0 with diff precision and multiply
    int diff = target_precision - cur_precision;
    EncodedNumber assist_plain;
    assist_plain.set_double(pk->n[0], 1.0, diff);
    djcs_t_aux_ep_mul(pk, res, cipher, assist_plain);
  }
}

void djcs_t_aux_double_vec_encryption(djcs_t_public_key *pk, hcs_random *hr,
                                      EncodedNumber *res, int size,
                                      const std::vector<double> &vec,
                                      int phe_precision) {
  check_size(size);
  int vec_size = (int)vec.size();
  if (size != vec_size) {
    log_error("The result size is not equal to vec size.");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < size; i++) {
    res[i].set_double(pk->n[0], vec[i], phe_precision);
    djcs_t_aux_encrypt(pk, hr, res[i], res[i]);
  }
}

void djcs_t_aux_int_vec_encryption(djcs_t_public_key *pk, hcs_random *hr,
                                   EncodedNumber *res, int size,
                                   const std::vector<int> &vec,
                                   int phe_precision) {
  check_size(size);
  int vec_size = (int)vec.size();
  if (size != vec_size) {
    log_error("The result size is not equal to vec size.");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < size; i++) {
    res[i].set_integer(pk->n[0], vec[i]);
    djcs_t_aux_encrypt(pk, hr, res[i], res[i]);
  }
}

void djcs_t_aux_vec_aggregate(djcs_t_public_key *pk, EncodedNumber &res,
                              EncodedNumber *ciphers, int size) {
  check_size(size);
  res = ciphers[0];
  for (int i = 1; i < size; i++) {
    djcs_t_aux_ee_add(pk, res, res, ciphers[i]);
  }
}

void djcs_t_aux_vec_ele_wise_ee_add(djcs_t_public_key *pk, EncodedNumber *res,
                                    EncodedNumber *ciphers1,
                                    EncodedNumber *ciphers2, int size) {
  check_size(size);
  check_ee_add_exponent(ciphers1[0], ciphers2[0]);
  check_encoded_public_key(ciphers1[0], ciphers2[0]);
  auto *tmp_res = new EncodedNumber[size];
  // element-wise phe addition
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < size; i++) {
    //    tmp_res[i] = ciphers1[i];
    djcs_t_aux_ee_add(pk, tmp_res[i], ciphers1[i], ciphers2[i]);
  }
  for (int i = 0; i < size; i++) {
    res[i] = tmp_res[i];
  }
  delete[] tmp_res;
}

void djcs_t_aux_vec_ele_wise_ee_add_ext(djcs_t_public_key *pk,
                                        EncodedNumber *res,
                                        EncodedNumber *ciphers1,
                                        EncodedNumber *ciphers2, int size) {
  check_size(size);
  check_encoded_public_key(ciphers1[0], ciphers2[0]);
  int prec1 = std::abs(ciphers1[0].getter_exponent());
  int prec2 = std::abs(ciphers2[0].getter_exponent());
  if (prec1 < prec2) {
    // increase ciphers1's precision
    djcs_t_aux_increase_prec_vec(pk, ciphers1, prec2, ciphers1, size);
  }
  if (prec1 > prec2) {
    // increase ciphers2's precision
    djcs_t_aux_increase_prec_vec(pk, ciphers2, prec1, ciphers2, size);
  }

  djcs_t_aux_vec_ele_wise_ee_add(pk, res, ciphers1, ciphers2, size);
}

void djcs_t_aux_inner_product(djcs_t_public_key *pk, hcs_random *hr,
                              EncodedNumber &res, EncodedNumber *ciphers,
                              EncodedNumber *plains, int size) {
  check_size(size);
  check_encoded_public_key(ciphers[0], plains[0]);

  mpz_t t1;
  mpz_init(t1);
  ciphers[0].getter_n(t1);

  // assume the elements in the plains have the same exponent, so does ciphers
  res.setter_n(t1);
  res.setter_exponent(ciphers[0].getter_exponent() +
                      plains[0].getter_exponent());
  res.setter_type(Ciphertext);

  auto *mpz_ciphers = (mpz_t *)malloc(size * sizeof(mpz_t));
  auto *mpz_plains = (mpz_t *)malloc(size * sizeof(mpz_t));
  for (int i = 0; i < size; i++) {
    mpz_init(mpz_ciphers[i]);
    mpz_init(mpz_plains[i]);
    ciphers[i].getter_value(mpz_ciphers[i]);
    plains[i].getter_value(mpz_plains[i]);
  }

  // homomorphic dot product
  mpz_t sum, t2; // why need another sum1 that can only be correct? weired
  mpz_init(sum);
  mpz_init(t2);
  mpz_set_si(t2, 0);
  djcs_t_encrypt(pk, hr, sum, t2);

  for (int j = 0; j < size; j++) {
    // store the encrypted_exact_answer
    mpz_t tmp;
    mpz_init(tmp);
    // Multiply a plaintext value mpz_plains[j] with an encrypted value
    // mpz_ciphers[j], storing the result in tmp.
    djcs_t_ep_mul(pk, tmp, mpz_ciphers[j], mpz_plains[j]);
    // Add an encrypted value tmp to an encrypted value sum, storing the result
    // in sum.
    djcs_t_ee_add(pk, sum, sum, tmp);

    mpz_clear(tmp);
  }
  res.setter_value(sum);

  // clear everything
  mpz_clear(t1);
  mpz_clear(t2);
  mpz_clear(sum);
  for (int i = 0; i < size; i++) {
    mpz_clear(mpz_ciphers[i]);
    mpz_clear(mpz_plains[i]);
  }
  free(mpz_ciphers);
  free(mpz_plains);
}

void djcs_t_aux_vec_ele_wise_ep_mul(djcs_t_public_key *pk, EncodedNumber *res,
                                    EncodedNumber *ciphers,
                                    EncodedNumber *plains, int size) {
  check_size(size);
  check_encoded_public_key(ciphers[0], plains[0]);
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < size; i++) {
    djcs_t_aux_ep_mul(pk, res[i], ciphers[i], plains[i]);
  }
}

void djcs_t_aux_increase_prec_vec(djcs_t_public_key *pk, EncodedNumber *res,
                                  int target_precision, EncodedNumber *ciphers,
                                  int size) {
  check_size(size);
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < size; i++) {
    djcs_t_aux_increase_prec(pk, res[i], target_precision, ciphers[i]);
  }
}

void djcs_t_aux_double_mat_encryption(
    djcs_t_public_key *pk, hcs_random *hr, EncodedNumber **res, int row_size,
    int column_size, const std::vector<std::vector<double>> &mat,
    int phe_precision) {
  check_size(row_size);
  check_size(column_size);
  int vec_row_size = (int)mat.size();
  int vec_column_size = (int)mat[0].size();
  if ((row_size != vec_row_size) || (column_size != vec_column_size)) {
    log_error("The result size is not equal to mat size");
    exit(EXIT_FAILURE);
  }
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < row_size; i++) {
    for (int j = 0; j < column_size; j++) {
      res[i][j].set_double(pk->n[0], mat[i][j], phe_precision);
      djcs_t_aux_encrypt(pk, hr, res[i][j], res[i][j]);
    }
  }
}

void djcs_t_aux_matrix_ele_wise_ee_add(djcs_t_public_key *pk,
                                       EncodedNumber **res,
                                       EncodedNumber **cipher_mat1,
                                       EncodedNumber **cipher_mat2,
                                       int row_size, int column_size) {
  check_size(row_size);
  check_size(column_size);
  check_ee_add_exponent(cipher_mat1[0][0], cipher_mat2[0][0]);
  check_encoded_public_key(cipher_mat1[0][0], cipher_mat2[0][0]);
  for (int i = 0; i < row_size; i++) {
    djcs_t_aux_vec_ele_wise_ee_add(pk, res[i], cipher_mat1[i], cipher_mat2[i],
                                   column_size);
  }
}

void djcs_t_aux_matrix_ele_wise_ee_add_ext(djcs_t_public_key *pk,
                                           EncodedNumber **res,
                                           EncodedNumber **cipher_mat1,
                                           EncodedNumber **cipher_mat2,
                                           int row_size, int column_size) {
  check_size(row_size);
  check_size(column_size);
  check_encoded_public_key(cipher_mat1[0][0], cipher_mat2[0][0]);

  int prec1 = std::abs(cipher_mat1[0][0].getter_exponent());
  int prec2 = std::abs(cipher_mat2[0][0].getter_exponent());
  if (prec1 < prec2) {
    // increase cipher_mat1's precision
    djcs_t_aux_increase_prec_mat(pk, cipher_mat1, prec2, cipher_mat1, row_size,
                                 column_size);
  }
  if (prec1 > prec2) {
    // increase cipher_mat2's precision
    djcs_t_aux_increase_prec_mat(pk, cipher_mat2, prec1, cipher_mat2, row_size,
                                 column_size);
  }

  djcs_t_aux_matrix_ele_wise_ee_add(pk, res, cipher_mat1, cipher_mat2, row_size,
                                    column_size);
}

void djcs_t_aux_vec_mat_ep_mult(djcs_t_public_key *pk, hcs_random *hr,
                                EncodedNumber *res, EncodedNumber *ciphers,
                                EncodedNumber **plains, int row_size,
                                int column_size) {
  check_size(row_size);
  check_size(column_size);
  check_encoded_public_key(ciphers[0], plains[0][0]);
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < row_size; i++) {
    djcs_t_aux_inner_product(pk, hr, res[i], ciphers, plains[i], column_size);
  }
}

void djcs_t_aux_mat_mat_ep_mult(djcs_t_public_key *pk, hcs_random *hr,
                                EncodedNumber **res, EncodedNumber **cipher_mat,
                                EncodedNumber **plain_mat, int cipher_row_size,
                                int cipher_column_size, int plain_row_size,
                                int plain_column_size) {
  check_size(cipher_row_size);
  check_size(cipher_column_size);
  check_size(plain_row_size);
  check_size(plain_column_size);
  if (plain_column_size != cipher_row_size) {
    log_error("The plaintext column size is not equal to ciphertext row size");
    exit(EXIT_FAILURE);
  }
  // matrix multiplication
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < plain_row_size; i++) {
    //    log_info("[djcs_t_aux_mat_mat_ep_mult] i = " + std::to_string(i));
    for (int j = 0; j < cipher_column_size; j++) {
      // re-arrange the ciphertext vector for calling inner product
      auto *cipher_vec_j = new EncodedNumber[cipher_row_size];
      for (int k = 0; k < cipher_row_size; k++) {
        cipher_vec_j[k] = cipher_mat[k][j];
      }
      djcs_t_aux_inner_product(pk, hr, res[i][j], cipher_vec_j, plain_mat[i],
                               cipher_row_size);
      delete[] cipher_vec_j;
    }
  }
}

void djcs_t_aux_ele_wise_mat_mat_ep_mult(
    djcs_t_public_key *pk, EncodedNumber **res, EncodedNumber **cipher_mat,
    EncodedNumber **plain_mat, int cipher_row_size, int cipher_column_size,
    int plain_row_size, int plain_column_size) {
  if ((plain_row_size != cipher_row_size) ||
      (plain_column_size != cipher_column_size)) {
    log_error("The two matrix dimension do not match for element-wise "
              "multiplication");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < plain_row_size; i++) {
    djcs_t_aux_vec_ele_wise_ep_mul(pk, res[i], cipher_mat[i], plain_mat[i],
                                   plain_column_size);
  }
}

void djcs_t_aux_increase_prec_mat(djcs_t_public_key *pk, EncodedNumber **res,
                                  int target_precision,
                                  EncodedNumber **cipher_mat, int row_size,
                                  int column_size) {
  check_size(row_size);
  check_size(column_size);
  omp_set_num_threads(NUM_OMP_THREADS);
#pragma omp parallel for
  for (int i = 0; i < row_size; i++) {
    for (int j = 0; j < column_size; j++) {
      djcs_t_aux_increase_prec(pk, res[i][j], target_precision,
                               cipher_mat[i][j]);
    }
  }
}

void djcs_t_public_key_copy(djcs_t_public_key *src, djcs_t_public_key *dest) {
  dest->s = src->s;
  dest->l = src->l;
  dest->w = src->w;
  mpz_set(dest->g, src->g);
  mpz_set(dest->delta, src->delta);
  dest->n = (mpz_t *)malloc(sizeof(mpz_t) * (src->s + 1));
  for (int i = 0; i < dest->s + 1; i++) {
    mpz_init_set(dest->n[i], src->n[i]);
  }
}

void djcs_t_auth_server_copy(djcs_t_auth_server *src,
                             djcs_t_auth_server *dest) {
  dest->i = src->i;
  mpz_set(dest->si, src->si);
}

void djcs_t_hcs_random_copy(hcs_random *src, hcs_random *dest) {
  // TODO: probably this function causes memory leak, need further examination
  gmp_randinit_set(dest->rstate, src->rstate);
}

void check_size(int size) {
  if (size == 0) {
    log_error("The vector size is zero.");
    exit(EXIT_FAILURE);
  }
}

void check_encoded_public_key(const EncodedNumber &num1,
                              const EncodedNumber &num2) {
  mpz_t t1, t2;
  mpz_init(t1);
  mpz_init(t2);
  num1.getter_n(t1);
  num2.getter_n(t2);
  if (mpz_cmp(t1, t2) != 0) {
    log_error("The two EncodedNumbers are not with the same public key.");
    exit(EXIT_FAILURE);
  }
  mpz_clear(t1);
  mpz_clear(t2);
}

void check_ee_add_exponent(const EncodedNumber &num1,
                           const EncodedNumber &num2) {
  int num1_exponent = num1.getter_exponent();
  int num2_exponent = num2.getter_exponent();
  if (num1_exponent != num2_exponent) {
    log_error("The two EncodedNumbers have different exponents, cannot add. "
              "First cipher's exp = " +
              std::to_string(num1_exponent) +
              " and second cipher's exp = " + std::to_string(num2_exponent));
    exit(EXIT_FAILURE);
  }
}