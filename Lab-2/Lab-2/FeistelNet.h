/*
 *  FeistelNet.h
 *  FeistelNetEcryption
 *
 *  Created by Evgeniy Morozov on 03.03.17.
 *  Copyright © 2017 Evgeniy Morozov. All rights reserved.
 */

#ifndef FeistelNet_h
#define FeistelNet_h

#include <stdint.h>
#include <limits.h>

#include <stdlib.h>

#define BITS(x) (sizeof(x) * CHAR_BIT)

#define ROL(x, y) (uint16_t)(((x) << ((y) % BITS(x))) | ((x) >> (BITS(x) - (y % BITS(x))) % BITS(x)))
#define ROR(x, y) (uint16_t)(((x) >> ((y) % BITS(x))) | ((x) << (BITS(x) - (y % BITS(x))) % BITS(x)))

typedef struct feist_block_s feist_block_t;
typedef struct feist_ctx_s   feist_ctx_t;

typedef uint16_t (*feist_keygen_cb)(uint16_t, size_t);
typedef uint16_t (*feist_round_fn_cb)(uint16_t, uint16_t, uint16_t, uint16_t);

feist_ctx_t * feist_init(size_t rounds, uint16_t init_key,
                         feist_keygen_cb key_generator_cb,
                         feist_round_fn_cb round_fn_cb);

char * feist_encrypt(feist_ctx_t * ctx, const char message[],
                     size_t * message_len);
char * feist_decrypt(feist_ctx_t * ctx, const char message[],
                     size_t * message_len);

#endif /* FeistelNet_h */
