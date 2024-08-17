/**
 * @file SeedSearch.c
 * @author Joey Hughes
 * Program to be used to analyze output from the main.c file (binary files of grey codes), 
 * and extract all of the seeds present in the sample of grey codes.
 * 
 * This program uses the GMP library, as sequence numbers can get very large, past the 64 bit integer limit.
 * 
 * This is my second attempt at making a program to extract sequences from a set of grey codes. The first SeedSearch
 * worked great, but it was too prohibitively slow to ever get through 5 digits. This uses a different approach,
 * where I load in all of the codes that start with 0102 at once, grab the first code, save it as a seed, and mark
 * all of it's children that start with 0102 for removal in the list, then go again grabbing the next non-marked
 * code as the next seed, doing the same with it's children, every so often removing the marked children from the array.
 * 
 * The input file is to be included in the command line running of the program. 
 * The file must be a binary file of the grey code steps as digit numbers (0,1,2,etc.)
 * for the same NUM_DIGITS as this program was compiled for.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>



/** This defines the macro of how many digits for the grey codes by default to be 0 and break everything. Should usually be set in compilation with -DNUM_DIGITS=X. */
#ifndef NUM_DIGITS
#define NUM_DIGITS 4
#endif

/** The length in steps of a grey code sequence. */
#define len (1 << NUM_DIGITS)

/** 10 for "_seeds.txt", and one for the null character. */
#define OUTPUT_FILE_NAME_EXTRA_CHARS 11



/** This defines a short name for each step's digit number that it's changing. */
typedef unsigned char step;

/** 
 * A sequence, or array of "len" steps. For every sequence, this should start with a 0.
 * Instead of using a separate flag for removing a sequence, I will use this fact and have the first zero
 * be the "removed" flag for each loadedSequence. so if sequence[0] or *sequence is 1, then the sequence is set for removal.
*/
typedef step sequence[len];



/** This keeps track of how many sequences are marked for removal, and is used to decide when to go through and remove the ones that are marked. */
int numberMarkedForRemoval;



/**
 * Performs a swap in a sequence. Goes through the sequence and whenever it sees a, it writes b and when it sees b it writes a.
 * @param seq The sequence array.
 * @param a The first step digit number to swap.
 * @param b The other step digit number to swap.
 * @param limit How many steps to look at. For normal sequences is len.
*/
void swap(step *seq, step a, step b, int limit)
{
    step *seqPtr = seq;
    while(seqPtr - seq < limit) {
        if(*seqPtr == a) *seqPtr = b;
        else if(*seqPtr == b) *seqPtr = a;
        seqPtr++;
    }
}



