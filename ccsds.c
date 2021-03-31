#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#define PIPELINE_PRIVATE 1
#include "pipeline.h"

const uint32_t CADU_ASM = 0x1ACFFC1D;
const uint32_t CADU_ASM_INV = 0xE53003E2;

struct ccsds_context {
   char d[512];
   struct module *prev;
   void *prev_context; 
   int debug;
   uint32_t u32;
   uint32_t count_of_packet;
   size_t n;
   size_t last_asm;
   uint8_t synced;
   uint8_t inverted;
   uint8_t scramble[120];
   char buffer[1024];
};

static void fill_scramble(struct ccsds_context *c) {
   uint8_t state = 0xFF;
   for(int i = 0; i < sizeof(c->scramble); i++) {
      uint8_t x = 0;
      for(int b = 0; b < 8; b++) {
         uint8_t bit = 0;
         x = (x<<1) | (state&1); 
        // x = (x>>1) | ((state&1)<<7);
         bit = (state >> 7) ^ (state >> 5) ^ (state >> 3) ^ (state >> 0);
         bit &= 1;
         state = (state>>1)|(bit ? 0x80: 0);
      }
      c->scramble[i] = x;
   }
   printf("\n");
}

static char *ccsds_name(void) {
   return "ccsds";
}

static int ccsds_input_type(void) {
   return TYPE_DOUBLE;
}

static int ccsds_output_type(void) {
   return TYPE_BIT;
}

static void *ccsds_setup(struct module *prev, void *prev_context, int paramc, char *paramv[]) {
   if(prev == NULL) {
      fprintf(stderr, "Filter must have a source!\n");
      return NULL;
   }

   if(prev->outputType() != TYPE_BIT) {
      fprintf(stderr, "Filter only accepts bits\n");
      return NULL;
   }

   struct ccsds_context *c = malloc(sizeof(struct ccsds_context));
   if(c == NULL) {
      fprintf(stderr, "Out of memeory\n");
      return NULL;
   }
   c->prev = prev;
   c->prev_context = prev_context;
   c->debug = 0;
   c->n = 0;
   c->last_asm = 0;
   c->buffer[0] = 0;
   fill_scramble(c);
   fprintf(stderr, "ccsds:\n");
   fprintf(stderr, "  No parameters\n");
   return c;
}

static size_t ccsds_pull(void *context, void *buf, size_t size) {
   struct ccsds_context *c = context;
   size_t eaten = 0;
   
   while(size > 0) {
      size_t to_pull = size;
      if(to_pull > sizeof(c->d))
        to_pull = sizeof(c->d);
      size_t pulled = c->prev->pull(c->prev_context, c->d, to_pull);
      if(pulled == 0) {
         if(c->debug)
            fprintf(stderr,"write_bits end of data\n");
         break;
      }

      int i;
      for(i = 0; i < pulled; i++) {
         c->u32 = (c->u32 << 1) | c->d[i];
         if(c->synced) {
            c->count_of_packet++;
            if(c->count_of_packet %8 == 0) {
               if(c->count_of_packet <= sizeof(c->scramble)*8) {
                  uint8_t y = (c->u32&0xFF) ^ (c->inverted ? 0xFF : 0);
                  y ^= c->scramble[(c->count_of_packet>>3)-1];
                  uint8_t x = 0;
                  if(y&0x01) x |= 0x80; 
                  if(y&0x02) x |= 0x40; 
                  if(y&0x04) x |= 0x20; 
                  if(y&0x08) x |= 0x10; 
                  if(y&0x10) x |= 0x08; 
                  if(y&0x20) x |= 0x04; 
                  if(y&0x40) x |= 0x02; 
                  if(y&0x80) x |= 0x01; 
                  x = y;

                  if(c->count_of_packet < 41*8)
                    sprintf(c->buffer+strlen(c->buffer), " %02X", x);

                  if(x < 32 || x > 127) x = '.';
                     putchar(x);
               } else {
                  c->synced = 0;
                  putchar('\n');
                  printf("                               %s\n",c->buffer);
                  c->buffer[0] = 0;
               }
            }
         } 
         if(c->u32 == CADU_ASM) {
           printf("%08X %10zi  (%6zi:%zi) ",c->u32, c->n, (c->n-c->last_asm)/8, (c->n-c->last_asm)%8);
           c->last_asm        = c->n;
           c->synced          = 1;
           c->inverted        = 0;
           c->count_of_packet = 0;
         } else if(c->u32 == CADU_ASM_INV) {
           printf("%08X %10zi  (%6zi:%zi) ",c->u32, c->n, (c->n-c->last_asm)/8, (c->n-c->last_asm)%8);
           c->last_asm        = c->n;
           c->synced          = 1;
           c->inverted        = 1;
           c->count_of_packet = 0;
         }
         c->n++;
      }
      size  -= i;
      eaten += i;
   }
   return eaten;
}

static int ccsds_teardown(void *v_d) {
   struct ccsds_context *c = v_d;
   free(c);
   return 1;
}

static struct module ccsds_module = {
  .setup      = ccsds_setup,
  .inputType  = ccsds_input_type,
  .outputType = ccsds_output_type,
  .pull       = ccsds_pull,
  .teardown   = ccsds_teardown,
  .name       = ccsds_name
};

__attribute__((constructor)) static void before_main(void) {
  pipelineRegister(&ccsds_module);
}
