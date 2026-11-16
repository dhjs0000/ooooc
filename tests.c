// ============================================================
//  tests.c —— O语言 v4 测试套件
//
//  运行：python oooo.py test
//  （oooo.py 会自动生成符号长度注册表并强制包含，
//    这样 N_MED_AUTO 不用手写数字。）
//
//  这里测试的不是“宏能不能编译”，而是“超智慧是不是真的”。
// ============================================================
#include <stdio.h>
#include <string.h>

#include "ooooc.h"
#include "ooooc_std.h"

// ============================================================
//  命名空间：符号编码 + 自动长度注册表
// ============================================================
namespace(calc)

static int N_MED_AUTO(calc, add, ii)(int a, int b)
{
    return a + b;
}

static int N_MED_AUTO(calc, mul, ii)(int a, int b)
{
    return a * b;
}

endnamespace(calc)

// 双层命名空间：验证 N_MED3 的长度拼接
namespace(ns1)
namespace(ns2)

static int N_MED3(ns1, ns2, add, ii)(int a, int b)
{
    return a + b + 1;
}

endnamespace(ns2)
endnamespace(ns1)

OOOOC_TEST(namespace_mangling)
{
    OOOOC_EXPECT_INT(N_MED_AUTO(calc, add, ii)(2, 3), 5);
    OOOOC_EXPECT_INT(N_MED_AUTO(calc, mul, ii)(6, 7), 42);
    // 手写长度的老写法也继续可用
    OOOOC_EXPECT_INT(N_MED(4, calc, 3, add, ii)(10, 20), 30);
    // 双层命名空间
    OOOOC_EXPECT_INT(N_MED3(ns1, ns2, add, ii)(2, 3), 6);
}

// ============================================================
//  测试用类：析构顺序
// ============================================================
static char dtor_log[8];
static int  dtor_pos;

OOOOC_ROOT_CLASS(DtorBase, int b;)

OOOOC_VTABLE(DtorBase,
    OOOOC_VMETHOD(int, value, DtorBase *))

OOOOC_DEFINE_DTOR(DtorBase)
{
    (void)ooooc_self_;
    dtor_log[dtor_pos++] = 'B';
}

static int DtorBase_value(DtorBase *self)
{
    return self->b;
}

OOOOC_DEFINE_META(DtorBase, DtorBase__destroy)
OOOOC_DEFINE_VTABLE(DtorBase, DtorBase_value)
OOOOC_DEFINE_INIT(DtorBase, self->b = 1;)

OOOOC_CLASS(DtorChild, DtorBase, int c;)

OOOOC_VTABLE(DtorChild,
    OOOOC_VMETHOD(int, value, DtorBase *)
    OOOOC_VMETHOD(int, child_only, DtorChild *))

OOOOC_DEFINE_DTOR(DtorChild)
{
    (void)ooooc_self_;
    dtor_log[dtor_pos++] = 'C';
}

static int DtorChild_value(DtorBase *self)
{
    return self->b * 10;
}

static int DtorChild_only(DtorChild *self)
{
    return self->c;
}

OOOOC_DEFINE_META(DtorChild, DtorChild__destroy)
OOOOC_DEFINE_VTABLE(DtorChild, DtorChild_value, DtorChild_only)
OOOOC_DEFINE_INIT(DtorChild,
    self->base.b = 2;
    self->c = 3;)

OOOOC_TEST(class_layout)
{
    DtorChild child;

    // 父类子对象必须在偏移 0
    OOOOC_EXPECT_INT((int)offsetof(DtorChild, base), 0);
    // 派生字段紧跟在父类之后
    OOOOC_EXPECT_INT((int)offsetof(DtorChild, c), (int)sizeof(DtorBase));

    DtorChild_init(&child);
    OOOOC_EXPECT_STR(OOOOC_CLASSNAME(&child), "DtorChild");
    OOOOC_EXPECT_STR(OOOOC_TYPENAME(&child), "DtorChild");
    OOOOC_EXPECT_INT(OOOOC_IS_EXACT(&child, DtorChild), 1);
    OOOOC_EXPECT_INT(OOOOC_IS_EXACT(&child, DtorBase), 0);
}

