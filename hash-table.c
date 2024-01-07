#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "hashtools.h"

/** forward declaration */
static HashAlgorithm lookupNamedHashStrategy(const char *name);
static HashProbe lookupNamedProbingStrategy(const char *name);

/**
 * Create a hash table of the given size,
 * which will use the given algorithm to create hash values,
 * and the given probing strategy
 *
 *  @param  hash  the HashAlgorithm to use
 *  @param  probingStrategy algorithm used for probing in the case of
 *				collisions
 *  @param  newHashSize  the size of the table (will be rounded up
 *				to the next-nearest larger prime, but see exception)
 *  @see         HashAlgorithm
 *  @see         HashProbe
 *  @see         Primes
 *
 *  @throws java.lang.IndexOutOfBoundsException if no prime number larger
 *				than newHashSize can be found (currently only primes
 *				less than 5000 are known)
 */
AssociativeArray *
aaCreateAssociativeArray(
		size_t size,
		char *probingStrategy,
		char *hashPrimary,
		char *hashSecondary
	)
{
	AssociativeArray *newTable;

	newTable = (AssociativeArray *) malloc(sizeof(AssociativeArray));

	newTable->hashAlgorithmPrimary = lookupNamedHashStrategy(hashPrimary);
	newTable->hashNamePrimary = strdup(hashPrimary);
	newTable->hashAlgorithmSecondary = lookupNamedHashStrategy(hashSecondary);
	newTable->hashNameSecondary = strdup(hashSecondary);
	newTable->hashProbe = lookupNamedProbingStrategy(probingStrategy);
	newTable->probeName = strdup(probingStrategy);

	newTable->size = getLargerPrime(size);

	if (newTable->size < 1) {
		fprintf(stderr, "Cannot create table of size %ld\n", size);
		free(newTable);
		return NULL;
	}

	newTable->table = (KeyDataPair *) malloc(newTable->size * sizeof(KeyDataPair));

	/** initialize everything with zeros */
	memset(newTable->table, 0, newTable->size * sizeof(KeyDataPair));

	newTable->nEntries = 0;

	newTable->insertCost = newTable->searchCost = newTable->deleteCost = 0;

	return newTable;
}

/**
 * deallocate all the memory in the store -- the keys (which we allocated),
 * and the store itself.
 * The user * code is responsible for managing the memory for the values
 */
void
aaDeleteAssociativeArray(AssociativeArray *aarray)
{

	if(aarray == NULL){  //nothing to delete 
		return;
	}

	free(aarray->table);  //free values in table
	free(aarray);        //free space used by table

	}


/**
 * iterate over the array, calling the user function on each valid value
 */
int aaIterateAction(
		AssociativeArray *aarray,
		int (*userfunction)(AAKeyType key, size_t keylen, void *datavalue, void *userdata),
		void *userdata
	)
{
	int i;

	for (i = 0; i < aarray->size; i++) {
		if (aarray->table[i].validity == HASH_USED) {
			if ((*userfunction)(
					aarray->table[i].key,
					aarray->table[i].keylen,
					aarray->table[i].value,
					userdata) < 0) {
				return -1;
			}
		}
	}
	return 1;
}

/** utilities to change names into functions, used in the function above */
static HashAlgorithm lookupNamedHashStrategy(const char *name)
{
    if (strncmp(name, "sum", 3) == 0) {
        return hashBySum;
    } else if (strncmp(name, "len", 3) == 0) {
        return hashByLength;
    } else if (strncmp(name, "wei", 3) == 0) {
        return hashByWeightSum;
    }

    fprintf(stderr, "Invalid hash strategy '%s' - using 'sum'\n", name);
    return hashBySum;
}


static HashProbe lookupNamedProbingStrategy(const char *name)
{
	if (strncmp(name, "lin", 3) == 0) {
		return linearProbe;
	} else if (strncmp(name, "qua", 3) == 0) {
		return quadraticProbe;
	} else if (strncmp(name, "dou", 3) == 0) {
		return doubleHashProbe;
	}

	fprintf(stderr, "Invalid hash probe strategy '%s' - using 'linear'\n", name);
	return linearProbe;
}

/**
 * Add another key and data value to the table, provided there is room.
 *
 *  @param  key  a string value used for searching later
 *  @param  value a data value associated with the key
 *  @return      the location the data is placed within the hash table,
 *				 or a negative number if no place can be found
 */
int aaInsert(AssociativeArray *aarray, AAKeyType key, size_t keylen, void *value)
{
    // Check if the table is full
    if (aarray->nEntries >= aarray->size)
    {
        // The table is full, cannot insert more entries
        printf("Reason for Error: Array too small/full to Accomodate for another insertion.\n");
        return -1;
    }


    // Calculate the initial hash index using the primary hash algorithm
    HashIndex index = aarray->hashAlgorithmPrimary(key, keylen, aarray->size);

    // Initialize variables for probing
    int originalIndex = index;
    int cost = 0;

    // Loop to find an empty slot or a slot marked as deleted
    while (aarray->table[index].validity == HASH_USED)
    {
        // Check if the key matches (including length)
        if (doKeysMatch(aarray->table[index].key, aarray->table[index].keylen, key, keylen))
        {
            // Key already exists, cannot insert
            printf("Key already exists");
            return -1;
        }

        // Increment the cost
        cost++;

        // Use the probing strategy to find the next slot
        index = aarray->hashProbe(aarray, key, keylen, originalIndex, 0, &cost);

        // If we've visited all slots, return an error
        if (index == originalIndex)
        {
            return -1;
        }
    }

    // Found an empty slot or a deleted slot, insert the new key and data
    aarray->table[index].key = (AAKeyType)strdup((char*) key);
    aarray->table[index].keylen = keylen;
    aarray->table[index].value = value;
    aarray->table[index].validity = HASH_USED;

    // Increment the number of entries
    aarray->nEntries++;

    // Return the index where the data was inserted
    return index;
}



