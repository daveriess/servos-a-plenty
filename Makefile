#      device
MCU  = atmega168

#      file names
MAIN = servos-a-plenty
RESULT = servos-a-plenty

#	result of Make is servos-a-plenty.hex
all: $(RESULT).hex

CC   = avr-gcc
COPY = avr-objcopy
PROG = avrdude

#  compile servos-a-plenty.c into servos-a-plenty.o
$(MAIN).o: $(MAIN).c
	$(CC) -c -g -O3 -Wall -mmcu=$(MCU) -I. $(MAIN).c -o $(MAIN).o

#  create "executable and linkable format" file; link servos-a-plenty.o
#  add other .o and .o dependencies here
$(RESULT).elf: $(MAIN).o
	$(CC) $(MAIN).o -Wl,-Map=$(MAIN).map,--cref -mmcu=$(MCU) -o $(RESULT).elf

#  create hex file from elf file
$(RESULT).hex: $(RESULT).elf
	$(COPY) -O ihex $(RESULT).elf $(RESULT).hex 

#  make clean
clean:
	rm -f *.o *.hex *.elf *.map *~
