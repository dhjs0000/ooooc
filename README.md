# O语言 · 超智慧编程

> **超智慧编程，真实不虚。**
>
> —— O语言唯一官方宗旨，也是本项目唯一不能删的注释

O语言是一个用 C 宏实现的面向对象语言。它不修改编译器，不生成中间代码，不引入运行时依赖；它只是把 C 预处理器推到了它自己都不认识的地方。

v4.0「太初」把 v3 的骨架补成了完整的一套玩具：真的 RTTI、真的接口、自动析构、自动接口绑定、异常、arena、泛型向量、字符串构造器、测试框架、静态检查工具链和性能基准。

另附 **CCCCCC 扩展包**：中文关键字、中文标准库、中文旗舰 demo。详见 [CCCCCC.md](CCCCCC.md)。编译器不查护照。

它不打算取代 C++。它打算让你在写 C 的时候，突然拥有 class、interface、`instanceof` 和一个会帮你数虚表槽位的 Python 编译器经纪人。

---

## 三定律

1. **能用宏解决的，绝不写运行时代码。**
2. **宏解决不了的，就写进诚实声明。**
3. **真实不虚。**

这三条不是营销口号，是工程约束。比如“多重继承”宏解决不了，所以 v4 用接口真实实现，而不是继续假装 `extends(...)` 有用。

---

## 30 秒上手

环境要求：Windows / Linux / macOS，GCC 或 Clang，Python 3.8+。

本机验证：Windows + MinGW GCC 16.2.0 + Clang 22.1.8。POSIX 分支代码已写，但未在 Linux/macOS 实测；第一次在别的平台跑，请先 `python oooo.py doctor`。

```powershell
python oooo.py doctor     # 环境体检
python oooo.py test       # 编译并运行测试（默认 -Werror）
python oooo.py demo       # 旗舰演示：超智慧动物园
python oooo.py bench      # 性能基准
python oooo.py report     # 静态分析：类、接口、虚表、去虚化建议
python oooo.py zh         # CCCCCC 中文演示：中华动物园
python oooo.py confidence # 文化自信检查（玩笑，数字是真的）
python oooo.py all        # 一条龙验收
```

只想看最朴素的例子：

```powershell
python oooo.py run demo_oop.c
```

手工编译也可以（单文件示例）：

```powershell
gcc -std=gnu11 -O2 demo_oop.c -o demo_oop
```

`ooooc_std.c` 只有用了 `ooooc_std.h` 里的异常或测试框架时才必须链接；核心 `ooooc.h` 是纯头文件。

---

## 特性表

| 特性 | 状态 | 说明 |
|---|---|---|
| 单继承 | ✅ 真 | 父类子对象固定在偏移 0 |
| 虚函数 / 动态派发 | ✅ 真 | 虚表第 0 槽是类型信息，方法从第 1 槽开始 |
| RTTI | ✅ 真 | `OOOOC_CLASSNAME`、`OOOOC_INSTANCEOF`、`OOOOC_AS` |
| 接口 | ✅ 真 | 多重继承以接口方式实现，支持跨接口转型 |
| 自动接口绑定 | ✅ 真 | 构造函数沿继承链自动写入所有接口 vptr |
| 自动析构 | ✅ 真 | 派生先于基类，逐级调用 |
| 命名空间 / 符号编码 | ✅ 真 | Itanium 风格，长度由 `oooo.py` 自动数 |
| 异常 | ⚠️ 真但有限 | `setjmp/longjmp`；不展开栈，局部变量见下方诚实声明 |
| arena 分配器 | ✅ 真 | 16 字节对齐，批量释放，`reset` 复用 |
| 泛型向量 | ✅ 宏 | `OOOOC_VEC(T)`，类型安全靠编译器 |
| 字符串构造器 | ✅ 真 | 自动扩容，`OOOOC_STR_APPENDF` |
| 中文关键字（CCCCCC） | ✅ 宏层 | 类、接口、异常、标准库的中文别名，见 [CCCCCC.md](CCCCCC.md) |
| 静态检查 | ✅ 真 | 槽位数、继承前缀、接口/META 一致性 |
| 多重继承 | ❌ 假 | 请用接口；`extends(...)` 仅保留用于安慰旧代码 |
| 模板 | ❌ 假 | 宏就是模板，模板就是宏 |
| 线程安全 | ❌ 不承诺 | RTTI 只读；异常帧是线程局部的；其他靠你 |
| ABI 稳定 | ❌ 不承诺 | 改一个槽，全部重编 |
| MSVC | ❌ 不支持 | 需要 GCC / Clang；MSVC 的宏不够超智慧 |

