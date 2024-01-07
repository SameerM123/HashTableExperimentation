#include <stdio.h>
#include <string.h> // for strcmp()
#include <ctype.h> // for isprint()

#include "hashtools.h"

/** check if the two keys are the same */
int
doKeysMatch(AAKeyType key1, size_t key1len, AAKeyType key2, size_t key2len)
{
	/** if the lengths don't match, the keys can't match */
	if (key1len != key2len)
		return 0;

	return memcmp(key1, key2, key1len) == 0;
}

/* provide the hex representation of a value */
static char toHex(int val)
{
	if (val < 10) return (char) ('0' + val);
	return (char) ('a' + (val - 10));
}

/**
 * Provide the key in a printable form.  Uses a static buffer,
 * which means that not only is this not thread-safe, but
 * even runs into trouble if called twice in the same printf().
 *
 * That said, is does provide a memory clean way to give a 
 * printable string return value to the calling code
 */
int
printableKey(char *buffer, int bufferlen, AAKeyType key, size_t printlen)
{
    int i, allChars = 1;
    char *loadptr;


    for (i = 0; allChars && i < printlen; i++) {
        if ( ! isprint(key[i])) allChars = 0;
    }

    if (allChars) {
        snprintf(buffer, bufferlen, "char key:[");
        loadptr = &buffer[strlen(buffer)];
        for (i = 0; i < printlen && loadptr - buffer < bufferlen - 2; i++) {
            *loadptr++ = key[i];
        }
        *loadptr++ = ']';
        *loadptr++ = 0;
    } else {
        snprintf(buffer, bufferlen, "hex key:[0x");
        loadptr = &buffer[strlen(buffer)];
        for (i = 0; i < printlen && loadptr - buffer < bufferlen - 4; i++) {
            *loadptr++ = toHex((key[i] & 0xf0) >> 4); // top nybble -> first hext digit
            *loadptr++ = toHex(key[i] & 0x0f);        // bottom nybble -> second digit
        }
        *loadptr++ = ']';
        *loadptr++ = 0;
    }
    return 1;
}

/**
 * Calculate a hash value based on the length of the key
 *
 * Calculate an integer index in the range [0...size-1] for
 * 		the given string key
 *
 *  @param  key  key to calculate mapping upon
 *  @param  size boundary for range of allowable return values
 *  @return      integer index associated with key
 *
 *  @see    HashAlgorithm
 */
HashIndex hashByLength(AAKeyType key, size_t keyLength, HashIndex size)
{
	return keyLength % size;
}

/**
 * Calculate a hash value based on the sum of the values in the key
 *
 * Calculate an integer index in the range [0...size-1] for
 * 		the given string key, based on the sum of the values
 *		in the key
 *
 *  param  key  key to calculate mapping upon
 *  param  size boundary for range of allowable return values
 *  return      integer index associated with key
 */
HashIndex hashBySum(AAKeyType key, size_t keyLength, HashIndex size)
{
    HashIndex sum = 0;

    // Calculate the sum of byte values of all characters in the key
    for (size_t i = 0; i < keyLength; i++) {
        sum += (HashIndex)key[i];
    }

    // Take the modulus to ensure the result fits within the table size
    return sum % size;
}


HashIndex hashByWeightSum(AAKeyType key, size_t keyLength, HashIndex size)
{
    HashIndex sum = 0;
    int multFactor = 1;

    // Calculate the sum of hex values of all characters in the key
    for (size_t i = 0; i < keyLength; i++) {
        sum += (HashIndex)(key[i]*multFactor);
        multFactor++;
    }
    // Take the modulus to ensure the result fits within the table size
    return sum % size;
}


/**
 * Locate an empty position in the given array, starting the
 * search at the indicated index, and restricting the search
 * to locations in the range [0...size-1]
 *
 *  @param  index where to begin the search
 *  @param  AssociativeArray associated AssociativeArray we are probing
 *  @param  invalidEndsSearch should the identification of a
 *				KeyDataPair marked invalid end our search?
 *				This is true if we are looking for a location
 *				to insert new data
 *  @return index of location where search stopped, or -1 if
 *				search failed
 *
 *  @see    HashProbe
 */
