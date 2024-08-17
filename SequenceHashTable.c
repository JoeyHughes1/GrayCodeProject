/**
 * @file SequenceHashTable.c
 * @author Joey Hughes
 * This contains an implementation of a resizable hash table that holds sequences. This table does not 
 * support removing elements. This table does support inserting, both checking and not checking for duplicates.
 * can check for if a key is contained, and has a constructor and destructor. This is meant for use in the 
 * GreyCodeChimera program in the main algorithm for finding the amount of seeds.
 * However, this has ended up becoming unused in the Chimera Program.
*/

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#ifndef GREY_CODE_TYPES_DEFINED
    #include "GreyCodeTypes.h"
#endif

/** If the fraction of filled buckets in this hash table gets above this ratio, then the hash table will be rehashed. */
#define SIZE_LIMIT_FRACTION (1.0/3.0)

/** The prime used in calculating the initial hash. */
#define HASH_PRIME 5

/** The prime used in calculating the double hash. */
#define DOUBLE_HASH_PRIME 7





/** Struct for a whole HashTable. Used in the seed extrapolation part. */
typedef struct {
    /** The hash table array. Holds HashNodes. */
    sequence *nodes;
    /** The array that describes which nodes are occupied. Not included in the structs for storage reasons, and makes emptying easier. */
    bool *occupied;
    /** The total size of the hash table, how many buckets it has. */
    size_t size;
    /** The number of actual elements in the hash table. */
    size_t count;
    /** Instead of having to modulus, since the size is a power of 2 we can use bit AND, this is the mask.*/
    unsigned long modMask;
} SeqHashTable;


/**
 * The first hash function for the table. This takes the key and turns it into an index in the table.
 * @param key The sequence to turn into an index.
 * @param table_size The capacity of the hash table.
 * @return The index in the hash table to try to store at.
*/
static unsigned long seqHash1(const sequence key, unsigned long modMask)
{
    const step *stepPtr = key;
    register unsigned long hash = 0;
    while(stepPtr - key < len) 
        hash = ((hash * HASH_PRIME) + *(stepPtr++)) & modMask;
    return hash;
}



/**
 * The double hashing function. This function takes the key and returns a number of indices to jump by in the hash table
 * when there is a collision.
 * @param key The sequence key.
 * @param table_size The capacity of the hash table.
 * @return The number of indices to jump by in the hash table.
*/
static unsigned long seqHash2(const sequence key, unsigned long modMask) 
{
    modMask >>= 1; // so we can safely add two if needed.
    const step *stepPtr = key + len/2;
    register unsigned long hash = 0;
    while(stepPtr - key < len) 
        hash = ((hash * DOUBLE_HASH_PRIME) + *(stepPtr++)) & modMask;
    return hash + 1 + (hash & 1);
}



/**
 * Creates a new HashTable and allocates it. The size of this type of hash table will always
 * be a power of two. The exponent parameter is which power of two this hash table will start
 * out as. For example, if 3, then this table will start with 2^(3) = 8 buckets.
 * @param size The exponent of two to make the size of the new HashTable.
 * @return Pointer to the new HashTable.
*/
SeqHashTable* createSeqTable(unsigned char exponent)
{
    SeqHashTable* table = (SeqHashTable*)malloc(sizeof(SeqHashTable));
    table->size = (1 << exponent);
    table->modMask = (table->size - 1);
    table->count = 0;
    table->nodes = (sequence *)malloc(table->size * sizeof(sequence));
    table->occupied = (bool *)calloc(table->size, sizeof(bool));
    return table;
}



/**
 * Called if the proportion of the table that is full is above the SIZE_LIMIT_FRACTION.
 * Increases the size of the table by a factor of 8 and rehashes all the items inside.
*/
static void resizeTable(SeqHashTable* table)
{
    // Increase the size by 2^3, update the modMask
    table->size *= 8;
    table->modMask = (table->size - 1);

    // Allocate the new arrays
    sequence *newBuckets = (sequence *)malloc(table->size * sizeof(sequence));
    bool *newOccupied = (bool *)calloc(table->size, sizeof(bool));

    // Loop through the occupied array and rehash every old element into the new arrays
    size_t rehashesLeft = table->count;        // The amount of rehashes left to do
    bool *occPtr = table->occupied;            // Pointer to inside occupied that searches for elements in the old table
    unsigned long index;                       // Holds the rehashed index for each element
    unsigned long doubleHash;                  // Holds the rehashed doublehash for each element
    step *beingRehashed;                       // References the sequence currently being rehashed
    while(rehashesLeft) {

        // Go until we have an occupied element
        while(!*occPtr) occPtr++;

        // We have found an occupied bucket, grab it's sequence and rehash it with the new size
        beingRehashed = table->nodes[occPtr - (table->occupied)];
        index = seqHash1(beingRehashed, table->modMask);
        if(!newOccupied[index]) goto rehashFunctionAddToTable; // if it's immediately open, don't double hash and just add

        // If it's not immediately open, double hash and start stepping
        doubleHash = seqHash2(beingRehashed, table->modMask);

        // Now loop until reaching an open spot
        do
            index = (index + doubleHash) & (table->modMask);
        while(newOccupied[index]);

        // We found the next open spot, add it
        rehashFunctionAddToTable:
        memcpy(newBuckets[index], beingRehashed, sizeof(step) * len);
        newOccupied[index] = true;
        
        // We have finished a rehash, decrement the counter and increment;
        rehashesLeft--;
        occPtr++;
    }

    // Now we have rehashed and copied all the elements into the new array.
    // Free the old ones
    free(table->nodes);
    free(table->occupied);

    // Set them to the new ones. Now we have new arrays, the size has been increased, count stayed the same, we are done.
    table->nodes = newBuckets;
    table->occupied = newOccupied;
}