---

## 一个类长什么样

来自 [`demo_oop.c`](demo_oop.c)：

```c
OOOOC_ROOT_CLASS(Animal, int age;)

OOOOC_VTABLE(Animal,
    OOOOC_VMETHOD(void, breathe, Animal *))

static void Animal_breathe(Animal *self)
{
    printf("Animal breathe (age=%d)\n", self->age);
}

OOOOC_DEFINE_META(Animal, OOOOC_NO_DTOR)
OOOOC_DEFINE_VTABLE(Animal, Animal_breathe)
OOOOC_DEFINE_INIT(Animal, self->age = 5;)
```

派生类：

```c
OOOOC_CLASS(Duck, Animal, int speed;)

OOOOC_VTABLE(Duck,
    OOOOC_VMETHOD(void, breathe, Animal *)   // 继承槽必须按父类顺序重列
    OOOOC_VMETHOD(void, quack, Duck *))

static void Duck_breathe(Animal *self) { /* override */ }
static void Duck_quack(Duck *self)     { /* new */ }

OOOOC_DEFINE_META(Duck, OOOOC_NO_DTOR)
OOOOC_DEFINE_VTABLE(Duck, Duck_breathe, Duck_quack)
OOOOC_DEFINE_INIT(Duck,
    self->base.age = 2;
    self->speed = 99;)
```

调用：

```c
Animal *a = OOOOC_NEW(Animal);
Duck   *d = OOOOC_NEW(Duck);
Animal *p = OOOOC_UPCAST(d, Animal);   // 偏移 0，免费

OOOOC_SEND(Animal, a, breathe);
OOOOC_SEND(Animal, p, breathe);        // 多态：Duck 的实现
OOOOC_SEND(Duck, d, quack);
```

---

## 对象布局：为什么 RTTI 是免费的

```
对象                          虚表                          类型信息
+----------------+          +---------------------+        +---------------------+
| vtable 指针     | -------> | meta 指针 (槽 0)     | -----> | name                |
+----------------+          +---------------------+        | parent              |
| 父类子对象/字段 |          | 方法槽 0             |        | size                |
| ...            |          | 方法槽 1 ...         |        | destroy             |
+----------------+          +---------------------+        | ifaces              |
                                                           | self_offset         |
                                                           | flags               |
                                                           +---------------------+
```

- 对象开销：一个指针（x86-64 下 8 字节）。
- RTTI 额外开销：**0 字节**，因为类型信息指针就藏在虚表第 0 槽。
- 动态派发：读对象第 0 个字 → 读虚表槽 → 间接调用。和 C++ 单继承同一量级。

接口不是玄学，是一个带 vptr 的小子对象：

```
Bird
+0   vptr        -> Bird_vtable
+8   wing
+16  fly.vptr    -> Bird::IFly 视图虚表
+24  sing.vptr   -> Bird::ISing 视图虚表
```

接口表项记录 `{接口名, 偏移, 接口虚表}`。构造函数 `OOOOC_DEFINE_INIT` 会沿着继承链自动把这些 vptr 绑好；`OOOOC_AS` 会先把接口指针减回完整对象，再按名字查类链和接口表。所以下面这些都是真的：

```c
IFly  *fly  = OOOOC_AS(IFly, bird);    // 类 -> 接口
ISing *sing = OOOOC_AS(ISing, fly);    // 接口 -> 另一个接口
Bird  *back = OOOOC_AS(Bird, fly);     // 接口 -> 类
OOOOC_DELETE(fly);                     // 通过接口指针删除，正确 free 完整对象
```

完整演示见 [`demo_zoo.c`](demo_zoo.c)。

---

## 接口完整写法

以 `demo_zoo.c` 的鸭子为例：

