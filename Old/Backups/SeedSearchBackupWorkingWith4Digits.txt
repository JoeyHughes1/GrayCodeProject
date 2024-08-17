/**
 * @file SeedSearch.c
 * @author Joey Hughes
 * Program to be used to analyze output from the main.c file (binary files of grey codes), 
 * and extract all of the seeds present in the sample of grey codes.
 * 
 * This program uses the GMP library, as sequence numbers can get very large, past the 64 bit integer limit.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <gmp.h>

/** This defines the macro of how many digits for the grey codes by default to be 0 and break everything. Should usually be set in compilation with -DNUM_DIGITS=X. */
#ifndef NUM_DIGITS
#define NUM_DIGITS 4
#endif

/** The length in steps of a grey code sequence. */
#define len (1 << NUM_DIGITS)

/** 
 * The length of the signature frequency list.
 * In a signature, every number is even and the maximum number possible is half of the length of the sequence, so
 * to have it where each element is how many times twice it's index appears in the signature, there must be half of half of the length + 1 elements
 * For example, with 4 digits, the sequence is 2^4 = 16 elements long, so the highest possible number in a signature is 8, and that frequency would be
 * stored at index 4, so the list needs 4 + 1 elements.
*/
#define SIG_FREQ_LIST_LEN ((1 << (NUM_DIGITS - 2)) + 1)



/** This defines a short name for each step's XOR mask. Essentialy this holds which digit is being changed at the current step. */
typedef unsigned char step;


/** This defines a short name for a large number type to store the sequence numbers.*/
typedef mpz_t sequenceNum;


/**
 * A struct that represents a unique seed that has been found. Holds it's sequence and sequence number, as well as it's signature, both sorted and unsorted.
*/
struct seedStruct
{
    /** The number of this sequence. The resulting number if the step sequence was treated as an n-digit number. */
    sequenceNum number;
    /** The full sequence array for this seed. */
    step sequence[len];
    /** The signature of this seed as it appears. */
    char signature[NUM_DIGITS];
    /** The frequency of the numbers in the signature, where the element at i indicates how many times i*2 appears in the signature. */
    char signatureFrequency[SIG_FREQ_LIST_LEN];
};
typedef struct seedStruct seed;



/**
 * Performs a swap in a sequence. Goes through the sequence and whenever it sees a, it writes b and when it sees b it writes a.
 * @param seq The sequence array.
 * @param a The first step digit number to swap.
 * @param b The other step digit number to swap.
*/
void swap(step *seq, step a, step b)
{
    // printf("Swapping %ds and %ds.\n", a, b);
    step *seqPtr = seq;
    while(seqPtr - seq < len) {
        if(*seqPtr == a) *seqPtr = b;
        else if(*seqPtr == b) *seqPtr = a;
        seqPtr++;
    }
}


/**
 * Calculates the sequence number for the given sequence. Assumes that rtn is initialized to 0.
 * @param sequence The sequence to get the number for.
 * @param rtn The GMP integer, that should be 0, where the sequence number will be stored.
 * @return The sequence number of the given sequence.
*/
void getSequenceNumber(step *sequence, sequenceNum rtn)
{
    step *ptr = sequence;
    while(ptr - sequence < len) {
        mpz_mul_ui(rtn, rtn, NUM_DIGITS);
        mpz_add_ui(rtn, rtn, *(ptr++));
    }
}