/**
 * Inserts an element into the HashTable, using double hashing if there is a collision. If
 * the key is found to already be in the hash table, then it is not inserted again and false
 * is returned. If the sequence is added successfully, then true is returned.
 * @param table The pointer to the HashTable to put the key in.
 * @param key The sequence to put into the HashTable.
 * @return True if the sequence was added, false if it was already in the list.
*/
bool seqHashInsertIfNotContains(SeqHashTable* table, const sequence key) 
{
    // If we are at the capacity limit, then resize the table.
    if(((double)table->count)/table->size > SIZE_LIMIT_FRACTION) 
        resizeTable(table);

    // Otherwise, hash to find the initial index
    unsigned long index = seqHash1(key, table->modMask);

    // If that index is already open, go ahead and add it there
    if(!table->occupied[index]) goto insertFunctionAddToTable;

    // Otherwise, check if it's the same sequence, and if so it's a duplicate, return false
    else if(memcmp(table->nodes[index], key, sizeof(step) * len) == 0) return false;

    // Otherwise, doubleHash and step once, then loop to find the next open spot
    unsigned long doubleHash = seqHash2(key, table->modMask);
    index = (index + doubleHash) & (table->modMask);
    while(table->occupied[index]) {
        if(memcmp(table->nodes[index], key, sizeof(step) * len) == 0)
            return false; // Key already in there, return false, don't add
        index = (index + doubleHash) & (table->modMask);
    }

    // We found the next open spot and the key's not already in there, so add it
    insertFunctionAddToTable:
    memcpy(table->nodes[index], key, sizeof(step) * len);
    table->occupied[index] = true;
    table->count++;
    return true;
}



/**
 * Inserts an element into the HashTable, using double hashing if there is a collision.
 * This does not check for duplicates.
 * @param table The pointer to the HashTable to put the key in.
 * @param key The sequence to put into the HashTable.
*/
void seqHashInsert(SeqHashTable* table, const sequence key) 
{
    // If we are at the capacity limit, then resize the table.
    if(((double)table->count)/table->size > SIZE_LIMIT_FRACTION) 
        resizeTable(table);

    // Otherwise, hash to find the initial index
    unsigned long index = seqHash1(key, table->modMask);

    // If that index is already open, go ahead and add it there
    if(!table->occupied[index]) goto insertFunctionAddToTable;

    // Otherwise, doubleHash and step once, then loop to find the next open spot
    unsigned long doubleHash = seqHash2(key, table->modMask);
    index = (index + doubleHash) & (table->modMask);
    while(table->occupied[index])
        index = (index + doubleHash) & (table->modMask);

    // We found the next open spot and the key's not already in there, so add it
    insertFunctionAddToTable:
    memcpy(table->nodes[index], key, sizeof(step) * len);
    table->occupied[index] = true;
    table->count++;
}



/**
 * Returns a bool of whether or not the given table contains the given key.
 * @param table The pointer to the HashTable to search in.
 * @param key The sequence to find.
 * @return True if the hash table already contains the key, false if not.
*/
bool seqHashContains(SeqHashTable* table, const sequence key) {

    // Hash the key, then find if it's in there or not.
    unsigned long index = seqHash1(key, table->modMask);

    // If that index is already open, go ahead and return false
    if(!table->occupied[index]) return false;

    // Otherwise, check if it's the same sequence, and if so, return true, we found it
    else if(memcmp(table->nodes[index], key, sizeof(step) * len) == 0) return true;

    // Otherwise, doubleHash and step once, then loop
    unsigned long doubleHash = seqHash2(key, table->modMask);
    index = (index + doubleHash) & (table->modMask);
    while(table->occupied[index]) {
        if(memcmp(table->nodes[index], key, sizeof(step) * len) == 0)
            return true; // We found it, return true
        index = (index + doubleHash) & (table->modMask);
    }
    return false; // We got to an open spot and never found it.
}



/**
 * Frees the HashTable, including clearing all the sequenceNums.
 * @param table The pointer to the HashTable to free.
*/
void freeSeqTable(SeqHashTable* table)
{
    free(table->nodes);
    free(table->occupied);
    free(table);
}
