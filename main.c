

#ifdef __cplusplus
extern "C" {
#endif
#define  _CRT_SECURE_NO_WARNINGS

#include "timing.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//ref https://github.com/mackron/dr_libs/blob/master/dr_wav.h
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
//   ref https://github.com/rafat/hsfft
#include "hsfft.h"

void resampler(char *in_file, char *out_file);

void wavWrite_float(char *filename, float *buffer, int sampleRate, uint32_t totalSampleCount) {
    drwav_data_format format;
    format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;          // <-- Any of the DR_WAVE_FORMAT_* codes.
    format.channels = 1;
    format.sampleRate = (drwav_uint32) sampleRate;
    format.bitsPerSample = 32;
    drwav *pWav = drwav_open_file_write(filename, &format);
    if (pWav) {
        drwav_uint64 samplesWritten = drwav_write(pWav, totalSampleCount, buffer);
        drwav_uninit(pWav);
        if (samplesWritten != totalSampleCount) {
            fprintf(stderr, "ERROR\n");
            exit(1);
        }
    }
}

float *wavRead_float(char *filename, uint32_t *sampleRate, uint64_t *totalSampleCount) {
    unsigned int channels;
    float *buffer = drwav_open_and_read_file_f32(filename, &channels, sampleRate, totalSampleCount);
    if (buffer == NULL) {
        printf("ERROR.");
    }
    if (channels != 1) {
        drwav_free(buffer);
        buffer = NULL;
        *sampleRate = 0;
        *totalSampleCount = 0;
    }
    return buffer;
}

void splitpath(const char *path, char *drv, char *dir, char *name, char *ext) {
    const char *end;
    const char *p;
    const char *s;
    if (path[0] && path[1] == ':') {
        if (drv) {
            *drv++ = *path++;
            *drv++ = *path++;
            *drv = '\0';
        }
    } else if (drv)
        *drv = '\0';
    for (end = path; *end && *end != ':';)
        end++;
    for (p = end; p > path && *--p != '\\' && *p != '/';)
        if (*p == '.') {
            end = p;
            break;
        }
    if (ext)
        for (s = end; (*ext = *s++);)
            ext++;
    for (p = end; p > path;)
        if (*--p == '\\' || *p == '/') {
            p++;
            break;
        }
    if (name) {
        for (s = p; s < end;)
            *name++ = *s++;
        *name = '\0';
    }
    if (dir) {
        for (s = path; s < p;)
            *dir++ = *s++;
        *dir = '\0';
    }
}

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// <RESAMPLING BASED ON FFT>
// https://www.dsprelated.com/showcode/54.php

void FFTResample(float *input, float *output, size_t input_size, size_t output_size) {
    fft_t *samples = (fft_t *) calloc(MAX(input_size, output_size), sizeof(fft_t));
    if (samples == NULL) {
        return;
    }
    fft_real_object fftPlan = fft_real_init(input_size, 1);
    fft_r2c_exec(fftPlan, input, samples);
    free_real_fft(fftPlan);
    if (output_size < input_size) {
        // remove some high frequency samples;
        size_t half_output = (output_size / 2);
        size_t diff_size = input_size - output_size;
        memset(samples + half_output, 0, diff_size * sizeof(fft_t));
    } else if (output_size > input_size) {
        size_t half_input = input_size / 2;
        // add some high frequency zero samples
        size_t diff_size = output_size - input_size;
        memmove(samples + half_input + diff_size, samples + half_input, half_input * sizeof(fft_t));
        memset(samples + half_input, 0, diff_size * sizeof(fft_t));
    }
    fft_real_object ifftPlan = fft_real_init(output_size, -1);
    fft_c2r_exec(ifftPlan, samples, output);
    free_real_fft(ifftPlan);
    float norm = 1.0f / input_size;
    for (int i = 0; i < output_size; i++) {
        output[i] = output[i] * norm;
    }
    free(samples);
}

void resampler(char *in_file, char *out_file) {
    uint32_t sampleRate = 0;
    uint64_t totalSampleCount = 0;
    float *data_in = wavRead_float(in_file, &sampleRate, &totalSampleCount);
    uint32_t out_sampleRate = 48000;
    uint32_t out_size = (uint32_t)(totalSampleCount * ((float) out_sampleRate / sampleRate));
    float *data_out = (float *) malloc(out_size * sizeof(float));

    if (data_in != NULL && data_out != NULL) {
        double startTime = now();
        FFTResample(data_in, data_out, totalSampleCount, out_size);
        double time_interval = calcElapsed(startTime, now());
        printf("time interval: %d ms\n ", (int) (time_interval * 1000));
        wavWrite_float(out_file, data_out, out_sampleRate, (uint32_t) out_size);
        free(data_in);
        free(data_out);
    } else {
        if (data_in) free(data_in);
        if (data_out) free(data_out);
    }
}

int main(int argc, char *argv[]) {
    printf("Audio Processing\n");
    printf("FFT Resampler\n");
    printf("blog:http://cpuimage.cnblogs.com/\n");
    if (argc < 2)
        return -1;

    char *in_file = argv[1];
    char drive[3];
    char dir[256];
    char fname[256];
    char ext[256];
    char out_file[1024];
    splitpath(in_file, drive, dir, fname, ext);
    sprintf(out_file, "%s%s%s_out%s", drive, dir, fname, ext);
    resampler(in_file, out_file);

    printf("press any key to exit.\n");
    getchar();
    return 0;
}

#ifdef __cplusplus
}
#endif

