#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#define PIPELINE_PRIVATE 1
#include "pipeline.h"

#include "interpolator_taps.h"


#define DATA_SIZE 1024
struct interpolate_context {
   struct module *prev;
   void *prev_context; 
   double data[DATA_SIZE];
   int data_consumed;
   int data_filled;
   int data_size;
   unsigned b;
   unsigned even;
   unsigned last;
   unsigned lastlast;
   unsigned in_rate;
   unsigned out_rate;
   unsigned interpolate_rate;
   unsigned debug;
};


static char *interpolate_name(void) {
   return "interpolate";
}

static int interpolate_input_type(void) {
   return TYPE_DOUBLE;
}

static int interpolate_output_type(void) {
   return TYPE_DOUBLE;
}

static void *interpolate_setup(struct module *prev, void *prev_context, int paramc, char *paramv[]) {
   struct interpolate_context *c = malloc(sizeof(struct interpolate_context));
   int irate = 0, orate = 0;
   if(prev == NULL) {
      fprintf(stderr, "Filter must have a source!\n");
      return NULL;
   }

   pipelineParameterInt(paramc, paramv, "irate", &irate);
   pipelineParameterInt(paramc, paramv, "orate", &orate);

   if(irate == 0 || orate == 0) {
     return NULL;
   }

   if(prev->outputType() != TYPE_DOUBLE) {
      fprintf(stderr, "Filter only accepts double\n");
      return NULL;
   }

   if(c == NULL) {
      return NULL;
   }
   c->data_filled   = 0;
   c->data_consumed = 0;
   c->data_size     = DATA_SIZE;
   c->prev = prev;
   c->prev_context = prev_context;
   c->b = 0;
   c->even = 0;
   c->last = 0;
   c->lastlast = 0;

   c->in_rate     = irate;
   c->out_rate    = orate;
   c->interpolate_rate = orate*2;
   fprintf(stderr, "interpolate:\n");
   fprintf(stderr, "  irate=%i\n",irate);
   fprintf(stderr, "  orate=%i\n",orate);
   c->debug = 0;
   return c;
}

static size_t interpolate_pull(void *context, void *buf, size_t size) {
   struct interpolate_context *c = context;
   size_t eaten = 0;
   double *o = buf;

   while(size >= sizeof(double)) {
      // Do we need to compact the buffer?
      if(c->data_consumed > 0 && c->data_consumed+I_NTAPS > c->data_filled) {
         for(int i = c->data_consumed; i < c->data_filled; i++) {
            c->data[i-c->data_consumed] = c->data[i];
         }
         c->data_filled -= c->data_consumed;
         c->data_consumed = 0;
      }

      // Fill the buffer if needed
      if(c->data_filled < I_NTAPS) {
         while(c->data_filled < c->data_size) {
            size_t f = c->prev->pull(c->prev_context, c->data+c->data_filled, sizeof(double)*(c->data_size-c->data_filled));
            if(f == 0) {
                break;
            }
            c->data_filled += f/sizeof(double);
         }
         if(c->data_filled < I_NTAPS) {
            if(c->debug)
               printf("interp end of data\n");
            break;
         }
      }

      // Are we in a position to output a value
      if(c->b < c->interpolate_rate) {
         // Work out which filter to use
         int step = I_NSTEPS-I_NSTEPS*c->b/c->interpolate_rate;

         // Apply the filter      
         double total = 0;
         for(int i = 0; i < I_NTAPS; i++) {
            total += i_taps[step][i] * c->data[c->data_consumed+i];
         }
         // Is this a bit we keep?
         int out = total > 0 ? 1 : 0;
         if(c->even) {
            *o  = total;
            o++;
            size -= sizeof(double);
            eaten += sizeof(double);

            // Work out if we need to adjust phase at all
            if(c->lastlast != out) {
               if(c->last == out) {
 
                 if(c->b > 0) {
                     c->b-=25;
                 }
               } else {
                 if(c->b < c->interpolate_rate-1) {
                     c->b+=25;
                 }
               }
            }
         }
         c->lastlast = c->last;
         c->last     = out;
         c->even     = !c->even;
         c->b       += c->in_rate;
      } else {
         c->data_consumed++;
         c->b -= c->interpolate_rate;
      }
   }
   return eaten;
}

static int interpolate_teardown(void *v_d) {
   struct interpolate_context *c = v_d;
   free(c);
   return 1;
}

static struct module interpolate_module = {
  .setup      = interpolate_setup,
  .inputType  = interpolate_input_type,
  .outputType = interpolate_output_type,
  .pull       = interpolate_pull,
  .teardown   = interpolate_teardown,
  .name       = interpolate_name
};

__attribute__((constructor)) static void before_main(void) {
  pipelineRegister(&interpolate_module);
}
