#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h> // For int64_t

#define FRAC_BITS 14
#define SCALE (1 << FRAC_BITS) // 2^14

typedef int fp_t; // Use 32-bit for Pintos compatibility

// Conversion Macros
#define INT_TO_FP(x) ((fp_t)((x) * SCALE)) // Integer → Fixed
#define FP_TO_INT_TRUNC(x) ((x) / SCALE)   // Truncate toward zero
#define FP_TO_INT_NEAREST(x) (((x) >= 0) ? (((x) + SCALE / 2) / SCALE) : (((x) - SCALE / 2) / SCALE))

// Arithmetic Macros
#define FP_ADD(x, y) ((x) + (y))                             // Fixed + Fixed
#define FP_SUB(x, y) ((x) - (y))                             // Fixed - Fixed
#define FP_MULT(x, y) ((fp_t)(((int64_t)(x) * (y)) / SCALE)) // Fixed × Fixed
#define FP_DIV(x, y) ((fp_t)(((int64_t)(x) * SCALE) / (y)))  // Fixed ÷ Fixed
#define FP_PLUS_INT(x, y) ((x) + (fp_t)(y) * SCALE)          // Fixed + Integer
#define FP_MINUS_INT(x, y) ((x) - (fp_t)(y) * SCALE)         // Fixed - Integer
#define FP_MULT_INT(x, n) ((x) * (n))                        // Fixed * Integer
#define FP_DIV_INT(x, n) ((x) / (n))                         // Fixed / Integer

#endif /* FIXED_POINT_H */