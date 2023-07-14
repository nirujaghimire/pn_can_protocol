//
// Created by peter on 6/13/2023.
//

#ifndef C_LIB_HASH_MAP_H
#define C_LIB_HASH_MAP_H

#define HASH_MAP_SIZE 3
#define HashMapType void*
#define HASH_MAP_NULL NULL

struct HashMapEntry {
    int key;
    HashMapType value;
    struct HashMapEntry *nextEntry;
};

struct HashMap {
    int size;
    struct HashMapEntry *entries[HASH_MAP_SIZE];
};
typedef struct HashMap HashMap;

struct HashMapControl {
    /**
     * Computation Cost : O(1)\n
     * It allocates the memory for HashMap and return allocated HashMap
     * @return : Allocated HashMap (!!! Must be free using free ) (OR) NULL if heap is full
     */
    HashMap *(*new)();

    /**
     * Computation Cost : O(1)\n
     * It inserts the value in map
     * @param map   : HashMap
     * @param key   : Key for value
     * @param value : Value to be inserted of type HashMapType
     * @return      : Same map (OR) NULL if heap is full or map is null
     */
    HashMap *(*insert)(HashMap *map, int key, HashMapType value);

    /**
     * Computation Cost : O(1)\n
     * It searches the value corresponding to the key and return value
     * @param map   : HashMap
     * @param key   : Key for value
     * @return      : Value corresponding to free the key (OR) NULL if key doesn't exist found
     */
    HashMapType(*get)(HashMap *map, int key);

    /**
     * Computation Cost : O(1)\n
     * It deletes the values in hash map corresponding to given key
     * @param map   : HashMap
     * @param key   : Key for value
     * @return      : Same map (OR) NULL if key is not found or map is NULL
     */
    HashMap *(*delete)(HashMap *map, int key);

    /**
     * Computation Cost : O(n)\n
     * It gives all the keys in the map
     * @param map   : HashMap
     * @param keys  : Array of integer of size equal to size of map
     * @return      : same map (OR) NULL if map is NULL
     */
    HashMap *(*getKeys)(HashMap *map, int keys[]);

    /**
     * Computation Cost : O(1)\n
     * It searches if key exist or not in map
     * @param map   : HashMap
     * @param key   : Key to be searched
     * @return      : 1 if key exist (OR) 0 if key doesn't exist
     */
    int (*isKeyExist)(HashMap *map, int key);

    /**
     * Computation Cost : O(n^2)\n
     * It delete all the entries and free memories allocated by map
     * @param mapPtr: Address of pointer toHashMap
     * @return      : 1 for success (OR) 0 for failed
     */
    int (*free)(HashMap **mapPtr);

    /**
     * Prints the contents of hash map
     * @param map  : Hash map
     */
    void (*print)(HashMap *map);

    /**
     * This return allocated memory for hash map till now
     * @return  : Allocated memories
     */
    int (*getAllocatedMemories)();
};

extern struct HashMapControl StaticHashMap;

#endif //C_LIB_HASH_MAP_H
