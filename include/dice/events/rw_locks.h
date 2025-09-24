#ifndef DICE_RW_LOCKS_H
#define DICE_RW_LOCKS_H

#include <pthread.h>

#include <dice/events/thread.h>

#define EVENT_ANNOTATE_RW_LOCK_CREATE    42
#define EVENT_ANNOTATE_RW_LOCK_DESTROY   43
#define EVENT_ANNOTATE_RW_LOCK_ACQ       44
#define EVENT_ANNOTATE_RW_LOCK_REL       45

#define EVENT_ANNOTATERWLOCKCREATE EVENT_ANNOTATE_RW_LOCK_CREATE
#define EVENT_ANNOTATERWLOCKDESTROY EVENT_ANNOTATE_RW_LOCK_DESTROY
#define EVENT_ANNOTATERWLOCKACQUIRED EVENT_ANNOTATE_RW_LOCK_ACQ
#define EVENT_ANNOTATERWLOCKRELEASED EVENT_ANNOTATE_RW_LOCK_REL

struct AnnotateRWLockCreate_event{
    const char *file;
    int line;
    const volatile void *lock;
    int ret;
};

struct annotate_rwlock_destroy_event{
    const char *file;
    int line;
    const volatile void *lock;
    int ret;
};

struct annotate_rwlock_acquire_event{
    const char *file;
    int line;
    const volatile void *lock;
    long is_w;
    int ret;
};

struct annotate_rwlock_release_event{
    const char *file;
    int line;
    const volatile void *lock;
    long is_w;
    int ret;
};

#endif /* DICE_RW_LOCKS_H */