/**
 * This is a recursive function that will add the necessary swaps to the queue to go through all the possible permutations.
 * This function returns how many total swaps were added to the queue.
 * @param n How many digits to be swapping. 2 is the base case of just one swap, 3 is swapping to get all permutations of 3 digits, etc.
 * @param queueIndexToAddFrom The index in the queue where new swaps should be added.
 * @param queue Reference to the queue array.
 * @param indicesToSwap Essentially the parameters of this function. Which indices of swapQueueDigits to swap. Length of this should equal n.
 * @return The total amount of swaps added to the queue during the calling of this function.
*/
int addQueueSwaps(int n, int queueIndexToAddFrom, step queue[][2], int indicesToSwap[])
{   
    // Base case of n == 2, just one swap
    if(n == 2) {
        queue[queueIndexToAddFrom][0] = indicesToSwap[0];
        queue[queueIndexToAddFrom][1] = indicesToSwap[1];
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
 * This function searches through the list of sequences using binary search and marks the one 
 * with sequenceNum matching the given one for removal.
 * @param seqs The list of pointers to sequences.
 * @param size The current size of the list of sequences.
 * @param seqToMark The sequence to mark.
*/
void markForRemoval(sequence *seqs[], int size, sequence seqToMark)
{
    numberMarkedForRemoval++;
    int low = 0;
    step *inToMark;
    step *inSeqs;
    // I'm saving declaring another variable and just treating size as our high index.
    while(low <= size) {
        int mid = low + ((size - low) >> 1);

        // Get the number at the middle and compare
        inToMark = seqToMark + 4;
        inSeqs = (step *)(*(seqs[mid])) + 4;
        while(*inToMark == *inSeqs) {
            inToMark++;
            if(inToMark - seqToMark == len) {
                (*(seqs[mid]))[0] = 1; // Setting the first step to 1 is our mark for removal.
                return;
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
    printf("Something went wrong, we didn't find it in the binary search.\n");
}



/**
 * Starts the program.
 * @return Exit status.
*/
int main(int argc, char* argv[])
{

    // ----- STAGE 1: INITIALIZATION
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

    // Open an output file
    int inFileNameLen = strlen(argv[1]);
    char outFileName[inFileNameLen + OUTPUT_FILE_NAME_EXTRA_CHARS];
    strcpy(outFileName, argv[1]);
    strcpy(outFileName + inFileNameLen, "_seeds.txt");
    outFileName[inFileNameLen + OUTPUT_FILE_NAME_EXTRA_CHARS - 1] = '\0';
    FILE *out = fopen(outFileName, "w");
    if(out == NULL) {
        printf("Something went wrong creating an output file.\n");
        return EXIT_FAILURE;
    }

    // The resizable array of imported sequences
    int seqListCapacity = 256;
    int seqListSize = 0;
    sequence **seqs = (sequence **)malloc(sizeof(sequence *) * seqListCapacity);

    // The resizable array of seeds
    int seedListCapacity = 256;
    int seedListSize = 0;
    sequence **seeds = (sequence **)malloc(sizeof(sequence *) * seedListCapacity);

    // Create the queue of swaps. It will be d! - 1 swaps long.
    int queueSize = 1;
    for(int i = NUM_DIGITS; i > 1; i--) {
        queueSize *= i;
    }
    step queue[--queueSize][2];
    int indicesToSwap[NUM_DIGITS];
    for(int i = 0; i < NUM_DIGITS; i++) {
        indicesToSwap[i] = i;
    }
    addQueueSwaps(NUM_DIGITS, 0, queue, indicesToSwap);

    // Initialize the global variable
    numberMarkedForRemoval = 0;

    // Variables
    step localSequence[len * 2];                          // Declare the stack variable for testing rotations of sequences, twice the length of a sequence.
    step *zeroPtr;                                        // Pointer in the localSequence that anchors at 0s and works to find 0102 in rotations.
    step *feelerPtr;                                      // The "feeler" pointer that feels ahead in the localSequence to see if the rotation matches 0102.
    sequence **openPtr;                                   // When doing removals, this points to the next open spot in seqs
    sequence **finderPtr;                                 // When doing removals, this goes to the end of seqs to find all the not removed items.
    step (*qPtr)[2];                                      // Pointer to inside the queue.
    bool foundNewSeed;                                    // Used when searching for a new unmarked sequence.

    // ----- STAGE 2: IMPORTING
    // Now we go through each sequence in the input file and read it in into the array of pointers.
    while(true) {

        // Allocate a new sequence in heap
        sequence *newSequence = (sequence *)malloc(sizeof(sequence));

        // Read in the next sequence into this sequence
        if(fread(newSequence, sizeof(step) * len, 1, in) == 0) {
            // Once there are no more sequences to read, we free that useless sequence we just made and break
            free(newSequence);
            break;
        }

        // Make sure the array has room, resize if necessary
        if(seqListSize == seqListCapacity) {
            seqListCapacity *= 2;
            seqs = (sequence **)realloc(seqs, sizeof(sequence *) * seqListCapacity);
        }

        // Add this new sequence to the array
        seqs[seqListSize++] = newSequence;
    }
    printf("\nWe have successfully loaded in %d sequences.\n", seqListSize);

    // Start the runtime timer
    clock_t start_time = clock();

    // ----- STAGE 3: CULLING
    // Now we can loop through, each loop grabbing a seed and culling it's children from the list
    while(seqListSize) {

        // Make sure we have space for a new seed, resize array if necessary
        if(seedListSize == seedListCapacity) {
            seedListCapacity *= 2;
            seeds = (sequence **)realloc(seeds, sizeof(sequence *) * seedListCapacity);
        }

        // Create a spot in our seeds array for our new seed.
        seeds[seedListSize] = (sequence *)malloc(sizeof(sequence));

        // Copy the first sequence (our new seed) into the seeds array, and twice into the localSequence.
        foundNewSeed = false;
        for(int i = 0; i < seqListSize; i++) {
            if(!**(seqs[i])) {
                foundNewSeed = true;
                memcpy(seeds[seedListSize++], seqs[i], sizeof(sequence));
                memcpy(localSequence, seqs[i], sizeof(sequence));
                memcpy(localSequence + len, localSequence, sizeof(sequence));
                break;
            }
        }
        if(!foundNewSeed) {
            printf("Didn't find a new unmarked seed, we are done!\n");
            break;
        }
        

        // printf("The sequence we are analyzing now is: (");
        // for(int j = 0; j < len; j++) printf(j != len - 1 ? "%d," : "%d)\n", localSequence[j]);

        /*
            This part is based on the fact that we only care about children who start with 0102, since all of the
            sequences in the input file should start with that, since (I think) that's what all seeds start with.
            We make two copies of the sequence next to eachother, and when we look at it from some point in the middle
            and go a length across, we are essentially seeing a rotation of the sequence. So we have pointers iterate
            across these rotations and try to find one that starts with 0102. When one is found, the sequence number for
            that rotation is calculated, it is found in the list of sequences, and marked for removal. Once all rotations 
            have been searched through, we know we have found all the rotations of that swapping that start with 0102,
            so we continue to the next swap and do it again until we have exhausted all the swaps and marked all the children.
        */
        // Then, do our fencepost initial rotation test.
        zeroPtr = localSequence;
        feelerPtr = localSequence;
        while(zeroPtr - localSequence < len) {

            // Send out the feeler until it doesn't equal 
            if(*zeroPtr != 0) goto moveForward;
            feelerPtr++;
            if(*feelerPtr != 1) goto moveForward;
            feelerPtr++;
            if(*feelerPtr != 0) goto moveForward;
            feelerPtr++;
            if(*feelerPtr != 2) goto moveForward;
            
            // If we are here, then this has 0102 at the start, we found a relevant child!
            // Calculate the sequence number of this child, and pass it into markForRemoval.
            markForRemoval(seqs, seqListSize, zeroPtr);
            moveForward:
            feelerPtr++;
            zeroPtr = feelerPtr;
        }

        // Now begin the loop of swaps and rotations
        qPtr = queue;
        while(qPtr - queue < queueSize) {

            // Do the next swap on the localSequence
            swap(localSequence, *(*qPtr), (*qPtr)[1], len * 2);
            qPtr++;

            // Now try to find a relevant child.
            zeroPtr = localSequence;
            feelerPtr = localSequence;
            while(zeroPtr - localSequence < len) {

                // Send out the feeler until it doesn't equal 
                if(*zeroPtr != 0) goto moveForward2;
                feelerPtr++;
                if(*feelerPtr != 1) goto moveForward2;
                feelerPtr++;
                if(*feelerPtr != 0) goto moveForward2;
                feelerPtr++;
                if(*feelerPtr != 2) goto moveForward2;
                
                // If we are here, then this has 0102 at the start, we found a relevant child!
                // Calculate the sequence number of this child, and pass it into markForRemoval.
                markForRemoval(seqs, seqListSize, zeroPtr);
                moveForward2:
                feelerPtr++;
                zeroPtr = feelerPtr;
            }
        }

        // Now all of the children of this seed have been marked for removal in the sequence.
        // We can initialize our pointers to the start of the sequences
        if(seqListSize / numberMarkedForRemoval <= 35) {
            openPtr = seqs;
            finderPtr = seqs;
            while(finderPtr - seqs < seqListSize) 

                // If it's not removed, then we copy the sequence pointer stored at finderPtr to the openPtr spot and advance them both
                if(!***finderPtr)
                    *(openPtr++) = *(finderPtr++);
                else 
                    // If it is removed, finderPtr goes ahead and frees it here and moves forward without openPtr.
                    free(*(finderPtr++));
            

            // Update the size of the sequence list.
            seqListSize = openPtr - seqs;
            numberMarkedForRemoval = 0;

            //printf("We just went through a removal. There are now %d sequences left. We have found %d seeds.\n", seqListSize, seedListSize);
        }
        
        // Now what needed to be removed is removed. We can move on to what is left at the start of the array and go again.
    }





    // ----- LAST STAGE: CLEANUP
    // Stop the runtime timer
    clock_t end_time = clock();
    printf("\n---\n\nFinished running in: %f seconds.\n", ((double) (end_time - start_time)) / CLOCKS_PER_SEC);

    // Print the results
    printf("We found a grand total of %d seeds.\n", seedListSize);
    for(int i = 0; i < seedListSize; i++) {

        // Print the sequence raw
        fprintf(out, "(");
        for(int j = 0; j < len; j++) fprintf(out, j != len - 1 ? "%d," : "%d), [", (*(seeds[i]))[j]);

        // Calculate the signature and print it
        char signature[NUM_DIGITS] = {};
        for(int j = 0; j < len; j++) {
            signature[(*(seeds[i]))[j]]++;
        }
        for(int j = 0; j < NUM_DIGITS; j++) fprintf(out, j != NUM_DIGITS - 1 ? "%2d," : "%2d]\n", signature[j]);
    }

    // Free all the sequences.
    for(int i = 0; i < seqListSize; i++) {
        free(seqs[i]);
    }
    free(seqs);
    for(int i = 0; i < seedListSize; i++) {
        free(seeds[i]);
    }
    free(seeds);

    // Close the files
    fclose(in);
    fclose(out);

    return EXIT_SUCCESS;
}

