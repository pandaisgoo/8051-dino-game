#include <8051.h>
#include "preemptive.h"
#include "keylib.h"
#include "lcdlib.h"

__data __at (0x08) volatile char buffer[3];
__data __at (0x0B) volatile unsigned char head;      
__data __at (0x0C) volatile unsigned char tail;      
__data __at (0x0D) volatile char mutex;     
__data __at (0x0E) volatile char full;      
__data __at (0x0F) volatile char empty;     

__data __at (0x10) volatile unsigned char state;
__data __at (0x11) volatile unsigned char difficulty;
__data __at (0x12) volatile unsigned char dino_row;   
__data __at (0x13) volatile char render_flag; 
__data __at (0x14) volatile unsigned int score;      
__data __at (0x16) volatile unsigned int map[2];     

__data __at (0x1A) volatile char kp_current_key;
__data __at (0x1B) volatile char kp_last_key;       

__data __at (0x1D) volatile char gc_key;
__data __at (0x1E) volatile unsigned char gc_shift_timer;
__data __at (0x1F) volatile unsigned char gc_spawn_timer;

__data __at (0x20) volatile unsigned char rt_i;
__data __at (0x21) volatile unsigned int rt_local_map0; 
__data __at (0x23) volatile unsigned int rt_local_map1; 
__data __at (0x25) volatile unsigned int rt_mask;     

__code const char dinosaur[] = {0x07, 0x05, 0x06, 0x07, 0x14, 0x17, 0x0E, 0x0A};
__code const char cactus[]   = {0x04, 0x05, 0x15, 0x15, 0x16, 0x0C, 0x04, 0x04};

__code const unsigned int bit_mask[16] = {
    0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000
};

void LCD_set_symbol(char index, const char symb[]) {
    LCD_setCgRamAddress(index * 8); 
    for(rt_i=0; rt_i<8; rt_i++) { LCD_write_char(symb[rt_i]); }
}


void Keypad_Ctrl(void) {
    kp_last_key = '\0'; 
    
    while(1) {
        kp_current_key = KeyToChar();
        
        if (kp_current_key != '\0' && kp_last_key == '\0') {
            SemaphoreWait(empty); 
            SemaphoreWait(mutex);
            
            buffer[tail] = kp_current_key;
            if (tail == 2) tail = 0; else tail++;
            
            SemaphoreSignal(mutex); 
            SemaphoreSignal(full);
        }
        
        kp_last_key = kp_current_key;
        ThreadYield();                
    }
}


void Game_Ctrl(void) {
    while(1) {
        EA = 0;
        if (full > 0) {
            full--; EA = 1;
            SemaphoreWait(mutex);
            gc_key = buffer[head];
            if (head == 2) head = 0; else head++;
            SemaphoreSignal(mutex); SemaphoreSignal(empty);
        } else { EA = 1; gc_key = '\0'; }

        if (gc_key != '\0') {
            if (state == 0) { 
                if (gc_key >= '0' && gc_key <= '9') {
                    difficulty = gc_key - '0'; render_flag |= 0x01; 
                } else if (gc_key == '#') { 
                    state = 1; render_flag |= 0x01; 
                }
            }
            else if (state == 1) { 
                if (gc_key == '2') { dino_row = 0; render_flag |= 0x02; }
                else if (gc_key == '8') { dino_row = 1; render_flag |= 0x02; }
            }
            gc_key = '\0'; // 確保訊號消耗
        }

        if (state == 1) {
            gc_shift_timer++;
            if (gc_shift_timer > (10 - difficulty) * 15) { 
                gc_shift_timer = 0; gc_spawn_timer++;
                
                // 碰撞檢測
                if (map[dino_row] & 1) state = 2; 
                else if ((map[0] & 1) || (map[1] & 1)) score++;

                EA = 0; map[0] >>= 1; map[1] >>= 1; EA = 1; 
                
                if (gc_spawn_timer > 3) { 
                    gc_spawn_timer = 0;
                    if (score % 2 == 0) map[1] |= 0x8000; else map[0] |= 0x8000; 
                }
                if (map[dino_row] & 1) state = 2; 
                
                render_flag |= 0x01; // 地圖刷新需求
            }
        }
        ThreadYield(); 
    }
}


