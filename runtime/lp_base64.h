#ifndef LP_BASE64_H
#define LP_BASE64_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


static const char lp_b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char lp_b64_url_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static inline char *lp_base64_encode_impl(const unsigned char *data,
                                          size_t input_length,
                                          const char *table) {
  size_t output_length = 4 * ((input_length + 2) / 3);
  char *encoded_data = (char *)malloc(output_length + 1);
  if (encoded_data == NULL)
    return NULL;

  for (size_t i = 0, j = 0; i < input_length;) {
    uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
    uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
    uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

    uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

    encoded_data[j++] = table[(triple >> 3 * 6) & 0x3F];
    encoded_data[j++] = table[(triple >> 2 * 6) & 0x3F];
    encoded_data[j++] = table[(triple >> 1 * 6) & 0x3F];
    encoded_data[j++] = table[(triple >> 0 * 6) & 0x3F];
  }

  for (size_t i = 0; i < (3 - input_length % 3) % 3; i++)
    encoded_data[output_length - 1 - i] = '=';

  encoded_data[output_length] = '\0';
  return encoded_data;
}

static inline const char *lp_base64_encode(const char *data) {
  if (!data)
    return strdup("");
  return lp_base64_encode_impl((const unsigned char *)data, strlen(data),
                               lp_b64_table);
}

static inline const char *lp_base64_urlsafe_encode(const char *data) {
  if (!data)
    return strdup("");
  return lp_base64_encode_impl((const unsigned char *)data, strlen(data),
                               lp_b64_url_table);
}

static inline const char *lp_base64_encode_bytes(const char *data,
                                                 int64_t len) {
  if (!data || len <= 0)
    return strdup("");
  return lp_base64_encode_impl((const unsigned char *)data, (size_t)len,
                               lp_b64_table);
}

static inline int lp_b64_decode_char(char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (c == '+' || c == '-')
    return 62;
  if (c == '/' || c == '_')
    return 63;
  return -1;
}

static inline const char *lp_base64_decode(const char *data) {
  if (!data)
    return strdup("");
  size_t input_length = strlen(data);
  if (input_length % 4 != 0)
    return strdup("");

  size_t output_length = input_length / 4 * 3;
  if (data[input_length - 1] == '=')
    output_length--;
  if (data[input_length - 2] == '=')
    output_length--;

  char *decoded_data = (char *)malloc(output_length + 1);
  if (decoded_data == NULL)
    return NULL;

  for (size_t i = 0, j = 0; i < input_length;) {
    uint32_t sextet_a =
        data[i] == '=' ? 0 & i++ : lp_b64_decode_char(data[i++]);
    uint32_t sextet_b =
        data[i] == '=' ? 0 & i++ : lp_b64_decode_char(data[i++]);
    uint32_t sextet_c =
        data[i] == '=' ? 0 & i++ : lp_b64_decode_char(data[i++]);
    uint32_t sextet_d =
        data[i] == '=' ? 0 & i++ : lp_b64_decode_char(data[i++]);

    uint32_t triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) +
                      (sextet_c << 1 * 6) + (sextet_d << 0 * 6);

    if (j < output_length)
      decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
    if (j < output_length)
      decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
    if (j < output_length)
      decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
  }

  decoded_data[output_length] = '\0';
  return decoded_data;
}

static inline const char *lp_base64_urlsafe_decode(const char *data) {
  return lp_base64_decode(data); /* Decode logic handles both +/ and -_ */
}

static inline int lp_base64_is_valid(const char *data) {
  if (!data)
    return 0;
  size_t len = strlen(data);
  if (len % 4 != 0)
    return 0;
  for (size_t i = 0; i < len; i++) {
    if (data[i] == '=') {
      if (i < len - 2)
        return 0;
    } else {
      if (lp_b64_decode_char(data[i]) == -1)
        return 0;
    }
  }
  return 1;
}

#endif