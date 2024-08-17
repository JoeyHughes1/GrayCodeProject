/**
 * @file GreyCodeChimera.c
 * @author Joey Hughes
 * This is the super optimized method for finding the number of grey codes for n >= 4. It combines main.c, SeedSearch2, and a new 
 * algorithm to extrapolate the seeds, so I've called it chimera, a three headed program to find the number of codes.
 * 
 * UPDATE: The school year is about to start August 2024, and I'm uploading this code to github, so I want to write a few clarifications and instructions.
 * This code used to have the main.c, SeedSearch2, and extrapolation algorithms all happen separately in stages, but now they're kind of mixed in with
 * eachother, so the chimera name doesn't make as much sense anymore, oops.
 * 
 * To run the program, compile with:
 * gcc -Wall -std=c99 -O2 -DNUM_DIGITS=# -DRUNTIME GreyCodeChimera.c -o GreyCodeChimera -lpthread -lgmp
 * The NUM_DIGITS macro will default to 4, but usually you want to set this in compilation with the DNUM_DIGITS=X flag
 * (of course replacing the X with the number of digits, like 4 or 5, really anything >= 4 will work, it will just stop being very useful past 5 or 6 lol.)
 * The -DRUNTIME flag is optional, if you include it then the program outputs runtime information, or basically just how long the program took to run.
 * I always used this flag, but it's technically optional if you want to not use it.
 * Make sure you have the gmp library for big numbers in a place where gcc can link it.
 * O2 is fastest, while O1 is second fastest and O3 ends up the slowest, I think.
 * 
 * I haven't done a deep dive into making sure all the comments are up to date, but most of them should get the idea across. Some of them might be wrong,
 * though, sorry :/.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#ifndef GREY_CODE_TYPES_DEFINED
    #include "GreyCodeTypes.h"
#endif
#include "GMPHashTable.c"
#include "ThreadSequenceCheckers.c"


/* The value of a stepMask that changes the highest order bit. */
#define LAST_DIGIT_STEP (1 << (NUM_DIGITS - 1))

/** The thread count is how many threads are made. */
#define THREAD_COUNT_CODE_SEARCH 5

/** How many threads to split the large extrapolation into. */
#define THREAD_COUNT_EXTRAPOLATION 4

/** This macro does the log base 2 of the given number. Used to go from the step representation to the digit number representation. */
#define log2(x) (__builtin_ctz(x))



/** The struct passed into each of the code searching threads. Also used to return out the final count of seeds and the reference to the seed list. */
typedef struct {
    /** Passing in, how many steps are set. When returned, then number of codes that were found. */
    unsigned long long count;
    /** Only used for passing in. Passes in the steps that are preset and once the last one of these is changed the thread is done. */
    stepMask setSteps[len];
    /** Pointer to the array that holds all the sequence pointers. This is allocated in each thread and accessed by the main thread. */
    seqPtrArray seeds;
    /** Pointer to the function that will be used to check if a sequence is a child of seed from a lower thread. */
    bool (*checkStepsLower)(stepMask *);
    /** How many steps ahead the checkSteps function looks. So that the thread knows how late in the sequence it can call. */
    int stepsBack;
    /** Checks if the steps ahead of the pointer could be swapped to have the same set start as the thread. Always looks forward the num of setSteps - 1 */
    bool (*checkStepsIn)(stepMask *);
    /** Used to pass in a pointer to the queue so it only has to be made once. */
    stepMask (*queueStepPointer)[2];
    /** How long the queue of extra steps is. For some it will be (n-3)! and for others (n-4)! */
    unsigned long long queueLen;
} CodeSearchThreadStruct;

/** The struct passed in to each of the extrapolation threads. Also used to return out the final tally of gray codes. */
typedef struct {
    /** For passing in the list of seeds to extrapolate for. */
    seqPtrArray seeds;
    /** For passing in how many seeds to extrapolate, or how far to go in the array. */
    unsigned long long numSeedsToExtrapolate;
    /** Returns the amount of grey codes extrapolated. */
    unsigned long long numGreyCodes;
    /** Used to pass in a pointer to the queue so it only has to be made once. */
    step (*queueStepPointer)[2];
    /** Used to pass in a pointer to the multiples lookup table so it only has to be made once. */
    mpz_t *multiplesTablePointer;
    /** Simple boolean for if the function should call pthread_exit after it's done or not. */
    bool exitOrNo;
} ExtrapolateThreadStruct;



/** This global variable is generated and written to in main and used by each of the threads so they don't have to all compute it again. */
stepMask lowest[len];

/** n! */
unsigned long long queueSize;



/**
 * (Extrapolating) Calculates the sequence number for the given sequence. Will set rtn to 0 first.
 * @param sequence The sequence to get the number for.
 * @param rtn The GMP integer where the sequence number will be stored.
 * @return The sequence number of the given sequence.
*/
void getSequenceNumber(step *sequence, sequenceNum rtn)
{
    mpz_set_ui(rtn, 0);
    int limit = len;
    while(limit) {
        mpz_mul_ui(rtn, rtn, NUM_DIGITS);
        mpz_add_ui(rtn, rtn, *(sequence++));
        limit--;
    }
}



