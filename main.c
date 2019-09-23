#include <stdio.h> /* fprintf, printf, sscanf */
#include <stdint.h> /* uint*_t, int*_t */
#include <complex.h> /* cexp, creal, rint */

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_FAILURE_STRINGS
#include "stb_image.h" /* stbi_load, stbi_image_free */

static double complex DFT_LOOKUP[4096];

static inline void
DFTInit()
{
    double complex t = -((2 * 3.14159265358979323846 * (double complex) I)/64);
    
    uint16_t i;
    for (i = 0; i < 4096; i++)
    {
        DFT_LOOKUP[i] = cexp(t * i);
    }
}

/* Too small data for using FFT */
static inline void
DFT(const double complex array[static 64], 
    double complex* restrict out) 
{
    uint8_t k, n;
    for (k = 0; k < 64; k++)
    {
        out[k] = 0;
        for (n = 0; n < 64; n++)
        {
            out[k] += array[n] * DFT_LOOKUP[n * k];
        }
    }
}

/* DCT will always output -64 <= x <= 64 */
static inline void
DCT(const double complex array[static 64],
    double complex* restrict out)
{
    double complex array2[64];
    uint8_t i;
    for (i = 0; i < 64; i++)
    {
        uint8_t j = i / 2;
        if ((i % 2) != 0)
        {
            j = 63 - j;
        }
        array2[j] = array[i];
    }

    DFT(array2, out);
    for (i = 0; i < 64; i++)
    {
        out[i] = creal(out[i]);
    }
}

static inline void
Biases(const double complex array[static 64],  
       uint8_t* out)
{
    double complex array2[64];
    DCT(array, array2);
  
    uint8_t i, j;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            out[i + (j * 4)] = ((uint8_t) creal(array2[i + (j * 8)])) + 64;
        }
    }
}

static inline double
Compare(uint8_t arr[static 16], uint8_t arr2[static 16])
{
    uint8_t i;
    double diff = 0;
    for (i = 0; i < 16; i++)
    {
        uint8_t n = arr[i] ^ arr2[i];
        while (n > 0) 
        { 
            n &= (n - 1);
            diff++; 
        } 
    }
    diff /= 128.0;
    return 1.0 - diff;
}

static inline uint8_t
FromImage(const char* filename, uint8_t* out)
{
    uint8_t ret = 0;

    int x, y, n;
    uint8_t * data = stbi_load(filename, &x, &y, &n, 4);

    if (data == NULL)
    {
        fprintf(stderr, "aspect: '%s' couldn't be loaded\n", filename);
        ret = 1;
    }
    else if (x > 65535 || y > 65535)
    {
        fprintf(stderr, "aspect: '%s' is larger than 65535x65535", filename);
        ret = 1;
    }
    else
    {
        double px = 0;
        double x8 = x / 7.0;
        double y8 = y / 7.0;
        
        uint16_t i;
        double complex data2[64];
        for (i = 0; i < 64; i++)
        {
            data2[i] = 0.5;
        }
        
        uint32_t idx;
        for (i = 0; i < y; i++)
        {
            uint16_t j;
            uint16_t si = (uint8_t) (rint(i / y8));
            for (j = 0; j < x; j++)
            {
                uint16_t sj = (uint8_t) (rint(j / x8));
                
                idx = (j + (i * (uint16_t) x)) * 4;
                px = (0.30 * (data[idx] / 255.0)) + 
                     (0.59 * (data[idx + 1] / 255.0)) + 
                     (0.11 * (data[idx + 2] / 255.0));

                px *= (data[idx + 3] / 255.0);
                data2[sj + (si * 8)] = (data2[sj + (si * 8)] + px) / 2.0;
            }
        }

        Biases(data2, out);
        stbi_image_free(data);
    }
    return ret;
}

static inline uint8_t
FromString(const char* hash, uint8_t* out)
{
    uint8_t ret = 0;

    if (strlen(hash) == 32)
    {
        int n, i, x;
        uint16_t j = 0; 
        for (i = 0; sscanf(&(hash[i]), "%02x%n", &x, &n) == 1; i += n)
        {
            out[j] = (uint8_t) x;
            j++;

            if (x > 128 || j > 16)
            {
                ret = 1;
                break;
            }
        }
        if (j < 16)
        {
            ret = 1;
        }
    }
    else
    {
        ret = 1;
    }

    if (ret == 1)
    {
        fprintf(stderr, "aspect: Hash '%s' in wrong format", hash);
    }
    return ret;
}

static inline uint8_t
LoopCompute(int argc, char* argv[])
{
    uint8_t ret = 0;

    DFTInit();
    int i;
    uint8_t out[16];
    for (i = 2; i < argc; i++)
    {
        ret = FromImage(argv[i], out);
        if (ret == 0)
        {
            int j;
            for (j = 0; j < 16; j++)
            {
                printf("%02x", out[j]);
            }
            printf("  %s", argv[i]);
            printf("\n");
        }
    }
    return ret;
}

static inline uint8_t
LoopCompare(int argc, char* argv[])
{
    uint8_t ret = 0;    

    uint8_t out[16];
    ret = FromString(argv[2], out);
    if (ret == 0)
    {
        int i;
        uint8_t out2[16];
        for (i = 3; i < argc; i++)
        {
            ret = FromString(argv[i], out2);
            if (ret == 0)
            {
                printf("%f  %s\n", Compare(out, out2), argv[i]);
            }
        }
    }
    return ret;
}

static inline uint8_t
Usage(void)
{
    fprintf(stderr, "Usage: aspect COMMAND ARGS\n");
    fprintf(stderr, "Compare images using perceptual hash\n\n");
    fprintf(stderr, "Avaliable commands:\n");
    fprintf(stderr, "   compute IMAGE...\n");
    fprintf(stderr, "   compare HASH HASHES...\n");
    return 1;
}

int
main(int argc, char* argv[])
{
    uint8_t ret = 0;

    if (argc < 2)
    {
        ret = Usage();
    }
    else if (strcmp("compute", argv[1]) == 0)
    {
        ret = LoopCompute(argc, argv);
    }
    else if (strcmp("compare", argv[1]) == 0)
    {
        ret = LoopCompare(argc, argv);
    }
    else
    {
        ret = Usage();
    }
    return ret;
}
