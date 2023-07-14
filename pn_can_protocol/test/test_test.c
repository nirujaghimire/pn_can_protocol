//
// Created by peter on 6/13/2023.
//

#include "test.h"

static int test1() { return STRING_EQUALS(__FILE__, __LINE__, "Niruja", "Peter"); }
static int test2() { return INT_EQUALS(__FILE__, __LINE__, 10, 11); }
static int test4(){return LONG_EQUALS(__FILE__, __LINE__, 15, 15);}
static int test3(){return UNSIGNED_EQUALS(__FILE__, __LINE__, 15, 15);}
static int test5(){return UNSIGNED_LONG_EQUALS(__FILE__, __LINE__, 15, 11);}
static int test6(){return FLOAT_EQUALS(__FILE__, __LINE__, 15.5f, 11);}
static int test7(){return DOUBLE_EQUALS(__FILE__, __LINE__, 15.6, 11);}
static int test8(){return CHAR_EQUALS(__FILE__, __LINE__, 'P', 'N');}

static int expectedInt[] = {1, 2, 3};
static int actualInt[] = {1, 4, 3};
static int test9(){return INT_ARRAY_EQUALS(__FILE__, __LINE__, expectedInt, actualInt, 3);}
static int test10(){return UNSIGNED_ARRAY_EQUALS(__FILE__, __LINE__, (unsigned int *) expectedInt, (unsigned int *) actualInt, 3);}
static int test11(){return LONG_ARRAY_EQUALS(__FILE__, __LINE__, (long *) expectedInt, (long *) actualInt, 3);}
static int test12(){return UNSIGNED_LONG_ARRAY_EQUALS(__FILE__, __LINE__, (unsigned long *) expectedInt, (unsigned long *) actualInt, 3);}

static float expectedFloat[] = {1, 2, 3};
static float actualFloat[] = {1, 4, 3};
static int test13(){return FLOAT_ARRAY_EQUALS(__FILE__, __LINE__, expectedFloat, actualFloat, 3);}

static double expectedDouble[] = {1, 2, 3};
static double actualDouble[] = {1, 4, 3};
static int test14(){return DOUBLE_ARRAY_EQUALS(__FILE__, __LINE__, expectedDouble, actualDouble, 3);}

static char expectedChar[] = {'A', 'B', 'C'};
static char actualChar[] = {'D', 'E', 'F'};
static int test15(){return CHAR_ARRAY_EQUALS(__FILE__, __LINE__, expectedChar, actualChar, 3);}


int pointer[]={1,2};
static int test16(){return POINTER_EQUALS(__FILE__, __LINE__,&pointer,&pointer);}

static int test17(){return BOOLEAN_IS_TRUE(__FILE__, __LINE__,1);}

//int main() {
//
//    Test test = StaticTest.new();
//
//    StaticTest.addTask(&test, test1);
//    StaticTest.addTask(&test, test2);
//    StaticTest.addTask(&test, test3);
//    StaticTest.addTask(&test, test4);
//    StaticTest.addTask(&test, test5);
//    StaticTest.addTask(&test, test6);
//    StaticTest.addTask(&test, test7);
//    StaticTest.addTask(&test, test8);
//    StaticTest.addTask(&test, test9);
//    StaticTest.addTask(&test, test10);
//    StaticTest.addTask(&test, test11);
//    StaticTest.addTask(&test, test12);
//    StaticTest.addTask(&test, test13);
//    StaticTest.addTask(&test, test14);
//    StaticTest.addTask(&test, test15);
//    StaticTest.addTask(&test, test16);
//    StaticTest.addTask(&test, test17);
//
//    StaticTest.run(&test);
//
//    return 0;
//}
