// ============================================================
//  demo_oop.c —— 最小可用示例：单继承、重写、RTTI
//
//  这是 v3 那个 Animal/Duck 示例的 v4 版本。
//  多出来的代码只有两行：OOOOC_DEFINE_META 和 RTTI 打印。
//
//  编译：python oooo.py run demo_oop.c
//  或者：gcc -std=gnu11 -O2 demo_oop.c -o demo_oop
// ============================================================
#include <stdio.h>

#include "ooooc.h"

// ---------- 根类：动物 ----------
OOOOC_ROOT_CLASS(Animal, int age;)

static void Animal_breathe(Animal *self)
{
    printf("Animal breathe (age=%d)\n", self->age);
}

OOOOC_VTABLE(Animal,
    OOOOC_VMETHOD(void, breathe, Animal *))

OOOOC_DEFINE_META(Animal, OOOOC_NO_DTOR)
OOOOC_DEFINE_VTABLE(Animal, Animal_breathe)
OOOOC_DEFINE_INIT(Animal,
    self->age = 5;)

// ---------- 派生类：鸭子（重写 breathe + 新增 quack） ----------
OOOOC_CLASS(Duck, Animal, int speed;)

static void Duck_breathe(Animal *self)   // override
{
    Animal *a = (Animal *)self;
    printf("Duck breathe (override!) age=%d\n", a->age);
}

static void Duck_quack(Duck *self)
{
    printf("quack! speed=%d\n", self->speed);
}

// 继承来的 breathe 必须按父类槽位顺序重新列出
OOOOC_VTABLE(Duck,
    OOOOC_VMETHOD(void, breathe, Animal *)
    OOOOC_VMETHOD(void, quack, Duck *))

OOOOC_DEFINE_META(Duck, OOOOC_NO_DTOR)
OOOOC_DEFINE_VTABLE(Duck, Duck_breathe, Duck_quack)
OOOOC_DEFINE_INIT(Duck,
    self->base.age = 2;
    self->speed = 99;)

int main(void)
{
    Animal *a = OOOOC_NEW(Animal);
    Duck   *d = OOOOC_NEW(Duck);

    OOOOC_SEND(Animal, a, breathe);                 // Animal breathe (age=5)
    OOOOC_SEND(Animal, OOOOC_UPCAST(d, Animal), breathe); // Duck breathe（重写生效）
    OOOOC_SEND(Duck, d, quack);                     // quack! speed=99

    Animal *p = OOOOC_UPCAST(d, Animal);            // 偏移 0，向上转型免费
    OOOOC_SEND(Animal, p, breathe);                 // 多态：还是 Duck 的实现

    printf("sizeof(Animal)=%d sizeof(Duck)=%d\n",
           (int)sizeof(Animal), (int)sizeof(Duck));

    // v4 新增：RTTI 是虚表第 0 槽，不额外占对象空间
    printf("RTTI: %s is Animal? %s, is Duck? %s\n",
           OOOOC_CLASSNAME(p),
           OOOOC_INSTANCEOF(p, Animal) ? "yes" : "no",
           OOOOC_INSTANCEOF(p, Duck) ? "yes" : "no");

    // 带检查的向下转型
    Duck *back = OOOOC_AS(Duck, p);
    printf("dynamic cast back to Duck: %s\n", back == d ? "same object" : "broken");

    OOOOC_DELETE(a);
    OOOOC_DELETE(d);
    return 0;
}
