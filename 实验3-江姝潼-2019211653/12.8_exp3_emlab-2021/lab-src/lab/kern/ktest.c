/*
 * ktest.c
 *
 *      Author: ljp
 */

#include <string.h>
#include <stdlib.h>

#include "ktest.h"
#include "task_api.h"
#include "debug.h"


enum KTEST_ERR {
	KTEST_PASSED = 0, KTEST_FAILED = 1, KTEST_SKIPPED = 2
};

typedef struct ktest_result {
	enum KTEST_ERR error;
	uint32_t passed_num;
	uint32_t failed_num;
} *ktest_result_t;

#define TC_FAIL_LIST_SIZE                64
#define TC_FAIL_LIST_MARK_FAILED(index)  (tc_fail_list[index / 8] |= (1UL << (index % 8)))
#define TC_FAIL_LIST_IS_FAILED(index)    (tc_fail_list[index / 8] &  (1UL << (index % 8)))

static int LOCAL_TRACE = TRACE_LVL_INFO;
static ktest_testcase_def_t __ktest_suite = NULL;
static int __tc_count;
static uint32_t __param_loops = 1;
static int __param_task_mode = 0;
static uint8_t tc_fail_list_data[TC_FAIL_LIST_SIZE];
static uint8_t *tc_fail_list = tc_fail_list_data;
static struct ktest_result ktest_results = { KTEST_PASSED, 0, 0 };

int ktest_suite_init(struct ktest_testcase_def *suite) {
	/* initialize the test case table.*/
	__tc_count = 0;
	__ktest_suite = suite;

	struct ktest_testcase_def *testcase = suite;
	while (testcase->test_run) {
		__tc_count++;
		testcase++;
	}

	TRACE_I("ktest is initialize success.");
	TRACE_I("total ktest testcase num: (%d)", __tc_count);
	return __tc_count;
}

void ktest_list_testcases(void) {
	int i = 0;

	TRACE_I("TestCase list : ");

	for (i = 0; i < __tc_count; i++) {
		TRACE_I("[testcase name]:%s; [run timeout]:%d", __ktest_suite[i].name,
				__ktest_suite[i].run_timeout);
	}
}

static const char* file_basename(const char *file) {
	char *end_ptr = NULL;
	char *rst = NULL;

	if (!((end_ptr = strrchr(file, '\\')) != NULL
			|| (end_ptr = strrchr(file, '/')) != NULL) || (strlen(file) < 2)) {
		rst = (char*) file;
	} else {
		rst = (char*) (end_ptr + 1);
	}
	return (const char*) rst;
}

#define SET_PARAM(p, v) if((int)(v) != PARAM_NO_CHANGE) {p = v;}
void ktest_set_run_param(int taskmode, int loop_count, int trace_lvl) {
	SET_PARAM(__param_task_mode, taskmode);
	SET_PARAM(__param_loops, loop_count);
	SET_PARAM(LOCAL_TRACE, trace_lvl);
}

int ktest_run_test_case(struct ktest_testcase_def *test_case) {
	int tc_fail_num = 0;

	TRACE_I("[----------] [ testcase ] (%s) started", test_case->name);

	if (test_case->setup != NULL) {
		if (test_case->setup() != K_OK) {
			TRACE_E("[  FAILED  ] [ result   ] testcase (%s)", test_case->name);
			goto test_done;
		}
	}

	if (test_case->test_run != NULL) {
		test_case->test_run();
		if (ktest_results.failed_num == 0) {
			TRACE_I("[  PASSED  ] [ result   ] testcase (%s)", test_case->name);
		} else {
			tc_fail_num = 1;
			TRACE_E("[  FAILED  ] [ result   ] testcase (%s)", test_case->name);
		}
	} else {
		TRACE_E("[  FAILED  ] [ result   ] testcase (%s)", test_case->name);
	}

	if (test_case->teardown != NULL) {
		if (test_case->teardown() != K_OK) {
			TRACE_E("[  FAILED  ] [ result   ] testcase (%s)", test_case->name);
			goto test_done;
		}
	}

	test_done: TRACE_I("[----------] [ testcase ] (%s) finished",
			test_case->name);

	return tc_fail_num;
}

static int test_name_match_wild(const char *ktest_name,
		const char *ktest_tc_name) {
	if (ktest_name) {
		int len = strlen(ktest_name);
		if (ktest_name[len - 1] == '*') {
			len -= 1;
		}
		if (memcmp(ktest_tc_name, ktest_name, len) != 0) {
			return 0;
		}
	}

	return 1;
}

