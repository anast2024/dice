/*
 * Copyright (C) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: 0BSD
 */
#include <assert.h>

#include <dice/chains/sequence.h>
#include <dice/events/memaccess.h>
#include <dice/events/thread.h>
#include <dice/log.h>
#include <dice/module.h>
#include <dice/self.h>
#include <dice/switcher.h>

DICE_MODULE_INIT()

static caslock_t lock_;
static uint64_t clock_;
static vatomic32_t exclusive_;

static void
_sequence(type_id type, void *event, struct plan *plan)
{
    if (self_retired(plan->self))
        return;

    if (!(plan->chain == CAPTURE_AFTER && plan->type == EVENT_THREAD_CREATE))
        assert(vatomic_read(&exclusive_) == 1);
    PS_PUBLISH(SEQUENCE_DECIDE, type, event, &plan->_);
    if (!(plan->chain == CAPTURE_AFTER && plan->type == EVENT_THREAD_CREATE))
        assert(vatomic_read(&exclusive_) == 1);
    if (plan->chain == CAPTURE_BEFORE && plan->type == EVENT_THREAD_CREATE) {
        assert(vatomic_dec_get(&exclusive_) == 0);
    }
    if (plan->wake) {
        assert(vatomic_dec_get(&exclusive_) == 0);
        switcher_wake(plan->next, 0);
    }
    if (plan->yield) {
        switcher_yield(self_id(plan->self), true);
        PS_PUBLISH(SEQUENCE_RESUME, type, event, &plan->_);

        assert(caslock_tryacquire(&lock_));
        clock_++;
        log_printf("CLOCK: %lu\n", clock_);
        caslock_release(&lock_);
        assert(vatomic_inc_get(&exclusive_) == 1);
    }
}


PS_SUBSCRIBE(CAPTURE_EVENT, ANY_TYPE, {
    struct plan plan;
    plan._     = (struct metadata){0};
    plan.self  = md;
    plan.chain = chain;
    plan.type  = type;
    plan.next  = ANY_THREAD;

    switch (type) {
        case EVENT_THREAD_EXIT:
            plan.wake  = true;
            plan.yield = false;
            // stop sequencing thread after this point until SELF_FINI
            // TODO: this must be reviewed
            break;
        case EVENT_SELF_INIT:
            // threads call this only ONCE except the main thread
            // plan.wake  = false;
            // if the thread creating another thread does not wake anybody, then
            // it is our task to wake another thread. The contract here should
            // be: nobody can take the thread ID of the newly created thread. To
            // make that deterministic, we need to enforce that OR we would need
            // to pass the thread ID from the parent. That would however require
            // the "self" component to be known already in the interceptor,
            // which is super ugly.
            assert(vatomic_inc_get(&exclusive_) == 1);
            plan.wake  = self_id(md) != MAIN_THREAD;
            plan.yield = self_id(md) != MAIN_THREAD;
            break;
        default:
            plan.wake  = true;
            plan.yield = true;
            break;
    }
    _sequence(type, event, &plan);
})

PS_SUBSCRIBE(CAPTURE_BEFORE, ANY_TYPE, {
    struct plan plan;
    plan._     = (struct metadata){0};
    plan.self  = md;
    plan.chain = chain;
    plan.type  = type;
    plan.next  = ANY_THREAD;

    // here we wake up a thread even if we are going to create another thread,
    // is this correct? If we do that, there might be multiple thread create at
    // the same time. I think this might break something. On replay, newly
    // created threads could have incorrect thread ids!!
    // plan.wake = true;
    plan.wake = type != EVENT_THREAD_CREATE;

    // either atomic memaccess or call. If memaccess, we should yield, otherwise
    // the yield happens after the call.
    plan.yield = is_memaccess(type);

    _sequence(type, event, &plan);
})

PS_SUBSCRIBE(CAPTURE_AFTER, ANY_TYPE, {
    struct plan plan;
    plan._     = (struct metadata){0};
    plan.self  = md;
    plan.chain = chain;
    plan.type  = type;
    plan.next  = ANY_THREAD;

    // in principle, we dont wake anybody here.
    plan.wake = false;
    // if we did a call, previously, then we should yield. No matter what call
    // that was (even THREAD_CREATE). We are assuming all calls are potentially
    // blocking.
    plan.yield = !is_memaccess(type);

    _sequence(type, event, &plan);
})
