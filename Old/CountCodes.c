/**
 * @file main.c
 * @author Joey Hughes
 * Thie file uses a backtracking algorithm and multithreading to count how many grey codes there are for a certain number of binary digits.
 * 
 * In it's current state, this program runs under one mathematical assumption that I suspect to be true but haven't proven: 
 * I think it's likely that for a certain number of digits, it is true that for grey codes of n digits (counting digits from 1 to n), the number of working codes that
 * start with flipping digit 1 is equal to the number of working codes that start with flipping digit 2, and so on. The number of codes
 * that start with flipping each digit is equal. Thus, this program only checks for codes that start with flipping digit 1, and multiplies by
 * the number of digits to arrive at a tentative value for how many codes there are.
 * 
 * I have ran this program without this assumption for n=2, 3, 4, and 5 and reached the true, no assumptions value, and they all match this property.
 * I suspect that this holds for all digits. If the goal is just to find the count, then this works great. If wanting to use this to
 * actually output every grey code, see the compilation options below. Try -DFILE_OUTPUT and -DCHECK_EVERYTHING.
 * 
 * The overall strategy of this program is as follows: a grey code is represented by an array of step (unsigned int) values.
 * A buffer holds each step along the grey code. The code starts at 0, then the first step is applied to that 0 to get the next value.
 * Then the next step is applied to that resulting value to get the next value. This is continued until the code finishes with a 0, in which case it is valid, or
 * until it repeats a number or reaches 0 prematurely. In that case the grey code is invalid. In either case, the code is "incremented".
 * 
 * The step values are actually powers of two, represented in binary as 0..0001, 0..0010, 0..0100, etc. These values are XOR'd to the previous value in the sequence to flip just one digit
 * (the one with the 1). But, the steps can instead be more easily thought of as numbers from 0 to n-1 of which digit to flip (0-based). (0 = 0..001, 1 = 0..010, etc.)
 * So, a sequence for 3 digits might be 0,1,0,2,0,1,0,2.
 * An "incrementation" is performed by treating this sequence of values as a base-n number, so in the example 01020102 (base 3), and literally incrementing it. In this case, that
 * would result in the number 01020110, or the sequence 0,1,0,2,0,1,1,0. This then loops back around to the top and this new sequence is checked.
 * 
 * For valid codes, it is incremented exactly as that, but for invalid codes, the code is not incremented from the end, but instead from whichever point in the sequence the code became invalid.
 * This works like this: following from the previous example, the code 0,1,0,2,0,1,1,0 is not valid. The point at which it becomes invalid is the point when there are two 1s in a row:
 * 0,1,0,2,0,1,1 <-,0. No amount of changing the steps after this point will result in a valid code, since the code is already invalid at this point.
 * So, the code is incremented to 0,1,0,2,0,1,2 <-,0, with that repeated 1 being incremented to a 2. Then, the rest of the code has to be reset to it's lowest possible state. 
 * If we didn't, we could skip some valid codes.
 * Instead of setting the rest of the code to 0s though, which would, if we set more than one 0, create a code with repeated 0s and which isn't valid, 
 * we copy from the "lowest" possible sequence of numbers that might work. This is the sequence that goes:
 * 0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,... and so on. This sequence is, I'm pretty sure (unproven), the "lowest" sequence that could possibly work as a grey code, since it
 * doesn't repeat any digits or sequence of digits twice in a row (since that would flip some bits, then immediately flip them back, necessarily causing a repeat.). 
 * This "lowest" sequence is copied into the steps from after the incremented digit to the end. So after the last 2 in 0,1,0,2,0,1,2,0. In this case, that would just replace that last 0 with another 0.
 * 
 * For a more enlightening example, imagine we just determined that 0,2,1,1,2,1,0,1 is not valid. First, we increment from the point at which it becomes invalid, 
 * 0,2,1,1 <-,2,1,0,1, which turns it into
 * 0,2,1,2 <-,2,1,0,1. Then, we copy from the "lowest" sequence into the rest, replacing the last 2,1,0,1 with 0,1,0,2, making the sequence into
 * 0,2,1,2 <-,0,1,0,2.
 * 
 * This whole process skips from the invalid code we had to the next sequence that could possibly have a chance at being valid.
 * 
 * So, essentially, we are searching from, for 3 digits in base 3 for example, 00000000 to 22222222, and checking which of those base 3 numbers makes a valid grey code sequence, but skipping lots of
 * codes that we know wouldn't work.
 * 
 * There are also other optimizations. First, the sequence is checked for repeats using an auxillary array where each index is a boolean value of whether that index number has appeared in the sequence yet.
 * If a number is reached whose value in the array is false, it is a new number, so that value is set to true. Otherwise, if that value is already true, then that means the number the code
 * just reached had already appeared, so the code is not valid, as there was a repeat number.
 * 
 * Along with that, the whole code isn't recalculated each time, only the part of the code from the last changed step onward is recalculated. This means the flags have to be changed with incrementation
 * as well as the steps. The whole thing is in careful balance where the flags are correct up to the point where the sequence is being recalculated.
 * 
 * Along with that, based on the assumption I described earlier, only codes starting with changing digit 0 (codes where the first step is 0...01) are checked, and the number of codes with this restriction are
 * multiplied by the number of digits to get the total number of codes for the number of digits. In the 3 digit example, this means we are only checking from 00000000 to 02222222.
 * 
 * But it's not just that, we know that 00000000 would never work as a grey code, it's literally repeated 0s. So, we use the "lowest" sequence with the last digit subtracted by one 
 * (to stay within the number of digits and get back to 0 at the end) as the first sequence, since, I posit, there are no codes less than that that are valid. In the 3 digit example, 
 * that means the first code checked is 01020102.
 * My proposition is that there are no valid codes in 3 digits between 00000000 and 01020102, and similar for all digit numbers. So, the search is actually searching from 01020102 to 02222222.
 * 
 * (1) The lowest array is constructed as such: Starting with a 0, copying it to 00, incrementing the last 0 to a 1 to get 01, then copying that whole thing over to get 0101,
 *     then incrementing the last to get 0102, then repeating, getting 01020102 -> 01020103, etc. until lowest is equal to the length it needs to be.
 *     After that, the last number will be one too high, so we decrement it. For 3 digits for example, it will be 01020103 at that point, where 3 means changing
 *     the 4th digit, but there is no 4th digit, so we decrement it so it becomes changing the 3rd digit. This algorithm will, I theorize, always give the lowest
 *     possible valid grey code. 
 * 
 * I also use multithreading in this program to check codes a lot faster. This is used like this: each first step after the initial 0 is fixed in a different thread.
 * So, for 3 digits, this means there are two other threads (not three since 00xxxxxx would never have any codes) that are checking from
 * 01xxxxxx (01020102 to 01222222), and
 * 02xxxxxx (02010201 to 02222222) at the same time. These ranges are distinct, so they won't double count anything, and added together they account for every possible valid code starting with step 0.
 * After these threads are done running, their counts are added up into the total number of codes for starting step 0. This is multiplied by n to get the total number of codes for n digits.
 *  
 * In summary, this program efficiently counts the number of grey codes for a certain number of digits using a backtracking algorithm approach.
 * 
 * To use this program, compile with the following options:
 * -O3                : This optimization flag doesn't affect the accuracy and allows the program to run the fastest.
 * -lpthread          : This links the program to the POSIX thread library.
 * -DNUM_DIGITS=n     : Replace n with how many digits to search for grey codes in. If this is omitted, the program defaults to searching in 4 digits.
 * -DRUNTIME          : This was mostly used in creating the program. This will print out the runtime of the program at the end of its execution.
 * -DFILE_OUTPUT      : This will activate the file output functionality. This will cause the program to print out all the valid codes it found to files. A different file for each thread.
 * -DCHECK_EVERYTHING : This will signal the program to not use the assumption and to instead check for every single grey code. This can be combined with -DFILE_OUTPUT to output every single grey code to file.
 * 
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>



/** This defines the macro of how many digits for the grey code search by default to be 4. Should usually be set in compilation with -DNUM_DIGITS=X */
#ifndef NUM_DIGITS
/** The number of digits. How many digits to have in the search for grey code sequences. */
#define NUM_DIGITS 4
#endif


