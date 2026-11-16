// ============================================================
//  bench.c —— 超智慧性能基准
//
//  运行：python oooo.py bench
//  编译标志：-O3 -march=native -flto -fno-plt（由 oooo.py 负责）
//
//  我们不生产数字，我们只是 CPU 的搬运工。
//  目标是把“超智慧”从形容词变成 ns/op。
// ============================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ooooc.h"
#include "ooooc_std.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <time.h>
#endif

// ============================================================
//  计时器
// ============================================================
#ifdef _WIN32
static double g_freq;

static void timer_init(void)
{
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_freq = (double)f.QuadPart;
}

static double now_sec(void)
{
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / g_freq;
}
#else
static void timer_init(void) { }

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
#endif

// ============================================================
//  被测类：Shape / Circle / Square
// ============================================================
OOOOC_ROOT_CLASS(Shape, double r;)

OOOOC_VTABLE(Shape,
    OOOOC_VMETHOD(double, area, Shape *))

static double Shape_area(Shape *self)
{
    return self->r;
}

OOOOC_DEFINE_META(Shape, OOOOC_NO_DTOR)
OOOOC_DEFINE_VTABLE(Shape, Shape_area)
OOOOC_DEFINE_INIT(Shape, self->r = 1.0;)

OOOOC_CLASS(Circle, Shape)

OOOOC_VTABLE(Circle,
    OOOOC_VMETHOD(double, area, Shape *))

OOOOC_NOINLINE static double Circle_area(Shape *self)
{
    double r = ((Circle *)self)->base.r;
    return 3.14159265358979323846 * r * r;
}

OOOOC_DEFINE_META(Circle, OOOOC_NO_DTOR)
OOOOC_DEFINE_VTABLE(Circle, Circle_area)
OOOOC_DEFINE_INIT(Circle, self->base.r = 1.5;)

OOOOC_CLASS(Square, Shape)

OOOOC_VTABLE(Square,
    OOOOC_VMETHOD(double, area, Shape *))

OOOOC_NOINLINE static double Square_area(Shape *self)
{
    double r = ((Square *)self)->base.r;
    return r * r;
}

OOOOC_DEFINE_META(Square, OOOOC_NO_DTOR)
OOOOC_DEFINE_VTABLE(Square, Square_area)
OOOOC_DEFINE_INIT(Square, self->base.r = 2.0;)

// ============================================================
//  基准数据
// ============================================================
static Circle g_circle;
static Square g_square;

// volatile 指针数组：防止 LTO 把两个具体类型推导出来
static Shape *volatile g_shapes[2];

struct tagged {
    int    kind;
    double r;
};

static struct tagged g_tagged[2];

static volatile double    g_sink_d;
static volatile long long g_sink_i;

// ============================================================
//  各个基准
// ============================================================
static void bench_virtual(long n)
{
    double acc = 0.0;

    for (long i = 0; i < n; ++i) {
        Shape *s = g_shapes[i & 1];
        acc += OOOOC_SEND(Shape, s, area);
    }
    g_sink_d += acc;
}

static void bench_direct(long n)
{
    Shape *c = &g_circle.base;
    Shape *q = &g_square.base;
    double acc = 0.0;

    for (long i = 0; i < n; ++i) {
        acc += (i & 1) ? Circle_area(c) : Square_area(q);
    }
    g_sink_d += acc;
}

static void bench_switch(long n)
{
    double acc = 0.0;

    for (long i = 0; i < n; ++i) {
        const struct tagged *p = &g_tagged[i & 1];
        int kind = *(volatile const int *)&p->kind;
        if (kind == 0) {
            acc += 3.14159265358979323846 * p->r * p->r;
        } else {
            acc += p->r * p->r;
        }
    }
    g_sink_d += acc;
}

static void bench_rtti(long n)
{
    long acc = 0;

    for (long i = 0; i < n; ++i) {
        Shape *s = g_shapes[i & 1];
        if (OOOOC_AS(Circle, s) != NULL) {
            ++acc;
        }
    }
    g_sink_i += acc;
}