```c
OOOOC_INTERFACE(ISwimmer,
    OOOOC_IMETHOD(const char *, swim, ISwimmer *))

OOOOC_CLASS(Duck, Animal,
    const char *sound;
    OOOOC_IMPL(ISwimmer, swimmer))          // 接口子对象 + 成员名

OOOOC_VTABLE(Duck,
    OOOOC_VMETHOD(const char *, speak, Animal *)
    OOOOC_VMETHOD(const char *, kind, Animal *)
    OOOOC_VMETHOD(const char *, quack, Duck *))

static const char *Duck__ISwimmer_swim(ISwimmer *self)
{
    Duck *d = OOOOC_SELF(Duck, swimmer, self);  // 接口视图 -> 实现类
    (void)d;
    return "扑通扑通";
}

// 顺序很重要：接口虚表必须先于 META 定义
OOOOC_DEFINE_IVTABLE(Duck, ISwimmer, swimmer, Duck__ISwimmer_swim)
OOOOC_DEFINE_META(Duck, Duck__destroy,
    OOOOC_IFACE(Duck, ISwimmer, swimmer))
OOOOC_DEFINE_VTABLE(Duck, Duck_speak, Duck_kind, Duck_quack)
OOOOC_DEFINE_INIT(Duck,
    self->base.name = "鸭子";
    self->base.age = 3;
    self->sound = "嘎";)
```

接口的 `vptr` 由构造函数自动绑定，不需要在 init 里手写 `OOOOC_INIT_IMPL`。`OOOOC_INIT_IMPL` 仍保留，用于特殊场合的手工补绑。

---

## 异常：快、小、不展开栈

```c
OOOOC_TRY {
    feed(duck, "肉");
} OOOOC_CATCH(ex) {
    printf("捕获 %s：%s\n", ex->type, ex->msg);
} OOOOC_ENDTRY;
```

`OOOOC_THROW("DietError", "%s 拒绝吃肉", name)` 会记录类型、消息、文件、行号，然后 `longjmp` 到最近的 `OOOOC_TRY`。

诚实声明：

- `longjmp` **不会**调用栈上对象的析构函数。异常安全不是本项目的强项。
- 在 `TRY` 外声明、`TRY` 内修改、longjmp 后还要读的局部变量请加 `volatile`。这是 `setjmp` 的老规矩，不是 O语言的新规矩。
- 消息缓冲区 512 字节，超出截断。
- `TRY` 块里提前 `return` / `break` 会通过 cleanup 属性正确弹栈；这一点比裸 `setjmp` 舒服。

---

## 标准库

[`ooooc_std.h`](ooooc_std.h) 提供：

```c
// arena：批量分配，统一释放
ooooc_arena *a = ooooc_arena_new(4096);
Duck *d = OOOOC_ARENA_NEW(Duck, a);
char *s = OOOOC_ARENA_STRDUP(a, "超智慧");
OOOOC_DESTROY(d);          // 只析构，不 free
ooooc_arena_free(a);       // 统统一把收走

// 向量：宏就是模板
OOOOC_VEC(Animal *) zoo;
OOOOC_VEC_INIT(&zoo);
OOOOC_VEC_PUSH(&zoo, animal);
OOOOC_VEC_EACH(Animal *, it, &zoo) { puts((*it)->name); }
OOOOC_VEC_FREE(&zoo);

// 字符串构造器
ooooc_str report;
OOOOC_STR_INIT(report);
OOOOC_STR_APPENDF(report, "共 %d 位居民\n", (int)zoo.len);
puts(OOOOC_STR_CSTR(report));
OOOOC_STR_FREE(report);
```

注意：向量宏会多次求值参数，请传简单左值；所有向量宏都接收“指向向量结构体的指针”。

---

## CCCCCC 扩展包：中文也能当第一等公民

[`CCCCCC.md`](CCCCCC.md) 里有一层中文关键字宏，能让下面这种代码编译、多态、RTTI、抛异常：

```c
#include "cccccc_std.h"

接口(会游泳, 接口方法(常量 字符 *, 游泳, 会游泳 *))
类(动物, 常量 字符 *名字; 整数 年龄;)
派生(鸭子, 动物, 常量 字符 *叫声; 实现接口(会游泳, 泳姿))

整数 主函数(空) {
    鸭子 *乙 = 新建(鸭子);
    打印("游泳：%s\n", 发送(会游泳, 当作(会游泳, 乙), 游泳));
    删除(乙);
    返回 0;
}
```

运行：

```powershell
python oooo.py zh           # 中华动物园
python oooo.py 自信         # 文化自信检查（玩笑，数字是真的）
python oooo.py report demo_zh.c
```

这不是新编译器，是宏层汉化；`#include` 仍是英文，因为预处理器只认这个。中文标识符依赖 GCC/Clang 的 UTF-8 支持。

> 编译器不查护照。文化自信不是排斥英文，是让中文也能当第一等公民。

---

