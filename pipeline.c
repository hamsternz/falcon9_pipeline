#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define PIPELINE_PRIVATE 1
#include "pipeline.h"

#define MAX_ELEMENTS 20
struct pipeline {
   struct {
      struct module *m;
      void *context;
   } elements[MAX_ELEMENTS];
   int count;
};

size_t pipelineTypeToSize(int type) {
   switch(type) {
      case TYPE_NULL:            return 0;
      case TYPE_BIT:             return sizeof(uint8_t);
      case TYPE_DOUBLE:          return sizeof(double);
      case TYPE_COMPLEX_DOUBLE:  return sizeof(struct complex_double);
   }
   fprintf(stderr,"Unknown type in pipelineTypeToSize()\n");
   exit(1);
}

struct pipeline *pipelineNew(void) {
   struct pipeline *p;
   p = malloc(sizeof(struct pipeline));
   if(p == NULL)
      return NULL;
   p->count = 0;
   return p;
}

static struct module *first_module;

void pipelineRegister(struct module *m) {
   m->next_module = first_module;
   first_module = m;   
}

int pipelineAdd(struct pipeline *p, char *name, int argc, char *argv[]) {
   if(p->count == MAX_ELEMENTS) 
      return 0;
   struct module *m = first_module;
   while(m != NULL && strcmp(name, m->name()) != 0) {
      m = m->next_module;
   }
   if(m == NULL) {
      fprintf(stderr,"Can not find module '%s'\n",name);
      fprintf(stderr,"Modules known are:\n");
      struct module *m = first_module;
      while(m!=NULL) {
         fprintf(stderr,"   %s\n", m->name());
         m = m->next_module;
      }
      exit(0);
      return 0;
   }

   struct module *prev_module = NULL;
   void *prev_context = NULL;

   if(p->count != 0) {
      prev_module = p->elements[p->count-1].m;
      prev_context = p->elements[p->count-1].context;
   }
   p->elements[p->count].context = m->setup(prev_module, prev_context, argc, argv);
   if(p->elements[p->count].context == NULL) 
      return 0;
   p->elements[p->count].m = m;
   p->count++;
   return 1;
}

size_t pipelinePull(struct pipeline *p, void *buffer, size_t size) {
   if(p->count == 0) 
      return 0;
   return p->elements[p->count-1].m->pull(p->elements[p->count-1].context, buffer, size);
}

void pipelineDelete(struct pipeline *p) {
   while(p->count > 0) {
      p->count--;
      p->elements[p->count].m->teardown(p->elements[p->count].context);
      p->elements[p->count].context = NULL;
      p->elements[p->count].m = NULL;
   }
   free(p);
}

int pipelineParameterInt(int paramc, char *paramv[], char *name, int *val) {
   size_t len = strlen(name);
   for(int i = 0; i < paramc; i++) {
      if(strncmp(name,paramv[i],len) == 0 && paramv[i][len] == '=') {
         *val = atoi(paramv[i]+len+1);
         return 1;
      }
   }
   return 0;
}

int pipelineParameterString(int paramc, char *paramv[], char *name, char **val) {
   size_t len = strlen(name);
   for(int i = 0; i < paramc; i++) {
      if(strncmp(name,paramv[i],len) == 0 && paramv[i][len] == '=') {
         *val = paramv[i]+len+1;
         return 1;
      }
   }
   return 0;
}
