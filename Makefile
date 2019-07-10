# Uses https://github.com/sudar/Arduino-Makefile. Make sure it's installed

# Fix this to be itsy bitsy M0
BOARD_TAG ?= uno

ARDUINO_DIR ?= "/home/arlo/code/arduino-1.8.8"
ARDUINO_LIB_PATH ?= "/home/arlo/Arduino/libraries"
ARDMK_DIR ?= "/usr/share/arduino"
AVR_TOOLS_DIR ?= "/home/arlo/code/arduino-1.8.8/hardware/tools/avr"

ARDUINO_LIBS = Adafruit_GFX_Library Adafruit_SHARP_Memory_Display RTClib

include $(ARDMK_DIR)/Arduino.mk