/**
 * (Extrapolating) Performs a swap in a sequence. Goes through the sequence and whenever it sees a, it writes b and when it sees b it writes a.
 * @param seq The sequence array.
 * @param a The first step digit number to swap.
 * @param b The other step digit number to swap.
 * @param limit How many steps to look at. For normal sequences is len.
*/
void swap(step *seq, step a, step b, int limit)
{
    while(limit) {
        if(*seq == a) *seq = b;
        else if(*seq == b) *seq = a;
        seq++;
        limit--;
    }
}



/**
 * (Seed Searching) Performs a swap in a sequence of stepMasks. Goes through the sequence and whenever it sees a,
 * it writes b and when it sees b it writes a.
 * @param seq The sequence array.
 * @param a The first step digit number to swap.
 * @param b The other step digit number to swap.
 * @param limit How many steps to look at. For normal sequences is len.
*/
void swapMasks(stepMask *seq, stepMask a, stepMask b, int limit)
{
    while(limit) {
        if(*seq == a) *seq = b;
        else if(*seq == b) *seq = a;
        seq++;
        limit--;
    }
}



/**
 * This is a recursive function that will add the necessary swaps to the queue to go through all the possible permutations.
 * This function returns how many total swaps were added to the queue.
 * This function takes in a queue of steps, not stepMasks.
 * @param n How many digits to be swapping. 2 is the base case of just one swap, 3 is swapping to get all permutations of 3 digits, etc.
 * @param queueIndexToAddFrom The index in the queue where new swaps should be added.
 * @param queue Reference to the queue array.
 * @param indicesToSwap Essentially the parameters of this function. Which digit numbers to swap. Length of this should equal n.
 * @return The total amount of swaps added to the queue during the calling of this function.
*/
int addQueueSwaps(int n, int queueIndexToAddFrom, step (*queue)[2], unsigned int indicesToSwap[])
{   
    // Base case of n == 2, just one swap
    if(n == 2) {
        queue[queueIndexToAddFrom][0] = indicesToSwap[0];
        queue[queueIndexToAddFrom][1] = indicesToSwap[1];
        return 1;
    } else if(n <= 1) return 0; // if n less than 2, no swaps.

    // Otherwise, we are n >= 3, so we do recursion
    // Initialize the index to add from to be the one we know is safe
    int currentIndexToAddFrom = queueIndexToAddFrom;

    // Initialize the new parameters to be all the ones we were given but leaving out the last one.
    unsigned int recursiveIndices[n - 1];
    for(int i = 0; i < n - 1; i++) {
        recursiveIndices[i] = indicesToSwap[i];
    }

    // Recurse through all the values in the indices to swap, 
    //    each of them getting a turn being left out of the recursive call, starting with the last one.
    for(int i = n - 1; i > 0; i--) {

        // Do the recursive call, getting all the swaps for one layer down
        currentIndexToAddFrom += addQueueSwaps(n - 1, currentIndexToAddFrom, queue, recursiveIndices);

        // Otherwise, we add a swap from i to i-1
        queue[currentIndexToAddFrom][0] = indicesToSwap[i];
        queue[currentIndexToAddFrom][1] = indicesToSwap[i - 1];
        currentIndexToAddFrom++;

        // Then, switch i and i-1 in the recursive indices so now i is being included again in the swaps the next loop.
        recursiveIndices[i - 1] = indicesToSwap[i];
    }

    // Do one last adding of swaps for when i = 0
    currentIndexToAddFrom += addQueueSwaps(n - 1, currentIndexToAddFrom, queue, recursiveIndices);

    // Now we have added all the swaps we were going to, go ahead and return how many we added
    return (currentIndexToAddFrom - queueIndexToAddFrom);
}



