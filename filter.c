#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#define PIPELINE_PRIVATE 1
#include "pipeline.h"

#define BUFFER_ENTRIES 1024
double x_kernel[] = { 
-0.0000537,
 0.0000931,
-0.0000000,
-0.0002125,
 0.0002975,
-0.0000000,
-0.0005323,
 0.0006884,
-0.0000000,
-0.0010961,
 0.0013556,
-0.0000000,
-0.0020089,
 0.0024126,
-0.0000000,
-0.0034034,
 0.0040038,
-0.0000000,
-0.0054548,
 0.0063249,
-0.0000000,
-0.0084171,
 0.0096722,
-0.0000000,
-0.0127151,
 0.0145691,
-0.0000000,
-0.0191968,
 0.0221333,
-0.0000000,
-0.0299666,
 0.0353963,
-0.0000000,
-0.0522236,
 0.0665693,
-0.0000000,
-0.1366458,
 0.2750696,
 0.6666667,
 0.2750696,
-0.1366458,
-0.0000000,
 0.0665693,
-0.0522236,
-0.0000000,
 0.0353963,
-0.0299666,
-0.0000000,
 0.0221333,
-0.0191968,
-0.0000000,
 0.0145691,
-0.0127151,
-0.0000000,
 0.0096722,
-0.0084171,
-0.0000000,
 0.0063249,
-0.0054548,
-0.0000000,
 0.0040038,
-0.0034034,
-0.0000000,
 0.0024126,
-0.0020089,
-0.0000000,
 0.0013556,
-0.0010961,
-0.0000000,
 0.0006884,
-0.0005323,
-0.0000000,
 0.0002975,
-0.0002125,
-0.0000000,
 0.0000931,
-0.0000537
};
#define X_K_SIZE (sizeof(x_kernel)/sizeof(x_kernel[0]))

struct filter_context {
   struct module *prev;
   void *prev_context; 

   int data_consumed;
   int data_filled;
   int data_size;
   struct complex_double *data;

   int kernel_size;
   double *kernel;
   unsigned debug;
};

static char *filter_name(void) {
   return "filter";
}

static int filter_input_type(void) {
   return TYPE_COMPLEX_DOUBLE;
}

static int filter_output_type(void) {
   return TYPE_COMPLEX_DOUBLE;
}

static void *filter_setup(struct module *prev, void *prev_context, int argc, char *argv[]) {
   struct filter_context *c = malloc(sizeof(struct filter_context));
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
   c->prev         = prev;
   c->prev_context = prev_context;

   c->data_size    = BUFFER_ENTRIES;
   c->data = malloc(sizeof(struct complex_double)*c->data_size);
   if(c->data == NULL) {
      free(c);
      return NULL;
   }
   c->data_filled      = 0;
   c->data_consumed    = 0;

   c->kernel_size  = X_K_SIZE;
   c->kernel = malloc(sizeof(double)*c->kernel_size);
   if(c->data == NULL) {
      free(c);
      free(c->data);
      return NULL;
   }
   memcpy(c->kernel, x_kernel, sizeof(double) * c->kernel_size);
   c->debug = 0;
   return c;
}

static size_t filter_pull(void *context, void *buf, size_t size) {
   struct filter_context *c = context;
   size_t eaten = 0;
   struct complex_double *o = buf;

   while(size >= sizeof(struct complex_double)) {
      // Do we need compact the buffer? 
      if(c->data_consumed > 0 && c->data_consumed + c->kernel_size > c->data_filled) {    
         // Push all the data to the front
         for(int i = c->data_consumed; i < c->data_filled; i++) {
            c->data[i-c->data_consumed].r = c->data[i].r; 
            c->data[i-c->data_consumed].i = c->data[i].i; 
         }
         c->data_filled -= c->data_consumed;
         c->data_consumed = 0;
      }
   
      // Do we need to read in some more data?
      if(c->data_filled < c->kernel_size) {
         // Attempt to fill the buffer
         while(c->data_filled < c->data_size) {
            int filled = c->prev->pull(c->prev_context, c->data+c->data_filled, sizeof(struct complex_double)*(c->data_size - c->data_filled));
            if(filled == 0)
               break;
            c->data_filled += filled/sizeof(struct complex_double);
         }
         if(c->data_filled < c->kernel_size) {
            if(c->debug)
               printf("filter end of data\n");
            break;
         }
      }
      // Now run the filter as many times as needed with the data we have in the buffer
      while(size >= sizeof(struct complex_double) && c->data_consumed+c->kernel_size <= c->data_filled) {
         o->r = o->i  = 0.0;
         for(int i = 0; i < c->kernel_size; i++) {
            o->r += c->data[i+c->data_consumed].r * c->kernel[i];
            o->i += c->data[i+c->data_consumed].i * c->kernel[i];
         }
         o++;
         c->data_consumed++;
         eaten += sizeof(struct complex_double);
         size  -= sizeof(struct complex_double);
      }
   }
   return eaten;
}

static int filter_teardown(void *v_d) {
   struct filter_context *c = v_d;
   free(c->data);
   free(c->kernel);
   free(c);
   return 1;
}

static struct module filter_module = {
  .setup      = filter_setup,
  .inputType  = filter_input_type,
  .outputType = filter_output_type,
  .pull       = filter_pull,
  .teardown   = filter_teardown,
  .name       = filter_name
};

__attribute__((constructor)) static void before_main(void) {
  pipelineRegister(&filter_module);
}
