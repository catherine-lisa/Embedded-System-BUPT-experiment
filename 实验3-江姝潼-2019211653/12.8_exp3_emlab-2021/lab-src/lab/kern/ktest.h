/*
 * test.h
 *
 *      Author: ljp
 */

#ifndef KTEST_H_
#define KTEST_H_

#include <stdint.h>

//////////////////////////////////////////////////////
//test define

typedef struct ktest_testcase_def {
    const char  *name;
    uint32_t     run_timeout;
    int   (*setup)(void);
    void  (*test_run)(void);
    int   (*teardown)(void);
} *ktest_testcase_def_t;

typedef void (*test_unit_func)(void);

// #define KTEST_NAME_MAX_LEN (128u)

//test suite define and run
#define KTEST_DEFINE_SUITE(suite, ...) \
	static struct ktest_testcase_def TEST_SUITE_NAME(suite)[] = { \
		__VA_ARGS__, { 0 } \
	}

#define KTEST_RUN_SUITE(suite) \
	ktest_run_test_suite(#suite, TEST_SUITE_NAME(suite))


//test case define and run
#define KTEST_DEFINE_CASE_BODY(testcase, name, setup, teardown, timeout)       \
    {                                                                          \
        name,                                                                  \
        timeout,                                                               \
        setup,                                                                 \
        testcase,                                                              \
        teardown                                                               \
    }
#define KTEST_DEFINE_CASE_SIMPLE_BODY(testcase) \
	KTEST_DEFINE_CASE_BODY(testcase, STRINGIFY(testcase), unit_test_noop, unit_test_noop, 1000)

#define KTEST_DEFINE_CASE(testcase, name, setup, teardown, timeout)            \
    static const struct ktest_testcase_def TEST_CASE_NAME(testcase) =          \
    KTEST_DEFINE_CASE_BODY(testcase, name, setup, teardown, timeout);

#define KTEST_DEFINE_CASE_SIMPLE(testcase) \
	KTEST_DEFINE_CASE(testcase, STRINGIFY(testcase), unit_test_noop, unit_test_noop, 1000)

#define KTEST_RUN_CASE(test_case_func)                                  \
do {                                                                    \
    KTEST_DEFINE_CASE_SIMPLE(test_case_func);                           \
    ktest_run_test_case(&TEST_CASE_NAME(test_case_func));               \
} while(0)


//test unit define and run
#define KTEST_DEFINE_UNIT(test_unit_func) \
    static void test_unit_func(void)

#define KTEST_RUN_UNIT(test_unit_func)                                         \
    ktest_run_unit(test_unit_func, #test_unit_func);                           \
    if(ktest_failed()) return;

//misc
#define PARAM_NO_CHANGE (-1)
#define KTEST_SET_RUN_PARAM(taskmode, loop_count, verbose)   \
    ktest_set_run_param(taskmode, loop_count, verbose)

#define TEST_CASE_NAME(testcase)    _ktestcase_##testcase
#define TEST_SUITE_NAME(suite)    _ktestsuite_##suite

#define K_STRINGIFY(x) #x
#define STRINGIFY(s) K_STRINGIFY(s)

void ktest_set_run_param(int taskmode, int loop_count, int verbose);
void ktest_run_test_suite(const char *suite_name, struct ktest_testcase_def *suite);
int ktest_run_test_case(struct ktest_testcase_def *test_case);
void ktest_run_unit(test_unit_func func, const char *unit_func_name);
int ktest_failed(void);

static inline int unit_test_noop(void) {
    return 0;
}

//////////////////////////////////////////////////////
//test assert

//internal
void ktest_assert(int value, 											   const char *file, int line, const char *func, const char *msg);
void ktest_assert_string(const char *a, const char *b, 			int equal, const char *file, int line, const char *func, const char *msg);
void ktest_assert_buf   (const char *a, const char *b, int sz,  int equal, const char *file, int line, const char *func, const char *msg);

#define __KTEST_ASSERT(value, msg) ktest_assert(value, __FILE__, __LINE__, __func__, msg)

//api
#define KASSERT_UNREACHABLE()    __KTEST_ASSERT(0, "reached unreachable code")
#define KASSERT_OK(cond)         __KTEST_ASSERT(!(cond), "(" #cond ") is non-zero")

#define KASSERT_TRUE(value)      __KTEST_ASSERT(value, "(" #value ") is false")
#define KASSERT_FALSE(value)     __KTEST_ASSERT(!(value), "(" #value ") is true")
#define KASSERT_NULL(value)      __KTEST_ASSERT((const char *)(value) == NULL, "(" #value ") is not null")
#define KASSERT_NOT_NULL(value)  __KTEST_ASSERT((const char *)(value) != NULL, "(" #value ") is null")
#define KCHECK(value)  			 KASSERT_NOT_NULL(value)
#define KFAIL(message)           __KTEST_ASSERT(0, message)

#define KASSERT_INT_EQUAL(a, b)      __KTEST_ASSERT((a) == (b), "(" #a ") not equal to (" #b ")")
#define KASSERT_INT_NOT_EQUAL(a, b)  __KTEST_ASSERT((a) != (b), "(" #a ") equal to (" #b ")")

#define KASSERT_PTR_EQUAL(a, b)	    __KTEST_ASSERT((void *)(a) == (void *)(b), "(" #a ") not equal to (" #b ")")
#define KASSERT_PTR_NOT_EQUAL(a, b)	__KTEST_ASSERT((void *)(a) != (void *)(b), "(" #a ") equal to (" #b ")")


#define KASSERT_STR_EQUAL(a, b)      ktest_assert_string((const char*)(a), (const char*)(b), RT_TRUE, __FILE__, __LINE__, __func__, "string not equal")
#define KASSERT_STR_NOT_EQUAL(a, b)  ktest_assert_string((const char*)(a), (const char*)(b), RT_FALSE, __FILE__, __LINE__, __func__, "string equal")

#define KASSERT_BUF_EQUAL(a, b, sz)      ktest_assert_buf((const char*)(a), (const char*)(b), (sz), RT_TRUE, __FILE__, __LINE__, __func__, "buf not equal")
#define KASSERT_BUF_NOT_EQUAL(a, b, sz)  ktest_assert_buf((const char*)(a), (const char*)(b), (sz), RT_FALSE, __FILE__, __LINE__, __func__, "buf equal")

#define KASSERT_IN_RANGE(value, min, max)     __KTEST_ASSERT(((value >= min) && (value <= max)), "(" #value ") not in range("#min","#max")")
#define KASSERT_NOT_IN_RANGE(value, min, max) __KTEST_ASSERT(!((value >= min) && (value <= max)), "(" #value ") in range("#min","#max")")


#endif /* KTEST_H_ */