/**
 * This is a recursive function that will add the necessary swaps to the queue to go through all the possible permutations.
 * This function returns how many total swaps were added to the queue.
 * This function takes in a queue of stepMasks, not steps.
 * @param n How many digits to be swapping. 2 is the base case of just one swap, 3 is swapping to get all permutations of 3 digits, etc.
 * @param queueIndexToAddFrom The index in the queue where new swaps should be added.
 * @param queue Reference to the queue array.
 * @param indicesToSwap Essentially the parameters of this function. Which digit numbers to swap. Length of this should equal n.
 * @return The total amount of swaps added to the queue during the calling of this function.
*/
int addQueueSwapsMasks(int n, int queueIndexToAddFrom, stepMask (*queue)[2], unsigned int indicesToSwap[])
{   
    // Base case of n == 2, just one swap
    if(n == 2) {
        queue[queueIndexToAddFrom][0] = indicesToSwap[0];
        queue[queueIndexToAddFrom][1] = indicesToSwap[1];
        return 1;
    } else if(n <= 1) return 0; // if n less than 2, no swaps.

    // Otherwise, we are n >= 3, so we do recursion
    // Initialize the index to add from to be the one we know is safe
    int currentIndexToAddFrom = queueIndexToAddFrom;

    // Initialize the new parameters to be all the ones we were given but leaving out the last one.
    unsigned int recursiveIndices[n - 1];
    for(int i = 0; i < n - 1; i++) {
        recursiveIndices[i] = indicesToSwap[i];
    }

    // Recurse through all the values in the indices to swap, 
    //    each of them getting a turn being left out of the recursive call, starting with the last one.
    for(int i = n - 1; i > 0; i--) {

        // Do the recursive call, getting all the swaps for one layer down
        currentIndexToAddFrom += addQueueSwapsMasks(n - 1, currentIndexToAddFrom, queue, recursiveIndices);

        // Otherwise, we add a swap from i to i-1
        queue[currentIndexToAddFrom][0] = indicesToSwap[i];
        queue[currentIndexToAddFrom][1] = indicesToSwap[i - 1];
        currentIndexToAddFrom++;

        // Then, switch i and i-1 in the recursive indices so now i is being included again in the swaps the next loop.
        recursiveIndices[i - 1] = indicesToSwap[i];
    }

    // Do one last adding of swaps for when i = 0
    currentIndexToAddFrom += addQueueSwapsMasks(n - 1, currentIndexToAddFrom, queue, recursiveIndices);

    // Now we have added all the swaps we were going to, go ahead and return how many we added
    return (currentIndexToAddFrom - queueIndexToAddFrom);
}



/**
 * (Seed Searching) This function searches through the list of sequences using binary search and marks the one 
 * with sequenceNum matching the given one for removal.
 * @param seqs The list of pointers to sequences.
 * @param size The current size of the list of sequences.
 * @param seqToMark The sequence to mark.
*/
bool markForRemoval(sequence *seqs[], int size, sequence seqToMark)
{
    int low = 0;
    step *inToMark;
    step *inSeqs;
    // I'm saving declaring another variable and just treating size as our high index.
    while(low <= size) {
        int mid = low + ((size - low) >> 1);

        // Get the number at the middle and compare
        inToMark = seqToMark + 2;
        inSeqs = (step *)(*(seqs[mid])) + 2;
        while(*inToMark == *inSeqs) {
            inToMark++;
            if(inToMark - seqToMark >= len) {
                (*(seqs[mid]))[0] = 1; // Setting the first step to 1 is our mark for removal.
                return true;
            }
            inSeqs++;
        }

        // If our num is greater, ignore left half, set low up
        if(*inToMark > *inSeqs)
            low = mid + 1;

        // If our num is smaller, ignore right half, set high (size) down
        else
            size = mid - 1;
    }

    // If we reach here, then element was not present
    //printf("Nope.\n");
    return false;
}



/**
 * This function returns true if the first argument, beingTested, is lower in terms
 * of sequence number than the second argument, original. If they are equal, false
 * is returned, as it is not strictly lower.
 * @param beingTested The first sequence.
 * @param original The second sequence.
 * @return True if the first is lower than the second. If true, the second is not a seed.
 */
bool isLower(stepMask *beingTested, stepMask *original)
{
    register int limit = len;
    while(limit && *beingTested == *original) {
        original++;
        beingTested++;
        limit--;
    }
    return limit && *beingTested < *original;
}






