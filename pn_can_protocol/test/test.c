//
// Created by peter on 6/13/2023.
//
#include "test.h"

#include <string.h>
#include <stdio.h>

#define MAX_SIZE TEST_MAX_SIZE


/**
 * Initialized the testing object and return it
 * @return : test
 */
static Test new() {
    Test testing = {.size=0};
    for (int i = 0; i < MAX_SIZE; ++i)
        testing.task[i] = NULL;
    return testing;
}

/**
 * This enqueue test task to testing que
 * @param test      : Test struct
 * @param task      : task to be tested
 * @return          : 1 for successfully added (OR) 0 for out of size
 */
static int addTask(Test *testing, TestTask task) {
    if(testing->size>=MAX_SIZE)
        return 0;
    testing->task[testing->size++] = task;
    return 1;
}

/**
 * This will run the test and print result of test
 * @param test   : Test struct
 */
static void run(Test *test) {
    int succeedTask = 0;
    int totalTask = test->size;
    for (int i = 0; i < totalTask; ++i) {
        succeedTask += test->task[i]();
    }

    float score = (float) succeedTask * 100.0f / (float) totalTask;
    if(score<30.0f)
        RED;
    else if(score<50.0f)
        YELLOW;
    else if(score<70.0f)
        BLUE;
    else if(score<99.99f)
        PURPLE;
    else
        GREEN;

    printf("\nTotal Task : %d\n",totalTask);
    printf("Task Succeed : %d\n",succeedTask);
    printf("Success Percentage : %0.2f%%\n", score);
    RESET;
}

struct TestController StaticTest={
    .new = new,
    .addTask = addTask,
    .run = run
};


int STRING_EQUALS(char *filename, int lineNumber, char *expected, char *actual) {
    int actualLen = (int) strlen(actual);
    int expectedLen = (int) strlen(expected);

    if (actualLen == expectedLen)
        if (strcmp(actual, expected) == 0)
            return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : \"%s\" (%d)\n", expected, expectedLen);
    printf("actual : \"%s\" (%d)\n", actual, actualLen);
    RESET;
    return 0;
}

int INT_EQUALS(char *filename, int lineNumber, int expected, int actual) {
    if (expected == actual)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : %d\n", expected);
    printf("actual : %d\n", actual);
    RESET;
    return 0;
}

int UNSIGNED_EQUALS(char *filename, int lineNumber, unsigned int expected, unsigned int actual) {
    if (expected == actual)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : %u\n", expected);
    printf("actual : %u\n", actual);
    RESET;
    return 0;
}

int LONG_EQUALS(char *filename, int lineNumber, long expected, long actual) {
    if (expected == actual)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : %ld\n", expected);
    printf("actual : %ld\n", actual);
    RESET;
    return 0;
}

int UNSIGNED_LONG_EQUALS(char *filename, int lineNumber, unsigned long expected, unsigned long actual) {
    if (expected == actual)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : %lu\n", expected);
    printf("actual : %lu\n", actual);
    RESET;
    return 0;
}

int FLOAT_EQUALS(char *filename, int lineNumber, float expected, float actual) {
    if (expected == actual)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : %f\n", expected);
    printf("actual : %f\n", actual);
    RESET;
    return 0;
}

int DOUBLE_EQUALS(char *filename, int lineNumber, double expected, double actual) {
    if (expected == actual)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : %f\n", expected);
    printf("actual : %f\n", actual);
    RESET;
    return 0;
}

int CHAR_EQUALS(char *filename, int lineNumber, char expected, char actual) {
    if (expected == actual)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : \'%c\'\n", expected);
    printf("actual : \'%c\'\n", actual);
    RESET;
    return 0;
}

int INT_ARRAY_EQUALS(char *filename, int lineNumber, int *expected, int *actual, int len) {
    int i = 0;
    for (; i < len; ++i) {
        if (expected[i] != actual[i])
            break;
    }
    if (i >= len)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%d", expected[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    printf("actual : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%d", actual[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    RESET;
    return 0;
}

int UNSIGNED_ARRAY_EQUALS(char *filename, int lineNumber, unsigned int *expected, unsigned int *actual, int len) {
    int i = 0;
    for (; i < len; ++i) {
        if (expected[i] != actual[i])
            break;
    }
    if (i >= len)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%u", expected[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    printf("actual : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%u", actual[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    RESET;
    return 0;
}

int LONG_ARRAY_EQUALS(char *filename, int lineNumber, long *expected, long *actual, int len) {
    int i = 0;
    for (; i < len; ++i) {
        if (expected[i] != actual[i])
            break;
    }
    if (i >= len)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%ld", expected[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    printf("actual : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%ld", actual[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    RESET;
    return 0;
}

int
UNSIGNED_LONG_ARRAY_EQUALS(char *filename, int lineNumber, unsigned long *expected, unsigned long *actual, int len) {
    int i = 0;
    for (; i < len; ++i) {
        if (expected[i] != actual[i])
            break;
    }
    if (i >= len)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%lu", expected[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    printf("actual : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%lu", actual[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    RESET;
    return 0;
}

int FLOAT_ARRAY_EQUALS(char *filename, int lineNumber, float *expected, float *actual, int len) {
    int i = 0;
    for (; i < len; ++i) {
        if (expected[i] != actual[i])
            break;
    }
    if (i >= len)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%f", expected[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    printf("actual : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%f", actual[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    RESET;
    return 0;
}

int DOUBLE_ARRAY_EQUALS(char *filename, int lineNumber, double *expected, double *actual, int len) {
    int i = 0;
    for (; i < len; ++i) {
        if (expected[i] != actual[i])
            break;
    }
    if (i >= len)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%f", expected[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    printf("actual : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("%f", actual[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    RESET;
    return 0;
}

int CHAR_ARRAY_EQUALS(char *filename, int lineNumber, char *expected, char *actual, int len) {
    int i = 0;
    for (; i < len; ++i) {
        if (expected[i] != actual[i])
            break;
    }
    if (i >= len)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("\'%c\'", expected[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    printf("actual : ");
    for (i = 0; i < len; ++i) {
        if (i == 0)
            printf("{");
        printf("\'%c\'", actual[i]);
        printf(i == len - 1 ? "}\n" : ",");
    }
    RESET;
    return 0;
}

int POINTER_EQUALS(char *filename, int lineNumber, void *expected, void *actual) {
    if (expected == actual)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : %p \n", expected);
    printf("actual : %p \n", actual);
    RESET;
    return 0;
}

int BOOLEAN_IS_TRUE(char *filename, int lineNumber, int isTrue) {
    if (isTrue)
        return 1;

    SOURCE(filename, lineNumber);
    RED;
    printf("expected : %s \n", "TRUE");
    printf("actual : %s \n", "FALSE");
    RESET;
    return 0;
}

