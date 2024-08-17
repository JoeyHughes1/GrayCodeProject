/**
 * @file analysis.c
 * @author Joey Hughes
 * File with a main function used for analyzing grey codes.
 * Contains an implementation of a basic, insert-only AVL Tree.
 * 
 * Some of the code used for the AVL Tree is taken from my CSC316 class, so
 * it is partially implemented by Dr. Jason King and the textbook:
 * 
 * Data Structures and Algorithms in Java, Sixth Edition Michael T. Goodrich,
 * Roberto Tamassia, and Michael H. Goldwasser John Wiley & Sons, 2014
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

/** The input file. */
#define INFILE "./4Digits.txt"

/** The number of binary digits for the grey codes we are considering. */
#define NUM_DIGITS 4

/** The length of each sequence of steps. Equal to 2 ^ NUM_DIGITS. */
#define LEN (1 << NUM_DIGITS)

/** The value of the highest order digit when treating a sequence of steps as a base NUM_DIGITS number. */
#define HIGHEST_POWER ((unsigned long long) pow(NUM_DIGITS, LEN - 1))

/** Macro function for checking if a tree does not contain a value. */
#define treeNotContains(tree, num) (findValue(tree, num)->value == 0ULL)


/**
 * Temporary global variable for how many nodes have been added. Used in counting how many new nodes are added based on a seed.
*/
int nodesAdded = 0;


/** The type of a sequence number. Each one uniquely identifies a sequence of steps. */
typedef unsigned long long sequence;


/**
 * A struct that represents a node in a AVLTree. Has a value, parent reference, and left/right child references.
*/
struct treeNodeStruct {
    /** The value of this treeNode. Sentinel nodes are nodes where this value is 0ULL. */
    sequence value;
    /** The height of the subtree of this node and its children (including this node). */
    int height;
    /** Pointer to the left child node of this node. */
    struct treeNodeStruct *left;
    /** Pointer to the right child node of this node. */
    struct treeNodeStruct *right;
    /** Pointer to the parent of this node. */
    struct treeNodeStruct *parent;
};

typedef struct treeNodeStruct treeNode;


/**
 * A struct that represents an AVL Tree. Has a root and a size.
*/
typedef struct {
    /** The root of this AVL Tree. */
    treeNode *root;
    /** The number of elements in this AVL Tree. */
    int size;
} AVLTree;



/**
 * This locates the tree node with the given value, or the sentinel node where this value
 * would be inserted into the tree.
 * @param tree The pointer to the AVL Tree to search
 * @param value The value to look for.
 * @return The treeNode pointer to the node or sentinel.
*/
treeNode *findValue(AVLTree *tree, sequence value)
{
    treeNode *current = tree->root;
    // Loop until reaching either a sentinel or the value
    while(current->value != 0ULL && current->value != value) {
        if(value > current->value) current = current->right; else current = current->left;
    }

    // If out here, that means the current node is either the one with the value
    //    or the sentinel where it should go. Return the pointer.
    return current;
}



/**
 * Helper function for restructuring the tree after adding. Gets the taller child, or if equal heights it
 * gets the same-aligned child as the node itself (or if the root, just the left child).
 * @param tree The AVLTree.
 * @param node The node to get the taller child of.
*/
treeNode *tallerChild(AVLTree *tree, treeNode *node)
{
    if(node->left->height > node->right->height) return node->left;  // If left is taller, left
    if(node->left->height < node->right->height) return node->right; // If right is taller, right
    if(tree->root == node) return node->left;                        // Otherwise, if root node, left
    if(node->parent->left == node) return node->left;                // If not, then same-aligned child.
    
    return node->right;

}



/**
 * Rotates the given node about it's parents. Used in the AVL tree to keep balance.
 * This code comes from my CSC316 class.
 * @param node The node to rotate.
*/
void rotate(AVLTree *tree, treeNode *node)
{
    treeNode *parent = node->parent;
    treeNode *grandparent = parent->parent;
    if(grandparent == NULL) {
        tree->root = node;
        node->parent = NULL;
    } else {
        if(grandparent->left == parent) {
            node->parent = grandparent;
            grandparent->left = node;
        } else {
            node->parent = grandparent;
            grandparent->right = node;
        }
    }

    if(node == parent->left) {
        node->right->parent = parent;
        parent->left = node->right;
        parent->parent = node;
        node->right = parent;
    } else {
        node->left->parent = parent;
        parent->right = node->left;
        parent->parent = node;
        node->left = parent;
    }
}



