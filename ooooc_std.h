// ============================================================
//  ooooc_std.h —— O语言标准库
//
//        超 智 慧 编 程 ， 真 实 不 虚
//
//  v4.0，一个头文件里塞下：
//    · arena 分配器（批量分配，统一释放）
//    · 泛型向量 OOOOC_VEC(T)
//    · 字符串构造器
//    · 异常 OOOOC_TRY / OOOOC_CATCH / OOOOC_THROW
//    · 测试框架
//
//  诚实声明
//  ------------------------------------------------
//  [真] arena 按 16 字节对齐，reset 后可复用
//  [真] 向量是宏，类型安全靠编译器，增长策略靠翻倍
//  [真] 异常是 setjmp/longjmp：快、小、不展开栈
//  [假] 异常不会调用栈上对象的析构函数
//       （longjmp 不认识 C 的作用域，这不是我们的错）
//  [假] 异常消息 512 字节封顶，超出部分被超智慧截断
//  [假] OOOOC_TRY 是 do { ... } while (0)，别塞进无花括号的 if/else
//  [假] 标准库不是线程安全的；部分可重入，部分看心情
//
//  用法：把 demo 里这行写进 main 第一句：
//      ooooc_console_utf8();
// ============================================================
#ifndef OOOOC_STD_H
#define OOOOC_STD_H

#include "ooooc.h"

#include <stdarg.h>
#include <setjmp.h>

// ============================================================
//  1. Arena —— 超智慧内存管理
// ============================================================
//  一次性分配一大块，按 16 字节对齐地往下发；
//  对象不需要单独 free，最后一个 ooooc_arena_free 全收。
typedef struct ooooc_arena_block ooooc_arena_block;

struct ooooc_arena_block {
    ooooc_arena_block *next;
    unsigned char     *data;
    size_t             cap;
    size_t             used;
};

typedef struct ooooc_arena {
    ooooc_arena_block *head;
    size_t             block_size;
    size_t             total;
} ooooc_arena;

#define OOOOC_ARENA_ALIGN 16

OOOOC_INLINE size_t ooooc__align_up(size_t n, size_t a)
{
    return (n + a - 1) & ~(a - 1);
}

OOOOC_INLINE ooooc_arena_block *ooooc__arena_block_new(size_t cap)
{
    ooooc_arena_block *b = (ooooc_arena_block *)ooooc_alloc(sizeof *b);
    b->next = NULL;
    b->data = (unsigned char *)ooooc_alloc(cap);
    b->cap = cap;
    b->used = 0;
    return b;
}

OOOOC_INLINE ooooc_arena *ooooc_arena_new(size_t block_size)
{
    ooooc_arena *a = (ooooc_arena *)ooooc_alloc(sizeof *a);
    a->head = NULL;
    a->block_size = block_size < 4096
        ? 4096
        : ooooc__align_up(block_size, OOOOC_ARENA_ALIGN);
    a->total = 0;
    return a;
}

OOOOC_INLINE void ooooc_arena_free(ooooc_arena *a)
{
    ooooc_arena_block *b;

    if (!a) {
        return;
    }
    b = a->head;
    while (b) {
        ooooc_arena_block *next = b->next;
        free(b->data);
        free(b);
        b = next;
    }
    free(a);
}

OOOOC_INLINE void *ooooc_arena_alloc(ooooc_arena *a, size_t size)
{
    size_t need = ooooc__align_up(size ? size : 1, OOOOC_ARENA_ALIGN);
    ooooc_arena_block *b = a->head;
    void *p;

    if (!b || b->cap - b->used < need) {
        size_t cap = a->block_size > need ? a->block_size : need;
        b = ooooc__arena_block_new(cap);
        b->next = a->head;
        a->head = b;
    }
    p = b->data + b->used;
    b->used += need;
    a->total += need;
    return p;
}

OOOOC_INLINE void *ooooc_arena_calloc(ooooc_arena *a, size_t count, size_t size)
{
    void *p = ooooc_arena_alloc(a, count * size);
    memset(p, 0, count * size);
    return p;
}

OOOOC_INLINE void ooooc_arena_reset(ooooc_arena *a)
{
    ooooc_arena_block *b;

    a->total = 0;
    for (b = a->head; b; b = b->next) {
        b->used = 0;
    }
}

OOOOC_INLINE size_t ooooc_arena_bytes(const ooooc_arena *a)
{
    return a ? a->total : 0;
}

OOOOC_INLINE char *ooooc_arena_strdup(ooooc_arena *a, const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *)ooooc_arena_alloc(a, n);
    memcpy(p, s, n);
    return p;
}

