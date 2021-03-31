#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#define PIPELINE_PRIVATE 1
#include "pipeline.h"

struct demod_context {
   struct module *prev;
   void *prev_context;
   struct complex_double last;
   int last_used;
   unsigned debug;
};

static char *demod_name(void) {
   return "demodulate_fsk";
}

static int demod_input_type(void) {
   return TYPE_COMPLEX_DOUBLE;
}

static int demod_output_type(void) {
   return TYPE_COMPLEX_DOUBLE;
}

static void *demod_setup(struct module *prev, void *prev_context, int argc, char *argv[]) {
   struct demod_context *c = malloc(sizeof(struct demod_context));
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
   c->prev = prev;
   c->prev_context = prev_context;
   c->last.r = 0.0;
   c->last.i = 0.0;
   c->last_used = 0;
   c->debug = 0;
   return c;
}

static size_t demod_pull(void *context, void *buf, size_t size) {
   struct demod_context *c = context;
   size_t eaten = 0;
   struct complex_double *o = buf;

   // Pull the last used
   if(c->last_used == 0) {
      if(c->prev->pull(c->prev_context, &c->last, sizeof(c->last)) == 0) {
         fprintf(stderr,"demond end of data\n");;
         return 0;
      }
      c->last_used = 1;
   } 

   while(size >= sizeof(double)) {
      struct complex_double d;
      if(c->prev->pull(c->prev_context, &d, sizeof(struct complex_double)) == 0) {
         if(c->debug)
            fprintf(stderr,"demond end of data\n");;
         break;
      }
      o->r =  d.r*c->last.r - d.i*c->last.i;
      o->i =  d.r*c->last.i + d.i*c->last.r;
      c->last.r = d.r;
      c->last.i = -d.i;

      eaten += sizeof(struct complex_double);
      size  -= sizeof(struct complex_double);
      o++;
   }
   return eaten;
}

static int demod_teardown(void *v_d) {
   struct filter_context *c = v_d;
   free(c);
   return 1;
}

static struct module demod_module = {
  .setup      = demod_setup,
  .inputType  = demod_input_type,
  .outputType = demod_output_type,
  .pull       = demod_pull,
  .teardown   = demod_teardown,
  .name       = demod_name
};

__attribute__((constructor)) static void before_main(void) {
  pipelineRegister(&demod_module);
}
