/**
    atomic.c - Atomic operations

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*********************************** Includes *********************************/

#include    "mpr.h"

/************************************ Code ************************************/

PUBLIC void mprAtomicBarrier()
{
    #ifdef VX_MEM_BARRIER_RW
        VX_MEM_BARRIER_RW();
    #elif MACOSX
        OSMemoryBarrier();
    #elif BIT_WIN_LIKE
        MemoryBarrier();
    #elif BIT_HAS_SYNC
        __sync_synchronize();
    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_X86 || BIT_CPU_ARCH == BIT_CPU_X64)
        asm volatile ("mfence" : : : "memory");

    #elif __GNUC__ && (BIT_CPU_ARCH == BIT_CPU_PPC)
        asm volatile ("sync" : : : "memory");
    #else
        getpid();
    #endif

#if FUTURE && KEEP
    asm volatile ("lock; add %eax,0");
#endif
}


/*
    Atomic compare and swap a pointer with a full memory barrier
 */
PUBLIC int mprAtomicCas(void * volatile *addr, void *expected, cvoid *value)
{
    #if MACOSX
        return OSAtomicCompareAndSwapPtrBarrier(expected, (void*) value, (void*) addr);
    #elif BIT_WIN_LIKE
        {
            void *prev;
            prev = InterlockedCompareExchangePointer(addr, (void*) value, expected);
            return expected == prev;
        }
    #elif BIT_HAS_SYNC_CAS
        return __sync_bool_compare_and_swap(addr, expected, value);
    #elif VXWORKS && _VX_ATOMIC_INIT && !BIT_64
        /* vxCas operates with integer values */
        return vxCas((atomic_t*) addr, (atomicVal_t) expected, (atomicVal_t) value);
    #elif BIT_CPU_ARCH == BIT_CPU_X86
        {
            void *prev;
            asm volatile ("lock; cmpxchgl %2, %1"
                : "=a" (prev), "=m" (*addr)
                : "r" (value), "m" (*addr), "0" (expected));
            return expected == prev;
        }
    #elif BIT_CPU_ARCH == BIT_CPU_X64
        {
            void *prev;
            asm volatile ("lock; cmpxchgq %q2, %1"
                : "=a" (prev), "=m" (*addr)
                : "r" (value), "m" (*addr),
                  "0" (expected));
            return expected == prev;
        }
    #else
        //  MOB - can do better than this
        mprGlobalLock();
        if (*addr == expected) {
            *addr = (void*) value;
            mprGlobalUnlock();
            return 1;
        }
        mprGlobalUnlock();
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
    #elif VXWORKS && _VX_ATOMIC_INIT
        vxAtomicAdd(ptr, value);

//  MOB - need this enabled
//  MOB - need GCC versions

    //  MOB - what about arm
    //  MOB - complete this
    #elif (BIT_CPU_ARCH == BIT_CPU_X86 || BIT_CPU_ARCH == BIT_CPU_X64) && FUTURE
        asm volatile ("lock; xaddl %0,%1"
            : "=r" (value), "=m" (*ptr)
            : "0" (value), "m" (*ptr)
            : "memory", "cc");
    #else
        mprGlobalLock();
        *ptr += value;
        mprGlobalUnlock();
    #endif
}


/*
    On some platforms, this operation is only atomic with respect to other calls to mprAtomicAdd64
 */
PUBLIC void mprAtomicAdd64(volatile int64 *ptr, int value)
{
#if MACOSX
    OSAtomicAdd64(value, ptr);
#elif BIT_WIN_LIKE && BIT_64
    InterlockedExchangeAdd64(ptr, value);

    //  MOB - complete this
    //  MOB - what about vxworks
#elif BIT_UNIX_LIKE && FUTURE
    asm volatile ("lock; xaddl %0,%1"
        : "=r" (value), "=m" (*ptr)
        : "0" (value), "m" (*ptr)
        : "memory", "cc");
#else
    //  MOB - need better than this
    mprGlobalLock();
    *ptr += value;
    mprGlobalUnlock();
#endif
}


PUBLIC void *mprAtomicExchange(void * volatile *addr, cvoid *value)
{
#if MACOSX
    void *old = *(void**) addr;
    OSAtomicCompareAndSwapPtrBarrier(old, (void*) value, addr);
    return old;
#elif BIT_WIN_LIKE
    return (void*) InterlockedExchange((volatile LONG*) addr, (LONG) value);
#elif BIT_HAS_SYNC
    return __sync_lock_test_and_set(addr, (void*) value);
#else
    {
        void    *old;
        mprGlobalLock();
        old = * (void**) addr;
        *addr = (void*) value;
        mprGlobalUnlock();
        return old;
    }
#endif
}


/*
    Atomic list insertion. Inserts "item" at the "head" of the list. The "link" field is the next field in item.
 */
PUBLIC void mprAtomicListInsert(void * volatile *head, volatile void **link, void *item)
{
    do {
        *link = *head;
    } while (mprAtomicCas(head, (void*) *link, item));
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
