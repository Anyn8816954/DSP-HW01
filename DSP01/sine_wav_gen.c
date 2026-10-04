#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define PI 3.14159265359
#define AMPLITUDE 10000

typedef struct {
    char chunkID[4];     // "RIFF"
    unsigned int chunkSize;
    char format[4];      // "WAVE"
} RIFFHeader;

typedef struct {
    char subchunk1ID[4]; // "fmt "
    unsigned int subchunk1Size;
    unsigned short audioFormat;
    unsigned short numChannels;
    unsigned int sampleRate;
    unsigned int byteRate;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
} FmtSubchunk;

typedef struct {
    char subchunk2ID[4]; // "data"
    unsigned int subchunk2Size;
} DataSubchunk;


int main(int argc, char *argv[])
{
    double fs;
    double f;
    double length;
    double samplePeriod;
    size_t sampleCount;
    size_t n;
    short *stereo;
    char *outputFileName;
    FILE *fp;
    RIFFHeader riff;
    FmtSubchunk fmt;
    DataSubchunk data;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s fs f L out_fn\n", argv[0]);
        return 1;
    }

    fs = atof(argv[1]);
    f = atof(argv[2]);
    length = atof(argv[3]);
    outputFileName = argv[4];

    if (fs <= 0 || f < 0 || length <= 0) {
        fprintf(stderr, "fs and L must be positive; f cannot be negative\n");
        return 1;
    }

    samplePeriod = 1.0 / fs;
    sampleCount = (size_t)(length * fs);
    stereo = (short *)malloc(sizeof(short) * sampleCount * 2);
    if (stereo == NULL) {
        fprintf(stderr, "Not enough memory for audio samples\n");
        return 1;
    }

    for (n = 0; n < sampleCount; ++n) {
        double time = n * samplePeriod;
        stereo[2 * n] = (short)floor(
            AMPLITUDE * sin(2 * PI * f * time) + 0.5);
        stereo[2 * n + 1] = (short)floor(
            AMPLITUDE * cos(2 * PI * f * time) + 0.5);
    }

    riff = (RIFFHeader){{'R', 'I', 'F', 'F'},
        36 + (unsigned int)(sampleCount * 2 * sizeof(short)), {'W', 'A', 'V', 'E'}};
    fmt = (FmtSubchunk){{'f', 'm', 't', ' '}, 16, 1, 2,
        (unsigned int)fs, (unsigned int)(fs * 2 * sizeof(short)), 4, 16};
    data = (DataSubchunk){{'d', 'a', 't', 'a'},
        (unsigned int)(sampleCount * 2 * sizeof(short))};

    fp = fopen(outputFileName, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Cannot save %s\n", outputFileName);
        free(stereo);
        return 1;
    }

    if (fwrite(&riff, sizeof(RIFFHeader), 1, fp) != 1
        || fwrite(&fmt, sizeof(FmtSubchunk), 1, fp) != 1
        || fwrite(&data, sizeof(DataSubchunk), 1, fp) != 1
        || fwrite(stereo, sizeof(short), sampleCount * 2, fp) != sampleCount * 2) {
        fprintf(stderr, "Cannot write %s\n", outputFileName);
        fclose(fp);
        free(stereo);
        return 1;
    }

    fclose(fp);
    free(stereo);
    return 0;
}