// This sets a few output file macros.
#ifdef FILE_OUTPUT
/** This macro generates an output file name based on the parameter. */
#define FILENAME(x) #x "Digits_N.txt"

/** This calls the other output filename macro, such that NUM_DIGITS can be passed as a parameter into this and the value of how many digits is used as the parameter for FILENAME. */
#define OUTFILE(x) FILENAME(x)

/** 13 for xDigits_N.txt, one for the null terminator. Note that this assumes the number of digits is less than 10. */
#define OUTFILE_NAME_LENGTH 14

/** This is the index of the character in the outfile name string that corresponds to the N, which is changed by each thread. */
#define OUTFILE_NAME_THREAD_NUM_INDEX 8
#endif


/** This calculates the length of the sequence of steps for each code at compile time and stores it in len. */
#define len (1 << NUM_DIGITS)


// This sets the value of skippedThreads based on if CHECK_EVERYTHING is set or not.
#ifdef CHECK_EVERYTHING 
/* This is how many step values, from 0 up, to skip and not make threads for. */
#define skippedThreads 0
#else
/* This is how many step values, from 0 up, to skip and not make threads for. */
#define skippedThreads 1
#endif


/* The value of a step that changes the highest order bit. */
#define LAST_DIGIT_STEP (1 << (NUM_DIGITS - 1))


