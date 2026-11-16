// ============================================================
//  demo_zoo.c —— 超智慧动物园 · v4 旗舰示例
//
//  演示：单继承、虚函数重写、接口、自动接口绑定、RTTI、
//        动态转型、arena、向量、字符串、异常、按年龄排序。
//
//  编译运行：python oooo.py demo
// ============================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ooooc.h"
#include "ooooc_std.h"

// ============================================================
//  接口：会说话 / 会游泳
// ============================================================
OOOOC_INTERFACE(ISpeaker,
    OOOOC_IMETHOD(const char *, speak, ISpeaker *))

OOOOC_INTERFACE(ISwimmer,
    OOOOC_IMETHOD(const char *, swim, ISwimmer *))

// ============================================================
//  Animal —— 根类，实现 ISpeaker
// ============================================================
OOOOC_ROOT_CLASS(Animal,
    const char *name;
    int age;
    OOOOC_IMPL(ISpeaker, speaker))

// 虚表结构必须先定义：接口实现里要用 OOOOC_SEND
OOOOC_VTABLE(Animal,
    OOOOC_VMETHOD(const char *, speak, Animal *)
    OOOOC_VMETHOD(const char *, kind, Animal *))

OOOOC_DEFINE_DTOR(Animal)
{
    Animal *self = (Animal *)ooooc_self_;
    printf("    [析构] Animal %s 离开动物园\n", self->name);
}

static const char *Animal_speak(Animal *self)
{
    (void)self;
    return "……（沉默的动物）";
}

static const char *Animal_kind(Animal *self)
{
    (void)self;
    return "动物";
}

// ISpeaker 的实现：转回 Animal，再走虚函数派发。
// 这样派生类重写 speak 后，接口调用也会自动用上新实现。
static const char *Animal__ISpeaker_speak(ISpeaker *self)
{
    Animal *a = OOOOC_SELF(Animal, speaker, self);
    return OOOOC_SEND(Animal, a, speak);
}

OOOOC_DEFINE_IVTABLE(Animal, ISpeaker, speaker, Animal__ISpeaker_speak)
OOOOC_DEFINE_META(Animal, Animal__destroy,
    OOOOC_IFACE(Animal, ISpeaker, speaker))
OOOOC_DEFINE_VTABLE(Animal, Animal_speak, Animal_kind)
OOOOC_DEFINE_INIT(Animal,
    self->name = "无名兽";
    self->age = 1;)

// ============================================================
//  Duck —— 继承 Animal，实现 ISwimmer
// ============================================================
OOOOC_CLASS(Duck, Animal,
    const char *sound;
    OOOOC_IMPL(ISwimmer, swimmer))

OOOOC_VTABLE(Duck,
    OOOOC_VMETHOD(const char *, speak, Animal *)
    OOOOC_VMETHOD(const char *, kind, Animal *)
    OOOOC_VMETHOD(const char *, quack, Duck *))

OOOOC_DEFINE_DTOR(Duck)
{
    Duck *self = (Duck *)ooooc_self_;
    printf("    [析构] Duck %s 回到池塘\n", self->base.name);
}

static const char *Duck_speak(Animal *self)
{
    Duck *d = (Duck *)self;      // Animal 子对象在偏移 0，免费转
    return d->sound;
}

static const char *Duck_kind(Animal *self)
{
    (void)self;
    return "鸭子";
}

static const char *Duck_quack(Duck *self)
{
    (void)self;
    return "嘎嘎嘎";
}

static const char *Duck__ISwimmer_swim(ISwimmer *self)
{
    Duck *d = OOOOC_SELF(Duck, swimmer, self);
    (void)d;
    return "扑通扑通";
}

// 接口虚表必须先于 META 定义：META 里的接口表要拿它的地址
OOOOC_DEFINE_IVTABLE(Duck, ISwimmer, swimmer, Duck__ISwimmer_swim)
OOOOC_DEFINE_META(Duck, Duck__destroy,
    OOOOC_IFACE(Duck, ISwimmer, swimmer))
OOOOC_DEFINE_VTABLE(Duck, Duck_speak, Duck_kind, Duck_quack)
OOOOC_DEFINE_INIT(Duck,
    self->base.name = "鸭子";
    self->base.age = 3;
    self->sound = "嘎";)

