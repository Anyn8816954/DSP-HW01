#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.14159265359

typedef struct {
    char chunkID[4];
    unsigned int chunkSize;
    char format[4];
} RIFFHeader;

typedef struct {
    char subchunk1ID[4];
    unsigned int subchunk1Size;
    unsigned short audioFormat;
    unsigned short numChannels;
    unsigned int sampleRate;
    unsigned int byteRate;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
} FmtSubchunk;

typedef struct {
    char subchunk2ID[4];
    unsigned int subchunk2Size;
} DataSubchunk;

int main(int argc, char *argv[])
{
    FILE *inputFile;
    FILE *outputFile;
    RIFFHeader riff;
    FmtSubchunk fmt;
    DataSubchunk data;
    short *samples;
    size_t sampleCount;
    size_t frameCount;
    size_t n;
    int channel;
    double previousY[2] = {0.0, 0.0};
    double R = 1000.0;
    double C = (1.0 / (2.0 * PI)) * (1.0 / 400.0) * (1.0 / 1000.0);
    double tau;
    double rc;
    double a;
    double b;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s in_fn out_fn\n", argv[0]);
        return 1;
    }

    inputFile = fopen(argv[1], "rb");
    if (inputFile == NULL) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        return 1;
    }

    if (fread(&riff, sizeof(RIFFHeader), 1, inputFile) != 1
        || fread(&fmt, sizeof(FmtSubchunk), 1, inputFile) != 1
        || fread(&data, sizeof(DataSubchunk), 1, inputFile) != 1) {
        fprintf(stderr, "Cannot read WAV header from %s\n", argv[1]);
        fclose(inputFile);
        return 1;
    }

    if (memcmp(riff.chunkID, "RIFF", 4) != 0
        || memcmp(riff.format, "WAVE", 4) != 0
        || memcmp(fmt.subchunk1ID, "fmt ", 4) != 0
        || memcmp(data.subchunk2ID, "data", 4) != 0
        || fmt.audioFormat != 1 || fmt.numChannels != 2
        || fmt.bitsPerSample != 16 || fmt.sampleRate == 0
        || data.subchunk2Size % sizeof(short) != 0) {
        fprintf(stderr, "Expected a stereo PCM 16-bit WAV file\n");
        fclose(inputFile);
        return 1;
    }

    sampleCount = data.subchunk2Size / sizeof(short);
    frameCount = sampleCount / 2;
    samples = (short *)malloc(sizeof(short) * sampleCount);
    if (samples == NULL) {
        fprintf(stderr, "Not enough memory for samples\n");
        fclose(inputFile);
        return 1;
    }

    if (fread(samples, sizeof(short), sampleCount, inputFile) != sampleCount) {
        fprintf(stderr, "Cannot read WAV samples from %s\n", argv[1]);
        free(samples);
        fclose(inputFile);
        return 1;
    }
    fclose(inputFile);

    tau = 1.0 / fmt.sampleRate;
    rc = R * C;
    a = rc / (rc + tau);
    b = tau / (rc + tau);

    for (n = 0; n < frameCount; ++n) {
        for (channel = 0; channel < 2; ++channel) {
            size_t index = n * 2 + channel;
            double x = samples[index];
            double y = a * previousY[channel] + b * x;

            samples[index] = (short)(y >= 0.0 ? y + 0.5 : y - 0.5);
            previousY[channel] = y;
        }
    }

    outputFile = fopen(argv[2], "wb");
    if (outputFile == NULL) {
        fprintf(stderr, "Cannot save %s\n", argv[2]);
        free(samples);
        return 1;
    }

    if (fwrite(&riff, sizeof(RIFFHeader), 1, outputFile) != 1
        || fwrite(&fmt, sizeof(FmtSubchunk), 1, outputFile) != 1
        || fwrite(&data, sizeof(DataSubchunk), 1, outputFile) != 1
        || fwrite(samples, sizeof(short), sampleCount, outputFile) != sampleCount) {
        fprintf(stderr, "Cannot write %s\n", argv[2]);
        fclose(outputFile);
        free(samples);
        return 1;
    }

    fclose(outputFile);
    free(samples);
    printf("Filtered %zu frames at %u Hz.\n", frameCount, fmt.sampleRate);
    return 0;
}