OOOOC_TEST(virtual_dispatch)
{
    DtorChild *child = OOOOC_NEW(DtorChild);
    DtorBase  *base = OOOOC_UPCAST(child, DtorBase);

    OOOOC_EXPECT_INT(OOOOC_SEND(DtorBase, base, value), 20);
    // 继承槽位的参数类型是父类视图，派生子对象要显式转上去
    OOOOC_EXPECT_INT(OOOOC_SEND(DtorBase, OOOOC_SUPER(child), value), 20);
    OOOOC_EXPECT_INT(OOOOC_SEND(DtorChild, child, child_only), 3);

    // 去虚化：编译器知道确切类型时的直路
    OOOOC_EXPECT_INT(OOOOC_DEVIRT(DtorChild_only, child), 3);

    OOOOC_DELETE(child);
}

OOOOC_TEST(rtti)
{
    DtorChild *child = OOOOC_NEW(DtorChild);
    DtorBase  *base = OOOOC_UPCAST(child, DtorBase);

    OOOOC_EXPECT_INT(OOOOC_INSTANCEOF(base, DtorChild), 1);
    OOOOC_EXPECT_INT(OOOOC_INSTANCEOF(base, DtorBase), 1);
    OOOOC_EXPECT_INT(OOOOC_INSTANCEOF(base, NoSuchClass), 0);
    OOOOC_EXPECT(OOOOC_AS(DtorChild, base) == child);
    OOOOC_EXPECT(OOOOC_AS(DtorBase, child) == base);

    {
        DtorBase plain;
        DtorBase_init(&plain);
        OOOOC_EXPECT(OOOOC_AS(DtorChild, &plain) == NULL);
        OOOOC_DESTROY(&plain);
    }

    OOOOC_DELETE(child);
}

// ============================================================
//  接口：两个接口、跨接口转型、接口删除
// ============================================================
OOOOC_INTERFACE(IFly,
    OOOOC_IMETHOD(int, fly, IFly *))

OOOOC_INTERFACE(ISing,
    OOOOC_IMETHOD(const char *, sing, ISing *))

static int bird_destroyed;

OOOOC_ROOT_CLASS(Bird,
    int wing;
    OOOOC_IMPL(IFly, fly);
    OOOOC_IMPL(ISing, sing))

OOOOC_VTABLE(Bird,
    OOOOC_VMETHOD(int, speed, Bird *))

OOOOC_DEFINE_DTOR(Bird)
{
    (void)ooooc_self_;
    ++bird_destroyed;
}

static int Bird_speed(Bird *self)
{
    return self->wing * 2;
}

static int Bird_fly(IFly *self)
{
    Bird *b = OOOOC_SELF(Bird, fly, self);
    return b->wing;
}

static const char *Bird_sing(ISing *self)
{
    (void)self;
    return "tweet";
}

OOOOC_DEFINE_IVTABLE(Bird, IFly, fly, Bird_fly)
OOOOC_DEFINE_IVTABLE(Bird, ISing, sing, Bird_sing)
OOOOC_DEFINE_META(Bird, Bird__destroy,
    OOOOC_IFACE(Bird, IFly, fly)
    OOOOC_IFACE(Bird, ISing, sing))
OOOOC_DEFINE_VTABLE(Bird, Bird_speed)
OOOOC_DEFINE_INIT(Bird, self->wing = 2;)

OOOOC_TEST(interfaces)
{
    Bird *bird = OOOOC_NEW(Bird);
    IFly *fly = OOOOC_AS(IFly, bird);
    ISing *sing;

    OOOOC_EXPECT(fly != NULL);
    OOOOC_EXPECT_INT(OOOOC_SEND(IFly, fly, fly), 2);
    OOOOC_EXPECT(OOOOC_SELF(Bird, fly, fly) == bird);
    OOOOC_EXPECT_STR(OOOOC_TYPENAME(fly), "Bird::IFly");
    OOOOC_EXPECT_STR(OOOOC_CLASSNAME(fly), "Bird");

    // 接口 -> 另一个接口：先回到完整对象，再找 ISing 子对象
    sing = OOOOC_AS(ISing, fly);
    OOOOC_EXPECT(sing != NULL);
    OOOOC_EXPECT_STR(OOOOC_SEND(ISing, sing, sing), "tweet");

    // 接口 -> 类
    OOOOC_EXPECT(OOOOC_AS(Bird, fly) == bird);
    OOOOC_EXPECT_INT(OOOOC_INSTANCEOF(fly, Bird), 1);
    OOOOC_EXPECT_INT(OOOOC_INSTANCEOF(fly, ISing), 1);
    OOOOC_EXPECT_INT(OOOOC_INSTANCEOF(fly, IFly), 1);

    // 自动接口绑定：构造函数已经写好了两个 vptr
    OOOOC_EXPECT(OOOOC_VPTR(&bird->fly) != NULL);
    OOOOC_EXPECT(OOOOC_VPTR(&bird->sing) != NULL);

    // 通过接口指针删除：析构 + 释放完整对象
    bird_destroyed = 0;
    OOOOC_DELETE(fly);
    OOOOC_EXPECT_INT(bird_destroyed, 1);
}

