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
// Created by wuyuncheng on 16/8/20.
//

#include "falcon/operator/phe/fixed_point_encoder.h"
#include <cmath>
#include <falcon/utils/logger/logger.h>
#include <iomanip>
#include <iostream>

EncodedNumber::EncodedNumber() {
  // mpz_init (mpz_t x) Initialize x, and set its value to 0.
  mpz_init(n);
  mpz_init(value);
  type = Plaintext;
}

EncodedNumber::EncodedNumber(const EncodedNumber &number) {
  mpz_init(n);
  mpz_init(value);

  mpz_t n_helper, value_helper;
  mpz_init(n_helper);
  mpz_init(value_helper);
  // assign the "n" value of the EncodedNumber instance "number" to n_helper
  number.getter_n(n_helper);
  number.getter_value(value_helper);

  // assign the value of n_helper to n of current EncodedNumber instance
  mpz_set(n, n_helper);
  mpz_set(value, value_helper);
  exponent = number.getter_exponent();
  type = number.getter_type();

  // delete from memory
  mpz_clear(n_helper);
  mpz_clear(value_helper);
}

EncodedNumber &EncodedNumber::operator=(const EncodedNumber &number) {
  // need to make sure the number has been initialized beforehand
  // mpz_init(n);
  // mpz_init(value);

  mpz_t n_helper, value_helper;
  mpz_init(n_helper);
  mpz_init(value_helper);
  number.getter_n(n_helper);
  number.getter_value(value_helper);

  mpz_set(n, n_helper);
  mpz_set(value, value_helper);
  exponent = number.getter_exponent();
  type = number.getter_type();

  mpz_clear(n_helper);
  mpz_clear(value_helper);
}

EncodedNumber::~EncodedNumber() {
  // gmp_printf("value = %Zd\n", value);
  mpz_clear(n);
  mpz_clear(value);
}

void EncodedNumber::set_integer(mpz_t pn, int v) {
  mpz_set(n, pn);
  fixed_pointed_encode(v, value, exponent);
}

void EncodedNumber::set_double(mpz_t pn, double v, int precision) {
  mpz_set(n, pn);
  fixed_pointed_encode(v, precision, value, exponent);
}

void EncodedNumber::decrease_exponent(int new_exponent) {
  if (new_exponent > exponent) {
    log_error("New exponent should be more negative than old exponent.");
    exit(EXIT_FAILURE);
  }

  if (new_exponent == exponent)
    return;

  unsigned long int exp_diff = exponent - new_exponent;
  mpz_t t;
  mpz_init(t);
  mpz_ui_pow_ui(t, PHE_FIXED_POINT_BASE, exp_diff);
  mpz_mul(value, value, t);
  exponent = new_exponent;

  mpz_clear(t);
}

void EncodedNumber::increase_exponent(int new_exponent) {
  if (new_exponent < exponent) {
    log_error("New exponent should be more positive than old exponent.");
    exit(EXIT_FAILURE);
  }

  if (new_exponent == exponent)
    return;

  mpz_t t, new_value;
  mpz_init(t);
  mpz_init(new_value);
  unsigned long int exp_diff = new_exponent - exponent;
  mpz_ui_pow_ui(t, PHE_FIXED_POINT_BASE, exp_diff);
  mpz_cdiv_q(new_value, value, t);

  exponent = new_exponent;
  mpz_set(value, new_value);

  mpz_clear(t);
  mpz_clear(new_value);
}

void EncodedNumber::decode(long &v) {
  if (exponent != 0) {
    // not integer, should not call this decode function
    log_error("Exponent is not zero, failed, should call decode with double.");
    exit(EXIT_FAILURE);
  }

  switch (check_encoded_number()) {
  case Positive:
    fixed_pointed_decode(v, value);
    break;
  case Negative:
    mpz_sub(value, value, n);
    fixed_pointed_decode(v, value);
    break;
  case Overflow:
    log_error("Encoded number is overflow.");
    exit(EXIT_FAILURE);
  default:
    log_error("Encoded number is corrupted.");
    exit(EXIT_FAILURE);
  }
}

void EncodedNumber::decode(double &v) {
  switch (check_encoded_number()) {
  case Positive:
    fixed_pointed_decode(v, value, exponent);
    break;
  case Negative:
    mpz_sub(value, value, n);
    fixed_pointed_decode(v, value, exponent);
    break;
  case Overflow:
    log_error("Encoded number is overflow.");
    exit(EXIT_FAILURE);
  default:
    log_error("Encoded number is corrupted.");
    exit(EXIT_FAILURE);
  }
}

