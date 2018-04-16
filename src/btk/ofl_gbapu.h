/*
 * GameBoy sound generators:
 *  1: rectangular + sweep (frequency change) + envelope (volume change)
 *  2: rectangular + envelope
 *  3: 4b wav (GB[C] 32 samples, GBA 32*2 samples + *.75 volume)
 *  4: white noise + envelope
 * The 2 GBA wave channels are signed 8b, bias 0.
 * Mixing of all channels is done in 9b.
 */

/***************************************************************************
 * HEADER
 **************************************************************************/

#ifndef INCLUDE_OFL_GBAPU_H
#define INCLUDE_OFL_GBAPU_H

#include <stdint.h>

#ifdef OFL_GBAPU_STATIC
#define OFL_GBAPU_DEF static
#else
#define OFL_GBAPU_DEF extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define OFL_GBAPU_SAMPLESPERSEC (120) // FIXME

struct ofl_gbapu_osc_sqr
{
    int32_t on;             // 0 / 1: channel active status
    int32_t autostop;       // 0 / 1: if 1, stop playback after duration_s (derived from len)
    int32_t freq;           // 0-2047: f_hz = OFL_GBAPU_FREQ / (32 * (2048 - x)) => [64, 131.1] KHz
    int32_t duty;           // 0-3: amplitude peaks proportion [12.5%, 25%, 50%, 75%]
    int32_t env_steps;      // 0-7: envelope steps steptime_s = n / 64
    int32_t env_dir;        // 0 / 1: sub or add volume
    int32_t env_val;        // 0-15: initial volume
    int32_t sweep_shift;    // 0-7 number of sweep shifts, result must fit in 11 bits otherwise chan0 is stopped
    int32_t sweep_dir;      // 0 / 1: add or sub freq
    int32_t sweep_time;     // 0-7: x / 128 s

    int32_t period;         // remaining time to phase change
    int32_t env_period;     // current envelope 64 Hz ticks remaining before update
    int32_t sweep_period;   // current sweep 128 Hz ticks remaining before update
    int32_t phase;          // current square step phase
    int32_t len;            // 0-64: duration_s = len / 256
    int32_t volume;         // current volume
    int32_t sample;         // current output sample
};
struct ofl_gbapu_osc_wav
{
    int32_t on;             // 0 / 1: channel active status
};
struct ofl_gbapu_osc_noi
{
    int32_t on;             // 0 / 1: channel active status
};
struct ofl_gbapu
{
    struct ofl_synth *syn;
    struct ofl_gbapu_osc_sqr chan0;
    struct ofl_gbapu_osc_sqr chan1; // no sweep
    struct ofl_gbapu_osc_wav chan2;
    struct ofl_gbapu_osc_noi chan3;
    uint32_t clock;
    int32_t freq;

};
OFL_GBAPU_DEF void ofl_gbapu_init(struct ofl_gbapu *s);
OFL_GBAPU_DEF void ofl_gbapu_fillbuf(struct ofl_gbapu *s, void *lbuf, void *rbuf, size_t buf_sz);

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_OFL_GBAPU_H

/***************************************************************************
 * IMPLEMENTATION
 **************************************************************************/

#ifdef OFL_GBAPU_IMPLEMENTATION

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    OFL_GBAPU_FREQ = 0x00400000, // 2^22
    OFL_GBAPU_DUTY = 0x3FF0C040, // up values for each duty setting
};

OFL_GBAPU_DEF void ofl_gbapu_init(struct ofl_gbapu *s)
{
    memset(s, 0, sizeof(*s));
    s->chan0.sweep_dir = 1;
    s->chan0.sweep_period = 8;
    s->chan0.env_period = 8;
    s->chan0.len = 64;
    s->chan1.env_period = 8;
    s->chan1.len = 64;
}