## 工具链：`oooo.py`

`oooo.py` 不是编译器，是编译器的经纪人。

| 命令 | 作用 |
|---|---|
| `python oooo.py doctor` | 检查 Python、GCC/Clang、文件、编译自检 |
| `python oooo.py test` | 构建并运行 `tests.c`，默认 `-Werror` |
| `python oooo.py demo` | 构建并运行旗舰演示 |
| `python oooo.py zh` | 构建并运行 CCCCCC 中文演示 |
| `python oooo.py confidence` | 文化自信检查（玩笑） |
| `python oooo.py bench` | 构建并运行性能基准 |
| `python oooo.py report` | 静态分析类、接口、虚表、去虚化建议 |
| `python oooo.py all` | 测试 + 分析 + 演示 + 中文演示 + 基准 |
| `python oooo.py build a.c` | 编译任意源文件 |
| `python oooo.py run a.c` | 编译并运行 |
| `python oooo.py registry -o reg.h` | 生成符号长度注册表 |
| `python oooo.py clean` | 清空 `build/` |

构建流程：

1. 扫描源文件和根目录头文件里的所有标识符；
2. 生成 `build/ooooc_registry.h`，给每个名字定义 `OOOOC_LENOF_<name>`；
3. 用 `-include` 强制包含注册表；
4. 调 GCC / Clang 编译；
5. 自动链接 `ooooc_std.c`（除非 `--no-std`）。

GCC 使用 `-O3 -march=native -flto -fno-plt`；Clang 自动改用 `-flto -fuse-ld=lld`（没有 lld 就不开 LTO）。

### 静态分析检查什么

[`oooo.py report`](oooo.py) 解析 O语言宏调用，检查：

- 类是否同时有 `OOOOC_VTABLE`、`OOOOC_DEFINE_VTABLE`、`OOOOC_DEFINE_META`、`OOOOC_DEFINE_INIT`；
- 虚表声明的槽位数与定义里填的实现数是否一致；
- 子类虚表前缀是否与父类兼容（名字、返回类型、参数签名；防止向上转型错位）；
- 接口方法数、`OOOOC_DEFINE_IVTABLE`、`META` 里的 `OOOOC_IFACE` 是否三方一致；
- 启发式去虚化建议：同一函数内 `OOOOC_NEW(X)` 出来的变量，如果接下来用 `OOOOC_SEND(X, var, ...)`，可以考虑 `OOOOC_DEVIRT`。

本次运行的真实输出（节选）：

```
类：10 个    接口：4 个    虚方法：15 个
OOOOC_SEND/发送：14 处    OOOOC_AS/当作：13 处    OOOOC_NEW/新建：7 处

类层次
  Animal [root]  (2 槽)
    |- Cat : Animal  (2 槽)
    `- Duck : Animal  (3 槽)
  Bird [root]  (1 槽)
  DtorBase [root]  (1 槽)
    `- DtorChild : DtorBase  (2 槽)
  Radio [root]  (1 槽)
  Shape [root]  (1 槽)
    |- Circle : Shape  (1 槽)
    `- Square : Shape  (1 槽)

检查
  [OK] 类定义、虚表槽位、继承前缀、接口绑定全部一致

去虚化建议（启发式）
  共 1 处可考虑 OOOOC_DEVIRT（省一次间接跳转）
  · tests.c:133  OOOOC_SEND(DtorChild, child, child_only)  <- DtorChild 由 OOOOC_NEW(DtorChild) 创建
