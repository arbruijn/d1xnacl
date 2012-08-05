#include <sys/time.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <stdarg.h>
#include <cstdio>
#include <vector> // vector
#include <deque>  // deque
#include <cmath>  // exp, log, ceil

#include <assert.h>
#include "dbopl.h"
#include "oplsnd.h"

//static const unsigned long PCM_RATE = 48000;
static const unsigned long PCM_RATE = 44100;
//static const unsigned MaxCards = 100;
static const unsigned MaxSamplesAtTime = 512; // 512=dbopl limitation
static const double AudioBufferLength = 0.05;
static const double OurHeadRoomLength = 0.1;


#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

class MutexType
{
    SDL_mutex* mut;
public:
    MutexType() : mut(SDL_CreateMutex()) { }
    ~MutexType() { SDL_DestroyMutex(mut); }
    void Lock() { SDL_mutexP(mut); }
    void Unlock() { SDL_mutexV(mut); }
};

static std::deque<short> AudioBuffer;


static MutexType AudioBuffer_lock;
static void mySDL_AudioCallback(void*, Uint8* stream, int len)
{
    //SDL_LockAudio();
    short* target = (short*) stream;
    AudioBuffer_lock.Lock();
    /*if(len != AudioBuffer.size())
        fprintf(stderr, "len=%d stereo samples, AudioBuffer has %u stereo samples",
            len/4, (unsigned) AudioBuffer.size()/2);*/
    unsigned ate = len/2; // number of shorts
    if(ate > AudioBuffer.size()) {ate = AudioBuffer.size();/*fprintf(stderr, "underflow!\n");*/}
    for(unsigned a=0; a<ate; ++a)
        target[a] = AudioBuffer[a] * 2;
    AudioBuffer.erase(AudioBuffer.begin(), AudioBuffer.begin() + ate);
    //fprintf(stderr, "ate %u - remain %u\n", ate, (unsigned) AudioBuffer.size()/2);
    AudioBuffer_lock.Unlock();
    //SDL_UnlockAudio();
}

static void SendMonoAudio(unsigned long count, int* samples) {
}

static void SendStereoAudio(unsigned long count, int* samples) {
    SDL_LockAudio();
    //AudioBuffer_lock.Lock();
    size_t pos = AudioBuffer.size();
    AudioBuffer.resize(pos + count*2);
    for (unsigned int i = 0; i < count; i++) {
        AudioBuffer[pos + i * 2 + 0] = samples[i * 2 + 0];
        AudioBuffer[pos + i * 2 + 1] = samples[i * 2 + 1];
    }
    //AudioBuffer_lock.Unlock();
    SDL_UnlockAudio();
}


int setupsdl(SDL_AudioSpec *obtained) {
    /*
    // Set up SDL
    static SDL_AudioSpec spec;
    spec.freq     = PCM_RATE;
    spec.format   = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples  = spec.freq * AudioBufferLength;
    spec.callback = mySDL_AudioCallback;
    Mix_Init(0);
    if(Mix_OpenAudio(spec.freq, spec.format, spec.channels, spec.samples) < 0)
    {
        std::fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        return 1;
    }
    */
    memset(obtained, 0, sizeof(*obtained));
    int freq, channels;
    Uint16 format;
    Mix_QuerySpec(&freq, &format, &channels);
    obtained->freq = freq;
    obtained->format = format;
    obtained->channels = channels;
    /*
    *obtained = spec;
    if(spec.samples != obtained->samples)
        std::fprintf(stderr, "Wanted (samples=%u,rate=%u,channels=%u); obtained (samples=%u,rate=%u,channels=%u)\n",
            spec.samples,    spec.freq,    spec.channels,
            obtained->samples,obtained->freq,obtained->channels);
    */
    return 0;
}

struct oplsnd {
    char *errmsg;
    DBOPL::Handler h;
    int playing;
    SDL_AudioSpec obtained;
};

void oplsnd_write(struct oplsnd *snd, int idx, int reg, int val) {
    snd->h.WriteReg((idx << 8) | reg, val);
}

static void oplsnd_seterrmsg(struct oplsnd *snd, const char *msg, ...) {
    va_list vp, vp2;
    va_start(vp, msg);
    if (snd->errmsg)
        free(snd->errmsg);
    va_copy(vp2, vp);
    int size = vsnprintf(NULL, 0, msg, vp);
    if (!(snd->errmsg = (char *)malloc(size + 1)))
        return;
    vsnprintf(snd->errmsg, size, msg, vp);
    va_end(vp);
}

struct oplsnd *oplsnd_init(void) {
    struct oplsnd *snd;
    return (struct oplsnd *)calloc(1, sizeof(*snd));
}

void oplsnd_done(struct oplsnd *snd) {
    if (snd->playing)
        SDL_PauseAudio(1);
    if (snd->errmsg)
        free(snd->errmsg);
    free(snd);
}

const char *oplsnd_errmsg(struct oplsnd *snd) {
    return snd->errmsg;
}

int oplsnd_open(struct oplsnd *snd) {
/*
    if (SDL_Init(SDL_INIT_AUDIO)) {
        oplsnd_seterrmsg(snd, "sdl init failed: %s", SDL_GetError());
        return -1;
    }
*/
    if (setupsdl(&snd->obtained)) {
        oplsnd_seterrmsg(snd, "sdl audio setup failed: %s", SDL_GetError());
        return -1;
    }
    snd->h.Init(snd->obtained.freq);
    return 0;
}

int oplsnd_getfreq(struct oplsnd *snd) {
    return snd->obtained.freq;
}

void oplsnd_generate_samples(struct oplsnd *snd, int samples) {
    while (samples > 0) {
        int n = samples > 512 ? 512 : samples;
        snd->h.Generate(SendMonoAudio, SendStereoAudio, n);
        samples -= n;
        while (AudioBuffer.size() > snd->obtained.samples + (snd->obtained.freq*2) * OurHeadRoomLength) {
            if (!snd->playing) {
                Mix_HookMusic(mySDL_AudioCallback, NULL);
                SDL_PauseAudio(0);
                snd->playing = 1;
            }

            SDL_Delay(1); // std::min(10.0, 1e3 * eat_delay) );
        }
    }
}