/**
 * (Code Search) This function calculates the count of how many grey codes begin with a given start step number. This is used
 * as the function ran by several threads. The parameter is a void pointer that initially points to the step number 
 * to start at, but at the end is used to return the final count for this digit.
 * @param threadVal The pointer to the CodeSearchThreadStruct for this thread. The count variable should hold the specific stepMask for this thread.
 * @return Nothing, but a void * return type is necessary to make the thread. 
*/
void *calculateCodesWithSetStart(void *threadVal)
{
    // Variables
    CodeSearchThreadStruct *threadStruct = ((CodeSearchThreadStruct *)threadVal); // Recasting to a different kind of pointer for convenience.
    stepMask test[len + threadStruct->stepsBack];                              // The step array. This holds steps, which are unsigned characters, that represent which binary digit is being switched during that step.
    stepMask *sptr                  = test;                                    // Pointer to a step within the test array. Used for looping through
    stepMask * const testEnd        = test + len + threadStruct->stepsBack;    // Pointer to just after the test array
    int bffr[len + 1];                                                         // This is the buffer that holds the values generated by actually performing the grey code sequence for testing it.
    int * const buf                 = bffr + 1;                                // Pointer to the "start" of the grey code. Since the first 0 never changes, but is used when calculating values, we start the pointer one val in.
    int *bptr                       = buf;                                     // Pointer to within the buffer. Used for looping through.
    int * const bufEndPtr           = buf + len - 1;                           // Points to the ending 0 in valid grey codes.
    bool flags[len]                 = {};                                      // This keeps track of if a number (the number of the index) has been reached in the sequence yet or not
    int numSetSteps                 = threadStruct->count;                     // The number of set steps for this thread.
    int setStepsCounter;                                                       // Counter used in looping to count to numSetSteps.
    stepMask * const limitStep      = test + numSetSteps - 1;                  // This is which step is being limited, as in when it changes then this thread is done and returns its count.
    step *copyingSeq;                                                          // Pointer to the start of an allocated that is being copied into.
    step *arraySeqPtr;                                                         // Pointer to inside an allocated sequence. Used to copy in the log2s of test.
    stepMask *testCopyPtr;                                                     // Pointer to inside test that is used to copy the log2s of the stepMasks into the allocated sequence.
    bool specialChecks              = threadStruct->checkStepsLower != NULL;   // boolean for if the special checks should be done or not.
    stepMask sequenceCopy[len * 2];                                            // Array to hold the sequence twice to do rotations.
    stepMask *permPtr;                                                         // Pointer in the test for seeds that finds relevant rotations (Starting with 0)
    stepMask (*qPtr)[2];                                                       // Pointer to inside the queue.
    

    // Allocate the array that the seeds will be held in
    int seedListCapacity = 256;            // The total capacity of the array. If this is reached, the array is reallocated.
    int seedListSize = 0;                  // The size of the array, as in the actual number of elements held, and the number of sequences found.
    threadStruct->seeds = (sequence **)malloc(sizeof(sequence *) * seedListCapacity);

    // The first value of the buffer is an unchanging 0
    bffr[0] = 0;

    // Initialize the test to the set steps, then the lowest array after that.
    stepMask limitStepValue = threadStruct->setSteps[numSetSteps - 1];
    step limitStepLog = log2(limitStepValue);
    memcpy(test, threadStruct->setSteps, sizeof(stepMask) * numSetSteps);
    memcpy(test + numSetSteps, lowest, (len - numSetSteps) * sizeof(stepMask));
    memcpy(test + len, threadStruct->setSteps, sizeof(stepMask) * (threadStruct->stepsBack));

    // I can start the loop already past the set start to eliminate the need to check 
    //    if we are far enough in the test array to do the backwards thread checks
    while(sptr - test < numSetSteps) {

        // Calculate the next number in the sequence.
        *bptr = *(bptr - 1) ^ (*sptr);

        // It's guaranteed valid, mark the number
        flags[*bptr] = true;

        // Go to the next step
        sptr++;
        bptr++;
    }
    
    // Loop until the limit step is changed from the starting value. Then we know we have all the codes with that set step.
    while(*limitStep == limitStepValue) {

        // -- Checking if it is a valid grey code sequence.
        // For every changed value in the steps, until reaching a 0.
        while(true) {
            
            // Do the special thread-specific check.
            if(specialChecks && (threadStruct->checkStepsLower(sptr))) goto increment;

            // Calculate the next number in the sequence.
            *bptr = *(bptr - 1) ^ (*sptr);

            // If it has reached 0, before the end: invalid, at the end: valid.
            if(!*bptr) {
                if(bptr < bufEndPtr) goto increment; else break; // If before the end, invalid, otherwise, valid.
            }

            // Check if this number has already been reached.
            if(flags[*bptr]) goto increment;

            // If it has passed all that, it's valid, mark the number as reached.
            flags[*bptr] = true;

            // Go to the next step
            sptr++;
            bptr++;
        }
        
        // 0 has been reached, there were no duplicates and it's the right length, so we have reached a valid grey code.
        // If we are a special thread, do an extra check. There could be something that reaches across a rotation.
        if(specialChecks) {
            testCopyPtr = sptr;
            while(testCopyPtr < testEnd)
                if(threadStruct->checkStepsLower(testCopyPtr++))
                    goto skipAdding;
        }
        
        // Check if it is has a lower permutation. If so, skip adding it as it is not a seed.
        memcpy(sequenceCopy, test, sizeof(stepMask) * len);
        memcpy(sequenceCopy + len, sequenceCopy, sizeof(stepMask) * len); // Copy again into the second half
        permPtr = sequenceCopy;
        while(permPtr - sequenceCopy < len) {

            // Go through rotations until finding a start that could be swapped
            if(!threadStruct->checkStepsIn(permPtr)) {
                permPtr++;
                continue;
            }

            // Now swap until it matches the threads set start
            for(setStepsCounter = 0; setStepsCounter < numSetSteps; setStepsCounter++)
                if(permPtr[setStepsCounter] != threadStruct->setSteps[setStepsCounter])
                    swapMasks(sequenceCopy, permPtr[setStepsCounter], threadStruct->setSteps[setStepsCounter], len * 2);
            
            // Now it matches the start of the thread. Check if it is lower.
            if(isLower(permPtr, test)) goto skipAdding;

            // Do all the extra swaps we have to do (if any)
            qPtr = threadStruct->queueStepPointer + 1;
            while(qPtr - threadStruct->queueStepPointer < threadStruct->queueLen) {
                // We have an extra swap to do. Do the swap, then check if it's lower
                swapMasks(sequenceCopy, (*qPtr)[0], (*qPtr)[1], len * 2);
                if(isLower(permPtr, test)) goto skipAdding;
                qPtr++; // Increment
            }

            permPtr++; // Increment
        }

        // If here, then it is officially a new seed.
        // Make sure the array has room, resize if necessary
        if(seedListSize == seedListCapacity) {
            seedListCapacity *= 2;
            threadStruct->seeds = (seqPtrArray)realloc(threadStruct->seeds, sizeof(sequence *) * seedListCapacity);
        }

        // Now allocate for the seed and copy it over
        threadStruct->seeds[seedListSize] = (sequence *)malloc(sizeof(sequence));
        testCopyPtr = test;
        copyingSeq = *(threadStruct->seeds[seedListSize]);
        for(arraySeqPtr = copyingSeq; arraySeqPtr - copyingSeq < len; arraySeqPtr++, testCopyPtr++)
            *arraySeqPtr = log2(*testCopyPtr);
        
        // A new seed has been added to the array, increase the size.
        seedListSize++;

        #if NUM_DIGITS == 6
        // For printing out the number of seeds intermittently in 6 digits
        if((numSetSteps == 5 && 
                    ((limitStepLog == 0 && (seedListSize & 0x7FFFF) == 0) ||
                    (limitStepLog == 1 && (seedListSize & 0x7FFFF) == 0) ||
                    (limitStepLog == 3 && (seedListSize & 0x01FF) == 0)
                    )) 
                || (numSetSteps == 4 &&
                    ((limitStepLog == 0 && (seedListSize & 0x0FF) == 0) ||
                    (limitStepLog == 3 && (seedListSize & 0x07F) == 0)
                    ))) {

            printf("Thr[%d,%d]Seed:%10d: ", numSetSteps, limitStepLog, seedListSize); 
            for(int j = 0; j < len; j++) printf(j != len - 1 ? "%d," : "%d\n", copyingSeq[j]);
        }
        #endif
            

        // Now to increment. The last three steps in a cyclical grey code are forced, so we will start incrementing three steps back
        //    That means that we can just step the pointers back three steps and let the below incrementing take over
        //    The 0 flag is never actually set to true, so it doesn't need to be reset.
        skipAdding:
        sptr -= 3;   
        flags[*(--bptr)] = false;
        flags[*(--bptr)] = false;
        flags[*(--bptr)] = false;


        increment:
        // -- We have encountered an invalid sequence, or are incrementing from a valid one.
        // Increment from the step that needs to be changed.
        while((*sptr) & LAST_DIGIT_STEP) {
            sptr--;
            flags[*(--bptr)] = false;
        }
        (*sptr) <<= 1;

        // If we have incremented it to the same as the previous, increment it again.
        if(*sptr == *(sptr - 1)) goto increment;

        // Copy from lowest to the rest of the steps after the last incremented one.
        memcpy(sptr + 1, lowest, (len - 1 - (sptr - test)) * sizeof(stepMask));
        
    }

    // Print the final statistics
    printf(" ---- Seeds found was %d with %d digits in thread [%d,%d]. \n\n", seedListSize, NUM_DIGITS, numSetSteps, limitStepLog);

    // Return the number of seeds found.
    threadStruct->count = seedListSize;

    // Exit the thread
    pthread_exit(NULL);
    return NULL;
}