void EncodedNumber::decode_with_truncation(double &v, int truncated_exponent) {
  switch (check_encoded_number()) {
  case Positive:
    fixed_pointed_decode_truncated(v, value, exponent, truncated_exponent);
    break;
  case Negative:
    mpz_sub(value, value, n);
    fixed_pointed_decode_truncated(v, value, exponent, truncated_exponent);
    break;
  case Overflow:
    log_error("Encoded number is overflow.");
    exit(EXIT_FAILURE);
  default:
    log_error("Encoded number is corrupted.");
    exit(EXIT_FAILURE);
  }
}

EncodedNumberState EncodedNumber::check_encoded_number() {
  // max_int is the threshold of positive number
  // neg_int is the threshold of negative number
  mpz_t max_int, neg_int;
  mpz_init(max_int);
  compute_decode_threshold(max_int);
  mpz_init(neg_int);
  mpz_sub(neg_int, n, max_int);

  // gmp_printf("max_int is %Zd\n", max_int);
  // gmp_printf("neg_int is %Zd\n", neg_int);

  EncodedNumberState state;
  if (mpz_cmp(value, n) >= 0) {
    state = Invalid;
  } else if (mpz_cmp(value, max_int) <= 0) {
    state = Positive;
  } else if (mpz_cmp(value, neg_int) >= 0) {
    state = Negative;
  } else {
    state = Overflow;
  }

  // printf("state is %d\n", state);
  mpz_clear(max_int);
  mpz_clear(neg_int);
  return state;
}

void EncodedNumber::compute_decode_threshold(mpz_t max_int) {
  mpz_t t;
  mpz_init(t);
  mpz_fdiv_q_ui(t, n, 3);
  mpz_sub_ui(max_int, t, 1); // this is max int
  mpz_clear(t);
}

void EncodedNumber::setter_n(mpz_t s_n) { mpz_set(n, s_n); }

void EncodedNumber::setter_exponent(int s_exponent) { exponent = s_exponent; }

void EncodedNumber::setter_value(mpz_t s_value) { mpz_set(value, s_value); }

void EncodedNumber::setter_type(EncodedNumberType s_type) { type = s_type; }

int EncodedNumber::getter_exponent() const { return exponent; }

void EncodedNumber::getter_n(mpz_t g_n) const { mpz_set(g_n, n); }

void EncodedNumber::getter_value(mpz_t g_value) const {
  mpz_set(g_value, value);
}

EncodedNumberType EncodedNumber::getter_type() const { return type; }

// helper functions bellow
long long fixed_pointed_integer_representation(double value, int precision) {
  auto ex = (long long)pow(PHE_FIXED_POINT_BASE, precision);
  std::stringstream ss;
  // todo: setprecision truncate the double based on 10 hex, but
  // PHE_FIXED_POINT_BASE is 2, why not set PHE_FIXED_POINT_BASE to 10 ? keep
  // precision decimal places.
  ss << std::fixed << std::setprecision(precision) << value;
  std::string s = ss.str();
  //  auto r = (long long) (::atof(s.c_str()) * ex);
  // convert string back to long double and upgrade with ex**precision, ex is 2
  // by default todo: convert double to long long will lost the precision party.
  auto r = (long long)(std::stold(s) * ex);
  return r;
}

void fixed_pointed_encode(long value, mpz_t res, int &exponent) {
  mpz_set_si(res, value);
  exponent = 0;
}

void fixed_pointed_encode(double value, int precision, mpz_t res,
                          int &exponent) {
  // if precision < PHE_MAXIMUM_FIXED_POINT_PRECISION, encode it directly
  // else, first encode the double value by PHE_MAXIMUM_FIXED_POINT_PRECISION
  //       then pow precision - PHE_MAXIMUM_FIXED_POINT_PRECISION
  long long r;
  if (precision <= PHE_MAXIMUM_FIXED_POINT_PRECISION) {
    r = fixed_pointed_integer_representation(value, precision);
    mpz_set_si(res, r);
  } else {
    r = fixed_pointed_integer_representation(value,
                                             PHE_MAXIMUM_FIXED_POINT_PRECISION);
    mpz_set_si(res, r);
    // set base^{rest_precision}
    int rest_precision = precision - PHE_MAXIMUM_FIXED_POINT_PRECISION;
    mpz_t tmp;
    mpz_init(tmp);
    mpz_ui_pow_ui(tmp, PHE_FIXED_POINT_BASE, rest_precision);
    // multiply to res
    mpz_mul(res, res, tmp);
    mpz_clear(tmp);
  }
  exponent = 0 - precision;
}

