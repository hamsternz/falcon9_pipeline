#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#define PIPELINE_PRIVATE 1
#include "pipeline.h"

struct quant_context {
   struct module *prev;
   void *prev_context; 
   double d[256];
   int debug;
};

static char *quant_name(void) {
   return "quantize";
}

static int quant_input_type(void) {
   return TYPE_DOUBLE;
}

static int quant_output_type(void) {
   return TYPE_BIT;
}

static void *quant_setup(struct module *prev, void *prev_context, int argc, char *argv[]) {
   struct quant_context *c = malloc(sizeof(struct quant_context));
   if(prev == NULL) {
      fprintf(stderr, "Filter must have a source!\n");
      return NULL;
   }

   if(prev->outputType() != TYPE_DOUBLE) {
      fprintf(stderr, "Filter only accepts double\n");
      return NULL;
   }

   if(c == NULL) {
      return NULL;
   }
   c->prev = prev;
   c->prev_context = prev_context;
   c->debug = 0;
   fprintf(stderr, "quantize:\n");
   fprintf(stderr, "  No parameters\n");
   return c;
}

static size_t quant_pull(void *context, void *buf, size_t size) {
   struct quant_context *c = context;
   size_t eaten = 0;
   uint8_t *o = buf;
   
   while(size > 0) {
      size_t to_pull = size/sizeof(uint8_t)*sizeof(double);
      if(to_pull > sizeof(c->d))
        to_pull = sizeof(c->d);
      size_t pulled = c->prev->pull(c->prev_context, c->d, to_pull);
      if(pulled == 0) {
         if(c->debug)
            fprintf(stderr,"quant end of data\n");
         break;
      }

      int i;
      for(i = 0; i < pulled/sizeof(double); i++) {
         o[i] = (c->d[i] > 0) ? 1 : 0;
      }
      o     += i;
      size  -= i;
      eaten += i;
   }
   return eaten;
}

static int quant_teardown(void *v_d) {
   struct quant_context *c = v_d;
   free(c);
   return 1;
}

static struct module quant_module = {
  .setup      = quant_setup,
  .inputType  = quant_input_type,
  .outputType = quant_output_type,
  .pull       = quant_pull,
  .teardown   = quant_teardown,
  .name       = quant_name
};

__attribute__((constructor)) static void before_main(void) {
  pipelineRegister(&quant_module);
}
