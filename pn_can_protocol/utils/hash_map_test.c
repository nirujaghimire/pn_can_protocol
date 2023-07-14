//
// Created by peter on 6/13/2023.
//

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "hash_map.h"
#include "sys/time.h"
#include "../test/test.h"

extern int hashFunc(int key);

static long getTimeInMicros() {
    struct timeval val;
    gettimeofday(&val, NULL);
    return val.tv_usec;
}

static int testNew() {
    HashMap *map = StaticHashMap.new();
    int check = BOOLEAN_IS_TRUE(__FILE__, __LINE__, NULL != map);
    for (int i = 0; i < HASH_MAP_SIZE; ++i)
        check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__,map->entries[i]==NULL);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,map->size);

    printf("testNew\n");
    StaticHashMap.free(&map);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticHashMap.getAllocatedMemories());
    return check;
}

static int testInsert() {
    //MAX SIZE = 10
    //hashKey = key % 10

    int keys[] = {1, 2, 3, 13};
    double values[] = {1.2, 3.4, 6.8, 7.9};

    HashMap *map = StaticHashMap.new();
    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.insert(map, keys[0], &values[0])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.insert(map, keys[1], &values[1])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.insert(map, keys[2], &values[2])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.insert(map, keys[3], &values[3])!=NULL);

    values[3] = 8.5;
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.insert(map, keys[3], &values[3])!=NULL);

    int mapKeys[4];
    mapKeys[0] = map->entries[hashFunc(keys[0])]->key;
    mapKeys[1] = map->entries[hashFunc(keys[1])]->key;
    mapKeys[2] = map->entries[hashFunc(keys[2])]->key;
    mapKeys[3] = map->entries[hashFunc(keys[3])]->nextEntry->key;

    double mapValues[4];
    mapValues[0] = *(double *)map->entries[hashFunc(keys[0])]->value;
    mapValues[1] = *(double *)map->entries[hashFunc(keys[1])]->value;
    mapValues[2] = *(double *)map->entries[hashFunc(keys[2])]->value;
    mapValues[3] = *(double *)map->entries[hashFunc(keys[3])]->nextEntry->value;

    check = check && INT_EQUALS(__FILE__, __LINE__,4,map->size);
    check = check && INT_ARRAY_EQUALS(__FILE__, __LINE__,keys,mapKeys,4);
    check = check && DOUBLE_ARRAY_EQUALS(__FILE__, __LINE__,values,mapValues,4);

    printf("testInsert\n");
    StaticHashMap.free(&map);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticHashMap.getAllocatedMemories());
    return check;
}

static int testGet() {
    int keys[] = {1, 2, 3, 13};
    double values[] = {1.2, 3.4, 6.8, 7.9};

    HashMap *map = StaticHashMap.new();
    StaticHashMap.insert(map, keys[0], &values[0]);
    StaticHashMap.insert(map, keys[1], &values[1]);
    StaticHashMap.insert(map, keys[2], &values[2]);
    StaticHashMap.insert(map, keys[3], &values[3]);

    double mapValues[4];
    for (int i = 0; i < 4; ++i)
        mapValues[i] = *(double *)StaticHashMap.get(map,keys[i]);

    int check = DOUBLE_ARRAY_EQUALS(__FILE__, __LINE__,values,mapValues,4);

    printf("testGet\n");
    StaticHashMap.free(&map);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticHashMap.getAllocatedMemories());
    return check;
}

static int testDelete() {
    int keys[] = {1, 2, 3, 13};
    double values[] = {1.2, 3.4, 6.8, 7.9};

    HashMap *map = StaticHashMap.new();
    StaticHashMap.insert(map, keys[0], &values[0]);
    StaticHashMap.insert(map, keys[1], &values[1]);
    StaticHashMap.insert(map, keys[2], &values[2]);
    StaticHashMap.insert(map, keys[3], &values[3]);

    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.delete(map,keys[0])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.delete(map, keys[1])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.delete(map, keys[2])!=NULL);

//    StaticHashMap.print(map);

    check = check && INT_EQUALS(__FILE__,__LINE__,1,map->size);

    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.get(map,keys[0])==NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.get(map,keys[1])==NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticHashMap.get(map,keys[2])==NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,map->entries[hashFunc(keys[3])]!=NULL);
    check = check && DOUBLE_EQUALS(__FILE__,__LINE__,*(double *)map->entries[hashFunc(keys[3])]->value,values[3]);

    printf("testDelete\n");
    StaticHashMap.free(&map);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticHashMap.getAllocatedMemories());
    return check;
}