OOOOC_TEST(interface_on_stack)
{
    Bird bird;

    Bird_init(&bird);
    OOOOC_EXPECT_INT(
        OOOOC_SEND(IFly, OOOOC_AS(IFly, &bird), fly), 2);

    bird_destroyed = 0;
    OOOOC_DESTROY(&bird);      // 只析构，不 free 栈内存
    OOOOC_EXPECT_INT(bird_destroyed, 1);
}

OOOOC_TEST(destructor_chain)
{
    DtorChild *child;

    dtor_pos = 0;
    memset(dtor_log, 0, sizeof dtor_log);

    child = OOOOC_NEW(DtorChild);
    OOOOC_DELETE(child);

    OOOOC_EXPECT_INT(dtor_pos, 2);
    OOOOC_EXPECT_STR(dtor_log, "CB");   // 派生先于基类
}

// ============================================================
//  arena
// ============================================================
OOOOC_TEST(arena)
{
    ooooc_arena *a = ooooc_arena_new(256);
    unsigned char *p1 = (unsigned char *)ooooc_arena_alloc(a, 1);
    unsigned char *p2 = (unsigned char *)ooooc_arena_alloc(a, 33);
    char *s;

    OOOOC_EXPECT(p1 != NULL && p2 != NULL);
    OOOOC_EXPECT(p1 != p2);
    OOOOC_EXPECT_INT((int)((size_t)p1 % OOOOC_ARENA_ALIGN), 0);
    OOOOC_EXPECT_INT((int)((size_t)p2 % OOOOC_ARENA_ALIGN), 0);
    OOOOC_EXPECT(ooooc_arena_bytes(a) >= 16 + 48);

    s = OOOOC_ARENA_STRDUP(a, "超智慧");
    OOOOC_EXPECT_STR(s, "超智慧");

    ooooc_arena_reset(a);
    OOOOC_EXPECT_INT((int)ooooc_arena_bytes(a), 0);
    OOOOC_EXPECT(ooooc_arena_alloc(a, 16) == p1);   // 复位后从第一块重新发

    ooooc_arena_free(a);
}

// ============================================================
//  向量
// ============================================================
OOOOC_TEST(vector)
{
    OOOOC_VEC(int) v;
    long sum = 0;

    OOOOC_VEC_INIT(&v);
    OOOOC_VEC_RESERVE(&v, 4);
    OOOOC_EXPECT((int)v.cap >= 4);

    for (int i = 0; i < 1000; ++i) {
        OOOOC_VEC_PUSH(&v, i);
    }
    OOOOC_EXPECT_INT((int)v.len, 1000);
    OOOOC_EXPECT_INT(OOOOC_VEC_AT(&v, 10), 10);

    OOOOC_VEC_EACH(int, it, &v)
    {
        sum += *it;
    }
    OOOOC_EXPECT_INT(sum, 499500);

    OOOOC_EXPECT_INT(OOOOC_VEC_POP(&v), 999);
    OOOOC_VEC_CLEAR(&v);
    OOOOC_EXPECT_INT((int)v.len, 0);

    OOOOC_VEC_FREE(&v);
    OOOOC_EXPECT(v.data == NULL);
    OOOOC_EXPECT_INT((int)v.cap, 0);
}

