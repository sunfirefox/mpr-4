/**
    atomic.c - Atomic operations

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*********************************** Includes *********************************/

#include    "mpr.h"

/*********************************** Local ************************************/

static MprSpin  atomicSpinLock;
static MprSpin  *atomicSpin = &atomicSpinLock;

/************************************ Code ************************************/

PUBLIC void mprAtomicOpen()
{
    mprInitSpinLock(atomicSpin);
}


/*
    Full memory barrier
 */
PUBLIC void mprAtomicBarrier()
{
    #if MACOSX
        OSMemoryBarrier();

    #elif defined(VX_MEM_BARRIER_RW)
        VX_MEM_BARRIER_RW();

    #elif BIT_WIN_LIKE
        MemoryBarrier();

    #elif BIT_HAS_ATOMIC
        __atomic_thread_fence(__ATOMIC_SEQ_CST);

    #elif BIT_HAS_SYNC
        __sync_synchronize();

    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_X86 || BIT_CPU_ARCH == BIT_CPU_X64)
        asm volatile ("mfence" : : : "memory");

    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_PPC)
        asm volatile ("sync" : : : "memory");

    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_ARM) && FUTURE
        asm volatile ("isync" : : : "memory");

    #else
        if (mprTrySpinLock(atomicSpin)) {
            mprSpinUnlock(atomicSpin);
        }
    #endif
}


/*
    Atomic compare and swap a pointer with a full memory barrier
 */
PUBLIC int mprAtomicCas(void * volatile *addr, void *expected, cvoid *value)
{
    #if MACOSX
        return OSAtomicCompareAndSwapPtrBarrier(expected, (void*) value, (void*) addr);

    #elif VXWORKS && _VX_ATOMIC_INIT && !BIT_64
        /* vxCas operates with integer values */
        return vxCas((atomic_t*) addr, (atomicVal_t) expected, (atomicVal_t) value);

    #elif BIT_WIN_LIKE
        return InterlockedCompareExchangePointer(addr, (void*) value, expected) == expected;

    #elif BIT_HAS_ATOMIC
        void *localExpected = expected;
        return __atomic_compare_exchange(addr, &localExpected, (void**) &value, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

    #elif BIT_HAS_SYNC_CAS
        return __sync_bool_compare_and_swap(addr, expected, value);

    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_X86)
        void *prev;
        asm volatile ("lock; cmpxchgl %2, %1"
            : "=a" (prev), "=m" (*addr)
            : "r" (value), "m" (*addr), "0" (expected));
        return expected == prev;

    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_X64)
        void *prev;
        asm volatile ("lock; cmpxchgq %q2, %1"
            : "=a" (prev), "=m" (*addr)
            : "r" (value), "m" (*addr),
              "0" (expected));
            return expected == prev;

    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_ARM) && FUTURE

    #else
        mprSpinLock(atomicSpin);
        if (*addr == expected) {
            *addr = (void*) value;
            mprSpinUnlock(atomicSpin);
            return 1;
        }
        mprSpinUnlock(atomicSpin);
        return 0;
    #endif
}


/*
    Atomic add of a signed value. Used for add, subtract, inc, dec
 */
PUBLIC void mprAtomicAdd(volatile int *ptr, int value)
{
    #if MACOSX
        OSAtomicAdd32(value, ptr);

    #elif BIT_WIN_LIKE
        InterlockedExchangeAdd(ptr, value);

    #elif BIT_HAS_ATOMIC
        //  OPT - could use __ATOMIC_RELAXED
        __atomic_add_fetch(ptr, value, __ATOMIC_SEQ_CST);

    #elif BIT_HAS_SYNC_CAS
        __sync_add_and_fetch(ptr, value);

    #elif VXWORKS && _VX_ATOMIC_INIT
        vxAtomicAdd(ptr, value);

    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_X86 || BIT_CPU_ARCH == BIT_CPU_X64)
        asm volatile("lock; addl %1,%0"
             : "+m" (*ptr)
             : "ir" (value));
    #else
        mprSpinLock(atomicSpin);
        *ptr += value;
        mprSpinUnlock(atomicSpin);

    #endif
}


/*
    On some platforms, this operation is only atomic with respect to other calls to mprAtomicAdd64
 */
PUBLIC void mprAtomicAdd64(volatile int64 *ptr, int64 value)
{
    #if MACOSX
        OSAtomicAdd64(value, ptr);
    
    #elif BIT_WIN_LIKE && BIT_64
        InterlockedExchangeAdd64(ptr, value);
    
    #elif BIT_HAS_ATOMIC
        //  OPT - could use __ATOMIC_RELAXED
        __atomic_add_fetch(ptr, value, __ATOMIC_SEQ_CST);

    #elif BIT_HAS_SYNC_CAS
        __sync_add_and_fetch(ptr, value);

    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_X86)
        asm volatile ("lock; xaddl %0,%1"
            : "=r" (value), "=m" (*ptr)
            : "0" (value), "m" (*ptr)
            : "memory", "cc");
    
    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_X64)
        asm volatile("lock; addq %1,%0"
             : "=m" (*ptr)
             : "er" (value), "m" (*ptr));

    #else
        mprSpinLock(atomicSpin);
        *ptr += value;
        mprSpinUnlock(atomicSpin);
    
    #endif
}


#if UNUSED
PUBLIC void *mprAtomicExchange(void *volatile *addr, cvoid *value)
{
    #if MACOSX
        void *old = *(void**) addr;
        OSAtomicCompareAndSwapPtrBarrier(old, (void*) value, addr);
        return old;
    
    #elif BIT_WIN_LIKE
        return (void*) InterlockedExchange((volatile LONG*) addr, (LONG) value);
    
    #elif BIT_HAS_ATOMIC
        __atomic_exchange_n(addr, value, __ATOMIC_SEQ_CST);

    #elif BIT_HAS_SYNC
        return __sync_lock_test_and_set(addr, (void*) value);
    
    #else
        void    *old;
        mprSpinLock(atomicSpin);
        old = *(void**) addr;
        *addr = (void*) value;
        mprSpinUnlock(atomicSpin);
        return old;
    #endif
}
#endif


/*
    Atomic list insertion. Inserts "item" at the "head" of the list. The "link" field is the next field in item.
 */
PUBLIC void mprAtomicListInsert(void **head, void **link, void *item)
{
    do {
        *link = *head;
    } while (!mprAtomicCas(head, (void*) *link, item));
}


/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2013. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
