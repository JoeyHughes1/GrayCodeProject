/**
 * @file SortingTest.c
 * @author Joey Hughes
 * Tests the speed of various sorting algorithms to see which is best for small sets of small integers.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

/** Defines the number of times to sort as ten million. */
#define NUMBER_OF_SORTS 10000000

/** Defines the number of elements in the short arrays. */
#define NUM_ELEMS 4

/** The number of times to time the running of each sorting algorithm. */
#define REPETITIONS 3




/**
 * Function to shuffle a list using rand().
 * @param list The list array to shuffle.
*/
void shuffle(int list[])
{
    int temp = 0;
    int index1;
    int index2;
    for(int i = NUM_ELEMS; i; i--) {
        index1 = rand() % NUM_ELEMS;
        index2 = rand() % NUM_ELEMS;
        temp = list[index1];
        list[index1] = list[index2];
        list[index2] = temp;
    }
}



/**
 * Performs counting sort on the list.
 * @param list The array to sort.
*/
void countSort(int list[])
{
    // Create variables
    int outputList[NUM_ELEMS];
    int i = 0;

    // Find the max
    int max = 2;
    for(i = 0; i < NUM_ELEMS; i++) if(list[i] > max) max = list[i];

    // Create the count array and do the rolling sum.
    int countArray[max + 1];
    for(i = 0; i <= max; i++) countArray[i] = 0;
    for(i = 0; i < NUM_ELEMS; i++) countArray[list[i]]++;
    for(i = 1; i <= max; i++) countArray[i] += countArray[i - 1];

    // Create the output array
    for(i = 0; i < NUM_ELEMS; i++) outputList[--countArray[list[i]]] = list[i];

    // Copy the sorted list over
    for(i = 0; i < NUM_ELEMS; i++) list[i] = outputList[i];
}


