
#define TYPE_NULL             0x0000
#define TYPE_BIT              0x0001
#define TYPE_DOUBLE           0x0010
#define TYPE_COMPLEX_DOUBLE   0x1000

struct complex_double {
  double r;
  double i;
};

#ifdef PIPELINE_PRIVATE
struct module {
  struct module *next_module;
  int (*inputType)(void);
  int (*outputType)(void);
  void *(*setup)(struct module *prior, void *context, int paramc, char *paramv[]);
  size_t (*pull)(void *data, void *buffer, size_t size);
  int (*teardown)(void *data);
  char *(*name)(void);
};
void pipelineRegister(struct module *m);
#endif

int pipelineParameterInt(int paramc, char *paramv[], char *name, int *val);
int pipelineParameterString(int paramc, char *paramv[], char *name, char **val);

size_t pipelineTypeToSize(int type);
struct pipeline *pipelineNew(void);
size_t pipelinePull(struct pipeline *p, void *buf, size_t size);
int pipelineAdd(struct pipeline *p, char *name, int paramc, char *paramv[]);
void pipelineDelete(struct pipeline *p);
