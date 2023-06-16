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

#include <iostream>
#include <string>

#include "falcon/operator/phe/djcs_t_aux.h"
#include <gtest/gtest.h>

using namespace std;

TEST(PHE, ThresholdPaillierScheme) {
  // init djcs_t parameters
  int client_num = 3;
  hcs_random *hr = hcs_init_random();
  djcs_t_public_key *pk = djcs_t_init_public_key();
  djcs_t_private_key *vk = djcs_t_init_private_key();
  auto **au =
      (djcs_t_auth_server **)malloc(client_num * sizeof(djcs_t_auth_server *));
  auto *si = (mpz_t *)malloc(client_num * sizeof(mpz_t));

  // generate key pair
  djcs_t_generate_key_pair(pk, vk, hr, 1, 1024, client_num, client_num);
  mpz_t *coeff = djcs_t_init_polynomial(vk, hr);
  for (int i = 0; i < client_num; i++) {
    mpz_init(si[i]);
    djcs_t_compute_polynomial(vk, coeff, si[i], i);
    au[i] = djcs_t_init_auth_server();
    djcs_t_set_auth_server(au[i], si[i], i);
  }

  //////////////////////////////////////
  /// Test encryption and decryption ///
  //////////////////////////////////////
  double original_number = -0.11428453151;
  EncodedNumber number;
  number.set_double(pk->n[0], original_number, 64);

  EncodedNumber encrypted_number, decrypted_number;
  djcs_t_aux_encrypt(pk, hr, encrypted_number, number);

  auto *partially_decryption = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_decryption[i],
                               encrypted_number);
  }
  djcs_t_aux_share_combine(pk, decrypted_number, partially_decryption,
                           client_num);

  double decrypted_decoded_number;
  decrypted_number.decode(decrypted_decoded_number);
  EXPECT_NEAR(original_number, decrypted_decoded_number, 1e-3);

  //////////////////////////////////////
  ///// Test homomorphic addition //////
  //////////////////////////////////////
  double a1 = 5.25;
  double a2 = -0.38;
  EncodedNumber number_a1, number_a2;
  number_a1.set_double(pk->n[0], a1, 16);
  number_a2.set_double(pk->n[0], a2, 16);

  EncodedNumber encrypted_number_a1, encrypted_number_a2;
  djcs_t_aux_encrypt(pk, hr, encrypted_number_a1, number_a1);
  djcs_t_aux_encrypt(pk, hr, encrypted_number_a2, number_a2);

  EncodedNumber encrypted_sum, decrypted_sum;
  djcs_t_aux_ee_add(pk, encrypted_sum, encrypted_number_a1,
                    encrypted_number_a2);

  auto *partially_sum_decryption = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_sum_decryption[i],
                               encrypted_sum);
  }
  djcs_t_aux_share_combine(pk, decrypted_sum, partially_sum_decryption,
                           client_num);

  double decrypted_decoded_sum;
  decrypted_sum.decode(decrypted_decoded_sum);
  EXPECT_NEAR(a1 + a2, decrypted_decoded_sum, 1e-3);

  //////////////////////////////////////////
  ///// Test homomorphic addition ext //////
  //////////////////////////////////////////
  double a11 = 5.25;
  double a22 = -0.38;
  EncodedNumber number_a11, number_a22;
  number_a11.set_double(pk->n[0], a11, 16);
  number_a22.set_double(pk->n[0], a22, 32);

  EncodedNumber encrypted_number_a11, encrypted_number_a22;
  djcs_t_aux_encrypt(pk, hr, encrypted_number_a11, number_a11);
  djcs_t_aux_encrypt(pk, hr, encrypted_number_a22, number_a22);

  EncodedNumber encrypted_sum1, decrypted_sum1;
  djcs_t_aux_ee_add_ext(pk, encrypted_sum1, encrypted_number_a11,
                        encrypted_number_a22);
  int res_prec = std::abs(encrypted_sum1.getter_exponent());
  EXPECT_EQ(res_prec, 32);

  auto *partially_sum_decryption1 = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_sum_decryption1[i],
                               encrypted_sum1);
  }
  djcs_t_aux_share_combine(pk, decrypted_sum1, partially_sum_decryption1,
                           client_num);

  double decrypted_decoded_sum1;
  decrypted_sum1.decode(decrypted_decoded_sum1);
  EXPECT_NEAR(a1 + a2, decrypted_decoded_sum1, 1e-3);

  //////////////////////////////////////
  /// Test homomorphic multiplication //
  //////////////////////////////////////
  double b1 = 5.55;
  double b2 = -0.58;
  EncodedNumber number_b1, number_b2;
  number_b1.set_double(pk->n[0], b1, 192);
  number_b2.set_double(pk->n[0], b2, 16);

  EncodedNumber encrypted_number_b1;
  djcs_t_aux_encrypt(pk, hr, encrypted_number_b1, number_b1);

  EncodedNumber encrypted_product, decrypted_product;
  djcs_t_aux_ep_mul(pk, encrypted_product, encrypted_number_b1, number_b2);

  auto *partially_product_decryption = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_product_decryption[i],
                               encrypted_product);
  }
  djcs_t_aux_share_combine(pk, decrypted_product, partially_product_decryption,
                           client_num);

  double decrypted_decoded_product;
  decrypted_product.decode(decrypted_decoded_product);
  EXPECT_NEAR(b1 * b2, decrypted_decoded_product, 1e-3);

  //////////////////////////////////////////////
  /// Test increase precision of a ciphertext //
  /////////////////////////////////////////////
  double inc = 6.66;
  EncodedNumber number_inc, encry_inc, new_encry_inc, new_decry_inc;
  number_inc.set_double(pk->n[0], inc, 32);
  djcs_t_aux_encrypt(pk, hr, encry_inc, number_inc);
  int target_precision = 96;
  djcs_t_aux_increase_prec(pk, new_encry_inc, target_precision, encry_inc);
  int new_precision = std::abs(new_encry_inc.getter_exponent());
  EXPECT_EQ(new_precision, target_precision);
  auto *partially_inc_decryption = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_inc_decryption[i],
                               new_encry_inc);
  }
  djcs_t_aux_share_combine(pk, new_decry_inc, partially_inc_decryption,
                           client_num);
  double decry_dec_inc;
  new_decry_inc.decode(decry_dec_inc);
  EXPECT_NEAR(inc, decry_dec_inc, 1e-3);

  ////////////////////////////////////////////////////
  /// Test homomorphic multiplication exponent test //
  ////////////////////////////////////////////////////
  double b11 = 5.55;
  double b22 = 0.99;
  EncodedNumber number_b11, number_b22;
  number_b11.set_double(pk->n[0], b11, 16);
  number_b22.set_double(pk->n[0], b22, 16);

  EncodedNumber encrypted_number_b11;
  djcs_t_aux_encrypt(pk, hr, encrypted_number_b11, number_b11);

  EncodedNumber encrypted_product2, decrypted_product2;
  djcs_t_aux_ep_mul(pk, encrypted_product2, encrypted_number_b11, number_b22);
  //  std::cout << "encrypted_product2.precision = " <<
  //  std::abs(encrypted_product2.getter_exponent()) << std::endl;

  for (int i = 0; i < 61; i++) {
    // multiply encrypted_product2 by 1.0 ten times
    //    std::cout << "i = " << i << std::endl;
    djcs_t_aux_ep_mul(pk, encrypted_product2, encrypted_product2, number_b22);
    //    std::cout << "encrypted_product2.precision = " <<
    //    std::abs(encrypted_product2.getter_exponent()) << std::endl;
  }

  auto *partially_product_decryption2 = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_product_decryption2[i],
                               encrypted_product2);
  }
  djcs_t_aux_share_combine(pk, decrypted_product2,
                           partially_product_decryption2, client_num);

  double true_mul_res = b11;
  for (int i = 0; i < 62; i++) {
    true_mul_res = true_mul_res * b22;
  }

  double decrypted_decoded_product2;
  decrypted_product2.decode(decrypted_decoded_product2);
  EXPECT_NEAR(true_mul_res, decrypted_decoded_product2, 1e-1);

  //////////////////////////////////////
  /// Test homomorphic inner product ///
  //////////////////////////////////////
  double c1[5] = {0.1, -0.2, 0.3, -0.4, 0.5};
  double c2[5] = {0.9, 0.8, -0.7, 0.6, 0.5};
  auto *number_c1 = new EncodedNumber[5];
  auto *number_c2 = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    number_c1[i].set_double(pk->n[0], c1[i], 32);
    number_c2[i].set_double(pk->n[0], c2[i], 32);
  }
  auto *encrypted_number_c1 = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    djcs_t_aux_encrypt(pk, hr, encrypted_number_c1[i], number_c1[i]);
  }

  EncodedNumber encrypted_inner_product, decrypted_inner_product;
  djcs_t_aux_inner_product(pk, hr, encrypted_inner_product, encrypted_number_c1,
                           number_c2, 5);

  auto *partially_inner_prod_decryption = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_inner_prod_decryption[i],
                               encrypted_inner_product);
  }
  djcs_t_aux_share_combine(pk, decrypted_inner_product,
                           partially_inner_prod_decryption, client_num);

  double decrypted_decoded_inner_product;
  decrypted_inner_product.decode(decrypted_decoded_inner_product);
  double inner_product = 0.0;
  for (int i = 0; i < 5; i++) {
    inner_product = inner_product + c1[i] * c2[i];
  }
  EXPECT_NEAR(inner_product, decrypted_decoded_inner_product, 1e-3);
  // printf("inner_product = %f\n", inner_product);
  // printf("decrypted_decoded_inner_product = %f\n",
  // decrypted_decoded_inner_product);

  //////////////////////////////////////////////
  /// Test homomorphic matrix multiplication ///
  //////////////////////////////////////////////
  double d1[5] = {0.1, -0.2, 0.3, -0.4, 0.5};
  double d2[3][5] = {{0.9, 0.8, -0.7, 0.6, 0.5},
                     {-0.8, 0.7, -0.6, 0.5, -0.4},
                     {0.7, -0.6, 0.5, -0.4, 0.3}};

  auto *number_d1 = new EncodedNumber[5];
  auto **number_d2 = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    number_d2[i] = new EncodedNumber[5];
  }
  for (int i = 0; i < 5; i++) {
    number_d1[i].set_double(pk->n[0], d1[i], 16);
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 5; j++) {
      number_d2[i][j].set_double(pk->n[0], d2[i][j], 16);
    }
  }

  auto *encrypted_number_d1 = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    djcs_t_aux_encrypt(pk, hr, encrypted_number_d1[i], number_d1[i]);
  }

  auto *encrypted_mat_result = new EncodedNumber[3];
  auto *decrypted_mat_result = new EncodedNumber[3];
  djcs_t_aux_vec_mat_ep_mult(pk, hr, encrypted_mat_result, encrypted_number_d1,
                             number_d2, 3, 5);

  for (int j = 0; j < 3; j++) {
    auto *partially_decryption_mat_res_j = new EncodedNumber[client_num];
    for (int i = 0; i < client_num; i++) {
      djcs_t_aux_partial_decrypt(pk, au[i], partially_decryption_mat_res_j[i],
                                 encrypted_mat_result[j]);
    }
    djcs_t_aux_share_combine(pk, decrypted_mat_result[j],
                             partially_decryption_mat_res_j, client_num);

    double decrypted_decoded_mat_res_j;
    decrypted_mat_result[j].decode(decrypted_decoded_mat_res_j);
    double mat_mult_j = 0.0;
    for (int i = 0; i < 5; i++) {
      mat_mult_j = mat_mult_j + d1[i] * d2[j][i];
    }
    EXPECT_NEAR(mat_mult_j, decrypted_decoded_mat_res_j, 1e-3);
    delete[] partially_decryption_mat_res_j;
  }

  ///////////////////////////////////////////
  /// Test homomorphic vector aggregation ///
  ///////////////////////////////////////////
  double e[5] = {0.1, -0.2, 0.3, -0.4, 0.5};
  auto *number_e = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    number_e[i].set_double(pk->n[0], e[i], 16);
  }
  auto *encrypted_number_e = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    djcs_t_aux_encrypt(pk, hr, encrypted_number_e[i], number_e[i]);
  }

  EncodedNumber encrypted_agg, decrypted_agg;
  djcs_t_aux_vec_aggregate(pk, encrypted_agg, encrypted_number_e, 5);

  auto *partially_agg_decryption = new EncodedNumber[client_num];
  for (int i = 0; i < client_num; i++) {
    djcs_t_aux_partial_decrypt(pk, au[i], partially_agg_decryption[i],
                               encrypted_agg);
  }
  djcs_t_aux_share_combine(pk, decrypted_agg, partially_agg_decryption,
                           client_num);

  double decrypted_decoded_agg;
  decrypted_agg.decode(decrypted_decoded_agg);
  double agg = 0.0;
  for (double i : e) {
    agg += i;
  }
  EXPECT_NEAR(agg, decrypted_decoded_agg, 1e-3);
  // printf("inner_product = %f\n", inner_product);

  ///////////////////////////////////////////
  /// Test homomorphic vector ele-wise add ///
  ///////////////////////////////////////////
  double f1[5] = {0.1, -0.2, 0.3, -0.4, 0.5};
  double f2[5] = {0.9, 0.8, -0.7, 0.6, 0.5};
  auto *number_f1 = new EncodedNumber[5];
  auto *number_f2 = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    number_f1[i].set_double(pk->n[0], f1[i], 16);
    number_f2[i].set_double(pk->n[0], f2[i], 16);
  }
  for (int i = 0; i < 5; i++) {
    djcs_t_aux_encrypt(pk, hr, number_f1[i], number_f1[i]);
    djcs_t_aux_encrypt(pk, hr, number_f2[i], number_f2[i]);
  }
  auto *res_ele_wise_add_vec = new EncodedNumber[5];
  auto *dec_res_ele_wise_add_vec = new EncodedNumber[5];
  djcs_t_aux_vec_ele_wise_ee_add(pk, res_ele_wise_add_vec, number_f1, number_f2,
                                 5);
  for (int j = 0; j < 5; j++) {
    auto *partially_ele_wise_add = new EncodedNumber[client_num];
    for (int i = 0; i < client_num; i++) {
      djcs_t_aux_partial_decrypt(pk, au[i], partially_ele_wise_add[i],
                                 res_ele_wise_add_vec[j]);
    }
    djcs_t_aux_share_combine(pk, dec_res_ele_wise_add_vec[j],
                             partially_ele_wise_add, client_num);

    double decoded_res_add_j;
    dec_res_ele_wise_add_vec[j].decode(decoded_res_add_j);
    double res_add_j = f1[j] + f2[j];
    EXPECT_NEAR(res_add_j, decoded_res_add_j, 1e-3);
    delete[] partially_ele_wise_add;
  }

  ////////////////////////////////////////////////
  /// Test homomorphic vector ele-wise add ext ///
  ////////////////////////////////////////////////
  double f11[5] = {0.1, -0.2, 0.3, -0.4, 0.5};
  double f22[5] = {0.9, 0.8, -0.7, 0.6, 0.5};
  auto *number_f11 = new EncodedNumber[5];
  auto *number_f22 = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    number_f11[i].set_double(pk->n[0], f11[i], 16);
    number_f22[i].set_double(pk->n[0], f22[i], 64);
  }
  for (int i = 0; i < 5; i++) {
    djcs_t_aux_encrypt(pk, hr, number_f11[i], number_f11[i]);
    djcs_t_aux_encrypt(pk, hr, number_f22[i], number_f22[i]);
  }
  auto *res_ele_wise_add_vec1 = new EncodedNumber[5];
  auto *dec_res_ele_wise_add_vec1 = new EncodedNumber[5];
  djcs_t_aux_vec_ele_wise_ee_add_ext(pk, res_ele_wise_add_vec1, number_f11,
                                     number_f22, 5);
  int res_prec1 = std::abs(res_ele_wise_add_vec1[0].getter_exponent());
  EXPECT_EQ(res_prec1, 64);
  for (int j = 0; j < 5; j++) {
    auto *partially_ele_wise_add1 = new EncodedNumber[client_num];
    for (int i = 0; i < client_num; i++) {
      djcs_t_aux_partial_decrypt(pk, au[i], partially_ele_wise_add1[i],
                                 res_ele_wise_add_vec1[j]);
    }
    djcs_t_aux_share_combine(pk, dec_res_ele_wise_add_vec1[j],
                             partially_ele_wise_add1, client_num);

    double decoded_res_add_j1;
    dec_res_ele_wise_add_vec[j].decode(decoded_res_add_j1);
    double res_add_j = f1[j] + f2[j];
    EXPECT_NEAR(res_add_j, decoded_res_add_j1, 1e-3);
    delete[] partially_ele_wise_add1;
  }

  ////////////////////////////////////////////////
  /// Test homomorphic vector ele-wise ep mult ///
  ////////////////////////////////////////////////
  double g1[5] = {0.1, -0.2, 0.3, -0.4, 0.5};
  double g2[5] = {0.9, 0.8, -0.7, 0.6, 0.5};
  auto *number_g1 = new EncodedNumber[5];
  auto *number_g2 = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    number_g1[i].set_double(pk->n[0], g1[i], 16);
    number_g2[i].set_double(pk->n[0], g2[i], 16);
  }
  for (int i = 0; i < 5; i++) {
    djcs_t_aux_encrypt(pk, hr, number_g1[i], number_g1[i]);
  }
  auto *res_ele_wise_ep_vec = new EncodedNumber[5];
  auto *dec_res_ele_wise_ep_vec = new EncodedNumber[5];
  djcs_t_aux_vec_ele_wise_ep_mul(pk, res_ele_wise_ep_vec, number_g1, number_g2,
                                 5);
  for (int j = 0; j < 5; j++) {
    auto *partially_ele_wise_ep_mul = new EncodedNumber[client_num];
    for (int i = 0; i < client_num; i++) {
      djcs_t_aux_partial_decrypt(pk, au[i], partially_ele_wise_ep_mul[i],
                                 res_ele_wise_ep_vec[j]);
    }
    djcs_t_aux_share_combine(pk, dec_res_ele_wise_ep_vec[j],
                             partially_ele_wise_ep_mul, client_num);

    double decoded_res_ep_j;
    dec_res_ele_wise_ep_vec[j].decode(decoded_res_ep_j);
    double res_mul_j = g1[j] * g2[j];
    EXPECT_NEAR(res_mul_j, decoded_res_ep_j, 1e-3);
    delete[] partially_ele_wise_ep_mul;
  }

  ////////////////////////////////////////////////
  /// Test encrypted vector increase precision ///
  ////////////////////////////////////////////////

  double inc_vec[5] = {1.11, 2.22, 3.33, 5.55, 6.66};
  auto *number_inc_vec = new EncodedNumber[5];
  auto *encry_inc_vec = new EncodedNumber[5];
  auto *new_encry_inc_vec = new EncodedNumber[5];
  auto *new_decry_inc_vec = new EncodedNumber[5];
  for (int i = 0; i < 5; i++) {
    number_inc_vec[i].set_double(pk->n[0], inc_vec[i], 16);
    djcs_t_aux_encrypt(pk, hr, encry_inc_vec[i], number_inc_vec[i]);
  }
  int target_precision_1 = 96;
  djcs_t_aux_increase_prec_vec(pk, new_encry_inc_vec, target_precision_1,
                               encry_inc_vec, 5);
  int new_precision_1 = std::abs(new_encry_inc_vec[0].getter_exponent());
  EXPECT_EQ(new_precision_1, target_precision_1);
  for (int k = 0; k < 5; k++) {
    auto *partially_inc_decryption_1 = new EncodedNumber[client_num];
    for (int i = 0; i < client_num; i++) {
      djcs_t_aux_partial_decrypt(pk, au[i], partially_inc_decryption_1[i],
                                 new_encry_inc_vec[k]);
    }
    djcs_t_aux_share_combine(pk, new_decry_inc_vec[k],
                             partially_inc_decryption_1, client_num);
    double decry_dec_inc_1;
    new_decry_inc_vec[k].decode(decry_dec_inc_1);
    EXPECT_NEAR(inc_vec[k], decry_dec_inc_1, 1e-3);
    delete[] partially_inc_decryption_1;
  }

  ////////////////////////////////////////////
  /// Test homomorphic matrix ele-wise add ///
  ////////////////////////////////////////////
  double h1[3][5] = {{0.1, -0.2, 0.3, -0.4, 0.5},
                     {0.2, -0.3, 0.4, -0.5, 0.6},
                     {-0.3, 0.4, -0.5, 0.6, -0.7}};
  double h2[3][5] = {{0.9, 0.8, -0.7, 0.6, 0.5},
                     {-0.8, 0.7, -0.6, 0.5, -0.4},
                     {0.7, -0.6, 0.5, -0.4, 0.3}};

  auto **number_h1 = new EncodedNumber *[3];
  auto **number_h2 = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    number_h1[i] = new EncodedNumber[5];
    number_h2[i] = new EncodedNumber[5];
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 5; j++) {
      number_h1[i][j].set_double(pk->n[0], h1[i][j], 16);
      number_h2[i][j].set_double(pk->n[0], h2[i][j], 16);
    }
  }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 5; j++) {
      djcs_t_aux_encrypt(pk, hr, number_h1[i][j], number_h1[i][j]);
      djcs_t_aux_encrypt(pk, hr, number_h2[i][j], number_h2[i][j]);
    }
  }

  auto **encrypted_mat_ele_wise_add = new EncodedNumber *[3];
  auto **decrypted_mat_ele_wise_add = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    encrypted_mat_ele_wise_add[i] = new EncodedNumber[5];
    decrypted_mat_ele_wise_add[i] = new EncodedNumber[5];
  }

  djcs_t_aux_matrix_ele_wise_ee_add(pk, number_h1, number_h1, number_h2, 3, 5);

  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 5; k++) {
      auto *partially_decryption_mat_res_jk = new EncodedNumber[client_num];
      for (int i = 0; i < client_num; i++) {
        djcs_t_aux_partial_decrypt(
            pk, au[i], partially_decryption_mat_res_jk[i], number_h1[j][k]);
      }
      djcs_t_aux_share_combine(pk, decrypted_mat_ele_wise_add[j][k],
                               partially_decryption_mat_res_jk, client_num);

      double decrypted_decoded_mat_res_jk;
      decrypted_mat_ele_wise_add[j][k].decode(decrypted_decoded_mat_res_jk);
      double mat_add_jk = h1[j][k] + h2[j][k];
      EXPECT_NEAR(mat_add_jk, decrypted_decoded_mat_res_jk, 1e-3);
      delete[] partially_decryption_mat_res_jk;
    }
  }

  ////////////////////////////////////////////////
  /// Test homomorphic matrix ele-wise add ext ///
  ////////////////////////////////////////////////
  double h11[3][5] = {{0.1, -0.2, 0.3, -0.4, 0.5},
                      {0.2, -0.3, 0.4, -0.5, 0.6},
                      {-0.3, 0.4, -0.5, 0.6, -0.7}};
  double h22[3][5] = {{0.9, 0.8, -0.7, 0.6, 0.5},
                      {-0.8, 0.7, -0.6, 0.5, -0.4},
                      {0.7, -0.6, 0.5, -0.4, 0.3}};

  auto **number_h11 = new EncodedNumber *[3];
  auto **number_h22 = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    number_h11[i] = new EncodedNumber[5];
    number_h22[i] = new EncodedNumber[5];
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 5; j++) {
      number_h11[i][j].set_double(pk->n[0], h11[i][j], 256);
      number_h22[i][j].set_double(pk->n[0], h22[i][j], 128);
    }
  }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 5; j++) {
      djcs_t_aux_encrypt(pk, hr, number_h11[i][j], number_h11[i][j]);
      djcs_t_aux_encrypt(pk, hr, number_h22[i][j], number_h22[i][j]);
    }
  }

  auto **encrypted_mat_ele_wise_add1 = new EncodedNumber *[3];
  auto **decrypted_mat_ele_wise_add1 = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    encrypted_mat_ele_wise_add1[i] = new EncodedNumber[5];
    decrypted_mat_ele_wise_add1[i] = new EncodedNumber[5];
  }

  djcs_t_aux_matrix_ele_wise_ee_add_ext(pk, encrypted_mat_ele_wise_add1,
                                        number_h11, number_h22, 3, 5);
  int res_prec2 = std::abs(encrypted_mat_ele_wise_add1[0][0].getter_exponent());
  EXPECT_EQ(res_prec2, 256);
  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 5; k++) {
      auto *partially_decryption_mat_res_jk1 = new EncodedNumber[client_num];
      for (int i = 0; i < client_num; i++) {
        djcs_t_aux_partial_decrypt(pk, au[i],
                                   partially_decryption_mat_res_jk1[i],
                                   encrypted_mat_ele_wise_add1[j][k]);
      }
      djcs_t_aux_share_combine(pk, decrypted_mat_ele_wise_add1[j][k],
                               partially_decryption_mat_res_jk1, client_num);

      double decrypted_decoded_mat_res_jk1;
      decrypted_mat_ele_wise_add1[j][k].decode(decrypted_decoded_mat_res_jk1);
      double mat_add_jk = h1[j][k] + h2[j][k];
      EXPECT_NEAR(mat_add_jk, decrypted_decoded_mat_res_jk1, 1e-3);
      delete[] partially_decryption_mat_res_jk1;
    }
  }

  ///////////////////////////////////////////////////////////
  /// Test homomorphic matrix cipher/plain multiplication ///
  ///////////////////////////////////////////////////////////
  double o1[3][5] = {{0.1, -0.2, 0.3, -0.4, 0.5},
                     {0.2, -0.3, 0.4, -0.5, 0.6},
                     {-0.3, 0.4, -0.5, 0.6, -0.7}};
  double o2[5][3] = {{0.9, 0.8, -0.7},
                     {0.6, 0.5, -0.8},
                     {0.7, -0.6, 0.5},
                     {-0.4, 0.7, -0.6},
                     {0.5, -0.4, 0.3}};

  auto **number_o1 = new EncodedNumber *[3];
  auto **number_o2 = new EncodedNumber *[5];
  for (int i = 0; i < 3; i++) {
    number_o1[i] = new EncodedNumber[5];
  }
  for (int i = 0; i < 5; i++) {
    number_o2[i] = new EncodedNumber[3];
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 5; j++) {
      number_o1[i][j].set_double(pk->n[0], o1[i][j], 16);
    }
  }
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 3; j++) {
      number_o2[i][j].set_double(pk->n[0], o2[i][j], 16);
      djcs_t_aux_encrypt(pk, hr, number_o2[i][j], number_o2[i][j]);
    }
  }

  auto **encrypted_mat_ep_mult = new EncodedNumber *[3];
  auto **decrypted_mat_ep_mult = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    encrypted_mat_ep_mult[i] = new EncodedNumber[3];
    decrypted_mat_ep_mult[i] = new EncodedNumber[3];
  }

  djcs_t_aux_mat_mat_ep_mult(pk, hr, encrypted_mat_ep_mult, number_o2,
                             number_o1, 5, 3, 3, 5);

  double mat_mult_res[3][3];
  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 3; k++) {
      double s = 0.0;
      for (int i = 0; i < 5; i++) {
        s += (o1[j][i] * o2[i][k]);
      }
      mat_mult_res[j][k] = s;
      //      std::cout << "j = " << j << ", k = " << k << ", mat_mult_res = "
      //      << mat_mult_res[j][k] << std::endl;
    }
  }

  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 3; k++) {
      auto *partially_decryption_mat_ep_mult_res_jk =
          new EncodedNumber[client_num];
      for (int i = 0; i < client_num; i++) {
        djcs_t_aux_partial_decrypt(pk, au[i],
                                   partially_decryption_mat_ep_mult_res_jk[i],
                                   encrypted_mat_ep_mult[j][k]);
      }
      djcs_t_aux_share_combine(pk, decrypted_mat_ep_mult[j][k],
                               partially_decryption_mat_ep_mult_res_jk,
                               client_num);

      double decrypted_decoded_mat_ep_res_jk;
      decrypted_mat_ep_mult[j][k].decode(decrypted_decoded_mat_ep_res_jk);
      //      std::cout << "decrypted[" << j << "][" << k << "] = " <<
      //      decrypted_decoded_mat_ep_res_jk << std::endl;
      EXPECT_NEAR(mat_mult_res[j][k], decrypted_decoded_mat_ep_res_jk, 1e-3);
      delete[] partially_decryption_mat_ep_mult_res_jk;
    }
  }

  /////////////////////////////////////
  /// Test vector double encryption ///
  /////////////////////////////////////
  std::vector<double> p{0.9, 0.8, -0.7, 0.6, 0.5};
  auto *number_p = new EncodedNumber[5];
  djcs_t_aux_double_vec_encryption(pk, hr, number_p, 5, p,
                                   PHE_FIXED_POINT_PRECISION);
  auto *decrypted_number_p = new EncodedNumber[5];
  for (int j = 0; j < 5; j++) {
    auto *partially_decrypt_j = new EncodedNumber[client_num];
    for (int i = 0; i < client_num; i++) {
      djcs_t_aux_partial_decrypt(pk, au[i], partially_decrypt_j[i],
                                 number_p[j]);
    }
    djcs_t_aux_share_combine(pk, decrypted_number_p[j], partially_decrypt_j,
                             client_num);

    double decoded_number_pj;
    decrypted_number_p[j].decode(decoded_number_pj);
    EXPECT_NEAR(p[j], decoded_number_pj, 1e-3);
    delete[] partially_decrypt_j;
  }

  //////////////////////////////////
  /// Test vector int encryption ///
  //////////////////////////////////
  std::vector<int> q{1, 2, 3, 4, 5};
  auto *number_q = new EncodedNumber[5];
  djcs_t_aux_int_vec_encryption(pk, hr, number_q, 5, q,
                                PHE_FIXED_POINT_PRECISION);
  auto *decrypted_number_q = new EncodedNumber[5];
  for (int j = 0; j < 5; j++) {
    auto *partially_decrypt_j = new EncodedNumber[client_num];
    for (int i = 0; i < client_num; i++) {
      djcs_t_aux_partial_decrypt(pk, au[i], partially_decrypt_j[i],
                                 number_q[j]);
    }
    djcs_t_aux_share_combine(pk, decrypted_number_q[j], partially_decrypt_j,
                             client_num);

    long decoded_number_qj;
    decrypted_number_q[j].decode(decoded_number_qj);
    EXPECT_EQ(q[j], decoded_number_qj);
    delete[] partially_decrypt_j;
  }

  /////////////////////////////////////
  /// Test matrix double encryption ///
  /////////////////////////////////////
  std::vector<std::vector<double>> r;
  std::vector<double> r1{0.9, 0.8, -0.7, 0.6, 0.5};
  std::vector<double> r2{0.1, 0.2, -0.3, 0.4, 0.5};
  std::vector<double> r3{0.8, 0.6, -0.7, 0.6, 0.3};
  r.push_back(r1);
  r.push_back(r2);
  r.push_back(r3);
  auto *number_mat_r = new EncodedNumber *[3];
  auto *decrypted_number_mat_r = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    number_mat_r[i] = new EncodedNumber[5];
    decrypted_number_mat_r[i] = new EncodedNumber[5];
  }
  djcs_t_aux_double_mat_encryption(pk, hr, number_mat_r, 3, 5, r,
                                   PHE_FIXED_POINT_PRECISION);
  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 5; k++) {
      auto *partially_decrypt_jk = new EncodedNumber[client_num];
      for (int i = 0; i < client_num; i++) {
        djcs_t_aux_partial_decrypt(pk, au[i], partially_decrypt_jk[i],
                                   number_mat_r[j][k]);
      }
      djcs_t_aux_share_combine(pk, decrypted_number_mat_r[j][k],
                               partially_decrypt_jk, client_num);

      double decoded_number_rjk;
      decrypted_number_mat_r[j][k].decode(decoded_number_rjk);
      EXPECT_NEAR(r[j][k], decoded_number_rjk, 1e-3);
      delete[] partially_decrypt_jk;
    }
  }

  /////////////////////////////////////////////////
  /// Test encrypted matrix increase encryption ///
  /////////////////////////////////////////////////
  std::vector<std::vector<double>> r_inc;
  std::vector<double> r1_inc{0.9, 0.8, -0.7, 0.6, 0.5};
  std::vector<double> r2_inc{0.1, 0.2, -0.3, 0.4, 0.5};
  std::vector<double> r3_inc{0.8, 0.6, -0.7, 0.6, 0.3};
  r_inc.push_back(r1_inc);
  r_inc.push_back(r2_inc);
  r_inc.push_back(r3_inc);
  auto *number_mat_r_inc = new EncodedNumber *[3];
  auto *decrypted_number_mat_r_inc = new EncodedNumber *[3];
  for (int i = 0; i < 3; i++) {
    number_mat_r_inc[i] = new EncodedNumber[5];
    decrypted_number_mat_r_inc[i] = new EncodedNumber[5];
  }
  djcs_t_aux_double_mat_encryption(pk, hr, number_mat_r_inc, 3, 5, r_inc,
                                   PHE_FIXED_POINT_PRECISION);
  int target_precision_2 = 108;
  djcs_t_aux_increase_prec_mat(pk, number_mat_r_inc, target_precision_2,
                               number_mat_r_inc, 3, 5);
  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 5; k++) {
      auto *partially_decrypt_jk = new EncodedNumber[client_num];
      for (int i = 0; i < client_num; i++) {
        djcs_t_aux_partial_decrypt(pk, au[i], partially_decrypt_jk[i],
                                   number_mat_r_inc[j][k]);
      }
      djcs_t_aux_share_combine(pk, decrypted_number_mat_r_inc[j][k],
                               partially_decrypt_jk, client_num);

      double decoded_number_rjk;
      decrypted_number_mat_r_inc[j][k].decode(decoded_number_rjk);
      EXPECT_NEAR(r_inc[j][k], decoded_number_rjk, 1e-3);
      delete[] partially_decrypt_jk;
    }
  }

  // free memory
  delete[] partially_decryption;
  delete[] partially_sum_decryption;
  delete[] partially_sum_decryption1;
  delete[] partially_product_decryption;
  delete[] partially_inc_decryption;
  delete[] partially_product_decryption2;
  delete[] partially_inner_prod_decryption;
  delete[] partially_agg_decryption;
  delete[] number_c1;
  delete[] number_c2;
  delete[] encrypted_number_c1;
  delete[] number_d1;
  delete[] number_e;
  delete[] number_f1;
  delete[] number_f2;
  delete[] number_f11;
  delete[] number_f22;
  delete[] number_g1;
  delete[] number_g2;
  delete[] encrypted_number_e;
  delete[] res_ele_wise_add_vec;
  delete[] dec_res_ele_wise_add_vec;
  delete[] res_ele_wise_add_vec1;
  delete[] dec_res_ele_wise_add_vec1;
  delete[] res_ele_wise_ep_vec;
  delete[] dec_res_ele_wise_ep_vec;
  delete[] number_inc_vec;
  delete[] encry_inc_vec;
  delete[] new_encry_inc_vec;
  delete[] new_decry_inc_vec;
  delete[] encrypted_number_d1;
  delete[] encrypted_mat_result;
  delete[] decrypted_mat_result;
  for (int i = 0; i < 3; i++) {
    delete[] number_d2[i];
  }
  delete[] number_d2;
  for (int i = 0; i < 3; i++) {
    delete[] number_h1[i];
    delete[] number_h2[i];
    delete[] number_h11[i];
    delete[] number_h22[i];
    delete[] encrypted_mat_ele_wise_add[i];
    delete[] encrypted_mat_ele_wise_add1[i];
    delete[] decrypted_mat_ele_wise_add[i];
    delete[] decrypted_mat_ele_wise_add1[i];
  }
  delete[] number_h1;
  delete[] number_h11;
  delete[] number_h2;
  delete[] number_h22;
  delete[] encrypted_mat_ele_wise_add;
  delete[] encrypted_mat_ele_wise_add1;
  delete[] decrypted_mat_ele_wise_add;
  delete[] decrypted_mat_ele_wise_add1;
  for (int i = 0; i < 3; i++) {
    delete[] number_o1[i];
    delete[] encrypted_mat_ep_mult[i];
    delete[] decrypted_mat_ep_mult[i];
  }
  delete[] number_o1;
  delete[] encrypted_mat_ep_mult;
  delete[] decrypted_mat_ep_mult;
  for (int i = 0; i < 5; i++) {
    delete[] number_o2[i];
  }
  delete[] number_o2;
  delete[] number_p;
  delete[] decrypted_number_p;
  delete[] number_q;
  delete[] decrypted_number_q;
  for (int i = 0; i < 3; i++) {
    delete[] number_mat_r[i];
    delete[] decrypted_number_mat_r[i];
    delete[] number_mat_r_inc[i];
    delete[] decrypted_number_mat_r_inc[i];
  }
  delete[] number_mat_r;
  delete[] decrypted_number_mat_r;
  delete[] number_mat_r_inc;
  delete[] decrypted_number_mat_r_inc;

  hcs_free_random(hr);
  djcs_t_free_public_key(pk);
  djcs_t_free_private_key(vk);

  for (int i = 0; i < client_num; i++) {
    mpz_clear(si[i]);
    djcs_t_free_auth_server(au[i]);
  }
  djcs_t_free_polynomial(vk, coeff);

  free(si);
  free(au);
}