static void bench_arena(long n)
{
    ooooc_arena *a = ooooc_arena_new(1024 * 1024);

    for (long i = 0; i < n; ++i) {
        unsigned char *p;
        // 每 4096 次复用一次热内存：这才是 arena 的日常用法
        if ((i & 4095) == 0) {
            ooooc_arena_reset(a);
        }
        p = (unsigned char *)ooooc_arena_alloc(a, 64);
        p[0] = (unsigned char)(i & 0xFF);
    }
    g_sink_i += (long long)ooooc_arena_bytes(a);
    ooooc_arena_free(a);
}

static void bench_malloc(long n)
{
    for (long i = 0; i < n; ++i) {
        unsigned char *p = (unsigned char *)malloc(64);
        if (!p) {
            abort();
        }
        p[0] = (unsigned char)(i & 0xFF);
        free(p);
    }
    g_sink_i += 1;
}

// ============================================================
//  跑分
// ============================================================
typedef void (*bench_fn)(long n);

static double bench(const char *name, long n, bench_fn fn)
{
    double best = 1e30;

    for (int round = 0; round < 5; ++round) {
        double t0 = now_sec();
        fn(n);
        double dt = now_sec() - t0;
        if (dt < best) {
            best = dt;
        }
    }

    {
        double ns = best / (double)n * 1e9;
        double mops = (double)n / best / 1e6;
        printf("  %-26s %9.3f ns/op  %11.1f Mops/s\n", name, ns, mops);
        return ns;
    }
}

int main(void)
{
    double t_virtual;
    double t_direct;
    double t_switch;
    double t_rtti;
    double t_arena;
    double t_malloc;

    ooooc_console_utf8();
    timer_init();

    Circle_init(&g_circle);
    Square_init(&g_square);
    g_shapes[0] = &g_circle.base;
    g_shapes[1] = &g_square.base;

    g_tagged[0].kind = 0;
    g_tagged[0].r = 1.5;
    g_tagged[1].kind = 1;
    g_tagged[1].r = 2.0;

    printf("O语言 v%s 性能基准\n", OOOOC_VERSION_STRING);
    printf("编译器：%s\n", __VERSION__);
    printf("对象开销：vptr = %d 字节，RTTI 额外 = 0 字节\n",
           (int)sizeof(void *));
    printf("布局：sizeof(Shape) = %d，sizeof(Circle) = %d\n\n",
           (int)sizeof(Shape), (int)sizeof(Circle));

    puts("  基准                          耗时           吞吐");
    puts("  ------------------------------------------------------------");

    t_virtual = bench("虚函数派发（多态）", 20000000L, bench_virtual);
    t_direct  = bench("直接调用（noinline）", 20000000L, bench_direct);
    t_switch  = bench("switch 派发", 20000000L, bench_switch);
    t_rtti    = bench("RTTI 动态转换（strcmp）", 5000000L, bench_rtti);
    t_arena   = bench("arena 分配 64B", 500000L, bench_arena);
    t_malloc  = bench("malloc/free 64B", 1000000L, bench_malloc);

    printf("\n  虚函数派发相对直接调用的额外开销：%.3f ns/op（+%.0f%%）\n",
           t_virtual - t_direct,
           t_direct > 0.0 ? (t_virtual / t_direct - 1.0) * 100.0 : 0.0);
    printf("  switch 相对虚函数：%+.3f ns/op\n", t_virtual - t_switch);
    printf("  RTTI 相对一次虚调用：%.2fx\n",
           t_virtual > 0.0 ? t_rtti / t_virtual : 0.0);
    printf("  arena 相对 malloc/free：%.2fx 更快\n",
           t_arena > 0.0 ? t_malloc / t_arena : 0.0);
    printf("\n  （数字取决于 CPU、编译器和当天的心情；真实不虚。）\n");

    // 防止整个程序被优化成“什么也没干”
    if (g_sink_d == 12345.6789 || g_sink_i == 987654321) {
        puts("sink");
    }
    return 0;
}
