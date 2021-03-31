#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#define PIPELINE_PRIVATE 1
#include "pipeline.h"

struct atan2_context {
   struct module *prev;
   void *prev_context;
   struct complex_double d[128];
   unsigned debug;
};

static char *atan2_name(void) {
   return "atan2";
}

static int atan2_input_type(void) {
   return TYPE_COMPLEX_DOUBLE;
}

static int atan2_output_type(void) {
   return TYPE_DOUBLE;
}

static void *atan2_setup(struct module *prev, void *prev_context, int argc, char *argv[]) {
   struct atan2_context *c = malloc(sizeof(struct atan2_context));
   if(prev == NULL) {
      fprintf(stderr, "Filter must have a source!\n");
      return NULL;
   }

   if(prev->outputType() != TYPE_COMPLEX_DOUBLE) {
      fprintf(stderr, "Filter only accepts complex double\n");
      return NULL;
   }

   if(c == NULL) {
      return NULL;
   }
   fprintf(stderr, "atan2:\n");
   fprintf(stderr, "  No parameters\n");
   c->prev = prev;
   c->prev_context = prev_context;
   c->debug = 0;
   return c;
}

static size_t atan2_pull(void *context, void *buf, size_t size) {
   struct atan2_context *c = context;
   size_t eaten = 0;

   while(size >= sizeof(double)) {
      size_t to_pull = size/sizeof(double)*sizeof(struct complex_double);

      if(to_pull > sizeof(c->d)) {
         to_pull = sizeof(c->d);  
      }

      size_t pulled = c->prev->pull(c->prev_context, c->d, to_pull);
      if(pulled == 0) {
         if(c->debug)
            fprintf(stderr,"atan2 end of data\n");
         break;
      }

      double *o = buf;
      int i;
      for(i = 0; i < pulled/sizeof(struct complex_double); i++) {
         o[i] = atan2(c->d[i].i, c->d[i].r);
      }
      buf    = ((unsigned char *)buf) + sizeof(double)*i;  
      eaten += sizeof(double)*i;
      size  -= sizeof(double)*i;
   }
   return eaten;
}

static int atan2_teardown(void *v_d) {
   struct atan2_context *c = v_d;
   free(c);
   return 1;
}

static struct module atan2_module = {
  .setup      = atan2_setup,
  .inputType  = atan2_input_type,
  .outputType = atan2_output_type,
  .pull       = atan2_pull,
  .teardown   = atan2_teardown,
  .name       = atan2_name
};

__attribute__((constructor)) static void before_main(void) {
  pipelineRegister(&atan2_module);
}
