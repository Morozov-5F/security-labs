/*
 *  FeistelNet.c
 *  FeistelNetEcryption
 *
 *  Created by Evgeniy Morozov on 03.03.17.
 *  Copyright © 2017 Evgeniy Morozov. All rights reserved.
 */

#include "FeistelNet.h"

#include <stdio.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#endif

struct feist_block_s
{
    uint16_t fnb_sub_1;
    uint16_t fnb_sub_2;
    uint16_t fnb_sub_3;
    uint16_t fnb_sub_4;
};

struct feist_ctx_s
{
    feist_block_t     fnc_cbc_sequence;
    uint16_t        * fnc_keys;
    feist_keygen_cb   fnc_key_generator;
    feist_round_fn_cb fnc_round_fn;
    size_t            fnc_rounds;
};

static
uint16_t s_feist_default_keygen(uint16_t start_key, size_t round)
{
    return ROR(start_key, round * 3);
}

static
uint16_t s_feist_default_round_fn(uint16_t x1, uint16_t x2,
                                  uint16_t x3, uint16_t k)
{
    uint16_t ret = 0;
    ret = ROL(x1, 9) ^ (~(ROL(k, 11) ^ x2)) ^ x3;
    return ret;
}

feist_ctx_t * feist_init(size_t rounds, uint16_t init_key,
                         feist_keygen_cb key_generator_cb,
                         feist_round_fn_cb round_fn_cb)
{
    feist_ctx_t * context = NULL;
    size_t i = 0;
    
    context = calloc(sizeof(feist_ctx_t), 1);
    assert(NULL != context);
    
    context->fnc_rounds = rounds;
    
    context->fnc_keys = calloc(sizeof(uint16_t), rounds);
    assert(NULL != context->fnc_keys);
    
    context->fnc_keys[0] = init_key;
    
    if (NULL == key_generator_cb)
    {
        key_generator_cb = s_feist_default_keygen;
    }
    context->fnc_key_generator = key_generator_cb;
    /* Generate keys for each round */
    for (i = 1; i < rounds; ++i)
    {
        context->fnc_keys[i] =
                            context->fnc_key_generator(context->fnc_keys[0], i);
    }

    if (NULL == round_fn_cb)
    {
        round_fn_cb = s_feist_default_round_fn;
    }
    context->fnc_round_fn = round_fn_cb;
    /* Generate random CBC sequence */
    context->fnc_cbc_sequence.fnb_sub_1 = rand();
    context->fnc_cbc_sequence.fnb_sub_2 = rand();
    context->fnc_cbc_sequence.fnb_sub_3 = rand();
    context->fnc_cbc_sequence.fnb_sub_4 = rand();
    
    return context;
}

char * feist_encrypt(feist_ctx_t * ctx, const char message[],
                     size_t * message_len)
{
    feist_block_t * encr_message = NULL;
    size_t i = 0;
    size_t j = 0;
    size_t new_message_len = 0;
    size_t blocks = 0;
    feist_block_t to_xor;
    
    assert(NULL != ctx);
    assert(NULL != message);
    assert(NULL != message_len);
    
    new_message_len = (*message_len / 8 + !!(*message_len % 8 != 0)) * 8;
    blocks = new_message_len / 8;
    printf("[INFO] Message length: %lu, blocks: %lu\n", new_message_len, blocks);
    
    encr_message = calloc(blocks, sizeof(feist_block_t));
    memcpy((uint8_t *)encr_message, message, new_message_len);
    
    to_xor = ctx->fnc_cbc_sequence;

    for (j = 0; j < blocks; ++j)
    {
        feist_block_t current_block;
        
        encr_message[j].fnb_sub_1 ^= to_xor.fnb_sub_1;
        encr_message[j].fnb_sub_2 ^= to_xor.fnb_sub_2;
        encr_message[j].fnb_sub_3 ^= to_xor.fnb_sub_3;
        encr_message[j].fnb_sub_4 ^= to_xor.fnb_sub_4;
        
        for (i = 0; i < ctx->fnc_rounds; ++i)
        {
            current_block = encr_message[j];
            encr_message[j].fnb_sub_1 = current_block.fnb_sub_2;
            encr_message[j].fnb_sub_2 = current_block.fnb_sub_3;
            encr_message[j].fnb_sub_3 = current_block.fnb_sub_4 ^ ctx->fnc_round_fn(current_block.fnb_sub_1, current_block.fnb_sub_2, current_block.fnb_sub_3, ctx->fnc_keys[i]);
            encr_message[j].fnb_sub_4 = current_block.fnb_sub_1;
        }
        
        current_block = encr_message[j];
        encr_message[j].fnb_sub_1 = current_block.fnb_sub_4;
        encr_message[j].fnb_sub_2 = current_block.fnb_sub_1;
        encr_message[j].fnb_sub_3 = current_block.fnb_sub_2;
        encr_message[j].fnb_sub_4 = current_block.fnb_sub_3;
        
        to_xor = encr_message[j];
    }
    
    *message_len = new_message_len;
    return (char *)encr_message;
}

char * feist_decrypt(feist_ctx_t * ctx, const char message[],
                     size_t * message_len)
{
    feist_block_t * encr_message = NULL;
    size_t i = 0;
    size_t j = 0;
    size_t new_message_len = 0;
    size_t blocks = 0;
    feist_block_t to_xor;
    feist_block_t to_xor_prev;
    
    assert(NULL != ctx);
    assert(NULL != message);
    assert(NULL != message_len);
    
    new_message_len = (*message_len / 8 + !!(*message_len % 8 != 0)) * 8;
    blocks = new_message_len / 8;
    printf("[INFO] Message length: %lu, blocks: %lu\n", new_message_len, blocks);
    
    encr_message = calloc(blocks, sizeof(feist_block_t));
    memcpy((uint8_t *)encr_message, message, new_message_len);
    
    to_xor = ctx->fnc_cbc_sequence;
    for (j = 0; j < blocks; ++j)
    {
        feist_block_t current_block;
        
        to_xor_prev = encr_message[j];
        
        for (i = ctx->fnc_rounds; i--> 0;)
        {
            current_block = encr_message[j];
            encr_message[j].fnb_sub_1 = current_block.fnb_sub_4 ^ ctx->fnc_round_fn(current_block.fnb_sub_1, current_block.fnb_sub_2, current_block.fnb_sub_3, ctx->fnc_keys[i]);
            encr_message[j].fnb_sub_2 = current_block.fnb_sub_1;
            encr_message[j].fnb_sub_3 = current_block.fnb_sub_2;
            encr_message[j].fnb_sub_4 = current_block.fnb_sub_3;
        }
        
        current_block = encr_message[j];
        
        encr_message[j].fnb_sub_1 = current_block.fnb_sub_2 ^ to_xor.fnb_sub_1;
        encr_message[j].fnb_sub_2 = current_block.fnb_sub_3 ^ to_xor.fnb_sub_2;
        encr_message[j].fnb_sub_3 = current_block.fnb_sub_4 ^ to_xor.fnb_sub_3;
        encr_message[j].fnb_sub_4 = current_block.fnb_sub_1 ^ to_xor.fnb_sub_4;
        
        to_xor = to_xor_prev;
    }
    
    *message_len = new_message_len;
    return (char *)encr_message;
}

