#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#define PIPELINE_PRIVATE 1
#include "pipeline.h"

struct write_bits_context {
   char d[512];
   struct module *prev;
   void *prev_context; 
   FILE *f;
   int debug;
   unsigned n;
   unsigned line_len;
};

static char *write_bits_name(void) {
   return "write_bits";
}

static int write_bits_input_type(void) {
   return TYPE_DOUBLE;
}

static int write_bits_output_type(void) {
   return TYPE_BIT;
}

static void *write_bits_setup(struct module *prev, void *prev_context, int paramc, char *paramv[]) {
   char *fname;
   FILE *f;
   int line_len = 80;

   pipelineParameterString(paramc, paramv, "filename", &fname);
   pipelineParameterInt(paramc, paramv, "line_len", &line_len);
   if(fname == NULL) {
      fprintf(stderr,"Must supply a filename parameter\n");
      return 0;
   }

   if(prev == NULL) {
      fprintf(stderr, "Filter must have a source!\n");
      return NULL;
   }

   if(prev->outputType() != TYPE_BIT) {
      fprintf(stderr, "Filter only accepts bits\n");
      return NULL;
   }
   f = fopen(fname,"w");
   if(f == NULL) {
      fprintf(stderr, "Unable to open output file\n");
      return NULL;
   }

   struct write_bits_context *c = malloc(sizeof(struct write_bits_context));
   if(c == NULL) {
      fprintf(stderr, "Out of memeory\n");
      return NULL;
   }
   c->prev = prev;
   c->prev_context = prev_context;
   c->debug = 0;
   c->n = 0;
   c->f = f;
   c->line_len = line_len;
   fprintf(stderr, "write_bits:\n");
   fprintf(stderr, "  filename=%s\n",fname);
   fprintf(stderr, "  line_len=%d\n",c->line_len);
   return c;
}

static size_t write_bits_pull(void *context, void *buf, size_t size) {
   struct write_bits_context *c = context;
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
         if(c->d[i])
            putc('1', c->f);
         else
            putc('0', c->f);

         c->n++;
         if(c->n == c->line_len) {
            putc('\n', c->f);
            c->n = 0;
         }
          
      }
      size  -= i;
      eaten += i;
   }
   return eaten;
}

static int write_bits_teardown(void *v_d) {
   struct write_bits_context *c = v_d;
   fclose(c->f);
   free(c);
   return 1;
}

static struct module write_bits_module = {
  .setup      = write_bits_setup,
  .inputType  = write_bits_input_type,
  .outputType = write_bits_output_type,
  .pull       = write_bits_pull,
  .teardown   = write_bits_teardown,
  .name       = write_bits_name
};

__attribute__((constructor)) static void before_main(void) {
  pipelineRegister(&write_bits_module);
}
