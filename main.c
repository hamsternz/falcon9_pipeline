#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "pipeline.h"

static char *readArgs[] = {"filename=falcon9-felix.wav"};
static char *interpArgs[] = {"irate=6000", "orate=3572"};
//static char *writeArgs[] = {"filename=out.txt","line_len=255"};

int main(int argc, char *argv[]) {
  unsigned n = 0;
  struct pipeline *p = pipelineNew();
  if(p == NULL) {
     fprintf(stderr,"Unable to create pipeline\n");
     return 0;
  }

  pipelineAdd(p, "read_wave",      1, readArgs);
  pipelineAdd(p, "filter",         0, NULL);
  pipelineAdd(p, "demodulate_fsk", 0, NULL);
  pipelineAdd(p, "atan2",          0, NULL);
  pipelineAdd(p, "interpolate",    2, interpArgs);
  pipelineAdd(p, "quantize",       0, NULL);
  pipelineAdd(p, "ccsds",          0, NULL);
//  pipelineAdd(p, "write_bits",     2, writeArgs);

  while(1) {
     uint8_t buf[255];
     size_t bits = pipelinePull(p, &buf, sizeof(buf));
     if(bits == 0)
       break;
     n+= bits;
  }
  pipelineDelete(p);
  fprintf(stderr, "%u symbols generated\n",n);
  return 0;
}