```

这不是真正的编译器，但对宏语言来说，能数槽位已经算超智慧了。

---

## 性能基准

测试机：**Intel Core i7-9750H @ 2.60GHz**，Windows，GCC 16.2.0，`-O3 -march=native -flto -fno-plt`，取 5 次最佳。

| 基准 | 耗时 | 吞吐 |
|---|---:|---:|
| 虚函数派发（多态） | 2.317 ns/op | 431.5 Mops/s |
| 直接调用（noinline） | 2.159 ns/op | 463.2 Mops/s |
| switch 派发 | 1.389 ns/op | 719.7 Mops/s |
| RTTI 动态转换（strcmp） | 9.252 ns/op | 108.1 Mops/s |
| arena 分配 64B | 3.097 ns/op | 322.9 Mops/s |
| malloc/free 64B | 71.516 ns/op | 14.0 Mops/s |

> 同一台机器重跑，数字会在 ±20%–30% 左右波动（笔记本 CPU、后台程序、缓存）。这张表是一次代表性运行，不是承诺。

解读（顺便诚实）：

- 虚派发比直接调用多 **0.1–0.5 ns/op**（这张表是 +7%，重跑可能到 +20% 以上）。害怕虚函数的人可以松一口气；现代 CPU 的间接调用预测很强，但具体数字看当天状态。
- `switch` 比虚派发快约 1 ns，但类型集合必须封闭。想加一种动物，`switch` 要改，虚表不用。
- RTTI 是虚调用的 **约 3–4.5 倍**，主要花在 `strcmp` 上。类型已知时用 `OOOOC_UPCAST` 或 `OOOOC_DEVIRT`；需要检查时再用 `OOOOC_AS`。
- arena 比 `malloc/free` 快 **约 18–23 倍**（64B 小对象、热内存）。批量生命周期用 arena，单对象用 `OOOOC_NEW`。
- 数字会随 CPU、编译器、后台程序变化；结论不一定。

复现：

```powershell
python oooo.py bench
```

---

## v3 → v4：发生了什么

v3 已经有“类、虚表、单继承、消息发送”，v4 补的是让它真的像一门语言的东西：

| v3 | v4 |
|---|---|
| 手动写 `OOOOC_LENOF_*` | `oooo.py` 自动扫描生成 |
| 无 RTTI | 虚表第 0 槽类型信息，`CLASSNAME` / `INSTANCEOF` / `AS` |
| 多重继承靠 `extends(...)` 信仰 | 接口真实实现，支持跨接口转型 |
| 析构靠 `free` | 自动析构，派生先于基类 |
| 接口要手工绑定 vptr | 构造函数自动绑定整条继承链 |
| 只有头文件 | 标准库：arena、向量、字符串、异常、测试框架 |
| 手工 gcc | `oooo.py` 一键构建/测试/基准 |
| 没有检查 | 静态检查虚表槽位、前缀、接口一致性 |
| 没有数据 | 真基准，真数字 |

破坏性变更（必须知道）：

1. 每个类都必须有 `OOOOC_DEFINE_META(Class, DTOR_OR_NO_DTOR, ...)`，否则链接失败。析构函数签名固定为 `void Class__destroy(void *self)`。
2. 有接口的类必须先定义 `OOOOC_DEFINE_IVTABLE`，再定义 `OOOOC_DEFINE_META`，因为 META 的接口表要拿虚表地址。
3. `OOOOC_SUPER(obj)` 现在返回父类视图指针（v3 返回父类子对象的值）。本项目的 demo 没有依赖旧语义。
4. 向量宏统一接收“指向向量结构体的指针”。
5. 接口成员名要显式写在 `OOOOC_IMPL(ISwimmer, swimmer)` 里，META 的 `OOOOC_IFACE(Duck, ISwimmer, swimmer)` 用同一个名字。

旧版 v3 文件完整保存在 [`legacy/`](legacy/)。

---

## 如何新增一个类：清单

1. `OOOOC_ROOT_CLASS(Class, fields...)` 或 `OOOOC_CLASS(Class, Parent, fields...)`；
2. `OOOOC_VTABLE(Class, slots...)`，**继承来的槽必须按父类顺序重列**；
3. 实现函数；
4. 析构函数 + `OOOOC_DEFINE_META(Class, destroy_or_no_dtor, iface_entries...)`；
5. `OOOOC_DEFINE_VTABLE(Class, impls...)`；
6. `OOOOC_DEFINE_INIT(Class, init_statements...)`；
7. 如果实现接口：`OOOOC_IMPL` 成员、`OOOOC_DEFINE_IVTABLE`（在 META 之前）、META 里的 `OOOOC_IFACE`。

> 字段声明请用分号分隔。`int a, b;` 里的逗号会被宏当成额外参数，请拆成两条声明。

跑一遍：

```powershell
python oooo.py report your_file.c
python oooo.py run your_file.c
```

`report` 会告诉你槽位数、前缀、接口三方是否对得上。

---

## 文件地图

| 文件 | 作用 |
|---|---|
| [`ooooc.h`](ooooc.h) | 核心：类、虚表、RTTI、接口、生命周期、消息发送 |
| [`ooooc_std.h`](ooooc_std.h) | 标准库头：arena、向量、字符串、异常、测试框架 |
| [`ooooc_std.c`](ooooc_std.c) | 异常运行时、测试计数器、控制台 UTF-8 |
| [`cccccc.h`](cccccc.h) | CCCCCC 中文关键字核心层 |
| [`cccccc_std.h`](cccccc_std.h) | CCCCCC 中文标准库别名 |
| [`demo_zh.c`](demo_zh.c) | 中文旗舰演示：中华动物园 |
| [`CCCCCC.md`](CCCCCC.md) | 扩展包文档、词表、诚实声明 |
| [`demo_oop.c`](demo_oop.c) | 最小可读示例：Animal / Duck |
| [`demo_zoo.c`](demo_zoo.c) | 旗舰示例：继承 + 接口 + RTTI + 异常 + arena + 排序 |
| [`tests.c`](tests.c) | 14 组测试：从命名空间到嵌套异常 |
| [`bench.c`](bench.c) | 派发、RTTI、arena、malloc 基准 |
| [`oooo.py`](oooo.py) | 构建、测试、基准、静态分析 |
| [`ooooc_reg.py`](ooooc_reg.py) | 符号长度注册表生成器（也可单独使用） |
| [`m.c`](m.c) | 命名空间/符号编码的最小示例，保留 v3 风格 |
| [`legacy/`](legacy/) | v3 原版文件归档 |

---

## 诚实声明（完整版）

这些限制不是“以后会修”，是当前设计的真实边界：

1. **只有单继承。** 多重继承用接口。`extends(...)` 是空的，它只负责安慰旧代码。
2. **没有模板。** `OOOOC_VEC(T)` 是宏；类型检查发生在宏展开之后，报错信息可能很长。
3. **RTTI 按名字比较。** 类名和接口名在整个程序里必须唯一。两个不同命名空间里的同名类会让 `OOOOC_AS` 认错亲。
4. **虚表前缀必须手工重列。** 漏一个槽，`report` 会警告，但编译器不会拦你。
5. **异常不展开栈。** longjmp 不认识 C 的作用域；栈上对象的析构函数不会被调用。
6. **`setjmp` 局部变量规则。** TRY 外声明、TRY 内修改、longjmp 后读取的变量请加 `volatile`。
7. **没有 ABI 稳定。** 加一个虚方法，所有子类的虚表都要重编。
8. **不支持 MSVC。** GCC / Clang；Clang 的 LTO 需要 `lld`。
9. **不是线程安全的。** RTTI 只读所以没问题；arena、字符串构造器、全局测试计数器都不保证并发安全。异常帧是 `__thread` 的。
10. **性能数字只对这台机器负责。** 你的 CPU、编译器和后台 Chrome 都会改变结果。
11. **`OOOOC_NEW` 不零初始化。** 它用 `malloc`；构造函数里没赋值的字段就是未定义值。
12. **中文标识符是编译器扩展。** GCC/Clang 支持 UTF-8 标识符；预处理指令仍是英文，宏层汉化改不了 `#include`。
13. **这不是生产级框架。** 它是一份认真做过的玩具，一份可以读、可以改、可以拿去面试时吓人的 C 代码。