static void ktest_run_engine(const char *ktest_name) {
	uint32_t index;
	// task_sleep(1000);

	for (index = 0; index < __param_loops; index++) {
		int i = 0;
		uint32_t tc_fail_num = 0;
		uint32_t tc_run_num = 0;

		if (tc_fail_list) {
			memset(tc_fail_list, 0, TC_FAIL_LIST_SIZE);
		}

		TRACE_I("[==========] [ ktest    ] loop %d/%d", index + 1, __param_loops);
		TRACE_I("[==========] [ ktest    ] started");

		for (i = 0; i < __tc_count; i++) {
			if (test_name_match_wild(ktest_name, __ktest_suite[i].name)) {
				int failed = ktest_run_test_case(&__ktest_suite[i]); //run the case
				if (failed) {
					tc_fail_num++;
					TC_FAIL_LIST_MARK_FAILED(i);
				}
				tc_run_num++;
			}
		}

		if (tc_run_num == 0 && ktest_name != NULL) {
			TRACE_I("[==========] [ ktest    ] Not find (%s)", ktest_name);
			TRACE_I("[==========] [ ktest    ] finished");
			break;
		}

		TRACE_I("[==========] [ ktest    ] finished");
		TRACE_I("[==========] [ ktest    ] %d tests from %d testcase ran.",
				tc_run_num, __tc_count);
		TRACE_I("[  PASSED  ] [ result   ] %d tests.", tc_run_num - tc_fail_num);

		if (tc_fail_list && (tc_fail_num > 0)) {
			TRACE_E("[  FAILED  ] [ result   ] %d tests, listed below:",
					tc_fail_num);
			for (i = 0; i < __tc_count; i++) {
				if (TC_FAIL_LIST_IS_FAILED(i)) {
					TRACE_E("[  FAILED  ] [ result   ] %s",
							__ktest_suite[i].name);
				}
			}
		}
	} //for
}

void ktest_run_test_suite(const char *suite_name,
		struct ktest_testcase_def *suite) {
	ktest_suite_init(suite);
	suite_name = "*";//ktest_run_engine match testcase not testsuit
	if (__param_task_mode) {
		task_create_simple(ktest_run_engine, suite_name);
	} else {
		ktest_run_engine(suite_name);
	}
}

void ktest_run_unit(test_unit_func func, const char *unit_func_name) {
	// TRACE_I("[==========] ktest unit name: (%s)", unit_func_name);
	unit_func_name = unit_func_name;
	ktest_results.error = KTEST_PASSED;
	ktest_results.passed_num = 0;
	ktest_results.failed_num = 0;

	if (func != NULL) {
		func();
	}
}

ktest_result_t ktest_result_get(void) {
	return (ktest_result_t) &ktest_results;
}

int ktest_failed(void) {
	return ktest_result_get()->failed_num;
}

//////////////////////////////////////////////////////////////////////
//test assert

void ktest_assert(int value, const char *file, int line, const char *func,
		const char *msg) {
	if (!(value)) {
		ktest_results.error = KTEST_FAILED;
		ktest_results.failed_num++;
		TRACE_E("[  ASSERT  ] [ unit     ] at (%s); func: (%s:%d); msg: (%s)",
				file_basename(file), func, line, msg);
	} else {
		TRACE_D("[    OK    ] [ unit     ] (%s:%d) is passed", func, line);
		ktest_results.error = KTEST_PASSED;
		ktest_results.passed_num++;
	}
}

void ktest_assert_string(const char *a, const char *b, int equal,
		const char *file, int line, const char *func, const char *msg) {
	if (a == NULL || b == NULL) {
		ktest_assert(0, file, line, func, msg);
	}

	if (equal) {
		if (strcmp(a, b) == 0) {
			ktest_assert(1, file, line, func, msg);
		} else {
			ktest_assert(0, file, line, func, msg);
		}
	} else {
		if (strcmp(a, b) == 0) {
			ktest_assert(0, file, line, func, msg);
		} else {
			ktest_assert(1, file, line, func, msg);
		}
	}
}

void ktest_assert_buf(const char *a, const char *b, int sz, int equal,
		const char *file, int line, const char *func, const char *msg) {
	if (a == NULL || b == NULL) {
		ktest_assert(0, file, line, func, msg);
	}

	if (equal) {
		if (memcmp(a, b, sz) == 0) {
			ktest_assert(1, file, line, func, msg);
		} else {
			ktest_assert(0, file, line, func, msg);
		}
	} else {
		if (memcmp(a, b, sz) == 0) {
			ktest_assert(0, file, line, func, msg);
		} else {
			ktest_assert(1, file, line, func, msg);
		}
	}
}

