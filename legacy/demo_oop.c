
#include <stdio.h>
#include "ooooc.h"

// ---------- 根类：动物 ----------
OOOOC_ROOT_CLASS(Animal, int age;)

void Animal_breathe_impl(Animal *self) {
    printf("Animal breathe (age=%d)\n", self->age);
}

OOOOC_VTABLE(Animal,
    OOOOC_VMETHOD(void, breathe, Animal*))
OOOOC_DEFINE_VTABLE(Animal, Animal_breathe_impl)
OOOOC_DEFINE_INIT(Animal, self->age = 5;)

// ---------- 派生类：鸭子（单继承 + 重写 breathe + 新增 quack） ----------
OOOOC_CLASS(Duck, Animal, int speed;)

void Duck_breathe_impl(Animal *self) {   // override
    printf("Duck breathe (override!) age=%d\n", self->age);
}
void Duck_quack_impl(Duck *self) {
    printf("quack! speed=%d\n", self->speed);
}

OOOOC_VTABLE(Duck,
    OOOOC_VMETHOD(void, breathe, Animal*)
    OOOOC_VMETHOD(void, quack, Duck*))
OOOOC_DEFINE_VTABLE(Duck, Duck_breathe_impl, Duck_quack_impl)
OOOOC_DEFINE_INIT(Duck,
    self->base.age = 2;
    self->speed = 99;)

int main(void) {
    Animal *a = OOOOC_NEW(Animal);
    Duck   *d = OOOOC_NEW(Duck);

    OOOOC_SEND(Animal, a, breathe);                 // 期望：Animal breathe (age=5)
    OOOOC_SEND(Animal, OOOOC_UPCAST(d, Animal), breathe);  // 继承来的方法走父类视图                 // 期望：Duck breathe (override!)
    OOOOC_SEND(Duck, d, quack);                   // 期望：quack! speed=99

    Animal *p = OOOOC_UPCAST(d, Animal);    // 向上转型，vtable 在偏移 0，免费
    OOOOC_SEND(Animal, p, breathe);                 // 期望：Duck breathe —— 多态成立！

    printf("sizeof(Animal)=%zu sizeof(Duck)=%zu\n", sizeof(Animal), sizeof(Duck));

    OOOOC_DELETE(a);
    OOOOC_DELETE(d);
    return 0;
}