/**
 * This is a thread function for extrapolating seeds. Takes in a pointer to a ExtrapolateThreadStruct which has the list
 * of seeds to extrapolate from and how many to extrapolate, and passes out through it the final tally.
 * @param threadVal The ExtrapolateThreadStruct.
 * @return Nothing, but a void * return type is necessary to make the thread.
*/
void *extrapolateSeeds(void *threadVal)
{

    // Variables
    ExtrapolateThreadStruct *threadStruct = ((ExtrapolateThreadStruct *)threadVal); // Recasting for convenience
    sequence localSequence;                                                  // A place on the stack to hold a sequence
    step *stepPtr;                                                           // Pointer to inside the localSequence
    GMPHashTable *uniquePermutations = createGMPTable((queueSize * 2) + 1);  // The hash table of unique permutations for the seed, size (3 * n!) + 1.
    sequenceNum originalRotation;                                            // The original sequence number after swapping before doing rotations.
    sequenceNum currentRotation;                                             // The variable to do the rotation calculations
    sequence **seedPtr;                                                      // Pointer to inside the seeds list.
    step (*qPtr)[2];                                                         // Pointer to inside the queue
    unsigned long long numGreyCodes  = 0;                                    // The final tally of how many grey codes there are.
    bool rotationallySymmetric;                                              // Whether or not the seed is rotationally symmetric (half as many rotations per permutation)
    const int numHalves              = __builtin_ctz(queueSize) + 1;         // The number of times n! can be evenly halved plus one.
    int halves[numHalves];                                                   // The list of the halves of n!, the number of possible unique permutations.
    int *currentNextHalf;                                                    // Pointer to the next half to be reached in the halves list.

    // Initialize the sequenceNums
    mpz_init(currentRotation);
    mpz_init(originalRotation);

    // Initialize the halves list
    //    This list will have all of the clean halves of itself plus itself normally at the end.
    //    For example, if the queueSize was 24, then the array would have (3, 6, 12, 24)
    for(int i = 0; i < numHalves; i++) {
        halves[i] = (queueSize >> (numHalves - 1 - i));
    }

    // For each seed
    seedPtr = threadStruct->seeds;
    while(seedPtr - threadStruct->seeds < threadStruct->numSeedsToExtrapolate) {

        // openPtr is pointing to the seed to analyze, copy it into localSequence
        memcpy(localSequence, *(*(seedPtr++)), sizeof(sequence));

        // Check for rotational symmetry
        rotationallySymmetric = memcmp(localSequence, localSequence + (len/2), sizeof(step) * len/2) == 0;

        // Clear the hashTable
        emptyGMPTable(uniquePermutations);

        // Reset the half tracker
        currentNextHalf = halves;
        // printf("With the sequence: ");
        // for(int j = 0; j < len; j++) printf(j != len - 1 ? "%d," : "%d\n", localSequence[j]);

        // Enter the loop of swaps
        qPtr = threadStruct->queueStepPointer;
        while(qPtr - threadStruct->queueStepPointer < queueSize) {

            // Do the next swap on the localSequence
            swap(localSequence, *(*qPtr), (*qPtr)[1], len);
            qPtr++;

            // Calculate it's number in originalRotation, copy to currentRotation
            getSequenceNumber(localSequence, originalRotation);
            mpz_set(currentRotation, originalRotation);

            // Do rotations until the number matches one of the previous permutations or until we have exhausted all the rotations
            stepPtr = localSequence;
            do {
                // Check if this rotation is equal to any of the unique permutations
                //    If we find one that is equal, then this permutation is not unique
                if(GMPHashContains(uniquePermutations, currentRotation)) goto notUniquePermutation;

                // Otherwise, rotate the sequence
                mpz_mul_ui(currentRotation, currentRotation, NUM_DIGITS);
                mpz_sub(currentRotation, currentRotation, threadStruct->multiplesTablePointer[*(stepPtr++)]);
            } while(mpz_cmp(currentRotation, originalRotation) != 0);

            // If none of this swap's rotations matched the previous permutations, then it is a unique permutation,
            //    add it officially to the list.
            GMPHashInsert(uniquePermutations, originalRotation);

            // If it matched one of the rotations, then this swap is the same as a previous, we do not need to add it.
            notUniquePermutation:
            if(uniquePermutations->count < *currentNextHalf) continue;
            // If we have reached a half milestone or gone past.
            // If blown past the half, we don't need to check anymore, increase.
            if(uniquePermutations->count > *currentNextHalf) {
                currentNextHalf++;

                // If we are past half of the possible unique permutations, we know we will get all of them.
                if(*currentNextHalf == queueSize) {
                    uniquePermutations->count = queueSize;
                    break;
                }
            } else 
                // If we are on that half value, to get to the next we need to double, but if
                //    the number of permutations left to check is less than that, we can't reach the next,
                //    so we know they must all be not unique, we can skip them all.
                if(*currentNextHalf > queueSize - (qPtr - threadStruct->queueStepPointer))
                    break;
                
        }

        // Now we should have added all the rotations from the unique permutations to the total count, we go onto the next seed.
        numGreyCodes += uniquePermutations->count * (rotationallySymmetric ? len/2 : len);
    }

    // Print the results
    printf(" ---------- The number of codes extrapolated from this thread was %lld.\n", numGreyCodes);

    // Free stuff that needs to be free
    freeGMPTable(uniquePermutations);
    mpz_clear(originalRotation);
    mpz_clear(currentRotation);

    // Output the number of grey codes
    threadStruct->numGreyCodes = numGreyCodes;

    // Exit
    if(threadStruct->exitOrNo) pthread_exit(NULL);
    return NULL;
}