void Render_Task(void) {
    LCD_set_symbol(1, dinosaur); 
    LCD_set_symbol(2, cactus);
    
    while(1) {
        EA = 0;
        char flag = render_flag;
        render_flag = 0; 
        EA = 1;
        
        if (flag != 0) {
            // 確保 LCD 準備就緒
            while (!LCD_ready()) { ThreadYield(); }
            
            // 處理全螢幕刷新 (flag == 1)
            if (flag & 0x01) { 
                if (state == 0) {
                    LCD_cursorGoTo(0,0); LCD_write_string("Level(0-9)+#/*:"); LCD_write_char(difficulty + '0');
                    LCD_cursorGoTo(1,0); LCD_write_string("                "); 
                } 
                else if (state == 1) {
                    // 清除背景 (使用空格清空)
                    LCD_cursorGoTo(0, 0); LCD_write_string("                ");
                    LCD_cursorGoTo(1, 0); LCD_write_string("                ");
                    
                    EA = 0; 
                    unsigned int m0 = map[0]; 
                    unsigned int m1 = map[1]; 
                    unsigned char row = dino_row;
                    EA = 1;
                    
                    // 繪製恐龍
                    LCD_cursorGoTo(row, 0); 
                    LCD_write_char(1);

                    // 繪製仙人掌
                    for (rt_i = 1; rt_i < 16; rt_i++) {
                        if (m0 & bit_mask[rt_i]) { LCD_cursorGoTo(0, rt_i); LCD_write_char(2); } 
                        if (m1 & bit_mask[rt_i]) { LCD_cursorGoTo(1, rt_i); LCD_write_char(2); }
                    }
                } 
                else if (state == 2) {
                    // 第一行印出 Game Over
                    LCD_cursorGoTo(0, 0); 
                    LCD_write_string("   Game Over!   ");
                    
                    // 第二行印出 Score
                    LCD_cursorGoTo(1, 0); 
                    LCD_write_string("Score: ");
                    
                    EA = 0; 
                    unsigned int final_score = score; 
                    EA = 1;

                    LCD_write_char((final_score / 100) % 10 + '0'); // 百位數
                    LCD_write_char((final_score / 10) % 10 + '0');  // 十位數
                    LCD_write_char((final_score % 10) + '0');       // 個位數
                    
                    LCD_write_string("      ");
                }
            } 
            else if (flag & 0x02) { 
                LCD_cursorGoTo(0, 0); LCD_write_char((dino_row == 0) ? 1 : ' ');
                LCD_cursorGoTo(1, 0); LCD_write_char((dino_row == 1) ? 1 : ' ');
            }
        }
        ThreadYield(); 
    }
}


void main(void) {
    Init_Keypad(); LCD_Init();
    head = 0; tail = 0;
    SemaphoreCreate(mutex, 1); SemaphoreCreate(full, 0); SemaphoreCreate(empty, 3);
    
    gc_key = '\0'; gc_shift_timer = 0; gc_spawn_timer = 0;
    state = 0; difficulty = 0; score = 0; dino_row = 1;
    map[0] = 0; map[1] = 0; render_flag = 1; 

    ThreadCreate(Keypad_Ctrl); 
    ThreadCreate(Game_Ctrl); 
    Render_Task();
}

void _sdcc_gsinit_startup(void) { __asm LJMP _Bootstrap __endasm; }
void _mcs51_genRAMCLEAR(void) {} void _mcs51_genXINIT(void) {} void _mcs51_genXRAMCLEAR(void) {}
void timer0_ISR(void) __interrupt(1) __naked { __asm ljmp _myTimer0Handler __endasm; }