/**
 * Quicksorts the list from indices low to high. Uses recursion.
 * @param arr The list to sort.
 * @param low The lowest index of the range to sort in.
 * @param high The highest index of the range to sort in.
*/
void quickSortHelper(int arr[], int low, int high)
{
    // when low is less than high
    if (low < high) {

        int pivot = arr[high];
        int temp;

        // Index of smaller element and Indicate
        // the right position of pivot found so far
        int i = low - 1;

        for (int j = low; j < high; j++) {
            // If current element is smaller than the pivot
            if (arr[j] < pivot) {
                // Increment index of smaller element
                i++;
                temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
        temp = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = temp;

        // Recursion Call
        // smaller element than pivot goes left and
        // higher element goes right
        quickSortHelper(arr, low, i);
        quickSortHelper(arr, i + 2, high);
    }
}

/**
 * Parent Quick Sort function. Calls the helper function and begins the recursive process.
*/
void quickSort(int arr[])
{
    quickSortHelper(arr, 0, NUM_ELEMS - 1);
}


/**
 * Helper function for heap sort.
 * @param arr The array.
 * @param N The size of the heap
 * @param i Index in the heap which is the root of the subtree to be heapified.
*/
void heapify(int arr[], int N, int i)
{
    // Find largest among root,
    // left child and right child

    // Initialize largest as root
    int largest = i;

    // left = 2*i + 1
    int left = 2 * i + 1;

    // right = 2*i + 2
    int right = 2 * i + 2;

    // If left child is larger than root
    if (left < N && arr[left] > arr[largest])

        largest = left;

    // If right child is larger than largest
    // so far
    if (right < N && arr[right] > arr[largest])

        largest = right;

    // Swap and continue heapifying
    // if root is not largest
    // If largest is not root
    if (largest != i) {

        int temp = arr[i];
        arr[i] = arr[largest];
        arr[largest] = temp;

        // Recursively heapify the affected
        // sub-tree
        heapify(arr, N, largest);
    }
}

/**
 * Function to do heap sort.
 * @param arr The list to heap sort.
*/
void heapSort(int arr[])
{

    // Build max heap
    for (int i = NUM_ELEMS / 2 - 1; i >= 0; i--)

        heapify(arr, NUM_ELEMS, i);

    // Heap sort
    for (int i = NUM_ELEMS - 1; i >= 0; i--) {

        int temp = arr[0];
        arr[0] = arr[i];
        arr[i] = temp;

        // Heapify root element
        // to get highest element at
        // root again
        heapify(arr, i, 0);
    }
}


/**
 * Comparison function for two integers.
 * @param a Pointer to the first integer.
 * @param b Pointer to the second integer.
 * @return 0 If they are equal, -1 if the first is less than the second, 1 if the first is bigger than the second.
*/
int intCompare(const void *a, const void *b)
{
    return (*(int *)a < *(int *)b) ? -1 : (*(int *)a > *(int *)b);
}


/**
 * An insertion sort algorithm for a list.
 * @param arr The list to sort.
*/
void insertionSort(int *arr)
{
    register int key;
    int *j;
    int *i = arr + 1;
    do {
        key = *i;
        j = i - 1;

        /* Move elements of arr[0..i-1], that are
          greater than key, to one position ahead
          of their current position */
        while (j >= arr && *j > key) {
            *(j + 1) = *j;
            j--;
        }
        *(j + 1) = key;
        i++;
    } while((i - arr) < NUM_ELEMS);
}

/**
 * Selection sort algorithm for a list.
 * @param arr The list to sort.
*/
void selectionSort(int arr[])
{
    int i, j, min_idx;

    // One by one move boundary of unsorted subarray
    for(i = 0; i < NUM_ELEMS - 1; i++)
    {
        // Find the minimum element in unsorted array
        min_idx = i;
        for(j = i + 1; j < NUM_ELEMS; j++)
            if(arr[j] < arr[min_idx]) min_idx = j;

        // Swap the found minimum element with the first element
        if(min_idx != i) {
            int temp = arr[min_idx];
            arr[min_idx] = arr[i];
            arr[i] = temp;
        }
    }
}


/**
 * I realized that instead of comparing sorted versions of the signatures, I could just make the frequency array for them and compare those.
 * So replacing the sorting of the list would be the construction of that frequency array, and then the comparisons would be the same for sorting.
 * @param arr The array of the signatures to make the frequency array out of.
*/
void notSortButMakeFrequencyArray(int *arr)
{
    int freq[(1 << (NUM_ELEMS - 2)) + 1] = {};
    int *arrPtr = arr;
    do {
        freq[((*(arrPtr++)) >> 1)]++;
    } while(arrPtr - arr < NUM_ELEMS);
}


/**
 * Tests a sorting algorithm running a bunch of times for multiple trials 
 * and averages the times to get the average runtime over the given list.
 * @param currentSort The function pointer to the sorting algorithm function.
 * @param name The name of the sorting algorithm as a string. Used for printing results.
 * @param list The list to be sorting.
*/
void testSort(void (*currentSort)(int []), char *name, int list[])
{
    double averageTime = 0.0;
    for(int i = 0; i < REPETITIONS; i++) {
        // Start the timing
        clock_t start_time = clock();

        // Run the sorting algorithm however many times
        for(int i = 0; i < NUMBER_OF_SORTS; i++) {
            shuffle(list);
            currentSort(list);
        }

        // Stop the time, add the runtime to the total.
        clock_t end_time = clock();
        averageTime += ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
    }

    // Divide the total runtime by the number of repetitions to get the average.
    averageTime /= REPETITIONS;

    // Print the average runtime for this algorithm
    printf("Average runtime of %d repetitions of %15s was: %f seconds.\n", REPETITIONS, name, averageTime);
}


/**
 * Starts the program.
 * @return Exit status.
*/
int main()
{
    printf("\nStarting to test sorting algorithms on a list with %d elements...\n\n", NUM_ELEMS);
    srand(time(NULL));

    // Setup the array
    int list[NUM_ELEMS] = {2, 4, 4, 6};

    // Test each of the algorithms
    // testSort(countSort, "Counting Sort", list);
    // testSort(quickSort, "Quick Sort", list);
    // testSort(heapSort, "Heap Sort", list);
    testSort(insertionSort, "Insertion Sort", list);
    testSort(selectionSort, "Selection Sort", list);
    testSort(notSortButMakeFrequencyArray, "Frequency List", list);
    printf("Testing finished.\n");
}

/*
With 4 and 5 elements it seems like insertion sort is the fastest.
*/