/**
 * This is a recursive function that will add the necessary swaps to the queue given how many overlapping digits there are in the signature.
 * This function returns how many total swaps were added to the queue
 * @param n How many digits to be swapping. 2 is the base case of just one swap, 3 is swapping to get all permutations of 3 digits, etc.
 * @param queueIndexToAddFrom The index in the queue where new swaps should be added.
 * @param queue Reference to the queue array.
 * @param swapQueueDigits The array of the actual step numbers to pass into the swap function.
 * @param indicesToSwap Essentially the parameters of this function. Which indices of swapQueueDigits to swap. Length of this should equal n.
 * @return The total amount of swaps added to the queue during the calling of this function.
*/
int addQueueSwaps(int n, int queueIndexToAddFrom, step queue[][2], step swapQueueDigits[], int indicesToSwap[])
{   
    // Base case of n == 2, just one swap
    if(n == 2) {
        queue[queueIndexToAddFrom][0] = swapQueueDigits[indicesToSwap[0]];
        queue[queueIndexToAddFrom][1] = swapQueueDigits[indicesToSwap[1]];
        return 1;
    }

    // Otherwise, we are n >= 3, so we do recursion
    // Initialize the index to add from to be the one we know is safe
    int currentIndexToAddFrom = queueIndexToAddFrom;

    // Initialize the new parameters to be all the ones we were given but leaving out the last one.
    int recursiveIndices[n - 1];
    for(int i = 0; i < n - 1; i++) {
        recursiveIndices[i] = indicesToSwap[i];
    }

    // Recurse through all the values in the indices to swap, 
    //    each of them getting a turn being left out of the recursive call, starting with the last one.
    for(int i = n - 1; i > 0; i--) {

        // Do the recursive call, getting all the swaps for one layer down
        currentIndexToAddFrom += addQueueSwaps(n - 1, currentIndexToAddFrom, queue, swapQueueDigits, recursiveIndices);

        // Otherwise, we add a swap from i to i-1
        queue[currentIndexToAddFrom][0] = swapQueueDigits[indicesToSwap[i]];
        queue[currentIndexToAddFrom][1] = swapQueueDigits[indicesToSwap[i - 1]];
        currentIndexToAddFrom++;

        // Then, switch i and i-1 in the recursive indices so now i is being included again in the swaps the next loop.
        recursiveIndices[i - 1] = indicesToSwap[i];
    }

    // Do one last adding of swaps for when i = 0
    currentIndexToAddFrom += addQueueSwaps(n - 1, currentIndexToAddFrom, queue, swapQueueDigits, recursiveIndices);

    // Now we have added all the swaps we were going to, go ahead and return how many we added
    return (currentIndexToAddFrom - queueIndexToAddFrom);
}