---

## FAQ

**Q：这和 C++ 有什么区别？**
A：C++ 的编译器知道自己在干什么；O语言不知道，但它很有礼貌地把不知道的部分写进了诚实声明。

**Q：为什么叫“超智慧编程”？**
A：因为名字越长，代码越智慧。这是本项目唯一不需要证明的定理。

**Q：虚函数真的不比直接调用慢多少吗？**
A：本机约 0.1–0.5 ns/op。你可以继续害怕，但数据说不用那么怕。

**Q：为什么接口名和类名必须唯一？**
A：RTTI 靠名字认亲。没有 `strcmp` 的 RTTI 需要更复杂的元数据；我们选择了简单和可读。

**Q：为什么必须写 `OOOOC_DEFINE_META`？**
A：因为生命不该被宏悄悄结束。析构、RTTI、接口表都要它。

**Q：为什么不用 C++？**
A：用了 C++ 就没有这个 README 了。

**Q：能用于生产吗？**
A：如果你问出这个问题，说明你还有理智。本项目的主要产出是快乐、几个可复用的宏，以及一份能跑的基准。

**Q：`extends(...)` 到底有什么用？**
A：它是一段历史。删掉它，旧代码会觉得自己被冒犯。

---

## 最后

O语言不试图证明宏能战胜语言。

它只想证明：**在 C 的边界之内，仍然可以认真地做一点好玩的东西，并且把它做对。**

超智慧编程，真实不虚。