// ============================================================
//  Cat —— 另一个 Animal 子类，只重写两个虚函数
// ============================================================
OOOOC_CLASS(Cat, Animal, int lives;)

static const char *Cat_speak(Animal *self)
{
    (void)self;
    return "喵——";
}

static const char *Cat_kind(Animal *self)
{
    (void)self;
    return "猫";
}

OOOOC_VTABLE(Cat,
    OOOOC_VMETHOD(const char *, speak, Animal *)
    OOOOC_VMETHOD(const char *, kind, Animal *))

OOOOC_DEFINE_META(Cat, OOOOC_NO_DTOR)
OOOOC_DEFINE_VTABLE(Cat, Cat_speak, Cat_kind)
OOOOC_DEFINE_INIT(Cat,
    self->base.name = "猫";
    self->base.age = 4;
    self->lives = 9;)

// ============================================================
//  Radio —— 不是动物，但也实现 ISpeaker
//  这就是接口相对于继承的意义：按能力聚合，而不是按血缘。
// ============================================================
OOOOC_ROOT_CLASS(Radio,
    int volume;
    OOOOC_IMPL(ISpeaker, speaker))

OOOOC_VTABLE(Radio,
    OOOOC_VMETHOD(const char *, broadcast, Radio *))

static const char *Radio_broadcast(Radio *self)
{
    static char buf[64];
    snprintf(buf, sizeof buf, "超智慧广播 FM %d.0", self->volume);
    return buf;
}

static const char *Radio__ISpeaker_speak(ISpeaker *self)
{
    Radio *r = OOOOC_SELF(Radio, speaker, self);
    return OOOOC_SEND(Radio, r, broadcast);
}

OOOOC_DEFINE_IVTABLE(Radio, ISpeaker, speaker, Radio__ISpeaker_speak)
OOOOC_DEFINE_META(Radio, OOOOC_NO_DTOR,
    OOOOC_IFACE(Radio, ISpeaker, speaker))
OOOOC_DEFINE_VTABLE(Radio, Radio_broadcast)
OOOOC_DEFINE_INIT(Radio,
    self->volume = 11;)

// ============================================================
//  业务逻辑
// ============================================================
static void announce(ISpeaker *s)
{
    if (!s) {
        printf("  [接口] 这位没有 ISpeaker 能力\n");
        return;
    }
    printf("  [接口] %s\n", OOOOC_SEND(ISpeaker, s, speak));
}

static void feed(Animal *a, const char *food)
{
    if (OOOOC_INSTANCEOF(a, Duck) && strcmp(food, "肉") == 0) {
        OOOOC_THROW("DietError", "%s 是鸭子，拒绝吃肉", a->name);
    }
    printf("  %s 吃了 %s\n", a->name, food);
}

static int by_age(const void *pa, const void *pb)
{
    const Animal *a = *(const Animal *const *)pa;
    const Animal *b = *(const Animal *const *)pb;
    return (a->age > b->age) - (a->age < b->age);
}

