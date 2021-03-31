pipeline: pipeline.c read_wave.c pipeline.h filter.c atan2.c demodulate_fsk.c interpolate.c quantize.c main.c write_bits.c ccsds.c
	gcc -o pipeline *.c -Wall -pedantic -lm -O3
