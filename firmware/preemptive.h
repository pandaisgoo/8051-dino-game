#ifndef __PREEMPTIVE_H__
#define __PREEMPTIVE_H__

#define MAXTHREADS 3

typedef char ThreadID;
typedef void (*FunctionPtr)(void);

ThreadID ThreadCreate(FunctionPtr);
void ThreadYield(void);
void ThreadExit(void);


#define CNAME(s) _ ## s

#define CONCAT(a, b) a ## b
#define CONCAT_EXPAND(a, b) CONCAT(a, b)

#define SemaphoreCreate(s, n) s = n;

#define SemaphoreSignal(s) \
    __asm \
        INC CNAME(s) \
    __endasm;

#define SemaphoreWait(s) \
    while (1) { \
        EA = 0; \
        if (s > 0) { \
            s--; \
            EA = 1; \
            break; \
        } \
        EA = 1; \
        ThreadYield(); \
    }

#endif // __PREEMPTIVE_H__