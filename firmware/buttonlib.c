#include <8051.h>
#include "buttonlib.h"


// returns true if any button is pressed. false if no button pressed.
char AnyButtonPressed(void) {
    // 只要 P2 不是 0xFF (11111111)，就代表至少有一個 bit 被拉低 (被按下了)
    return (P2 != 0xFF); 
}


char ButtonToChar(void) {
    char pressed = ~P2; 

    if (pressed & 0x80) {
        return '7';
    } else if (pressed & 0x40) {
        return '6';
    } else if (pressed & 0x20) {
        return '5';
    } else if (pressed & 0x10) {
        return '4';
    } else if (pressed & 0x08) {
        return '3';
    } else if (pressed & 0x04) {
        return '2';
    } else if (pressed & 0x02) {
        return '1';
    } else if (pressed & 0x01) {
        return '0';
    }
    
    return '\0'; 
}