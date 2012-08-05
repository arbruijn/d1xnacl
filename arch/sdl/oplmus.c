#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "hmpfile.h"
#include "oplsnd.h"
#include "hmpopl.h"
#include "loadfile.h"
#include "oplmus.h"
#include <SDL/SDL.h>

static struct oplsnd *snd;

static void writereg(void *data, int idx, int reg, int val) {
    if (snd)
    	oplsnd_write(snd, idx, reg, val);
}

static int loadbankfile(hmpopl *h, const char *filename, int isdrum) {
	int size;
	void *data = (void *)loadfile(filename, &size);
	if (!data)
	    goto err;
	if (hmpopl_set_bank(h, data, size, isdrum))
		goto err;
	free(data);
	return 0;
err:
	if (data)
		free(data);
	return -1;
}


static int mus_switch, mus_loop;
static const char *mus_filename, *mus_melodic_file, *mus_drum_file;

int mus_thread(void *thread_arg) {
    struct hmp_file *hf;
    hmpopl *h;
    char buf[32];

    snd = oplsnd_init();
    if (oplsnd_open(snd)) {
        fprintf(stderr, "mus: %s\n", oplsnd_errmsg(snd));
        return 0;
    }
    for (;;) {
    while (!mus_switch || !mus_filename)
        SDL_Delay(1);
    mus_switch = 0;

    if (!(h = hmpopl_new())) {
    	fprintf(stderr, "create hmpopl failed\n");
    	continue;
    }

	if (loadbankfile(h, mus_melodic_file, 0)) {
    	fprintf(stderr, "load bnk %s failed\n", mus_melodic_file);
    	continue;
    }
	if (loadbankfile(h, mus_drum_file, 1)) {
    	fprintf(stderr, "load bnk %s failed\n", mus_drum_file);
    	continue;
    }

    int song_size;
	void *song_data;
	
    if (strlen(mus_filename) < sizeof(buf)) {
        strcpy(buf, mus_filename);
        buf[strlen(buf) - 1] = 'q';
        song_data = (void *)loadfile(buf, &song_size);
    } else
        song_data = NULL;
    if (!song_data && !(song_data = (void *)loadfile(mus_filename, &song_size))) {
        fprintf(stderr, "open %s failed\n", mus_filename);
        continue;
    }

    if (!(hf = hmp_open(song_data, song_size, 0xa009))) {
        fprintf(stderr, "read %s failed\n", mus_filename);
        continue;
    }
    free(song_data);

    hmpopl_set_write_callback(h, writereg, NULL);

    hmpopl_start(h);
    hmpopl_set_write_callback(h, writereg, NULL);

    hmpopl_start(h);

	int rc;
	do {
	hmp_event ev;
	hmp_reset_tracks(hf);
	hmpopl_reset(h);
	
	while (!(rc = hmp_get_event(hf, &ev)) && !mus_switch) {
	    if (ev.datalen)
	        continue;
        if ((ev.msg[0] & 0xf0) == 0xb0 && ev.msg[1] == 7) {
            int vol = ev.msg[2];
            vol = (vol * 127) >> 7;
            vol = (vol * 127) >> 7;
            ev.msg[2] = vol;
        }
        if (ev.delta) {
            if (snd)
                oplsnd_generate_samples(snd, (oplsnd_getfreq(snd) / 120) * ev.delta);
        }
        hmpopl_play_midi(h, ev.msg[0] >> 4, ev.msg[0] & 0x0f, ev.msg[1], ev.msg[2]);
	}
	} while (!mus_switch && mus_loop);
    hmpopl_done(h);
    hmp_close(hf);

    }
    return 0;
}

void oplmus_init(void) {
    SDL_CreateThread(mus_thread, NULL);
}

void oplmus_play(const char *filename, const char *melodic_file, const char *drum_file, int loop) {
    mus_filename = filename;
    mus_melodic_file = melodic_file;
    mus_drum_file = drum_file;
    mus_switch = 1;
    mus_loop = loop;
}