/**
 * Starts the program.
 * @return Exit status.
*/
int main() 
{
    // ----- STAGE 1: INITIALIZATION
    // Print opening empty line
    printf("\n");

    #ifdef RUNTIME
    // Get starting benchmark time
    clock_t start_time = clock();
    #endif


    // First, calculate the "lowest" grey code, the 01020103...
    lowest[0] = 1;
    stepMask *init = lowest + 1;
    for(int i = 0; i < NUM_DIGITS; i++) {

        // Copy the previous block over
        memcpy(init, lowest, (init - lowest) * sizeof(stepMask));

        // Move forward almost to the end
        init += init - lowest - 1;

        // Increment the last number and move to the end
        *init <<= 1;
        init++;
    }
    lowest[len - 1] >>= 1; // The last number will be over, so decrement it to get the the "first" grey code.


    // Create the queue of swaps (to get through all swap permutations of each seed), n! swaps long (The first being a non-swap).
    queueSize = 1;
    for(int i = NUM_DIGITS; i > 1; i--) 
        queueSize *= i;
    step queue[queueSize][2];
    queue[0][0] = 99;
    queue[0][1] = 99;
    unsigned int indicesToSwap[NUM_DIGITS];
    for(int i = 0; i < NUM_DIGITS; i++) 
        indicesToSwap[i] = i;
    addQueueSwaps(NUM_DIGITS, 1, queue, indicesToSwap);

    // Create two queues of swaps, one for n-3 digits, one for n-4 digits. Used to swap the non-set digits when 
    //    finding relevant children in code and seed searching.
    unsigned long long nMinus3Factorial = 1;
    unsigned long long nMinus4Factorial = 1;
    for(int i = NUM_DIGITS - 3; i > 1; i--)
        nMinus3Factorial *= i;
    for(int i = NUM_DIGITS - 4; i > 1; i--)
        nMinus4Factorial *= i;
    
    stepMask queueMinus3[nMinus3Factorial][2];
    stepMask queueMinus4[nMinus4Factorial][2];
    // The initial useless swaps
    queueMinus3[0][0] = 99;
    queueMinus3[0][1] = 99;
    queueMinus4[0][0] = 99;
    queueMinus4[0][1] = 99;

    // Add the extra swaps, if they are needed.
    if(NUM_DIGITS > 4) {
        unsigned int indicesToSwapMinus3[NUM_DIGITS - 3];
        for(int i = NUM_DIGITS - 1; i >= 3; i--)
            indicesToSwapMinus3[NUM_DIGITS - 1 - i] = (1 << i);
        addQueueSwapsMasks(NUM_DIGITS - 3, 1, queueMinus3, indicesToSwapMinus3);
    }
    if(NUM_DIGITS > 5) {
        unsigned int indicesToSwapMinus4[NUM_DIGITS - 4];
        for(int i = NUM_DIGITS - 1; i >= 4; i--) 
            indicesToSwapMinus4[NUM_DIGITS - 1 - i] = (1 << i);
        addQueueSwapsMasks(NUM_DIGITS - 4, 1, queueMinus4, indicesToSwapMinus4);
    }
    
    
    
    



    // Create the quick rotation lookup table
    /*  I got to a nice formula for calculating rotations. Where N is the sequence number and n is the number of digits, and
        f is the first step in the sequence, the next rotation (moving the first step to the end) can be calculated by doing:
        N*n - f( n^(2^n) - 1 ). Everything in the parentheses is based on constants, so we can calculate that and all the possible
        multiples of that with f in a table, then just do a multiplication and subtract a value from the table based on f.
    */
    mpz_t multiplesTable[NUM_DIGITS];
    mpz_init(multiplesTable[0]);
    mpz_init_set_ui(multiplesTable[1], 1);
    // We perform the exponentiation by n here (len is 2^n)
    for(int i = 0; i < len; i++) {
        mpz_mul_ui(multiplesTable[1], multiplesTable[1], NUM_DIGITS);
    }
    mpz_sub_ui(multiplesTable[1], multiplesTable[1], 1); // Subtracting one we now we have n^(2^n) - 1 in index 1
    // We fill in the rest of the table by multiplying with index 1.
    for(int i = 2; i < NUM_DIGITS; i++) {
        mpz_init(multiplesTable[i]);
        mpz_mul_ui(multiplesTable[i], multiplesTable[1], i);
    }



    // ----- STAGE 2: CODE AND SEED SEARCHING
    /* This stage involves using the main.c algorithm to find all of the grey codes starting with either
    01020, 01021, or 01023 using three different threads. The threads are created and passed in a struct with their initial
    step values, then the thread exits after filling the struct with how many codes it found, and an allocated array of
    pointers to allocated sequnces (which are arrays of steps)
    */

    // Total count for all the threads. This is out final answer.
    unsigned long long totalNumGreyCodes = 0;
    unsigned long long totalNumSeeds = 0;

    // Array of the thread ids
    pthread_t threadIds[THREAD_COUNT_CODE_SEARCH];

    // Initialize the extrapolate val's unchanging references.
    ExtrapolateThreadStruct extrapolateVal;
    extrapolateVal.multiplesTablePointer = multiplesTable;
    extrapolateVal.queueStepPointer = queue;
    extrapolateVal.exitOrNo = false;

    // Array of all the initial steps
    CodeSearchThreadStruct threadVals[THREAD_COUNT_CODE_SEARCH];

    // Create all the initial step values that will be used
    threadVals[0] = (CodeSearchThreadStruct){5, {1,2,1,4,1}, NULL, NULL, 0, checkStepsIn01020, queueMinus3, nMinus3Factorial};
    threadVals[1] = (CodeSearchThreadStruct){5, {1,2,1,4,2}, NULL, checkStepsForLower01021, 4, checkStepsIn01021, queueMinus3, nMinus3Factorial};
    threadVals[2] = (CodeSearchThreadStruct){5, {1,2,1,4,8}, NULL, checkStepsForLower01023, 4, checkStepsIn01023, queueMinus4, nMinus4Factorial};
    threadVals[3] = (CodeSearchThreadStruct){4, {1,2,4,1}, NULL, checkStepsForLower0120, 2, checkStepsIn0120, queueMinus3, nMinus3Factorial};
    threadVals[4] = (CodeSearchThreadStruct){4, {1,2,4,8}, NULL, checkStepsForLower0123, 3, checkStepsIn0123, queueMinus4, nMinus4Factorial};

    // Generate the threads and pass in the value.

    for(int i = 0; i < THREAD_COUNT_CODE_SEARCH; i++) {
        //if(i == 0) pthread_create(threadIds + i, NULL, &calculateCodesWithSetStart, (void *)(threadVals + i));
        pthread_create(threadIds + i, NULL, &calculateCodesWithSetStart, (void *)(threadVals + i));
    }

    // Wait for all the earlier threads and add up their totals.
    for(int i = THREAD_COUNT_CODE_SEARCH - 1; i; i--) {

        // Wait for it to finish
        pthread_join(threadIds[i], NULL);
        totalNumSeeds += threadVals[i].count;

        // Initialize the struct
        extrapolateVal.seeds = threadVals[i].seeds;
        extrapolateVal.numSeedsToExtrapolate = threadVals[i].count;

        // Extrapolate this thread's seeds
        extrapolateSeeds((void *)&extrapolateVal);

        // Add it's count to the total
        totalNumGreyCodes += extrapolateVal.numGreyCodes;

        // Free the seeds and the seed list
        for(int j = 0; j < threadVals[i].count; j++)
            free(threadVals[i].seeds[j]);
        free(threadVals[i].seeds);
    }

    // Wait for the long thread
    pthread_join(threadIds[0], NULL);

    // Add it's count
    totalNumSeeds += threadVals[0].count;

    // Print message of how many were found
    printf(" ---------- The num of seeds found total for %d digits was: \e[31m%lld\e[0m\n", NUM_DIGITS, totalNumSeeds);
    printf(" ---------- Using the upper bound, we can say that the number of codes is close to but less than \e[33m%lld\e[0m.\n", 
        (unsigned long long)(((double)totalNumSeeds - (double)3/4) * queueSize * len));

    #ifdef RUNTIME
    // Get end benchmarking time
    clock_t end_time = clock();
    printf("\n-- Got here in %f seconds.\n", ((double) (end_time - start_time)) / CLOCKS_PER_SEC );
    #endif


    // ----- STAGE 3: FINAL EXTRAPOLATION

    #ifdef RUNTIME
    // Get end benchmarking time
    clock_t segment_start_time = clock();
    #endif

    // Update message
    printf("\n ------- Beginning the seed extrapolating...\n");

    // Variables
    int seedsPerThread = threadVals[0].count / THREAD_COUNT_EXTRAPOLATION;  // The amount of seeds per thread

    // Array of the thread ids
    pthread_t extrapolateThreadIds[THREAD_COUNT_EXTRAPOLATION];

    // Array of all the values
    ExtrapolateThreadStruct extrapolateThreadVals[THREAD_COUNT_EXTRAPOLATION];

    // Initialize all the values.
    for(int i = 0; i < THREAD_COUNT_EXTRAPOLATION; i++) {
        // Each one gets a different start in the seeds array
        extrapolateThreadVals[i].seeds = threadVals[0].seeds + (seedsPerThread * i);

        // The first ones get their fraction, the last does the rest.
        if(i != THREAD_COUNT_EXTRAPOLATION - 1) 
            extrapolateThreadVals[i].numSeedsToExtrapolate = seedsPerThread;
        else 
            extrapolateThreadVals[i].numSeedsToExtrapolate = threadVals[0].count - (seedsPerThread * 3);
        
        // Pass in the list references it needs
        extrapolateThreadVals[i].queueStepPointer = queue;
        extrapolateThreadVals[i].multiplesTablePointer = multiplesTable;

        // Make it exit at the end
        extrapolateThreadVals[i].exitOrNo = true;
    }

    // Start each of the threads
    for(int i = 0; i < THREAD_COUNT_EXTRAPOLATION; i++) {
        pthread_create(extrapolateThreadIds + i, NULL, &extrapolateSeeds, (void *)(extrapolateThreadVals + i));
    }

    // Wait for all the threads, add up their totals
    for(int i = 0; i < THREAD_COUNT_EXTRAPOLATION; i++) {
        pthread_join(extrapolateThreadIds[i], NULL);
        totalNumGreyCodes += extrapolateThreadVals[i].numGreyCodes;
    }

    printf("\n ---------- The number of grey codes with %d digits is \e[31m%lld\e[0m.", NUM_DIGITS, totalNumGreyCodes);


    #ifdef RUNTIME
    // Get end benchmarking time
    end_time = clock();
    printf("\n-- This segment took %f seconds.\n", ((double) (end_time - segment_start_time)) / CLOCKS_PER_SEC );
    printf("-- This run took %f seconds.\n", ((double) (end_time - start_time)) / CLOCKS_PER_SEC );
    #endif

    // ----- FINAL STAGE: CLOSING
    // Free all of the seeds and the list itself
    for(int i = 0; i < threadVals[0].count; i++) {
        free(threadVals[0].seeds[i]);
    }
    free(threadVals[0].seeds);
    

    // Free the quick rotation lookup table
    for(int i = 0; i < NUM_DIGITS; i++) {
        mpz_clear(multiplesTable[i]);
    }

    // Exit, success!!
    pthread_exit(NULL);
    return EXIT_SUCCESS;
}