/** This defines a short name for a simple byte that stores which digit is being changed in this step. */
typedef unsigned int step;


/** This global variable is generated and written to in main and used by each of the threads so they don't have to all compute it again. */
step lowest[len];



/**
 * This function calculates the count of how many grey codes begin with a given start step number. This is used
 * as the function ran by several threads. The parameter is a void pointer that initially points to the step number 
 * to start at, but at the end is used to return the final count for this digit.
 * @param threadVal This is a pointer to what step value this thread is responsible for. This is passed in using pthread_create.
 * @return NULL. This is necesarry to have a matching pointer in pthread_create() but I don't use this return value.
 * The real return value of the final count is returned by changing the value stored at the threadVal pointer.
*/
void *calculateCodesWithSetStart(void *threadVal)
{

    // -- Create all local variables first.

    // Dereference the threadVal immediately, I'm not sure if there might be issues with multiple threads accessing a pointer to a similar location,
    // but this might alleviate that if that were the case.
    int startStepValue   = *(int *)threadVal;
    int count            = 0;               // The count variable of how many grey codes have been found.
    step testXtra[len+1] = {};              // The actual step array. This has an extra 0 at the front so that incrementing with a high-order val as the first step will behave.
    step *test           = testXtra + 1;    // The step array. This holds steps, which are unsigned characters, that represent which binary digit is being switched during that step.
    step *sptr           = test;            // Pointer to a step within the test array. Used for looping through
    int bffr[len+1];                        // This is the buffer that holds the values generated by actually performing the grey code sequence for testing it.
    int *buf             = bffr + 1;        // Pointer to the "start" of the grey code. Since the first 0 never changes, but is used when calculating values, we start the pointer one val in.
    int *bptr            = buf;             // Pointer to within the buffer. Used for looping through.
    int *bufEndPtr       = buf + len - 1;   // Points to the ending 0 in valid grey codes.
    bool flags[len]      = {};              // This keeps track of if a number (the number of the index) has been reached in the sequence yet or not
    step *limitStep      = test + 1;        // This is which step is being limited, as in when it changes then this thread is done and returns its count.



    // -- Perform initialization before starting the search.

    // Get the start step digit number, converting from the power of two to the digit number (0-based).
    int startStepDigitNum = 0;
    while((startStepValue >> startStepDigitNum) != 1) startStepDigitNum++;

    // Set the first value of the buffer to an unchanging 0 (all grey codes start at all 0s).
    bffr[0] = 0;

    // If we are outputting results to files, initialize that.
    #ifdef FILE_OUTPUT
        // This generates the output file name based on how many digits and which thread this is. Each thread outputs to a different file.
        char outfileName[OUTFILE_NAME_LENGTH] = OUTFILE(NUM_DIGITS);
        outfileName[OUTFILE_NAME_THREAD_NUM_INDEX] = '0' + startStepDigitNum;

        // This creates the output file pointer.
        FILE *out = fopen(outfileName, "w");
        if(out == NULL) { // If there was a problem getting the output file, return -1 and exit.
            printf("In the thread with first step %d, there was an issue opening the output file. Exiting.\n", startStepDigitNum);
            *(int *)threadVal = -1;
            pthread_exit(NULL);
        }
    #endif

    // Initialize the step array differently based on if checking everything or with a fixed first step.
    #ifdef CHECK_EVERYTHING 
        // If we aren't using the assumption, then initialize test without a fixed first step.
        if(startStepValue == 1) memcpy(test, lowest, (len) * sizeof(step)); else {
            test[0] = startStepValue;
            memcpy(test + 1, lowest, (len - 1) * sizeof(step));
        }

        // Also, update the limit step, and set the extra early step value to a non-zero.
        // If the extra test value is not set here, then the "if(*sptr == *(sptr - 1))" line under
        // increment will cause an infinite loop on the thread where the set step is the highest order.
        limitStep = test;
        testXtra[0] = 1;
    #else
        // If using the assumption, initialize the test to the start digit, then the lowest array after that
        test[0] = 1;
        test[1] = startStepValue;
        memcpy(test + 2, lowest, (len - 2) * sizeof(step));
    #endif
    


    // -- Begin the search.

    // Loop until the limit step has changed and this thread has gone past it's search space.
    while(*limitStep == startStepValue) {

        // -- Checking if it is a valid grey code sequence.

        while(true) {
            // Calculate the next number in the sequence.
            *bptr = *(bptr - 1) ^ (*sptr);

            // If it has reached 0, break out of the loop.
            if(!*bptr) {
                if(bptr < bufEndPtr) goto increment; else break; // If before the end, invalid, otherwise, valid.
            }

            // Check if this number has already been reached. If it has, invalid.
            if(flags[*bptr]) goto increment;

            // If it has passed all that, it's valid. Mark this number as reached.
            flags[*bptr] = true;

            // Go to the next step
            sptr++;
            bptr++;
        }



        // -- We have reached a valid gray code

        // 0 has been reached at the right time, and no duplicates were found, so it's valid.
        count++;

        // If outputting to a file, do that here.
        #ifdef FILE_OUTPUT
            //fwrite(test, sizeof(step), len, out);
            for(int j = 0; j < len; j++) fprintf(out, j != len - 1 ? "%x," : "%x\n", test[j]);
        #endif

        // We increment the sequence now from the very end.
        // We only need to check the very last for being maxed, since a valid code will never repeat a digit twice in a row.
        if((*sptr) & LAST_DIGIT_STEP) { 
            *(sptr--) = 1;
            flags[*(bptr--)] = false;
        }
        (*sptr) <<= 1;
        flags[*(bptr)] = false;
        continue;



        increment:
        // -- We have encountered an invalid sequence.

        // Increment from the invalid step that needs to be changed.
        while((*sptr) & LAST_DIGIT_STEP) {
            sptr--;
            flags[*(--bptr)] = false;
        }
        (*sptr) <<= 1;

        // If we have incremented it into having a duplicate, increment it again. This saves some runtime by skipping some memcpy calls.
        if(*sptr == *(sptr - 1)) goto increment;

        // Copy from lowest to the rest of the steps after the last incremented one.
        memcpy(sptr + 1, lowest, (len - 1 - (sptr - test)) * sizeof(step));
    }


    // Print the final statistics (different format if checking everything)
    #ifdef CHECK_EVERYTHING 
    printf(" ----- In total the num found was %d with %d digits when starting with: %d,...\n\n", count, NUM_DIGITS, startStepDigitNum);
    #else
    printf(" ----- In total the num found was %d with %d digits when starting with: 0, %d,...\n\n", count, NUM_DIGITS, startStepDigitNum);
    #endif

    // Close the file, if doing file output.
    #ifdef FILE_OUTPUT
    fclose(out);
    #endif

    // Write the count to the thread value (essentially returning it).
    *(int *)threadVal = count;

    // Exit the thread
    pthread_exit(NULL);
    return NULL;
}