// ============================================================
//  字符串
// ============================================================
OOOOC_TEST(string_builder)
{
    ooooc_str s;

    OOOOC_STR_INIT(s);
    OOOOC_STR_APPEND(s, "hello");
    OOOOC_STR_APPENDF(s, " %d %s", 42, "ooooc");
    OOOOC_EXPECT_STR(OOOOC_STR_CSTR(s), "hello 42 ooooc");

    OOOOC_STR_APPEND(s, "!");
    OOOOC_EXPECT_STR(OOOOC_STR_CSTR(s), "hello 42 ooooc!");
    OOOOC_EXPECT_INT((int)s.len, (int)strlen("hello 42 ooooc!"));

    OOOOC_STR_FREE(s);
    OOOOC_EXPECT(s.data == NULL);
}

// ============================================================
//  异常
// ============================================================
static void throw_outer(void)
{
    OOOOC_THROW("Outer", "outer %d", 1);
}

OOOOC_TEST(exception_basic)
{
    int caught = 0;

    OOOOC_TRY {
        OOOOC_THROW("TestError", "boom %d", 7);
    } OOOOC_CATCH(ex) {
        caught = 1;
        OOOOC_EXPECT_STR(ex->type, "TestError");
        OOOOC_EXPECT_STR(ex->msg, "boom 7");
        OOOOC_EXPECT_INT(ex->line > 0, 1);
    } OOOOC_ENDTRY;

    OOOOC_EXPECT_INT(caught, 1);

    // 捕获后重来一次，帧栈应当干净
    OOOOC_TRY {
        OOOOC_EXPECT_INT(1, 1);
    } OOOOC_CATCH(ex) {
        (void)ex;
        OOOOC_EXPECT_INT(0, 1);
    } OOOOC_ENDTRY;
}

OOOOC_TEST(exception_nested)
{
    volatile int inner = 0;
    volatile int outer = 0;

    OOOOC_TRY {
        OOOOC_TRY {
            OOOOC_THROW("Inner", "inner");
        } OOOOC_CATCH(ex) {
            inner = 1;
            OOOOC_EXPECT_STR(ex->type, "Inner");
        } OOOOC_ENDTRY;

        throw_outer();
    } OOOOC_CATCH(ex) {
        outer = 1;
        OOOOC_EXPECT_STR(ex->type, "Outer");
        OOOOC_EXPECT_STR(ex->msg, "outer 1");
    } OOOOC_ENDTRY;

    OOOOC_EXPECT_INT(inner, 1);
    OOOOC_EXPECT_INT(outer, 1);
}

OOOOC_TEST(exception_rethrow)
{
    int caught = 0;

    OOOOC_TRY {
        OOOOC_TRY {
            OOOOC_THROW("First", "first");
        } OOOOC_CATCH(ex) {
            (void)ex;
            OOOOC_THROW("Second", "second");
        } OOOOC_ENDTRY;
    } OOOOC_CATCH(ex) {
        caught = 1;
        OOOOC_EXPECT_STR(ex->type, "Second");
        OOOOC_EXPECT_STR(ex->msg, "second");
    } OOOOC_ENDTRY;

    OOOOC_EXPECT_INT(caught, 1);
}

static int try_early_return(void)
{
    OOOOC_TRY {
        return 42;
    } OOOOC_CATCH(ex) {
        (void)ex;
        return -1;
    } OOOOC_ENDTRY;
    return 0;
}

OOOOC_TEST(exception_early_return)
{
    struct ooooc_try_frame *saved = ooooc__try_top;

    OOOOC_EXPECT_INT(try_early_return(), 42);
    OOOOC_EXPECT(ooooc__try_top == saved);
}

// ============================================================
//  运行
// ============================================================
int main(void)
{
    ooooc_console_utf8();
    printf("O语言 v%s 测试套件\n\n", OOOOC_VERSION_STRING);

    OOOOC_RUN(namespace_mangling);
    OOOOC_RUN(class_layout);
    OOOOC_RUN(virtual_dispatch);
    OOOOC_RUN(rtti);
    OOOOC_RUN(interfaces);
    OOOOC_RUN(interface_on_stack);
    OOOOC_RUN(destructor_chain);
    OOOOC_RUN(arena);
    OOOOC_RUN(vector);
    OOOOC_RUN(string_builder);
    OOOOC_RUN(exception_basic);
    OOOOC_RUN(exception_nested);
    OOOOC_RUN(exception_rethrow);
    OOOOC_RUN(exception_early_return);

    return OOOOC_TEST_SUMMARY();
}