#define OOOOC_ARENA_NEW(NAME, arena) \
    (NAME##_init((NAME *)ooooc_arena_alloc((arena), sizeof(NAME))))
#define OOOOC_ARENA_STRDUP(arena, s) ooooc_arena_strdup((arena), (s))

// ============================================================
//  2. 泛型向量 —— 宏就是模板
// ============================================================
//      OOOOC_VEC(Animal *) zoo;
//      OOOOC_VEC_INIT(&zoo);
//      OOOOC_VEC_PUSH(&zoo, duck);
//      OOOOC_VEC_EACH(Animal *, it, &zoo) { ... *it ... }
//      OOOOC_VEC_FREE(&zoo);
//
//  所有向量宏都收“指向向量结构体的指针”。
//  注意：宏参数请传简单左值，别传函数调用，宏会多次求值。
#define OOOOC_VEC(T) struct { T *data; size_t len; size_t cap; }

#define OOOOC_VEC_INIT(v) \
    ((v)->data = NULL, (v)->len = 0, (v)->cap = 0)

OOOOC_INLINE void ooooc__vec_grow(void **data, size_t *cap,
                                  size_t need, size_t esz)
{
    size_t ncap = *cap ? *cap : 8;
    void *p;

    while (ncap < need) {
        if (ncap > ((size_t)-1) / 2) {
            fputs("[ooooc] 向量太大，宇宙放不下\n", stderr);
            abort();
        }
        ncap <<= 1;
    }
    if (ncap > ((size_t)-1) / esz) {
        fputs("[ooooc] 向量溢出，超智慧到此为止\n", stderr);
        abort();
    }
    p = realloc(*data, ncap * esz);
    if (!p) {
        fputs("[ooooc] 向量扩容失败：超智慧也需要 RAM\n", stderr);
        abort();
    }
    *data = p;
    *cap = ncap;
}

#define OOOOC_VEC_RESERVE(v, n) \
    (((v)->cap >= (size_t)(n)) ? (void)0 : \
     ooooc__vec_grow((void **)&(v)->data, &(v)->cap, (size_t)(n), \
                     sizeof *(v)->data))

#define OOOOC_VEC_PUSH(v, x) \
    (OOOOC_VEC_RESERVE((v), (v)->len + 1), \
     (v)->data[(v)->len++] = (x))

#define OOOOC_VEC_POP(v)    ((v)->data[--(v)->len])
#define OOOOC_VEC_AT(v, i)  ((v)->data[(i)])
#define OOOOC_VEC_CLEAR(v)  ((v)->len = 0)
#define OOOOC_VEC_EACH(T, it, v) \
    for (T *it = (v)->data; it != (v)->data + (v)->len; ++it)
#define OOOOC_VEC_FREE(v) \
    (free((v)->data), (v)->data = NULL, (v)->len = 0, (v)->cap = 0)

// ============================================================
//  3. 字符串构造器
// ============================================================
typedef struct ooooc_str {
    char  *data;
    size_t len;
    size_t cap;
} ooooc_str;

OOOOC_INLINE void ooooc__str_reserve(ooooc_str *s, size_t need)
{
    size_t ncap = s->cap ? s->cap : 32;
    char *p;

    if (s->cap >= need) {
        return;
    }
    while (ncap < need) {
        if (ncap > ((size_t)-1) / 2) {
            fputs("[ooooc] 字符串太长，宇宙放不下\n", stderr);
            abort();
        }
        ncap <<= 1;
    }
    p = (char *)realloc(s->data, ncap);
    if (!p) {
        fputs("[ooooc] 字符串扩容失败：超智慧也需要 RAM\n", stderr);
        abort();
    }
    s->data = p;
    s->cap = ncap;
}

OOOOC_INLINE void ooooc_str_init(ooooc_str *s)
{
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
    ooooc__str_reserve(s, 32);
    s->data[0] = '\0';
}

OOOOC_INLINE void ooooc_str_free(ooooc_str *s)
{
    free(s->data);
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
}

OOOOC_INLINE void ooooc_str_clear(ooooc_str *s)
{
    s->len = 0;
    if (s->data) {
        s->data[0] = '\0';
    }
}

OOOOC_INLINE void ooooc_str_append(ooooc_str *s, const char *text)
{
    size_t n = strlen(text);
    ooooc__str_reserve(s, s->len + n + 1);
    memcpy(s->data + s->len, text, n + 1);
    s->len += n;
}

OOOOC_INLINE void ooooc_str_appendf(ooooc_str *s, const char *fmt, ...)
{
    va_list ap;
    va_list ap2;
    int n;

    va_start(ap, fmt);
    va_copy(ap2, ap);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n > 0) {
        ooooc__str_reserve(s, s->len + (size_t)n + 1);
        vsnprintf(s->data + s->len, s->cap - s->len, fmt, ap2);
        s->len += (size_t)n;
    }
    va_end(ap2);
}

OOOOC_INLINE const char *ooooc_str_cstr(ooooc_str *s)
{
    return s->data ? s->data : "";
}

#define OOOOC_STR_INIT(s)   ooooc_str_init(&(s))
#define OOOOC_STR_FREE(s)   ooooc_str_free(&(s))
#define OOOOC_STR_APPEND(s, text) ooooc_str_append(&(s), (text))
#define OOOOC_STR_APPENDF(s, ...) ooooc_str_appendf(&(s), __VA_ARGS__)
#define OOOOC_STR_CSTR(s)   ooooc_str_cstr(&(s))

