/*
 * debug.h
 *
 *      Author: ljp
 */

#ifndef DEBUG_H_
#define DEBUG_H_


extern int trace_printf(const char *format, ...);

#define TRACE_LVL_FATAL                  1
#define TRACE_LVL_ERROR                  2
#define TRACE_LVL_WARNING                3
#define TRACE_LVL_INFO                   4
#define TRACE_LVL_DEBUG                  5

#define TRACEK(x...) do { if (LOCAL_TRACE) { trace_printf(x); } } while (0)

#define TRACE_LEVEL(level, fmt, x...) do { if (LOCAL_TRACE >= (level)) { trace_printf(fmt"\n", ## x); } } while (0)
#define TRACE_F(x...) TRACE_LEVEL(TRACE_LVL_FAITAL, x)
#define TRACE_E(x...) TRACE_LEVEL(TRACE_LVL_ERROR, x)
#define TRACE_W(x...) TRACE_LEVEL(TRACE_LVL_WARNING, x)
#define TRACE_I(x...) TRACE_LEVEL(TRACE_LVL_INFO, x)
#define TRACE_D(x...) TRACE_LEVEL(TRACE_LVL_DEBUG, x)

#define PANIC(msg...) do { trace_printf(msg); while(1); }while(0)


void assert_fail(const char *assertion, const char *file, unsigned int line, const char *function);

#define ASSERT(expr) do { \
    if(!(expr)) \
	{ \
		assert_fail(#expr, __FILE__, __LINE__, __func__); \
    } \
} while(0)

#define ASSERT_MSG(expr, fmt, ...) do { \
    if(!(expr)) \
	{ \
		trace_printf(fmt, ##__VA_ARGS__); \
		assert_fail(#expr, __FILE__, __LINE__, __func__); \
    } \
} while(0)

#define BUG_ON(cond, msg) do { \
		if ((cond)) \
			assert_fail(#msg, __FILE__, __LINE__, __func__); \
	}while(0)

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#ifdef __cplusplus
#define STATIC_ASSERT(e) static_assert(e, #e)
#else
#define STATIC_ASSERT(e) _Static_assert(e, #e)
#endif
#else
#define STATIC_ASSERT(e) extern char (*ct_assert(void)) [sizeof(char[1 - 2*!(e)])]
#endif


#define _STRINGIFY(x) #x
#define _TOSTRING(x) _STRINGIFY(x)
#define __WHERE __FILE__ ":" _TOSTRING(__LINE__)

#ifdef CONFIG_DEBUG
// #define DEBUG_CONTEXT			, __FILE__ ":" _TOSTRING(__LINE__)
#define DEBUG_CONTEXT			, __PRETTY_FUNCTION__
#define DEBUG_CONTEXT_ARGS		, const char *where
#define DEBUG_PASS_CONTEXT		, where
#else
#define DEBUG_CONTEXT
#define DEBUG_CONTEXT_ARGS
#define DEBUG_PASS_CONTEXT
#endif


void debug_irq_save(int PRIMASK, const char *file, int line);
void debug_irq_restore(int PRIMASK, const char *file, int line);


#endif /* DEBUG_H_ */
