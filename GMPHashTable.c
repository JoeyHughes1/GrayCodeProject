/**
 * @file GMPHashTable.c
 * @author Joey Hughes
 * This is code for a hash table implementation that stores GMP integers. This is for the GreyCodeChimera.c program,
 * so it uses the types from the GreyCodeTypes.h file.
 * This hash table supports GMP Integers and supports inserting, checking if it contains a GMP integer, and emptying
 * the entire table of all the integers. This does not support removing or resizing.
*/

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#ifndef GREY_CODE_TYPES_DEFINED
    #include "GreyCodeTypes.h"
#endif


/** Node for the Hashtable. Used in the seed extrapolation part. */
typedef struct GMPHashNodeStruct {
    /** The sequenceNum Held by this node, this bucket in the hash table. */
    sequenceNum key;
} GMPHashNode;

/** Struct for a whole HashTable. Used in the seed extrapolation part. */
typedef struct {
    /** The hash table array. Holds HashNodes. */
    GMPHashNode* nodes;
    /** The array that describes which nodes are occupied. Not included in the structs for storage reasons, and makes emptying easier. */
    bool* occupied;
    /** The total size of the hash table, how many buckets it has. */
    size_t size;
    /** The number of actual elements in the hash table. */
    size_t count;
} GMPHashTable;


/**
 * The first hash function for the table. This takes the key and turns it into an index in the table.
 * @param key The GMP integer to turn into an index.
 * @param table_size The capacity of the hash table.
 * @return The index in the hash table to try to store at.
*/
static inline unsigned long GMPhash1(const mpz_t key, size_t table_size) 
{
    return mpz_fdiv_ui(key, table_size);
}



/**
 * The double hashing function. This function takes the key and returns a number of indices to jump by in the hash table
 * when there is a collision.
 * @param key The GMP integer key.
 * @param table_size The capacity of the hash table.
 * @return The number of indices to jump by in the hash table.
*/
static inline unsigned long GMPhash2(const mpz_t key) 
{
    return mpz_fdiv_ui(key, (NUM_DIGITS - 1)) + 1;
}



/**
 * Creates a new HashTable and allocates it, initializing all the keys as well.
 * @param size The capacity to give the new HashTable.
 * @return Pointer to the new HashTable.
*/
GMPHashTable* createGMPTable(size_t size) 
{
    GMPHashTable* table = (GMPHashTable*)malloc(sizeof(GMPHashTable));
    table->size = size;
    table->count = 0;
    table->nodes = (GMPHashNode*)calloc(size, sizeof(GMPHashNode));
    table->occupied = (bool *)calloc(size, sizeof(bool));
    for (size_t i = 0; i < size; i++) 
        mpz_init((table->nodes[i]).key);
    return table;
}



/**
 * Inserts an element into the HashTable, using double hashing if there is a collision.
 * @param table The pointer to the HashTable to put the key in.
 * @param key The sequenceNum to put into the HashTable.
*/
void GMPHashInsert(GMPHashTable* table, const sequenceNum key) 
{
    // Only double hash as long as the first index is occupied.
    unsigned long index = GMPhash1(key, table->size);
    if(table->occupied[index]) {
        unsigned long step = GMPhash2(key);
        while(table->occupied[index]) 
            index = (index + step) % (table->size);
    }

    mpz_set(table->nodes[index].key, key);
    table->occupied[index] = true;
    table->count++;
}



/**
 * Returns a bool of whether or not the given table contains the given key.
 * @param table The pointer to the HashTable to search in.
 * @param key The sequenceNum which contains the number to look for.
 * @return True if the hash table already contains the key, false if not.
*/
bool GMPHashContains(GMPHashTable* table, const sequenceNum key) {
    unsigned long index = GMPhash1(key, table->size);

    // If the first index is open, it's not there.
    if(!table->occupied[index]) return false;

    // If the first index is the element we're looking for, return true right away
    else if(mpz_cmp(table->nodes[index].key, key) == 0) return true;

    // Otherwise, then we have to double hash and start looping.
    unsigned long step = GMPhash2(key);
    index = (index + step) % table->size;
    while(table->occupied[index]) {
        if(mpz_cmp(table->nodes[index].key, key) == 0) 
            return true; // Key found
        index = (index + step) % table->size;
    }
    return false; // Key not found
}



/**
 * Sets all the boolean flags in the table to not occupied, and sets the count to 0,
 * effectively emptying the table.
 * @param table The pointer to the HashTable to empty.
*/
void emptyGMPTable(GMPHashTable *table)
{
    memset(table->occupied, false, table->size);
    table->count = 0;
}



/**
 * Frees the HashTable, including clearing all the sequenceNums.
 * @param table The pointer to the HashTable to free.
*/
void freeGMPTable(GMPHashTable* table) 
{
    for (size_t i = 0; i < table->size; i++) {
        mpz_clear(table->nodes[i].key);
    }
    free(table->nodes);
    free(table->occupied);
    free(table);
}
