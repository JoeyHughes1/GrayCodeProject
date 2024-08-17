/**
 * @file GreyCodeTypes.h
 * @author Joey Hughes
 * This file contains the typedefs and constants I use with my Grey Code Programs. This also includes
 * GMP in order to define sequenceNum.
 * This defines the fabled NUM_DIGITS and len, so this is pretty important stuff.
*/

#include <gmp.h>

/** A flag to mark that the types have already been defined. */
#define GREY_CODE_TYPES_DEFINED 1


/** This defines the macro of how many digits for the grey code search by default to be 4. Should usually be set in compilation with -DNUM_DIGITS=X (at least 4 !) */
#ifndef NUM_DIGITS
#define NUM_DIGITS 4
#endif

/** The length of the sequence. 2^n */
#define len (1 << NUM_DIGITS)

/** This defines a short name for each step's XOR mask. Essentialy this holds which digit is being changed at the current step. */
typedef unsigned short stepMask;

/** This defines a short name for each step's digit number that it's changing. */
typedef unsigned char step;

/** A sequence, or array of "len" steps. */
typedef step sequence[len];

/** A typedef for an array of sequence pointers. Used to simplify the code for the SeedSearch2 part and make it more understandable. */
typedef sequence **seqPtrArray;

/** This defines a short name for a large number type to store the sequence numbers.*/
typedef mpz_t sequenceNum;