static int testGetKeys() {
    int keys[] = {1, 2, 3, 13};
    double values[] = {1.2, 3.4, 6.8, 7.9};

    HashMap *map = StaticHashMap.new();
    StaticHashMap.insert(map, keys[0], &values[0]);
    StaticHashMap.insert(map, keys[1], &values[1]);
    StaticHashMap.insert(map, keys[2], &values[2]);
    StaticHashMap.insert(map, keys[3], &values[3]);

    int mapKeys[4];

    int check = BOOLEAN_IS_TRUE(__FILE__, __LINE__,StaticHashMap.getKeys(map,mapKeys)!=NULL);
    for (int i = 0; i < 4; ++i) {
        int each_check = 0;
        for (int j = 0; j < 4; ++j) {
            each_check = (keys[i] == mapKeys[j]) != 0;
            if(each_check)
                break;
        }
        if(!each_check){
            check = 0;
            break;
        }
    }
    check = BOOLEAN_IS_TRUE(__FILE__, __LINE__,check);

    printf("testGetKeys\n");
    StaticHashMap.free(&map);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticHashMap.getAllocatedMemories());
    return check;
}

static int testIsKeyExist() {
    int keys[] = {1, 2, 3, 13};
    double values[] = {1.2, 3.4, 6.8, 7.9};

    HashMap *map = StaticHashMap.new();
    StaticHashMap.insert(map, keys[0], &values[0]);
    StaticHashMap.insert(map, keys[1], &values[1]);
    StaticHashMap.insert(map, keys[2], &values[2]);
    StaticHashMap.insert(map, keys[3], &values[3]);

    int check = BOOLEAN_IS_TRUE(__FILE__, __LINE__,StaticHashMap.isKeyExist(map,keys[0]));
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__,StaticHashMap.isKeyExist(map,keys[1]));;
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__,StaticHashMap.isKeyExist(map,keys[2]));;
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__,StaticHashMap.isKeyExist(map,keys[3]));;

    printf("testIsKeyExist\n");
    StaticHashMap.free(&map);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticHashMap.getAllocatedMemories());
    return check;
}

static int testFree() {
    int keys[] = {1, 2, 3, 13};
    double values[] = {1.2, 3.4, 6.8, 7.9};

    HashMap *map = StaticHashMap.new();
    StaticHashMap.insert(map, keys[0], &values[0]);
    StaticHashMap.insert(map, keys[1], &values[1]);
    StaticHashMap.insert(map, keys[2], &values[2]);
    StaticHashMap.insert(map, keys[3], &values[3]);

    int check = BOOLEAN_IS_TRUE(__FILE__, __LINE__,StaticHashMap.free(&map));
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__,StaticHashMap.free(&map)==0);

    StaticHashMap.free(&map);
    printf("testFree\n");
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticHashMap.getAllocatedMemories());
    return check;
}


static void testPrint() {
    int keys[] = {1, 2, 3, 13};
    double values[] = {1.2, 3.4, 6.8, 7.9};

    HashMap *map = StaticHashMap.new();
    StaticHashMap.insert(map, keys[0], &values[0]);
    StaticHashMap.insert(map, keys[1], &values[1]);
    StaticHashMap.insert(map, keys[2], &values[2]);
    StaticHashMap.insert(map, keys[3], &values[3]);

    StaticHashMap.print(map);

    StaticHashMap.free(&map);

}

static void test(){
    Test test = StaticTest.new();

    StaticTest.addTask(&test, testNew);
    StaticTest.addTask(&test, testInsert);
    StaticTest.addTask(&test, testGet);
    StaticTest.addTask(&test, testDelete);
    StaticTest.addTask(&test, testGetKeys);
    StaticTest.addTask(&test, testIsKeyExist);
    StaticTest.addTask(&test, testFree);

    StaticTest.run(&test);
    testPrint();
}

static void demo(){
    int keys[] = {1, 2, 3, 6};
    double values[] = {1.2, 3.4, 6.8, 7.9};

    //Initialized Hash map
    HashMap *map = StaticHashMap.new();
    StaticHashMap.print(map);//Print hash map
    printf("\n");

    //Insert contents in hash map
    StaticHashMap.insert(map, keys[0], &values[0]);
    StaticHashMap.insert(map, keys[1], &values[1]);
    StaticHashMap.insert(map, keys[2], &values[2]);
    StaticHashMap.insert(map, keys[3], &values[3]);
    StaticHashMap.print(map);//Print hash map
    printf("\n");

    //Get content from hash map using key
    double* valuePtr = StaticHashMap.get(map,keys[0]);
//    printf("%d -> %f\n",keys[0],*valuePtr);

    //Get all keys
    int mapKeys[map->size];
    StaticHashMap.getKeys(map,mapKeys);
    for (int i = 0; i < map->size; ++i)
        printf("%d, ",mapKeys[i]);
    printf("\n");

    //Delete content using keys
    StaticHashMap.delete(map,keys[2]);
    StaticHashMap.delete(map,keys[1]);
    StaticHashMap.print(map);//Print hash map
    printf("\n");

    //Check if key exist or not
    printf("Key %d %s\n",keys[3],StaticHashMap.isKeyExist(map,keys[3])?"exists":"doesn't exist");
    printf("Key %d %s\n",keys[1],StaticHashMap.isKeyExist(map,keys[1])?"exists":"doesn't exist");

    //Free memory of hashmap
    printf("%s\n",StaticHashMap.free(&map)?"Map is destroyed":"Map is NULL");
    printf("%s\n",map==NULL?"Map is NULL":"Map is not NULL");

}

//int main() {
//    test();
////    demo();
//    return 0;
//}
