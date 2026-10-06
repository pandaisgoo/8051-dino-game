#include <8051.h>
#include "preemptive.h"
#include "buttonlib.h"
#include "keylib.h"
#include "lcdlib.h"

__data __at (0x08) volatile char buffer[3];
__data __at (0x0B) volatile unsigned char head;      
__data __at (0x0C) volatile unsigned char tail;      
__data __at (0x0D) volatile char mutex;     
__data __at (0x0E) volatile char full;      
__data __at (0x0F) volatile char empty;     

void Producer1(void)
{
    char current_btn;
    char last_btn = '\0';

    while (1)
    {
        current_btn = ButtonToChar();
        
        if (current_btn != '\0' && last_btn == '\0') 
        {
            SemaphoreWait(empty);
            SemaphoreWait(mutex);
            
            buffer[tail] = current_btn;
            if (tail == 2) tail = 0;
            else tail++;
            
            SemaphoreSignal(mutex);
            SemaphoreSignal(full);
        }
        
        last_btn = current_btn;
        ThreadYield(); 
    }
}

void Producer2(void)
{
    char current_key;
    char last_key = '\0';

    while (1)
    {
        current_key = KeyToChar();

        if (current_key != '\0' && last_key == '\0') 
        {
            SemaphoreWait(empty);
            SemaphoreWait(mutex);
            
            buffer[tail] = current_key;
            if (tail == 2) tail = 0;
            else tail++;
            
            SemaphoreSignal(mutex);
            SemaphoreSignal(full);
        }
        
        last_key = current_key;
        ThreadYield(); 
    }
}

void Consumer(void)
{
    char c;
    while (1)
    {
        SemaphoreWait(full);
        SemaphoreWait(mutex);
        
        c = buffer[head];
        if (head == 2) head = 0;
        else head++;
        
        SemaphoreSignal(mutex);
        SemaphoreSignal(empty);

        while (!LCD_ready()) {
            ThreadYield();
        }
        LCD_write_char(c);
    }
}

void main(void)
{
    Init_Keypad();
    LCD_Init();
    
    head = 0;
    tail = 0;
    SemaphoreCreate(mutex, 1);
    SemaphoreCreate(full, 0);
    SemaphoreCreate(empty, 3);
    
    ThreadCreate(Producer1);
    ThreadCreate(Producer2);
    Consumer();
}

void _sdcc_gsinit_startup(void)
{
    __asm
        LJMP _Bootstrap
    __endasm;
}

void _mcs51_genRAMCLEAR(void) {}
void _mcs51_genXINIT(void) {}
void _mcs51_genXRAMCLEAR(void) {}

void timer0_ISR(void) __interrupt(1) __naked {
        __asm
                ljmp _myTimer0Handler
        __endasm;
}