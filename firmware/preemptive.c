#include <8051.h>
#include "preemptive.h"

void __sdcc_dummy_registers(void) __naked 
{
    __asm
    .globl ar0, ar1, ar2, ar3, ar4, ar5, ar6, ar7
    ar0 = 0x00
    ar1 = 0x01
    ar2 = 0x02
    ar3 = 0x03
    ar4 = 0x04
    ar5 = 0x05
    ar6 = 0x06
    ar7 = 0x07
    __endasm;
}

#define MAXTHREADS 3

__data __at (0x7A) volatile char saved_sp[MAXTHREADS];
__data __at (0x7D) volatile ThreadID current_thread;
__data __at (0x7E) volatile char thread_mask;

#define SAVESTATE \
    { \
        __asm__("PUSH ACC \n PUSH B \n PUSH DPL \n PUSH DPH \n PUSH PSW \n" \
                "PUSH 0x00 \n PUSH 0x01 \n PUSH 0x02 \n PUSH 0x03 \n PUSH 0x04 \n PUSH 0x05 \n PUSH 0x06 \n PUSH 0x07 \n" \
                "MOV A, #0x7A \n ADD A, _current_thread \n MOV r0, A \n MOV @r0, _SP"); \
    }

#define RESTORESTATE \
    { \
        __asm__("MOV A, #0x7A \n ADD A, _current_thread \n MOV r0, A \n MOV _SP, @r0 \n" \
                "POP 0x07 \n POP 0x06 \n POP 0x05 \n POP 0x04 \n POP 0x03 \n POP 0x02 \n POP 0x01 \n POP 0x00 \n" \
                "POP PSW \n POP DPH \n POP DPL \n POP B \n POP ACC"); \
    }

extern void main(void);

void Bootstrap(void) __naked
{
    thread_mask = 0;
    SP = 0x76;
    TMOD = 0;   
    IE = 0x82;  
    TR0 = 0;    

    current_thread = ThreadCreate(main);
    RESTORESTATE;
    
    __asm__("SETB _TR0"); 
    __asm__("RET"); 
}

ThreadID ThreadCreate(FunctionPtr fp) 
{
    ThreadID availableThread;
    char tmp_sp;
    char ptr_l, ptr_h, new_psw;
    char saved_EA = EA; 
    
    EA = 0; 

    if (thread_mask == 0x07) // 3 個 Thread 全滿 (0000 0111)
    {
        EA = saved_EA; 
        return -1;
    }

    for (availableThread = 0; availableThread < MAXTHREADS; ++availableThread)
    {
        if (((thread_mask >> availableThread) & 1) == 0) break;
    }
    
    thread_mask |= (1 << availableThread);
    
    ptr_l = (char)((unsigned int)fp & 0xFF);
    ptr_h = (char)(((unsigned int)fp >> 8) & 0xFF);
    new_psw = 0; 
    
    tmp_sp = SP;
    SP = 0x32 + (availableThread * 0x18);
        
    DPL = ptr_l;
    DPH = ptr_h;
    ACC = new_psw;
    
    __asm
        PUSH DPL 
        PUSH DPH 
        MOV B, #0
        PUSH B   
        PUSH B   
        PUSH B   
        PUSH B   
        PUSH ACC 
        PUSH B   
        PUSH B   
        PUSH B   
        PUSH B   
        PUSH B   
        PUSH B   
        PUSH B   
        PUSH B   
    __endasm;
    
    saved_sp[availableThread] = SP;
    SP = tmp_sp;
    EA = saved_EA; 
    
    return availableThread; 
}

void ThreadYield(void) __naked
{
    // 🚨 拯救系統的最關鍵一行：一進來立刻關中斷，絕對禁止被打斷！
    __asm__("CLR _EA"); 
    
    SAVESTATE;
    
    __asm
    Check_Next:
        INC _current_thread
        MOV A, _current_thread
        
        CJNE A, #3, No_Reset
        MOV _current_thread, #0
        MOV A, #0
        
    No_Reset:
        MOV A, _current_thread
        MOV B, A
        MOV A, _thread_mask
        INC B
        
    Shift_Loop:
        RRC A
        DJNZ B, Shift_Loop
        
        JNC Check_Next
    __endasm;
    
    RESTORESTATE;
    __asm
        SETB _EA
        RET
    __endasm;
}

void myTimer0Handler(void) __naked
{
    SAVESTATE;
    
    __asm
    Check_Next_ISR:
        INC _current_thread
        MOV A, _current_thread
        
        CJNE A, #3, No_Reset_ISR
        MOV _current_thread, #0
        MOV A, #0
        
    No_Reset_ISR:
        MOV A, _current_thread
        MOV B, A
        MOV A, _thread_mask
        INC B
        
    Shift_Loop_ISR:
        RRC A
        DJNZ B, Shift_Loop_ISR
        
        JNC Check_Next_ISR
    __endasm;
    
    RESTORESTATE;
    __asm__("RETI");
}

void ThreadExit(void) __naked
{
    __asm__("CLR _EA");
    RESTORESTATE;
    __asm
        SETB _EA
        RET
    __endasm;
}