void fixed_pointed_decode(long &value, mpz_t res) { value = mpz_get_si(res); }

void fixed_pointed_decode(double &value, mpz_t res, int exponent) {
  if (exponent >= 0) {
    log_error("Decode mpz_t for double value failed.");
    exit(EXIT_FAILURE);
  }
  if (exponent < 0 - PHE_MAXIMUM_FIXED_POINT_PRECISION) {
    fixed_pointed_decode_truncated(value, res, exponent,
                                   0 - PHE_MAXIMUM_FIXED_POINT_PRECISION);
  } else {
    char *t = mpz_get_str(NULL, PHE_STR_BASE, res);
    long long v = ::atoll(t);

    if (v == 0) {
      value = 0;
    } else {
      value = ((double)v * pow(PHE_FIXED_POINT_BASE, exponent));
    }
    free(t);
  }
}

void fixed_pointed_decode_truncated(double &value, mpz_t res, int exponent,
                                    int truncated_exponent) {
  if (exponent >= 0 || truncated_exponent >= 0) {
    log_error("Decode mpz_t for double value failed.");
    exit(EXIT_FAILURE);
  }
  //  gmp_printf("res = %Zd\n", res);
  //  log_info("exponent = " + std::to_string(exponent));
  //  log_info("truncated_exponent = " + std::to_string(truncated_exponent));

  // check whether the truncated precision is larger than
  // PHE_MAXIMUM_FIXED_POINT_PRECISION if so, set the truncated exponent to (0 -
  // PHE_MAXIMUM_FIXED_POINT_PRECISION) else, continue to do the following steps
  if (abs(truncated_exponent) > PHE_MAXIMUM_FIXED_POINT_PRECISION) {
    truncated_exponent = 0 - PHE_MAXIMUM_FIXED_POINT_PRECISION;
  }
  //  std::cout << "exponent = " << exponent << std::endl;
  //  std::cout << "truncated_exponent = " << truncated_exponent << std::endl;

  // the true precision is acceptable, no need to truncate
  if (exponent >= truncated_exponent) {
    char *t = mpz_get_str(NULL, PHE_STR_BASE, res);
    long long v = ::atoll(t);
    if (v == 0) {
      value = 0;
    } else {
      double multiplier = pow(PHE_FIXED_POINT_BASE, exponent);
      value = ((double)v) * multiplier;
      // printf("decoded value = %f\n", value);
    }
    free(t);
  } else {
    // convert res to truncated_exponent
    unsigned long long int exp_diff = truncated_exponent - exponent;
    // printf("exp_diff = %ld\n", exp_diff);
    mpz_t tmp, new_value;
    mpz_init(tmp);
    mpz_init(new_value);
    mpz_ui_pow_ui(tmp, PHE_FIXED_POINT_BASE, exp_diff);
    mpz_cdiv_q(new_value, res, tmp);
    // gmp_printf("tmp = %Zd\n", tmp);
    // gmp_printf("new_value = %Zd\n", new_value);

    // decode with new_value
    char *t_new = mpz_get_str(NULL, PHE_STR_BASE, new_value);
    long long v_new = ::atoll(t_new);

    if (v_new == 0) {
      value = 0;
    } else {
      double multiplier = pow(PHE_FIXED_POINT_BASE, truncated_exponent);
      value = ((double)v_new) * multiplier;
    }
    free(t_new);
    mpz_clear(tmp);
    mpz_clear(new_value);
  }
}

void metrics_to_encoded_num(EncodedNumber **result,
                            const std::vector<std::vector<double>> &two_d_vec,
                            mpz_t pn, int exponent) {

  // the result is created outside using
  //  auto **encoded_shares = new EncodedNumber *[n_shares_row];
  //  for (int i = 0; i < n_shares_row; i++) {
  //      encoded_shares[i] = new EncodedNumber[n_shares_col];
  //  }

  if (two_d_vec.empty()) {
    log_error("[vector_to_encoded_num] convert to two-d encoded number failed "
              "due ot 0 len of shares");
    exit(EXIT_FAILURE);
  }
  int row_size = two_d_vec.size();
  int column_size = two_d_vec[0].size();
  for (int i = 0; i < row_size; i++) {
    for (int j = 0; j < column_size; j++) {
      result[i][j].set_double(pn, two_d_vec[i][j], exponent);
    }
  }
}