/**
 * This adds a node with the given value into the AVL Tree. This does NOT check if the value is already in the tree.
 * Only use this on new values that aren't already in the tree.
 * This code comes from my CSC316 class.
*/
void addNode(AVLTree *tree, sequence value)
{
    // Increment the nodes added number
    nodesAdded++;

    // First, find the sentinel to add the value to.
    treeNode *newNode = findValue(tree, value);
    // Change the sentinel to have a value
    newNode->value = value;
    newNode->height = 1;
    // Allocate two new sentinels as children
    newNode->left = (treeNode *)malloc(sizeof(treeNode));
    newNode->left->value = 0ULL;
    newNode->left->parent = newNode;
    newNode->left->height = 0;
    newNode->right = (treeNode *)malloc(sizeof(treeNode));
    newNode->right->value = 0ULL;
    newNode->right->parent = newNode;
    newNode->right->height = 0;

    // REBALANCE
    int oldHeight = 0;
    int newHeight = 0;
    treeNode *current = newNode;
    do{
        if(abs(current->left->height - current->right->height) > 1) { // if current is not balanced
            treeNode *child = tallerChild(tree, current);
            treeNode *grandchild = tallerChild(tree, child);
            // Restructure
            if((child->left == grandchild && current->left == child) || (child->right == grandchild && current->right == child)) {
                rotate(tree, child);
                current = child;
            } else {
                rotate(tree, grandchild);
                rotate(tree, grandchild);
                current = grandchild;
            }

            // Recompute height for current->left
            current->left->height = 1 + __max(current->left->left->height, current->left->right->height);
            // Recompute height for current->right
            current->right->height = 1 + __max(current->right->left->height, current->right->right->height);
        }
        // Recompute height for current
        current->height = 1 + __max(current->left->height, current->right->height);
        newHeight = current->height;
        current = current->parent;
    } while(oldHeight != newHeight && current != NULL);
    tree->size++;
}



/**
 * Allocates and initializes a new AVLTree. This must be freed later with freeTree().
 * @return The pointer to the new allocated AVLTree.+
*/
AVLTree *makeTree()
{
    // Allocate a new tree object
    AVLTree *newTree = (AVLTree *)malloc(sizeof(AVLTree));
    newTree->size = 0;

    // Create a sentinel node as the root.
    newTree->root = (treeNode *)malloc(sizeof(treeNode));
    newTree->root->value = 0ULL;
    newTree->root->parent = NULL; // The root has a null parent reference.
    newTree->root->height = 0;
    return newTree;
}



/**
 * Frees the subtree starting at the given node using recursive post-traversal of the tree.
 * The base case is freeing a sentinel node.
 * @param node The root of the subtree to free.
*/
void freeHelper(treeNode *node)
{
    // If a sentinel node, just free it and return.
    if(node->value == 0ULL) {
        free(node);
        return;
    }
    // Otherwise, use post-traversal to free all the nodes
    freeHelper(node->left);
    freeHelper(node->right);
    free(node);
}



/**
 * This frees an entire tree, including all it's nodes.
 * @param tree The AVLTree to free.
*/
void freeTree(AVLTree *tree)
{
    // Use the recursive function to free all the nodes in the tree
    freeHelper(tree->root);
    // Then free the tree struct itself
    free(tree);
}



/**
 * Helper function for printInOrder() that prints the contents of a tree using an in-order traversal.
 * @param node The root node of the subtree to print in-order.
*/
void inOrderHelper(treeNode *node)
{
    // Do not print sentinel nodes
    if(node->value == 0ULL) return;
    
    // If not a sentinel node, print in-order
    inOrderHelper(node->left);
    printf("%lld ", node->value);
    inOrderHelper(node->right);
}



/**
 * Prints the contents of the given tree using an in-order traversal.
*/
void printInOrder(AVLTree *tree)
{
    printf("AVLTree of size %d has root height %d and contents: ", tree->size, tree->root->height);
    inOrderHelper(tree->root);
    printf("\n");
}



