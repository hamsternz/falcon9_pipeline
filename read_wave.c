#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <assert.h>

#define PIPELINE_PRIVATE 1
#include "pipeline.h"

#define DATA_SIZE 1000000

////////////////////////////////////////////////////////////////////////
struct read_wave_data {
  unsigned char buffer[8192];
  int consumedBytes;
  int filledBytes;  
  unsigned sampleRate;
  unsigned debug;
  FILE *f;
};

struct WaveFileHeader {
  uint8_t  chunkID[4];
  uint8_t  chunkSize[4];
  uint8_t  chunkFormat[4];

  uint8_t  subchunk1ID[4];
  uint8_t  subchunk1Size[4];
  uint8_t  audioFormat[2];
  uint8_t  numChannels[2];
  uint8_t  sampleRate[4];
  uint8_t  byteRate[4];
  uint8_t  blockAlign[2];
  uint8_t  bitsPerSample[2];

  uint8_t  subchunk2ID[4];
  uint8_t  subchunk2Size[4];
};


static int u4ToInt(uint8_t *d) {
  return d[0] | (d[1]<<8) | (d[2]<<16) | (d[3]<<24);
}

static int u2ToInt(uint8_t *d) {
  return d[0] | (d[1]<<8);
}
#if 0
static void intToU4(uint8_t *d, int val) {
   d[0] = val;
   d[1] = val>>8;
   d[2] = val>>16;
   d[3] = val>>24;
}

static void intToU2(uint8_t *d, int val) {
   d[0] = val&0xFF;
   d[1] = val>>8;
}
#endif

static int read_header(FILE *f, unsigned *sampleRate) {
   struct WaveFileHeader header;
   if(fread(&header, sizeof(header), 1, f) != 1) {
      printf("Unable to read header\n");
      return 0;
   }
   fprintf(stderr, "    Format %d\n",          u2ToInt(header.audioFormat));
   fprintf(stderr, "    Channels %d\n",        u2ToInt(header.numChannels));
   fprintf(stderr, "    Sample rate %d\n",     u4ToInt(header.sampleRate));
   fprintf(stderr, "    Bits per sample %d\n", u2ToInt(header.bitsPerSample));
   if(u2ToInt(header.bitsPerSample) != 16 || u2ToInt(header.numChannels) != 2) {
      fprintf(stderr, "Unsuppoted WAV file format");
      return 0;
   }
   *sampleRate = u2ToInt(header.bitsPerSample);
   return 1;
}


static int read_sample(FILE *f, int16_t samples[2]) {
   uint8_t o[4];

   if(fread(o, sizeof(o), 1, f) != 1) {
     return 0;
   }
   samples[0] = (o[1]<<8)| o[0];
   samples[1] = (o[3]<<8)| o[2];
   
   return 1;
}

static void *read_wave_setup(struct module *prev, void *prev_context, int paramc, char *paramv[]) {
   char *fname = NULL;
   if(prev != NULL) {
      fprintf(stderr,"Wave reader can not have a source");
      return 0;
   }

   pipelineParameterString(paramc, paramv, "filename", &fname);

   if(fname == NULL) {
      fprintf(stderr,"Must supply a filename parameter\n");
      return 0;
   }

   struct read_wave_data *data = malloc(sizeof(struct read_wave_data));
   if(data == NULL) return NULL;
   data->debug = 0;
   data->f = fopen(fname,"r");
   if(data->f == NULL) {
     free(data);
     fprintf(stderr,"Unable to open '%s'\n",fname);
     return 0;
   } 
   data->filledBytes = 0;
   data->consumedBytes = 0;


   fprintf(stderr, "read_wave:\n");
   fprintf(stderr, "  filename=%s\n",fname);
   if(!read_header(data->f, &(data->sampleRate))) {
      fclose(data->f);
      free(data);
   }
#if 0
#if 0
   fseek(data->f, 1000*1000*1000, SEEK_CUR);
   fseek(data->f, 1000*1000*1000, SEEK_CUR);
   fseek(data->f, 1000*1000*1000, SEEK_CUR);
   fseek(data->f, 1000*1000*1000, SEEK_CUR);
   fseek(data->f, 1000*1000*1000, SEEK_CUR);
   fseek(data->f, 1000*1000*1000, SEEK_CUR);
#else
   fseek(data->f, 200*1024*1024, SEEK_SET);
#endif
#else
//   fseek(data->f, 2000* 1000*1000, SEEK_SET);
//   fseek(data->f, 1090* 1000*1000, SEEK_CUR);
#endif
   return data;
}

static char *read_wave_name(void) {
   return "read_wave"; 
}

static int read_wave_input_type(void) {
   return TYPE_NULL; 
}

static int read_wave_output_type(void) {
   return TYPE_COMPLEX_DOUBLE; 
}

static size_t read_wave_pull(void *v_d, void *buff, size_t size) {
   assert(size % sizeof(struct complex_double) == 0);
   struct read_wave_data *d = v_d;
   size_t taken = 0;

   while(size >= sizeof(struct complex_double) && !feof(d->f)) {
      int16_t samples[2];
      if(!read_sample(d->f, samples)) {
         if(d->debug)
            fprintf(stderr,"read wave end of data\n");
         break;
      }
      struct complex_double *entry = buff;
      entry->r = samples[0]/1000.0;
      entry->i = samples[1]/1000.0;
      buff = ((unsigned char *)buff)+sizeof(struct complex_double);
      size -= sizeof(struct complex_double);
      taken += sizeof(struct complex_double);
   }
   return taken;
}

static int read_wave_teardown(void *v_data) {
   struct read_wave_data *data = v_data;
   if(v_data == NULL) {
      return 0;
   }
   if(data->f != NULL)
      fclose(data->f);
   free(data);
   return 1;
}

static struct module read_wave_module = {
  .setup      = read_wave_setup,
  .inputType  = read_wave_input_type,
  .outputType = read_wave_output_type,
  .pull       = read_wave_pull,
  .teardown   = read_wave_teardown,
  .name       = read_wave_name
};

__attribute__((constructor)) static void before_main(void) {
  pipelineRegister(&read_wave_module);
}