HashIndex linearProbe(AssociativeArray *aarray, AAKeyType key, size_t keyLength, int index, int stopOnInvalid, int *cost) {
    int currentIndex = index;
    int probeCost = 0;

    while (probeCost < aarray->size) {
        currentIndex = (currentIndex + 1) % aarray->size;
        probeCost++;

        if (aarray->table[currentIndex].validity != HASH_USED || doKeysMatch(key, keyLength, aarray->table[currentIndex].key, aarray->table[currentIndex].keylen)) {
            // If the slot is empty or has a matching key, return the index
            if (aarray->table[currentIndex].validity != HASH_USED) {
                // Increment the insert cost if an empty slot is found
                aarray->insertCost++;
            }
            return currentIndex;
        }

        if (stopOnInvalid && aarray->table[currentIndex].validity == HASH_DELETED) {
            // Stop probing if we encounter a deleted slot.
            return -1;
        }
    }

    // If we reach here, the table is full, and we couldn't find a matching key.
    return -1;
}



/**
 * Locate an empty position in the given array, starting the
 * search at the indicated index, and restricting the search
 * to locations in the range [0...size-1]
 *
 *  @param  index where to begin the search
 *  @param  hashTable associated HashTable we are probing
 *  @param  invalidEndsSearch should the identification of a
 *				KeyDataPair marked invalid end our search?
 *				This is true if we are looking for a location
 *				to insert new data
 *  @return index of location where search stopped, or -1 if
 *				search failed
 *
 *  @see    HashProbe
 */
HashIndex quadraticProbe(AssociativeArray *table, AAKeyType key, size_t keyLength, int index, int stopOnInvalid, int *cost) {

    for (int attempt = 1; /*no value needed here as stopOnInavlid is used later*/; attempt++) {
        // Calculate the quadratic probing index
        HashIndex newIndex = (index + attempt * attempt) % table->size;

        if (table->table[newIndex].validity != HASH_USED || doKeysMatch(table->table[newIndex].key, table->table[newIndex].keylen, key, keyLength)) {
            // If the slot is empty or has a matching key, return the index
            if (cost != NULL) {
                // Increment the cost counter if cost pointer is provided
                table->insertCost++;
            }
            return newIndex;
        }

        table->insertCost++;

        if (stopOnInvalid && table->table[newIndex].validity == HASH_DELETED) {
            // If we encounter a deleted slot and 'stopOnInvalid' is enabled, stop probing
            return -1;
        }
    }
}




/**
 * Locate an empty position in the given array, starting the
 * search at the indicated index, and restricting the search
 * to locations in the range [0...size-1]
 *
 *  @param  index where to begin the search
 *  @param  hashTable associated HashTable we are probing
 *  @param  invalidEndsSearch should the identification of a
 *				KeyDataPair marked invalid end our search?
 *				This is true if we are looking for a location
 *				to insert new data
 *  @return index of location where search stopped, or -1 if
 *				search failed
 *
 *  @see    HashProbe
 */
HashIndex doubleHashProbe(AssociativeArray *aarray, AAKeyType key, size_t keyLength, int index, int stopOnInvalid, int *cost) {
    // Calculate the step size using the secondary hash function
    HashIndex stepSize = aarray->hashAlgorithmSecondary(key, keyLength, aarray->size);

    for (int attempt = 0; ; attempt++) {
        // Calculate the next index based on the current index and step size
        HashIndex newIndex = (index + attempt * stepSize) % aarray->size;

        if (aarray->table[newIndex].validity != HASH_USED || doKeysMatch(aarray->table[newIndex].key, aarray->table[newIndex].keylen, key, keyLength)) {
            // If the slot is empty or has a matching key, return the index
            if (aarray->table[newIndex].validity != HASH_USED) {
                // Increment the insert cost if an empty slot is found
                aarray->insertCost++;
            }
            return newIndex;
        }
        aarray -> insertCost++;
        if (stopOnInvalid && aarray->table[newIndex].validity == HASH_DELETED) {
            // If we encounter a deleted slot and 'stopOnInvalid' is enabled, stop probing
            return -1;
        }
    }
}