/**
 * Starts the program.
 * @return Exit status.
*/
int main() 
{
    // If measuring runtime, get the start time.
    #ifdef RUNTIME
    clock_t start_time = clock();
    #endif



    // For details on how the "lowest" code is constructed, see (1) in the file documentation comment.
    lowest[0] = 1;
    step *init = lowest + 1;
    for(int i = 0; i < NUM_DIGITS; i++) {

        // Copy the previous block over
        memcpy(init, lowest, (init - lowest) * sizeof(step));

        // Move forward almost to the end
        init += init - lowest - 1;

        // Increment the last number and move to the end
        *init <<= 1;
        init++;
    }
    lowest[len - 1] >>= 1; // The last number will be over, so decrement it to get the the "first" grey code.



    // -- Then, create and run all of the threads.
    
    pthread_t threadIds[NUM_DIGITS];   // Array of the thread ids
    int threadVals[NUM_DIGITS];        // Array of all the initial steps, and where the threads write their final counts to.

    // Create all the initial step values that will be used
    for(int i = skippedThreads; i < NUM_DIGITS; i++) threadVals[i] = (1 << i);

    // Generate the threads and pass in their values.
    for(int i = skippedThreads; i < NUM_DIGITS; i++) pthread_create(threadIds + i, NULL, &calculateCodesWithSetStart, (void *)(threadVals + i));

    // Wait for all the threads to finish and add up their totals.
    int totalCount = 0;
    for(int i = skippedThreads; i < NUM_DIGITS; i++) {
        pthread_join(threadIds[i], NULL);
        totalCount += threadVals[i];
    }



    // -- Last, print the results. Format changes depending on if checking everything.
    #ifdef CHECK_EVERYTHING 
    printf(" ---------- In total, there are %d codes for %d digits.\n", totalCount, NUM_DIGITS);
    #else
    printf(" ---------- The num of codes with a fixed first step is: %d\n", totalCount);
    printf(" ---------- Tentatively, that might imply that in total there are %d codes for %d digits.\n", NUM_DIGITS * totalCount, NUM_DIGITS);
    #endif
    
    // If measuring runtime, then output the final runtime here.
    #ifdef RUNTIME
    // Get end benchmarking time
    clock_t end_time = clock();
    printf("-- This run took %f seconds.\n", ((double) (end_time - start_time)) / CLOCKS_PER_SEC );
    #endif

    // Exit the main thread.
    pthread_exit(NULL);
    return EXIT_SUCCESS;
}