static void ofl_gbapu_osc_sqr_gen(struct ofl_gbapu_osc_sqr *c)
{
    if (c->period && --c->period == 0) c->period = 2 * (0x800 - c->freq), c->phase = (c->phase + 1) & 7;
    // TODO dégager sample var
    c->sample = c->on && (1 << c->phase) & (OFL_GBAPU_DUTY >> c->duty*8) ? c->volume : 0;
}
static void ofl_gbapu_osc_sqr_len(struct ofl_gbapu_osc_sqr *c)
{
    if (c->autostop && c->len && 0 == --c->len) c->on = 0;
}
static void ofl_gbapu_osc_sqr_sweep(struct ofl_gbapu_osc_sqr *c)
{
    if (--c->sweep_period) return;
    c->sweep_period = c->sweep_time ? c->sweep_time : 8;
    if ((c->sweep_dir && 0 == c->sweep_shift) || 0 == c->sweep_time) return;
    int32_t nfreq = c->freq >> c->sweep_shift; if (c->sweep_dir) nfreq = -nfreq; nfreq += c->freq;
    if (nfreq >= 0x800) c->on = 0;
    else c->freq = nfreq, c->period = 2 * (0x800 - nfreq);
    if (c->sweep_shift && !c->sweep_dir && c->freq + (c->freq >> c->sweep_shift) >= 0x800) c->on = 0;
}
static void ofl_gbapu_osc_sqr_env(struct ofl_gbapu_osc_sqr *c)
{
    if (!c->on || !c->env_steps || --c->env_period) return;
    c->env_period = c->env_steps ? c->env_steps : 8;
    if (c->env_dir) { if (c->volume < 15) ++c->volume; }
    else if (c->volume > 0) --c->volume;
}
OFL_GBAPU_DEF void ofl_gbapu_fillbuf(struct ofl_gbapu *s, float *lbuf, float *rbuf, size_t buf_sz)
{
    uint32_t ticks_to_run = buf_sz * OFL_GBAPU_SAMPLESPERSEC;
    while (ticks_to_run > 0)
    {
        /* Step   Length Ctr  Vol Env     Sweep
         * ---------------------------------------
         * 0      Clock       -           -
         * 1      -           -           -
         * 2      Clock       -           Clock
         * 3      -           -           -
         * 4      Clock       -           -
         * 5      -           -           -
         * 6      Clock       -           Clock
         * 7      -           Clock       -
         * ---------------------------------------
         * Rate   256 Hz      64 Hz       128 Hz   */

        uint32_t clock = s->clock;
        uint32_t ticks_to_frame = OFL_GBAPU_FREQ/512 - (clock & (OFL_GBAPU_FREQ/512 - 1));
        uint32_t ticks = ticks_to_run < ticks_to_frame ? ticks_to_run : ticks_to_frame;
        // generate samples
        for (uint32_t i = 0; i < ticks; ++i)
        {
            // TODO use l/rbuf, set to 0, add each chan in an individual for loop, mix in the end
            ofl_gbapu_osc_sqr_gen(&s->chan0);
            ofl_gbapu_osc_sqr_gen(&s->chan1);
            ofl_gbapu_osc_wav_gen(&s->chan2);
            ofl_gbapu_osc_noi_gen(&s->chan3);
            int32_t sample = s->chan0.sample + s->chan1.sample + s->chan2.sample + s->chan3.sample;
        }
        s->clock += ticks;
        ticks_to_run -= ticks;
        if (ticks < ticks_to_frame) break;

        // 256 Hz steps [0, 2, 4, 6]
        if (0 == (clock & (OFL_GBAPU_FREQ/256 - 1)))
        {
            ofl_gbapu_osc_sqr_len(&s->chan0);
            ofl_gbapu_osc_sqr_len(&s->chan1);
            ofl_gbapu_osc_wav_len(&s->chan2);
            ofl_gbapu_osc_noi_len(&s->chan3);
        }
        // 128 Hz steps [2, 6]
        if (0 == ((clock + OFL_GBAPU_FREQ/256) & (OFL_GBAPU_FREQ/128 - 1)))
        {
            ofl_gbapu_osc_sqr_sweep(&s->chan0);
        }
        // 64 Hz steps [7]
        if (0 == ((clock + OFL_GBAPU_FREQ/512) & (OFL_GBAPU_FREQ/64 - 1)))
        {
            ofl_gbapu_osc_sqr_env(&s->chan0);
            ofl_gbapu_osc_sqr_env(&s->chan1);
            ofl_gbapu_osc_noi_env(&s->chan3);
        }
    }
}

