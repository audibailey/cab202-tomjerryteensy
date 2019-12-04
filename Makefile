#set this
TARGETS=  \
	TomJerryTeensy.hex \

all: $(TARGETS)

#set this
# Set the name of the folder containing libcab202_teensy.a
CAB202_TEENSY_FOLDER = cab202_teensy

# Set the name of the folder containing usb_serial.o
USB_SERIAL_FOLDER = usb_serial
USB_SERIAL_OBJ = usb_serial/usb_serial.o

# Set the name of the folder containing uart.o
ADC_FOLDER = cab202_adc
ADC_OBJ = cab202_adc/cab202_adc.o

# do not modify below this line             ----------------
TEENSY_LIBS = $(USB_SERIAL_OBJ) $(ADC_OBJ) -lcab202_teensy -lprintf_flt -lm

TEENSY_DIRS =-I$(CAB202_TEENSY_FOLDER) -L$(CAB202_TEENSY_FOLDER) \
	-I$(USB_SERIAL_FOLDER) -I$(ADC_FOLDER)

TEENSY_FLAGS = \
	-std=gnu99 \
	-mmcu=atmega32u4 \
	-DF_CPU=8000000UL \
	-Wl,-u,vfprintf \
	-Wl,-gc-sections \
	-funsigned-char \
	-funsigned-bitfields \
	-ffunction-sections \
	-fpack-struct \
	-fshort-enums \
	-Wall \
	-Wno-strict-aliasing \
	-lprintf_flt \
	-Os

clean:
	rm html.pch; \
	rm -f *.csrc; \
	rm -f *.hsrc; \
	for f in $(TARGETS); do \
		if [ -f $$f ]; then rm $$f; fi; \
		if [ -f $$f.exe ]; then rm $$f.exe; fi; \
		if [ -f $$f.elf ]; then rm $$f.elf; fi; \
		if [ -f $$f.obj ]; then rm $$f.obj; fi; \
		if [ -f $$f.hex ]; then rm $$f.hex; fi; \
	done

rebuild: clean all


%.hex : %.c
	avr-gcc $< $(TEENSY_FLAGS) $(TEENSY_DIRS) $(TEENSY_LIBS) -o $@.obj
	avr-objcopy -O ihex $@.obj $@

