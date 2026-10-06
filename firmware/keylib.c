#include <8051.h>
void Init_Keypad(void) {
	P3_3 = 1; // input mode from AND gate.
	P0 = 0xf0; // configure column 3 bits (top) as input,
}

char AnyKeyPressed(void) {
	P0 = 0xf0;  // set all rows to pull-down
	return !P3_3; // true if any button is connected to pull-down
}

char KeyToChar(void) {
    P0 = 0xFF; 

    P0 = 0xf7; // test the top row
    if (P0 == 0xb7) { return '1'; }
    if (P0 == 0xd7) { return '2'; }
    if (P0 == 0xe7) { return '3'; }

    P0 = 0xFF; 
    P0 = 0xfb; 
    if (P0 == 0xbb) { return '4';}
    if (P0 == 0xdb) { return '5'; }
    if (P0 == 0xeb) { return '6'; }

    P0 = 0xFF; 
    P0 = 0xfd;  
    if (P0 == 0xbd) { return '7';}
    if (P0 == 0xdd) { return '8'; }
    if (P0 == 0xed) { return '9'; }

    P0 = 0xFF; 
    P0 = 0xfe;  // test the last row ('0' 在這裡)
    if (P0 == 0xbe) { return '*'; }
    if (P0 == 0xde) { return '0'; }
    if (P0 == 0xee) { return '#'; }

    return '\0'; // 沒按鍵時回傳 Null 字元
}