int main(void)
{
    ooooc_arena *arena;
    ooooc_str report;
    size_t i;

    OOOOC_VEC(Animal *) zoo;

    ooooc_console_utf8();

    puts("==============================================");
    puts("   O语言 v4 · 超智慧动物园");
    puts("   超智慧编程，真实不虚");
    puts("==============================================");

    arena = ooooc_arena_new(4096);

    Animal *a = OOOOC_NEW(Animal);
    a->name = "无名兽";
    a->age = 1;

    Duck *d = OOOOC_ARENA_NEW(Duck, arena);
    d->base.name = "唐老鸭";
    d->base.age = 3;
    d->sound = "嘎";

    Cat *c = OOOOC_NEW(Cat);
    c->base.name = "喵星人";
    c->base.age = 4;

    Radio *r = OOOOC_NEW(Radio);
    r->volume = 11;

    OOOOC_VEC_INIT(&zoo);
    OOOOC_VEC_PUSH(&zoo, a);
    OOOOC_VEC_PUSH(&zoo, OOOOC_UPCAST(d, Animal));
    OOOOC_VEC_PUSH(&zoo, OOOOC_UPCAST(c, Animal));

    // ---- 1. 多态 ----
    puts("\n[1] 多态点名：同一行 OOOOC_SEND，不同的实现");
    for (i = 0; i < zoo.len; ++i) {
        Animal *x = zoo.data[i];
        printf("  %s：%s（%s）\n",
               OOOOC_CLASSNAME(x),
               OOOOC_SEND(Animal, x, speak),
               OOOOC_SEND(Animal, x, kind));
    }

    // ---- 2. 接口 ----
    puts("\n[2] 能力查询：只有鸭子实现了 ISwimmer");
    for (i = 0; i < zoo.len; ++i) {
        Animal *x = zoo.data[i];
        ISwimmer *sw = OOOOC_AS(ISwimmer, x);
        if (sw) {
            printf("  %-8s 会游泳：%s\n", x->name,
                   OOOOC_SEND(ISwimmer, sw, swim));
        } else {
            printf("  %-8s 不会游泳\n", x->name);
        }
    }

    // ---- 3. 面向接口，不关心是不是动物 ----
    puts("\n[3] 面向接口讲话：动物和收音机走同一个入口");
    for (i = 0; i < zoo.len; ++i) {
        announce(OOOOC_AS(ISpeaker, zoo.data[i]));
    }
    announce(OOOOC_AS(ISpeaker, r));

    // ---- 4. RTTI ----
    puts("\n[4] RTTI：类型名、instanceof、跨接口动态转型");
    ISwimmer *sw = OOOOC_AS(ISwimmer, d);
    Duck *back = OOOOC_AS(Duck, sw);
    printf("  鸭子 -> ISwimmer：%s\n", sw ? "成功" : "失败");
    printf("  视图类型名：%s，类名：%s\n",
           OOOOC_TYPENAME(sw), OOOOC_CLASSNAME(sw));
    printf("  再转回 Duck：%s\n", back == d ? "还是同一个对象" : "转丢了");
    printf("  Duck 是 Animal？%s\n", OOOOC_INSTANCEOF(d, Animal) ? "是" : "否");
    printf("  Duck 是 ISpeaker？%s\n", OOOOC_INSTANCEOF(d, ISpeaker) ? "是" : "否");
    printf("  Cat 是 Duck？%s\n", OOOOC_INSTANCEOF(c, Duck) ? "是" : "否");

    // ---- 5. 异常 ----
    puts("\n[5] 异常：鸭子拒绝吃肉");
    for (i = 0; i < zoo.len; ++i) {
        OOOOC_TRY {
            feed(zoo.data[i], i == 1 ? "肉" : "面包");
        } OOOOC_CATCH(ex) {
            printf("  捕获 %s：%s\n", ex->type, ex->msg);
        } OOOOC_ENDTRY;
    }

    // ---- 6. 排序 ----
    puts("\n[6] 按年龄排序：qsort 不认识 O语言，但认识指针");
    qsort(zoo.data, zoo.len, sizeof *zoo.data, by_age);
    for (i = 0; i < zoo.len; ++i) {
        printf("  %d 岁：%s\n", zoo.data[i]->age, zoo.data[i]->name);
    }

    // ---- 7. 字符串构造器 ----
    puts("\n[7] 动物园报告");
    OOOOC_STR_INIT(report);
    OOOOC_STR_APPENDF(report, "共 %d 位居民：\n", (int)zoo.len);
    for (i = 0; i < zoo.len; ++i) {
        OOOOC_STR_APPENDF(report, "  - %s（%s，%d 岁）\n",
                          zoo.data[i]->name,
                          OOOOC_SEND(Animal, zoo.data[i], kind),
                          zoo.data[i]->age);
    }
    printf("%s", OOOOC_STR_CSTR(report));
    OOOOC_STR_FREE(report);

    // ---- 8. 析构 ----
    puts("\n[8] 析构：派生先于基类，arena 最后统一回收");
    OOOOC_VEC_FREE(&zoo);
    OOOOC_DELETE(a);
    OOOOC_DELETE(c);
    OOOOC_DELETE(r);
    OOOOC_DESTROY(d);
    ooooc_arena_free(arena);

    puts("\n超智慧动物园闭园。");
    return 0;
}
