#ifdef __cplusplus
extern "C" {
#endif
struct oplsnd;
struct oplsnd *oplsnd_init(void);
int oplsnd_open(struct oplsnd *snd);
void oplsnd_write(struct oplsnd *snd, int idx, int reg, int val);
void oplsnd_generate_samples(struct oplsnd *snd, int samples);
int oplsnd_getfreq(struct oplsnd *snd);
void oplsnd_done(struct oplsnd *snd);
const char *oplsnd_errmsg(struct oplsnd *snd);

#ifdef __cplusplus
}
#endif
