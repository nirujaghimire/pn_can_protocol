//
// Created by peter on 6/13/2023.
//

#ifndef C_LIB_TEST_H
#define C_LIB_TEST_H

//#define BLACK   printf("\033[0;30m")
#define RED     printf("\033[0;31m")
#define GREEN   printf("\033[0;32m")
#define YELLOW  printf("\033[0;33m")
#define BLUE    printf("\033[0;34m")
#define PURPLE  printf("\033[0;35m")
//#define CYAN    printf("\033[0;36m")
//#define WHITE   printf("\033[0;37m")
#define RESET   printf("\033[0m") //Resets the text to default color
#define SOURCE(filename, line) printf("%s:%d\n",filename,line)

#define TEST_MAX_SIZE 100
typedef int (*TestTask)();
typedef struct{
    int size;
    TestTask task[TEST_MAX_SIZE];
}Test;

struct TestController{
    /**
     * Initialized the test object and return it
     * @return : Test
     */
    Test (*new)();
    /**
     * This enqueue testing task to testing que
     * @param test      : Test struct
     * @param task      : task to be tested
     * @return          : 1 for successfully added (OR) 0 for out of size
     */
    int (*addTask)(Test *test, TestTask task);
    /**
     * This will run the test and print result of test
     * @param test   : Test struct
     */
    void (*run)(Test *test);
};
extern struct TestController StaticTest;

int STRING_EQUALS( char *filename, int lineNumber,  char *expected,  char *actual);
int INT_EQUALS(char *filename, int lineNumber, int expected, int actual);
int UNSIGNED_EQUALS(char *filename, int lineNumber, unsigned int expected, unsigned int actual);
int LONG_EQUALS(char *filename, int lineNumber, long expected, long actual);
int UNSIGNED_LONG_EQUALS(char *filename, int lineNumber, unsigned long expected, unsigned long actual);
int FLOAT_EQUALS(char *filename, int lineNumber, float expected, float actual);
int DOUBLE_EQUALS(char *filename, int lineNumber, double expected, double actual);
int CHAR_EQUALS(char *filename, int lineNumber, char expected, char actual);
int INT_ARRAY_EQUALS(char *filename, int lineNumber, int *expected, int *actual, int len);
int UNSIGNED_ARRAY_EQUALS(char *filename, int lineNumber, unsigned int *expected, unsigned int *actual, int len);
int LONG_ARRAY_EQUALS(char *filename, int lineNumber, long *expected, long *actual, int len);
int UNSIGNED_LONG_ARRAY_EQUALS(char *filename, int lineNumber, unsigned long *expected, unsigned long *actual, int len);
int FLOAT_ARRAY_EQUALS(char *filename, int lineNumber, float *expected, float *actual, int len);
int DOUBLE_ARRAY_EQUALS(char *filename, int lineNumber, double *expected, double *actual, int len);
int CHAR_ARRAY_EQUALS(char *filename, int lineNumber, char *expected, char *actual, int len);
int POINTER_EQUALS(char *filename, int lineNumber, void *expected, void *actual);
int BOOLEAN_IS_TRUE(char *filename, int lineNumber, int isTrue);
int BOOLEAN_IS_FALSE(char *filename, int lineNumber, int isFalse);

#endif //C_LIB_TEST_H
