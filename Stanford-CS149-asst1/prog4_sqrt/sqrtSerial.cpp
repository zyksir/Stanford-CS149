#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>


void sqrtSerial(int N,
                float initialGuess,
                float values[],
                float output[])
{

    static const float kThreshold = 0.00001f;

    for (int i=0; i<N; i++) {

        float x = values[i];
        float guess = initialGuess;

        float error = fabs(guess * guess * x - 1.f);

        while (error > kThreshold) {
            guess = (3.f * guess - x * guess * guess * guess) * 0.5f;
            error = fabs(guess * guess * x - 1.f);
        }

        output[i] = x * guess;
    }
}

void sqrtAVX(int N,
                float initialGuess,
                float values[],
                float output[]) {

    static const float kThreshold = 0.00001f;
    static const int VECTOR_WIDTH = 8;
    __m256 kThre = _mm256_set1_ps(kThreshold);
    __m256 initGuess = _mm256_set1_ps(initialGuess);
    __m256 constOne = _mm256_set1_ps(1.f);
    __m256 constZero = _mm256_set1_ps(-0.0f);
    __m256 constHalf = _mm256_set1_ps(0.5f);
    __m256 constThree = _mm256_set1_ps(3.f);
    __m256 x, error, guess;
    __m256 tmp, gt;

    for (int i=0; i<N; i+=VECTOR_WIDTH) {

        x = _mm256_load_ps(values + i);           // float x = values[i];
        
        // // cheat!
        // __m256 result = _mm256_sqrt_ps(x);
        // _mm256_storeu_ps(output + i, result);

        guess = initGuess;   // float guess = initialGuess;
        tmp = _mm256_mul_ps(guess, guess);
        error = _mm256_mul_ps(tmp, x);
        error = _mm256_sub_ps(error, constOne);
        // _mm_andnot_ps clears the sign bit making the value non-negative. 
        error = _mm256_andnot_ps(constZero, error); // float error = fabs(guess * guess * x - 1.f);

        gt = _mm256_cmp_ps(error, kThre, _CMP_GT_OQ);
        unsigned mask = _mm256_movemask_ps(gt);
        while(_mm_popcnt_u32(mask)) { // while (error > kThreshold) 
            // tmp = _mm256_mul_ps(guess, guess);
            tmp = _mm256_mul_ps(tmp, guess);
            tmp = _mm256_mul_ps(tmp, x);

            error = _mm256_mul_ps(guess, constThree);
            tmp = _mm256_sub_ps(error, tmp);
            guess = _mm256_mul_ps(tmp, constHalf); // guess = (3.f * guess - x * guess * guess * guess) * 0.5f;

            tmp = _mm256_mul_ps(guess, guess);
            error = _mm256_mul_ps(tmp, x);
            error = _mm256_sub_ps(error, constOne);
            error = _mm256_andnot_ps(constZero, error); // error = fabs(guess * guess * x - 1.f);
            gt = _mm256_cmp_ps(error, kThre, _CMP_GT_OQ);
            mask = _mm256_movemask_ps(gt);
        }

        guess = _mm256_mul_ps(x, guess); // output[i] = x * guess;
        _mm256_storeu_ps(output + i, guess);
    }
}
