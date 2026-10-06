#
# makefile for testing preemptive multithreading with LCD, Keypad, Button
#

CC = sdcc
CFLAGS = -c --model-small
LDFLAGS = 

# 🚨 1. 這裡把所有硬體的 .rel 檔都加進來了！
LCD_OBJ = dino.rel preemptive.rel lcdlib.rel buttonlib.rel keylib.rel

# 🚨 2. 最終輸出的 hex 檔名改為 dino.hex
all: dino.hex

# 🚨 3. 這裡的規則名稱必須跟 all 後面的一模一樣！
dino.hex: $(LCD_OBJ)
	$(CC) $(LDFLAGS) -o dino.hex $(LCD_OBJ)

clean:
	rm -f *.hex *.ihx *.lnk *.lst *.map *.mem *.rel *.rst *.sym *.lk *.asm

%.rel:  %.c    preemptive.h Makefile
	$(CC) $(CFLAGS) $<