/**
 * Starts the program.
 * @return Exit status.
*/
int main(int argc, char* argv[])
{

    // Check if a file was given.
    if(argc != 2) {
        printf("Usage: ./SeedSearch Filename\n");
        return EXIT_FAILURE;
    }

    // Open the binary file to analyze
    FILE *in = fopen(argv[1], "rb");
    if(in == NULL) {
        printf("Something went wrong opening that file.\n");
        return EXIT_FAILURE;
    }

    // Start the runtime timer
    clock_t start_time = clock();

    
    /*
        I got to a nice algorithm for calculating rotations. Where N is the sequence number and d is the number of digits, and
        f is the first step in the sequence, the next rotation (moving the first step to the end) can be calculated by doing:
        N*d - f( d^(2^d) - 1 ). Everything in the parentheses is based on constants, so we can calculate that and all the possible
        multiples of that with f in a table, then just do a multiplication and subtract a value from the table based on f.
    */
    mpz_t multiplesTable[NUM_DIGITS];
    mpz_init(multiplesTable[0]);
    mpz_init_set_ui(multiplesTable[1], 1);
    // We perform the exponentiation by d here (len is 2^d)
    for(int i = 0; i < len; i++) {
        mpz_mul_ui(multiplesTable[1], multiplesTable[1], NUM_DIGITS);
    }
    mpz_sub_ui(multiplesTable[1], multiplesTable[1], 1); // Subtracting one we now we have d^(2^d) - 1 in index 1
    // We fill in the rest of the table by multiplying with index 1.
    for(int i = 2; i < NUM_DIGITS; i++) {
        mpz_init(multiplesTable[i]);
        mpz_mul_ui(multiplesTable[i], multiplesTable[1], i);
    }

    // The resizable array of seeds
    int seedListCapacity = 4;
    int seedListSize = 0;
    seed *seeds = (seed *)malloc(sizeof(seed) * seedListCapacity);

    // Create the queue of swaps. It will max be d! swaps long, and that will happen, so might
    //    as well create it that size already.
    int queueCapacity = 1;
    int queueSize = 0;
    for(int i = NUM_DIGITS; i > 1; i--) {
        queueCapacity *= i;
    }
    step queue[queueCapacity][2];

    // Create an array to hold which digits are involved in a swap queueing.
    int swapQueueSize = 0;
    step swapQueueDigits[NUM_DIGITS];

    // Variables
    step seq[len];                             // This holds the sequence currently being analyzed
    step *seqPtr;                              // Pointer to inside the sequence.
    char *sigPtr;                              // Pointer to inside the signature array (which is initialized in the loop).
    char *freqPtr;                             // Pointer to inside the signature frequency array.
    step (*queuePtr)[2];                       // Pointer to inside the queue array.
    seed *seedPtr;                             // Pointer to inside the seeds array. Makes code much cleaner.
    sequenceNum originalNum, rotatedNum;       // The variables to hold the large sequence numbers when checking for rotations.
    int debugCount = 0;

    // Initializing the GMP large integers
    mpz_inits(originalNum, rotatedNum, NULL); 
    
    // For each sequence in the input file.
    while(fread(seq, sizeof(step), len, in) > 0) {

        // If debugging, this tells me how many sequences have been checked
        debugCount++;
        
        // // Print the sequence
        // for(int j = 0; j < len; j++) printf(j != len - 1 ? "%x," : "%x\n", seq[j]);
        
        // printf("-----New sequence-----\n");

        // Variables
        char signatureNatural[NUM_DIGITS] = {};              // An array for the signature of seq (frequency of numbers in the sequence).
        char sigFreq[SIG_FREQ_LIST_LEN] = {};                // An array for the frequencies of numbers in the signature.

        // Calculate the signature of the sequence, which is the list of how many times each digit is flipped in the sequence.
        seqPtr = seq;
        do {
            signatureNatural[*(seqPtr++)]++;
        } while(seqPtr - seq < len);

        // Calculate the frequencies of the numbers in the signature as an array where
        //    the element at i indicates how many times in the signature i*2 appears.
        sigPtr = signatureNatural;
        do {
            sigFreq[(*(sigPtr++)) >> 1]++;
        } while(sigPtr - signatureNatural < NUM_DIGITS);

        // Loop through the seeds array. Try to catch it being a child of one of the seeds and continue the while. If we don't find it being a child of any, then it's a new seed.
        for(seedPtr = seeds; seedPtr - seeds < seedListSize; seedPtr++) {
            
            // If it has a different signature frequency, it's not a child of this seed, we go next
            if(memcmp(seedPtr->signatureFrequency, sigFreq, sizeof(sigFreq))) {
                continue;
            }

            // Now, the hard part. Since they have the same signature frequency, it's possible for it to be a child of this one.
            // Now we need to copy the sequence in question and see if there is a way to manipulate it into having it's sequence number match this seed's.
            // If we exhaust all those possibilities and it hasn't matched, it wasn't a child of this one, try the next.
            // If it ever matched, it was a child of this seed, and it is therefore not a new seed, we goto continueWhile.
            
            //First, copy the sequence into a new array we can manipulate.
            step sequence[len];
            memcpy(sequence, seq, sizeof(seq));

            //Then, copy the signature into a new array we can manipulate
            char signature[NUM_DIGITS] = {};
            memcpy(signature, signatureNatural, sizeof(signatureNatural));

            // printf("\nThe sequence trying to match is: ");
            // for(int j = 0; j < len; j++) printf(j != len - 1 ? "%x," : "%x\n", sequence[j]);
            // printf("With the signature: ");
            // for(int j = 0; j < NUM_DIGITS; j++) printf(j != NUM_DIGITS - 1 ? "%x," : "%x\n", signature[j]);

            // Now, we initially make swaps to get their signatures to match.
            // Loop through the signature arrays and if they don't match, make a swap to match them.
            // We don't need to do the last digit since there will only be one option left, they will have to match.
            for(int i = 0; i < NUM_DIGITS - 1; i++) {
                if(seedPtr->signature[i] == signature[i]) continue; // If they already match, continue
                //If not, then we need to make a swap with one of the later ones that does match
                // Loop through the rest of our signature to find which one matches
                int j = i + 1;
                while(signature[j] != seedPtr->signature[i]) j++;
                // Now we need to switch indices j and i in our signature and the sequence. Then, the signatures in this position match.
                swap(sequence, (step)i, (step)j);
                int temp = signature[j];
                signature[j] = signature[i];
                signature[i] = temp;
            }

            // printf("\n---After the for loop now the sequence is: ");
            // for(int j = 0; j < len; j++) printf(j != len - 1 ? "%x," : "%x\n", sequence[j]);
            // printf("with the signature: ");
            // for(int j = 0; j < NUM_DIGITS; j++) printf(j != NUM_DIGITS - 1 ? "%x," : "%x\n", signature[j]);




            // Create a queue of the extra swaps to be done if the rotation doesn't turn it into the other one.
            queueSize = 0;
            for(freqPtr = sigFreq + 1; freqPtr - sigFreq < SIG_FREQ_LIST_LEN; freqPtr++) {
                if(*freqPtr > 1) {
                    // Find out what are the digits involved
                    int signatureValueToLookFor = ((freqPtr - sigFreq) * 2);
                    int digitsLeftToFind = *freqPtr;
                    // printf("Right now looking for %d digits with the signature value %d\n", digitsLeftToFind, signatureValueToLookFor);
                    swapQueueSize = 0;
                    sigPtr = signature;
                    while(digitsLeftToFind > 0) {
                        if(*sigPtr == signatureValueToLookFor) {
                            swapQueueDigits[swapQueueSize++] = sigPtr - signature;
                            digitsLeftToFind--;
                        }
                        sigPtr++;
                    }

                    // For when just adding a two swap, we can do it simply like this
                    if(*freqPtr == 2) {
                        queue[queueSize][0] = swapQueueDigits[0];
                        queue[queueSize][1] = swapQueueDigits[1];
                        // If there were any earlier steps, this will copy them to be done after as well with the new swapping of the 2.
                        memcpy(queue + queueSize + 1, queue, (sizeof(step) * 2) * queueSize);
                        // Update the size
                        queueSize = queueSize * 2 + 1;
                    } else {
                        // When more than two, we use the special recursive function.
                        // First, we add the parameters to say we want to swap all of the indices of swapQueueDigits
                        int indicesToSwap[*freqPtr];
                        for(int i = 0; i < *freqPtr; i++) 
                            indicesToSwap[i] = i;

                        // Now, we check for the special case where there is already a swap in the queue from a previous 2 frequency (more than 2 this breaks)
                        //    This happens with 5-digit with (8,8,8,4,4).
                        if(queueSize > 0) {

                            // If there is, we need to store a copy of whatever two-swap is already there
                            step swapCopy[2];
                            swapCopy[0] = queue[0][0];
                            swapCopy[1] = queue[0][1];

                            // Now, we overwrite it with our series of swaps
                            queueSize = 0;
                            queueSize += addQueueSwaps(*freqPtr, queueSize, queue, swapQueueDigits, indicesToSwap);

                            // Then, we add back in the two swap we overwrote
                            queue[queueSize][0] = swapCopy[0];
                            queue[queueSize][1] = swapCopy[1];

                            // Now, we append a copy of our longer series of swaps to the end. This will ensure we hit all the possibilities.
                            memcpy(queue + queueSize + 1, queue, (sizeof(step) * 2) * queueSize);

                            // And update the size to be accurate.
                            queueSize = queueSize * 2 + 1;
                        } else {
                            // If not, call the recursive function. This will return how many swaps were added, so add that to the queue size.
                            queueSize += addQueueSwaps(*freqPtr, queueSize, queue, swapQueueDigits, indicesToSwap);
                        }
                        
                    }
                }
            }

            
            // printf("The queue of swaps is: ");
            // for(int i = 0; i < queueSize; i++) {
            //     printf("[%d & %d],", queue[i][0], queue[i][1]);
            // }
            



            // Now we have a matching signature, we begin the process of rotating and looking through alternate swaps.
            // First, we check if this sequence is immediately some rotation away from
            queuePtr = queue;
            while(true) {
                
                // printf("\nPerforming rotations for this sequence: ");
                // for(int j = 0; j < len; j++) printf(j != len - 1 ? "%x," : "%x\n", sequence[j]);

                mpz_set_ui(originalNum, 0);
                getSequenceNumber(sequence, originalNum);
                mpz_set(rotatedNum, originalNum);
                seqPtr = sequence;
                do {
                    // Check if equal to the seed. If so, it was a child.
                    if(mpz_cmp(rotatedNum, seedPtr->number) == 0) {
                        // printf("We got it, it rotated into it.\n");
                        goto continueWhile;
                    }
                    // Otherwise, rotate the sequence
                    mpz_mul_ui(rotatedNum, rotatedNum, NUM_DIGITS);
                    mpz_sub(rotatedNum, rotatedNum, multiplesTable[*(seqPtr++)]);
                    // Check if it is back equal to the original number
                } while (mpz_cmp(rotatedNum, originalNum) != 0);

                // If we're here, it didn't work, so if there is more swaps queued, do them and loop back
                if(queuePtr - queue < queueSize) {
                    // Do the swap, continue
                    swap(sequence, (*queuePtr)[0], (*queuePtr)[1]);
                    queuePtr++;
                } else break; // If no more queued, break
            }
        }

        // New seed!
        printf("New Seed! Num %4d, with signature [", seedListSize + 1);
        for(int j = 0; j < NUM_DIGITS; j++) printf(j != NUM_DIGITS - 1 ? "%2d," : "%2d]: (", signatureNatural[j]);
        for(int j = 0; j < len; j++) printf(j != len - 1 ? "%x," : "%x)\n", seq[j]);
        
        if(seedListSize == seedListCapacity) { // Resize array if necessary
            seedListCapacity *= 2;
            seeds = (seed *)realloc(seeds, sizeof(seed) * seedListCapacity);
        }

        // Set the number value
        seedPtr = seeds + seedListSize;
        seedListSize++;
        mpz_init(seedPtr->number);
        getSequenceNumber(seq, seedPtr->number);

        // Copy the sequence array and the signatures over.
        memcpy(seedPtr->sequence, seq, sizeof(seq));
        memcpy(seedPtr->signature, signatureNatural, sizeof(signatureNatural));
        memcpy(seedPtr->signatureFrequency, sigFreq, sizeof(sigFreq));
        // The seed struct is all set now.

        continueWhile:
        continue;
    }

    // Finish the runtime and print out the result
    clock_t end_time = clock();
    printf("\n---\n\nFinished running in: %f seconds.\n", ((double) (end_time - start_time)) / CLOCKS_PER_SEC);

    // Print out the results
    printf("We found %d unique seeds.\n\n", seedListSize);
    printf("%d sequences were checked.\n", debugCount);
    for(int i = 0; i < seedListSize; i++) {
        printf("Seed num %d: ", i + 1);
        for(int j = 0; j < len; j++) printf(j != len - 1 ? "%x," : "%x\n", seeds[i].sequence[j]);
        printf("Signature: ");
        for(int j = 0; j < NUM_DIGITS; j++) {
            printf(j != NUM_DIGITS - 1 ? "%d," : "%d\n", seeds[i].signature[j]);
        }
        printf("Signature Frequency: ");
        for(int j = 0; j < SIG_FREQ_LIST_LEN; j++) {
            printf(j != SIG_FREQ_LIST_LEN - 1 ? "%d," : "%d\n", seeds[i].signatureFrequency[j]);
        }
        printf("\n");
    }



    

    // Clear all the GMP integers
    mpz_clears(originalNum, rotatedNum, NULL);
    for(int i = 0; i < NUM_DIGITS; i++) {
        mpz_clear(multiplesTable[i]);
    }
    for(int i = 0; i < seedListSize; i++) {
        mpz_clear(seeds[i].number);
    }

    // Free the seeds array
    free(seeds);

    return EXIT_SUCCESS;
}