/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
#ifndef DICE_SEQUENCE_CHAIN_H
#define DICE_SEQUENCE_CHAIN_H
#include <stdbool.h>

#include <dice/pubsub.h>
#include <dice/self.h>

#define SEQUENCE_PREPARE 7
#define SEQUENCE_RESUME  8

struct plan {
    metadata_t _;
    metadata_t *self;
    chain_id chain;
    type_id type;
    thread_id next;
    bool wake;
    bool yield;
};

#endif /* DICE_SEQUENCE_CHAIN_H */
