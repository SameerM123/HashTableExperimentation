Name: Sameer Mahmood
Student Number: 1228591
Recourses Used for Assistance: N/A
State of Implementation: Fully Functional/Complete
Hashing Algorithm: Weighted Sum

The weighted sum algorithm takes the ASCII code of the first
character in the string/key, and multiplies it by 1. The 
character to the right of that is multiplied by 2, and the 
next is multiplied by 3, and so on. The resulting index
is the weighted sum %(modulus) by the size of the hash table.

Must: "be able to be used to lovate your values based on key"
    My algorithm meets said requirement.

Should: "be able to use all the space in the table"
    Since weighted sums can sometimes produce very large 
    sums, the modulus makes the index repeat starting from 0 again,
    and therefore the range of the indexes produced by 
    this algorithm spans the entire hashtable. 

Ideally: "Should avoid creating clusters"
    Due to the large range of indexes produced, the amount 
    of resulting clustering is entirely dependant on the 
    size of the array. The larger the array, the less clustering
    occurs. If the size of the array is JUST larger than the 
    # of keys being placed into it, then clustering is 
    unavoidable. 