// ============================================================
//  4. 异常 —— 快、小、不展开栈
// ============================================================
//      OOOOC_TRY {
//          OOOOC_THROW("DietError", "%s 拒绝吃 %s", name, food);
//      } OOOOC_CATCH(ex) {
//          printf("%s: %s\n", ex->type, ex->msg);
//      } OOOOC_ENDTRY;
//
//  实现是 setjmp/longjmp + cleanup 属性：
//  TRY 块里提前 return / break / goto 也会正确弹栈。
//  但 longjmp 不会调用栈上对象的析构函数 —— 真实不虚。
//
//  另一个 C 的老规矩：在 TRY 外声明、TRY 内修改、longjmp 后还要读的
//  局部变量，请加 volatile。否则 setjmp 规则会让它变成“薛定谔的值”。
//  （本库真正使用 setjmp 的是这个宏，不是你的函数；但编译器只认规则。）
struct ooooc_exception {
    const char *type;
    char        msg[512];
    const char *file;
    int         line;
};

struct ooooc_try_frame {
    jmp_buf                  jb;
    struct ooooc_try_frame  *prev;
};

extern OOOOC_THREAD_LOCAL struct ooooc_try_frame *ooooc__try_top;
extern OOOOC_THREAD_LOCAL struct ooooc_exception ooooc__current_exc;

OOOOC_NORETURN void ooooc_throw(const char *type, const char *file, int line,
                                const char *fmt, ...);

OOOOC_INLINE void ooooc__try_cleanup(struct ooooc_try_frame *frame)
{
    if (ooooc__try_top == frame) {
        ooooc__try_top = frame->prev;
    }
}

#define OOOOC_TRY \
    do { \
        struct ooooc_try_frame ooooc__frame \
            OOOOC_CLEANUP(ooooc__try_cleanup); \
        ooooc__frame.prev = ooooc__try_top; \
        ooooc__try_top = &ooooc__frame; \
        if (setjmp(ooooc__frame.jb) == 0) {

#define OOOOC_CATCH(var) \
        } else { \
            struct ooooc_exception *var OOOOC_UNUSED = &ooooc__current_exc;

#define OOOOC_ENDTRY \
        } \
    } while (0)

#define OOOOC_THROW(TYPE, ...) \
    ooooc_throw((TYPE), __FILE__, __LINE__, __VA_ARGS__)

// ============================================================
//  5. 测试框架 —— 让超智慧可验证
// ============================================================
extern int ooooc__test_failures;

#define OOOOC_TEST(name) static void ooooc_test_##name(void)

#define OOOOC_RUN(name) \
    do { \
        int ooooc__before_ = ooooc__test_failures; \
        printf("  %-30s ", #name); \
        fflush(stdout); \
        ooooc_test_##name(); \
        if (ooooc__test_failures == ooooc__before_) { \
            printf("ok\n"); \
        } else { \
            printf("FAILED\n"); \
        } \
    } while (0)

#define OOOOC_EXPECT(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "    %s:%d: EXPECT failed: %s\n", \
                    __FILE__, __LINE__, #cond); \
            ++ooooc__test_failures; \
        } \
    } while (0)

#define OOOOC_EXPECT_INT(actual, expected) \
    do { \
        long long ooooc__a_ = (long long)(actual); \
        long long ooooc__e_ = (long long)(expected); \
        if (ooooc__a_ != ooooc__e_) { \
            fprintf(stderr, "    %s:%d: EXPECT_INT failed: %s == %lld, want %lld\n", \
                    __FILE__, __LINE__, #actual, ooooc__a_, ooooc__e_); \
            ++ooooc__test_failures; \
        } \
    } while (0)

#define OOOOC_EXPECT_STR(actual, expected) \
    do { \
        const char *ooooc__a_ = (actual); \
        const char *ooooc__e_ = (expected); \
        if (!ooooc__a_ || !ooooc__e_ || strcmp(ooooc__a_, ooooc__e_) != 0) { \
            fprintf(stderr, "    %s:%d: EXPECT_STR failed: %s == \"%s\", want \"%s\"\n", \
                    __FILE__, __LINE__, #actual, \
                    ooooc__a_ ? ooooc__a_ : "(null)", \
                    ooooc__e_ ? ooooc__e_ : "(null)"); \
            ++ooooc__test_failures; \
        } \
    } while (0)

#define OOOOC_TEST_SUMMARY() \
    (printf("\n%d failure(s)\n", ooooc__test_failures), \
     ooooc__test_failures ? 1 : 0)

// ============================================================
//  6. 平台小工具
// ============================================================
// Windows 控制台切 UTF-8；其他平台什么也不做。
void ooooc_console_utf8(void);

#endif // OOOOC_STD_H
