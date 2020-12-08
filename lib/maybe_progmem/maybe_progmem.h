#ifndef MAYBE_PROGMEM_H
#define MAYBE_PROGMEM_H

#ifdef WINDOWS
#define PROGMEM 
#define PROGMEM_GET_BYTE(array, index) array[index]
#define PROGMEM_GET_FLOAT(array, index) array[index]
#else
#include <Arduino.h>
#define PROGMEM_GET_BYTE(array, index) pgm_read_byte(array + index)
#define PROGMEM_GET_FLOAT(array, index) pgm_read_float(array + index)
#endif

#endif