/**
 * Locates the KeyDataPair associated with the given key, if
 * present in the table.
 *
 *  @param  key  the key to search for
 *  @return      the KeyDataPair containing the key, if the key
 *				 was present in the table, or NULL, if it was not
 *  @see         KeyDataPair
 */
void *aaLookup(AssociativeArray *aarray, AAKeyType key, size_t keylen)
{
    // Calculate the initial hash index using the primary hash algorithm
    HashIndex index = aarray->hashAlgorithmPrimary(key, keylen, aarray->size);

    // Initialize variables for probing
    int originalIndex = index;
    int cost = 0;

    // Loop to search for the key
    while (aarray->table[index].validity == HASH_USED)
    {
        aarray -> searchCost++;
        // Check if the key matches (including length)
        if (doKeysMatch(aarray->table[index].key, aarray->table[index].keylen, key, keylen))
        {
            // Key found, return the associated value
            return aarray->table[index].value;
        }

        
        

        // Use the probing strategy to find the next slot
        index = aarray->hashProbe(aarray, key, keylen, originalIndex, 0, &cost);

        // If we've visited all slots, the key is not in the table
        if (index == originalIndex)
        {
            break;
        }
    }

    // Key not found in the table
    return NULL;
}




/**
 * Locates the KeyDataPair associated with the given key, if
 * present in the table.
 *
 *  @param  key  the key to search for
 *  @return      the index of the KeyDataPair containing the key,
 *				 if the key was present in the table, or (-1),
 *				 if no key was found
 *  @see         KeyDataPair
 */
void *aaDelete(AssociativeArray *aarray, AAKeyType key, size_t keylen)
{
    // Calculate the initial hash index using the primary hash algorithm
    HashIndex index = aarray->hashAlgorithmPrimary(key, keylen, aarray->size);

    // Initialize variables for probing
    int originalIndex = index;
    int cost = 0;

    // Loop to search for the key
    while (aarray->table[index].validity == HASH_USED)
    {
        // Check if the key matches (including length)
        aarray -> deleteCost++;
        if (doKeysMatch(aarray->table[index].key, aarray->table[index].keylen, key, keylen))
        {
            // Key found, mark the slot as deleted (tombstone)
            aarray->table[index].validity = HASH_DELETED;
            // Return the associated value
            return aarray->table[index].value;
        }

        
        

        // Use the probing strategy to find the next slot
        index = aarray->hashProbe(aarray, key, keylen, originalIndex, 0, &cost);

        // If we've visited all slots, the key is not in the table
        if (index == originalIndex)
        {
            break;
        }
        // Increment the cost
        
    }

    // Key not found in the table
    return NULL;
}


/**
 * Print out the entire aarray contents
 */
void aaPrintContents(FILE *fp, AssociativeArray *aarray, char * tag)
{
	char keybuffer[128];
	int i;

	fprintf(fp, "%sDumping aarray of %d entries:\n", tag, aarray->size);
	for (i = 0; i < aarray->size; i++) {
		fprintf(fp, "%s  ", tag);
		if (aarray->table[i].validity == HASH_USED) {
			printableKey(keybuffer, 128,
					aarray->table[i].key,
					aarray->table[i].keylen);
			fprintf(fp, "%d : in use : '%s'\n", i, keybuffer);
		} else {
			if (aarray->table[i].validity == HASH_EMPTY) {
				fprintf(fp, "%d : empty (NULL)\n", i);
			} else if ( aarray->table[i].validity == HASH_DELETED) {
				printableKey(keybuffer, 128,
						aarray->table[i].key,
						aarray->table[i].keylen);
				fprintf(fp, "%d : empty (deleted - was '%s')\n", i, keybuffer);
			} else {
				fprintf(fp, "%d : invalid validity state %d\n", i,
						aarray->table[i].validity);
			}
		}
	}
}



/**
 * Print out a short summary
 */
void aaPrintSummary(FILE *fp, AssociativeArray *aarray)
{
	fprintf(fp, "Associative array contains %d entries in a table of %d size\n",
			aarray->nEntries, aarray->size);
	fprintf(fp, "Strategies used: '%s' hash, '%s' secondary hash and '%s' probing\n",
			aarray->hashNamePrimary, aarray->hashNameSecondary, aarray->probeName);
	fprintf(fp, "Costs accrued due to probing:\n");
	fprintf(fp, "  Insertion : %d\n", aarray->insertCost);
	fprintf(fp, "  Search    : %d\n", aarray->searchCost);
	fprintf(fp, "  Deletion  : %d\n", aarray->deleteCost);
}

