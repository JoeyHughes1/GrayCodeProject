/**
 * @file ThreadSequenceCheckers.c
 * @author Joey Hughes
 * This is a file that contains boolean functions that look through sequences and either find if they
 * could be swapped to start with the thread's set start, or if they could be swapped to start with
 * one of the lower threads' set starts.
 */

#ifndef GREY_CODE_TYPES_DEFINED
    #include "GreyCodeTypes.h"
#endif
#include <stdbool.h>



/**
 * Part of the family of functions for each thread that returns true if the set of steps
 * indicates that the sequence could be swapped to start with the same set start as the
 * thread.
 * This one is true if the first, third, and fifth are the same.
 * Looks forward 4 steps.
 * @param stepPtr The pointer to the steps to check (Checks forward).
 * @returns True if the sequence could be swapped, false otherwise.
 */
bool checkStepsIn01020(stepMask *stepPtr)
{
    return (stepPtr[0] == stepPtr[2] && stepPtr[2] == stepPtr[4]); 
}



/**
 * Part of the family of functions for each thread that returns true if the set of steps
 * indicates that the sequence could be swapped to start with the same set start as the
 * thread.
 * This one is true if the first and second are the same and the second and fifth 
 * are the same.
 * Looks forward 4 steps.
 * @param stepPtr The pointer to the steps to check (Checks forward).
 * @returns True if the sequence could be swapped, false otherwise.
 */
bool checkStepsIn01021(stepMask *stepPtr)
{
    return (stepPtr[0] == stepPtr[2] && stepPtr[1] == stepPtr[4]); 
}



/**
 * Part of the family of functions for each thread that returns true if the set of steps
 * indicates that the sequence could be swapped to start with the same set start as the
 * thread.
 * This one is true if the first and third are the same and the fifth is not the same
 * as the first or second.
 * Looks forward 4 steps.
 * @param stepPtr The pointer to the steps to check (Checks forward).
 * @returns True if the sequence could be swapped, false otherwise.
 */
bool checkStepsIn01023(stepMask *stepPtr)
{
    return (stepPtr[0] == stepPtr[2] && stepPtr[4] != stepPtr[0] && stepPtr[4] != stepPtr[1]); 
}



/**
 * Part of the family of functions for each thread that returns true if the set of steps
 * indicates that the sequence could be swapped to start with the same set start as the
 * thread.
 * This one is true if the first and third are the same.
 * Looks forward 3 steps.
 * @param stepPtr The pointer to the steps to check (Checks forward).
 * @returns True if the sequence could be swapped, false otherwise.
 */
bool checkStepsIn0120(stepMask *stepPtr)
{
    return (stepPtr[0] == stepPtr[3]); 
}



/**
 * Part of the family of functions for each thread that returns true if the set of steps
 * indicates that the sequence could be swapped to start with the same set start as the
 * thread.
 * This one is true if the first and third are not the same, and the fourth is not 
 * the same as the first or second.
 * Looks forward 3 steps.
 * @param stepPtr The pointer to the steps to check (Checks forward).
 * @returns True if the sequence could be swapped, false otherwise.
 */
bool checkStepsIn0123(stepMask *stepPtr)
{
    return (stepPtr[0] != stepPtr[2] && stepPtr[3] != stepPtr[0] && stepPtr[3] != stepPtr[1]); 
}





/**
 * Part of the family of functions for each thread that returns true if the set of steps
 * indicates that the sequence is a child of a seed from a lower thread.
 * This one returns true if the first, third, and fifth step are the same, cause then
 * it could be swapped into 01020.
 * Looks back 4 steps.
 * @param stepPtr The pointer to the steps to check (checks backward).
 * @return True if the sequence should be skipped, false if not.
*/
bool checkStepsForLower01021(stepMask *stepPtr)
{
    return (stepPtr[-4] == stepPtr[-2] && stepPtr[-2] == stepPtr[0]);
}



/**
 * Part of the family of functions for each thread that returns true if the set of steps
 * indicates that the sequence is a child of a seed from a lower thread.
 * This one returns true if either the first, third, and fifth step are all the same,
 * in which case it could be swapped to 01020, or if the first and third are the same, and
 * the second and fifth are the same too, cause then it could be swapped into 01021.
 * Looks back 4 steps.
 * @param stepPtr The pointer to the steps to check (checks backward).
 * @return True if the sequence should be skipped, false if not.
*/
bool checkStepsForLower01023(stepMask *stepPtr)
{
    return (stepPtr[-4] == stepPtr[-2]) && (stepPtr[0] == stepPtr[-4] || stepPtr[0] == stepPtr[-3]);
}



/**
 * Part of the family of functions for each thread that returns true if the set of steps
 * indicates that the sequence is a child of a seed from a lower thread.
 * This one checks if the first and third are the same, cause if so then it definitely
 * appears in the 010 threads.
 * Looks back 2 steps.
 * @param stepPtr The pointer to the steps to check (checks backward).
 * @return True if the sequence should be skipped, false if not.
*/
bool checkStepsForLower0120(stepMask *stepPtr)
{
    return (stepPtr[-2] == stepPtr[0]);
}



/**
 * Part of the family of functions for each thread that returns true if the set of steps
 * indicates that the sequence is a child of a seed from a lower thread.
 * This one checks if the first and third are the same, cause if so then it definitely
 * appears in the 010 threads, but also if the first and fourth are the same, cause if
 * so then it could appear in the 0120 thread.
 * Looks back 3 steps.
 * @param stepPtr The pointer to the steps to check (checks backward).
 * @return True if the sequence should be skipped, false if not.
*/
bool checkStepsForLower0123(stepMask *stepPtr)
{
    return (stepPtr[-3] == stepPtr[-1]) || (stepPtr[-3] == stepPtr[0]);
}