/**
 * Reads in a sequence from the given input file into the given string.
 * If the EOF was reached, this returns false.
 * @param in The file pointer to read in from
 * @param line The string to read the number characters into.
 * @return True if read successfully, false if EOF was reached.
*/
bool readLine(FILE *in, char *line)
{
    // Reads in as much as needed, throwing away commas and newlines
    int read;
    for(int i = 0; i < LEN; i++) {
        // Read in the next character
        read = fgetc(in);
        // If it is the end of the file, we couldn't finish, so return false
        if(read == EOF) return false;
        // Otherwise, store it in the array
        line[i] = read;
        // Throw away the next character, be it a comma or newline
        (void) fgetc(in);
    }
    // If we made it here, we read all the characters into the array, we didn't reach EOF, so return true.
    return true;
}



/**
 * This "rotates" a sequence returns the result. This means for every rotation, 
 * the first step is being moved to become the last step.
 * @param original The original sequence to rotate
 * @return The new sequence number generated by this.
*/
sequence rotateSequence(sequence original)
{
    sequence remainder = original % HIGHEST_POWER;
    return remainder * NUM_DIGITS + (original - remainder)/HIGHEST_POWER;
}



/**
 * This swaps the two given characters with eachother in the line sequence. All occurences of swapA will become swapB, and swapB will become swapA.
 * @param line The array of characters to swap in
 * @param swapA The first character to swap.
 * @param swapB The second character to swap.
*/
void swapSequence(char *line, char swapA, char swapB)
{
    for(char *lptr = line; lptr - line < LEN; lptr++) 
        if(*lptr == swapA) *lptr = swapB; 
        else if(*lptr == swapB) *lptr = swapA;
}



/**
 * Main method. Runs the program
 * @return Exit status.
*/
int main() 
{
    // Open the input file
    FILE *in = fopen(INFILE, "r");
    if(in == NULL) { // If it is null, exit
        printf("The dang file is null.\n");
        return EXIT_FAILURE;
    }
    
    // First, make the AVL Tree that will hold all of the sequence numbers
    AVLTree *seqList = makeTree();
    // Then, make an AVL Tree to hold specifically all the seed sequence numbers.
    AVLTree *seedsList = makeTree();

    // Get the constant swap list
    char swaps[] = {'0','1','0','2','0','1','0','2','0','1','0',
        '3','0','1','0','2','0','1','0','2','0','1','1','3','0',
        '1','0','2','0','1','0','2','0','1','2','3','0','1','0',
        '2','0','1','0','2','0','1'};

    // Read lines until there aren't any more.
    char line[LEN + 1] = {};
    sequence seqNum = 0ULL;
    while(readLine(in, line)) {

        // Construct the sequence num
        seqNum = (sequence) (line[0] - '0');
        for(int i = 1; i < LEN; i++) {
            seqNum *= NUM_DIGITS;
            seqNum += line[i] - '0';
        }

        // Search if not already in the list of sequences (i.e. if not based on an already known seed)
        if(treeNotContains(seqList, seqNum)) {

            // If not (we get sentinel), we have a new seed!!!
            // Add to seed and sequence list
            addNode(seqList, seqNum);
            addNode(seedsList, seqNum);
            printf("New Seed: %s, with # total additions:\n", line);
            nodesAdded = 1;

            // Check the initial rotations
            for(sequence currentSeq = rotateSequence(seqNum); 
                    currentSeq != seqNum; currentSeq = rotateSequence(currentSeq)) 
                if(treeNotContains(seqList, currentSeq)) addNode(seqList, currentSeq);
            
            // Then, for each swap
            for(int i = 0; i < 46; i += 2) { // 10 for 3 digits, 46 for 4

                // Swap the appropriate numbers
                swapSequence(line, swaps[i], swaps[i + 1]);

                // Recompute the sequence number
                seqNum = (sequence) (line[0] - '0');
                for(int j = 1; j < LEN; j++) {
                    seqNum *= NUM_DIGITS;
                    seqNum += line[j] - '0';
                }

                // Check this new number. If not in the list, add it
                if(treeNotContains(seqList, seqNum)) addNode(seqList, seqNum);

                // Then check all the rotations. If not in the list, add it
                for(sequence currentSeq = rotateSequence(seqNum); 
                        currentSeq != seqNum; currentSeq = rotateSequence(currentSeq)) 
                    if(treeNotContains(seqList, currentSeq)) addNode(seqList, currentSeq);
            }

            printf("%d\n", nodesAdded);
        }
    }
    
    // Print the sequence tree list.
    printInOrder(seqList);
    printInOrder(seedsList);

    return EXIT_SUCCESS;
}