OFL_GBAPU_DEF int32_t ofl_gbapu_peek(struct ofl_gbapu *s, int32_t zp)
{
    switch (zp & 0xFF)
    {
    case 0x10: return 0x80 | s->chan0.sweep_time << 4 | s->chan0.sweep_dir << 3 | s->chan0.sweep_shift;
    case 0x11: return s->chan0.duty << 6 | 0x3F;
    case 0x12: return s->chan0.env_val << 4 | s->chan0.env_dir << 3 | s->chan0.env_steps;
    case 0x14: return 0x80 | s->chan0.autostop << 6 | 0x3F;
    case 0x16: return s->chan1.duty << 6 | 0x3F;
    case 0x17: return s->chan1.env_val << 4 | s->chan1.env_dir << 3 | s->chan1.env_steps;
    case 0x19: return 0x80 | s->chan1.autostop << 6 | 0x3F;
    }
    return 0xFF;
}
OFL_GBAPU_DEF void ofl_gbapu_poke(struct ofl_gbapu *s, int32_t zp, int32_t v)
{
    switch (zp & 0xFF)
    {
    case 0x10:
        if (s->chan0.sweep_time && s->chan0.sweep_dir && (v>>3 & 1) == 0) s->chan0.on = 0;
        s->chan0.sweep_time = v>>4 & 7; s->chan0.sweep_dir = v>>3 & 1; s->chan0.sweep_shift = v & 7;
        break;
    case 0x11: s->chan0.duty = v>>5 & 3; s->chan0.len = 64 - (x & 0x3F); break;
    case 0x12:
        s->chan0.env_val = v>>4 & 0xF; s->chan0.env_dir = v>>3 & 1; s->chan0.env_steps = v & 7;
        if (!s->chan0.env_val && !s->chan0.env_dir) s->chan0.on = 0;
        break;
    case 0x13: s->chan0.freq &= ~0xFF; s->chan0.freq |= v & 0xFF;  break;
    case 0x14:
        if (0 == (s->clock>>9 & 1) && !s->chan0.autostop && (v>>6 & 1) && s->chan0.len && 0 == --s->chan0.len) s->chan0.on = 0;
        s->chan0.freq &= 0xFF; s->chan0.freq |= (v&7) << 8; s->chan0.autostop = v>>6 & 1;
        if (v & 0x80)
        {
            s->chan0.on = s->chan0.env_val || s->chan0.env_dir;
            if (0 == s->chan0.len) { s->chan0.len = 64; if (0 == (s->clock>>9 & 1) && s->chan0.autostop) --s->chan0.len; }
            s->chan0.period = 2 * (0x800 - s->chan0.freq);
            s->chan0.volume = s->chan0.env_val;
            s->chan0.env_period = s->chan0.env_steps ? s->chan0.env_steps : 8;
            s->chan0.sweep_period = s->chan0.sweep_time ? s->chan0.sweep_time : 8;
            if (s->chan0.sweep_shift && !s->chan0.sweep_dir && s->chan0.freq + (s->chan0.freq >> s->chan0.sweep_shift) >= 0x800) c->on = 0;
        }
        break;
    case 0x16: s->chan1.duty = v>>5 & 3; s->chan1.len = 64 - (x & 0x3F); break;
    case 0x17:
        s->chan1.env_val = v>>4 & 0xF; s->chan1.env_dir = v>>3 & 1; s->chan1.env_steps = v & 7;
        if (!s->chan1.env_val && !s->chan1.env_dir) s->chan1.on = 0;
        break;
    case 0x18: s->chan1.freq &= ~0xFF; s->chan1.freq |= v & 0xFF;  break;
    case 0x19:
        if (0 == (s->clock>>9 & 1) && !s->chan1.autostop && (v>>6 & 1) && s->chan1.len && 0 == --s->chan1.len) s->chan1.on = 0;
        s->chan1.freq &= 0xFF; s->chan1.freq |= (v&7) << 8; s->chan1.autostop = v>>6 & 1;
        if (v & 0x80)
        {
            s->chan1.on = s->chan1.env_val || s->chan1.env_dir;
            if (0 == s->chan1.len) { s->chan1.len = 64; if (0 == (s->clock>>9 & 1) && s->chan1.autostop) --s->chan1.len; }
            s->chan1.period = 2 * (0x800 - s->chan1.freq);
            s->chan1.volume = s->chan1.env_val;
            s->chan1.env_period = s->chan1.env_steps ? s->chan1.env_steps : 8;
        }
        break;
    }
}

#ifdef __cplusplus
}
#endif

#endif // OFL_GBAPU_IMPLEMENTATION

