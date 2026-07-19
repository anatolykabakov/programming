# C++ — план отработки (Middle+ / Senior)

Короткие `.cpp`-скетчи для повторения синтаксиса и подготовки к собесу — не копии Python-примеров, а свой набор кейсов «руками».
Цель: синтаксис и STL → **Middle+** (уверенный modern C++, concurrency, design) → **Senior** (memory model, архитектура, trade-offs, production).

**Стандарт:** C++17 (база), C++20 — отдельные темы для Senior
**Компиляция одного файла:**
```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic <file>.cpp -o <file> && ./<file>
```

---

## Карта уровней

| Уровень | Кого закрывает | Где в репо | Критерий «готов» |
|---------|----------------|------------|------------------|
| **Foundation** | Junior → Middle | `basics/` | пишешь vector/map/class без подглядывания |
| **Middle+** | Middle → Middle+ | `basics/` + `multithreading/` + `exception_safety/` + `questions/` | объясняешь move, UB, deadlock, exception safety |
| **Senior** | Middle+ → Senior | `questions/` + `software_design/` + `profiling/` + production code | проектируешь API, выбираешь trade-offs, debugging под нагрузкой |

> **Foundation — prerequisite.** Без него Middle+ не держится. Ниже — полный трек с акцентом на Middle+ / Senior.

---

## Что ждут на Middle+

Уверенно и **с примерами из кода**, не только теория:

| Тема | Вопросы на собесе | Где отработать |
|------|-------------------|----------------|
| **Память** | stack/heap, RAII, `unique_ptr` vs `shared_ptr`, dangling ref, alignment | `smart_pointers.cpp`, `questions/myunique_ptr.cpp`, `questions/circle_ref_shared_ptr.cpp` |
| **Move / forwarding** | lvalue/rvalue, NRVO, когда copy elision, `std::forward` | `move_semantics.cpp`, `questions/move_forward.cpp`, `questions/perfect_forward.cpp` |
| **OOP** | virtual dtor, vtable (как устроен), `override`, slicing | `inheritance.cpp`, `questions/vtable.cpp`, `questions/virtual_dctor.cpp` |
| **Шаблоны** | SFINAE, variadic, CRTP, `enable_if` / concepts | `functions.cpp`, `questions/sfinae.cpp`, `questions/crtp.cpp`, `questions/variadic_templates.cpp` |
| **STL глубже** | iterator invalidation, complexity, custom comparator, allocator (обзор) | `iterators.cpp`, `questions/unordered_set_requirements.cpp` |
| **Concurrency** | mutex, deadlock, condition_variable, future/promise, data race | `multithreading/` (весь каталог) |
| **Exception safety** | guarantee: basic / strong / noexcept | `exception_safety/`, `questions/ctor_exception.cpp`, `questions/exceptions.cpp` |
| **Const correctness** | `const` метод, `mutable`, `const` и multithreading | `questions/const.cpp` |
| **Паттерны** | PIMPL, Factory, Observer, RAII-обёртки | `questions/pimpl.cpp`, `software_design/solid/`, `software_design/ood/` |
| **Build / tooling** | CMake targets, `-Wall`, sanitizers (ASan/TSan), Conan basics | `software_design/clean_arch/`, `static_analysis/` |

**Typical live-coding Middle+:** реализовать `unique_ptr`, thread-safe queue, LRU cache, producer-consumer, parse + aggregate на `unordered_map`.

---

## Что ждут на Senior

Не «знаю синтаксис», а **решения и последствия**:

| Тема | Вопросы на собесе | Где отработать |
|------|-------------------|----------------|
| **Memory model** | happens-before, acquire/release, seq_cst vs relaxed, false sharing | `multithreading/race_condition.cpp`, `concurency_in_action/` |
| **Atomics** | lock-free (обзор), `atomic<T>`, CAS, ABA problem | `multithreading/custom_mutex.cpp` + литература |
| **API design** | value vs observer, `string_view` lifetime, noexcept contract | `strings.cpp`, Core Guidelines (`cppcoreguidelines/`) |
| **Architecture** | SOLID на реальном коде, dependency direction, Clean/Hexagonal | `software_design/clean_arch/`, `software_design/solid/` |
| **Performance** | cache locality, SOA vs AOS, branch prediction, аллокации | `profiling/`, `gbenchmark/`, `bloaty/` |
| **Compile time** | template instantiation cost, explicit instantiation, PIMPL ради compile time | `questions/pimpl.cpp`, `questions/templates.cpp` |
| **Debugging prod** | core dump, backtrace, symbolize, TSan/ASan | `exception_handler/` |
| **Embedded / realtime** | no heap, deterministic latency, `constexpr`, MISRA-мышление | ваш `adcu_soc_sw/`, `BLIS/` |
| **System design** | pub/sub node, backpressure, fault isolation, versioning API | `multithreading/pubsub.cpp`, `software_design/pubsub.cpp`, nodes в prod |
| **Ownership at scale** | when `shared_ptr` is wrong, intrusive ptr, object pools | `questions/intrusive_container.cpp`, `questions/myshared_ptr.cpp` |

**Typical Senior interview:** code review чужого класса, спроектировать subsystem (logging, scheduler, message bus), разобрать data race / use-after-move / exception leak.

---

## Как работать

1. Открыть файл, прочитать комментарии с ожидаемым выводом.
2. Скомпилировать и запустить.
3. Изменить пример: добавить свой кейс, сломать и починить.
4. **Middle+:** объяснить вслух «почему так», найти UB.
5. **Senior:** записать trade-offs (память vs скорость, простота vs гибкость).
6. Без IDE: написать фрагмент «на доске».
7. Отметить `[x]` в чеклисте.

---

# Level 1 — Foundation (`basics/`)

> Пройти первым. ~2 недели. Без этого Middle+ не начинать.

**Прогресс:** `vars` ✅ (~) · `loops` ✅ · `if_statements` ✅ (~) · `functions` ✅ (~) · `pointers` ✅ · `arrays` ✅ · `strings` ✅ · `classes` ✅ · `inheritance` ✅ · **`move_semantics` / `smart_pointers` ⬜ следующий** (9/19 foundation)

## Структура файлов

| Файл | Статус | Что отработать |
|------|--------|----------------|
| `vars.cpp` | ✅ **пройден** (~) | типы, `const`, `auto`, `constexpr`, `static`, linkage |
| `loops.cpp` | ✅ **пройден** | range-for, `while`, `vector`, `const` |
| `functions.cpp` | ✅ **пройден** (~) | перегрузка, templates, lambda, `std::function` |
| `if_statements.cpp` | ✅ **пройден** (~) | `if/else`, `&& \|\| !`, ternary, `switch` |
| `pointers.cpp` | ✅ **пройден** | указатели, ссылки, `nullptr`, const-проекции |
| `arrays.cpp` | ✅ **пройден** | `vector`, `array`, sort, 2D |
| `strings.cpp` | ✅ **пройден** | `string`, `string_view`, parse/format |
| `classes.cpp` | ✅ **пройден** | class/struct, ctor/dtor, const methods |
| `inheritance.cpp` | ✅ **пройден** | virtual, override, Rule of 3/5/0 |
| `move_semantics.cpp` | ⬜ **следующий** (файл пустой) | move, forward, value categories |
| `smart_pointers.cpp` | ⬜ создать | unique/shared/weak, RAII |
| `hashmap.cpp` | ⬜ создать | `unordered_map`, `map` |
| `hashset.cpp` | ⬜ создать | `unordered_set`, dedup |
| `tuples.cpp` | ⬜ создать | `pair`, `tuple`, structured bindings |
| `queues.cpp` | ⬜ создать | queue, deque, stack |
| `heaps.cpp` | ⬜ создать | `priority_queue`, min/max heap |
| `iterators.cpp` | ⬜ создать | begin/end, invalidation |
| `stl_algorithms.cpp` | ⬜ создать | find, sort, lower_bound, transform |
| `math.cpp` | ⬜ создать | деление, `%`, `<cmath>`, overflow |

## Foundation — чеклисты по файлам

### `loops.cpp` ✅ пройден
- [x] range-for `const auto&`
- [x] `while` + индекс, классический `for`, обратный проход
- [x] `break`, `continue`, `do-while`
- [x] `const vector` vs копия, `const vector&` в функции
- [x] `auto&` мутация элементов

### `if_statements.cpp` ✅ пройден (~90%)
- [x] `if / else if / else`
- [x] `&&`, `||`, скобки при смешении
- [ ] `!` — добавить пример
- [x] тернарный `cond ? a : b`
- [x] `switch`, `break`, `default`, fallthrough (`case 1: case 2:`)
- [x] `enum class` + `switch`
- [ ] `=` vs `==` — комментарий/ловушка

### `pointers.cpp` ✅ пройден
- [x] `int x`, `&x`, `int* p`, `*p`, `nullptr` (check в `passByPointer` / `swapByPointer`)
- [x] ссылка vs указатель — `ref`, `p`, `swapByRef` / `swapByPointer`
- [x] `const int*`, `int* const`, `const int* const`
- [x] передача в функцию: value, `T*`, `T&`, `const T&`
- [x] массив + указатель, swap ref/pointer

### `arrays.cpp` ✅ пройден
- [x] `vector`: `[]` vs `at`, `front`/`back`, insert/erase, push/pop
- [x] `reserve` vs `resize` vs `capacity`
- [x] `std::sort` + lambda comparator
- [x] iterator invalidation (комментарий)
- [x] `std::array`, C-массив, `vector<vector<int>>` 2D

### `strings.cpp` ✅ пройден
- [x] `std::string`: `+`, `+=`, `size`, `empty`, `front`/`back`
- [x] `substr`, `find`, `compare`, `npos` (в `split`)
- [x] `std::stoi`, `std::stod`, `std::to_string`
- [x] `split()` по разделителю
- [x] `std::string_view` (комментарий lifetime)
- [x] символ `'a'` vs строка `"a"`

### `classes.cpp` ✅ пройден
- [x] class, public/private, ctor/dtor
- [x] `const` метод (`size`)
- [x] `const Foo&` в функции (`print`)
- [x] `static` поле + определение вне класса
- [x] `static` метод (`Foo::bar`)
- [x] `this` / chaining (`Builder`)

### `inheritance.cpp` ✅ пройден
- [x] public inheritance, `virtual`, `override`, `final`
- [x] virtual dtor + `unique_ptr<Person>`
- [x] slicing (комментарий + demo)
- [x] Rule of Five (`RuleOfFive` — все 5)
- [x] Rule of Zero (`RuleOfFiveProd` + `vector`)

### `vars.cpp` ✅ пройден (~85%)
- [x] `const` vs mutable, `auto`-ловушки (`"..."` → `const char*`)
- [x] `constexpr` vs `const`, `static`, linkage, Meyers singleton (`MayersSingleton` — не вызван в `main`)
- [x] `{}` init vs UB (`braced_init{0}` vs `uninit`)
- [ ] ссылки — добавить в `vars` или считать закрытым через `pointers.cpp`
- [ ] `enum class` — добавить в `vars` или считать закрытым через `if_statements.cpp`

### `functions.cpp` ✅ пройден (~85%)
- [x] overload, default args, `constexpr`, `initializer_list`
- [x] templates, variadic, fold, lambda captures (`FuncContainer` + `std::function`)
- [ ] pass by value / `const&` / pointer — добавить явные примеры (есть в `pointers.cpp`)
- [ ] `inline`, ODR — только declare/define `bar`, без объяснения ODR

### `move_semantics.cpp` + `smart_pointers.cpp` (после Foundation)
- [ ] lvalue/rvalue, `std::move`, `std::forward`
- [ ] `unique_ptr` vs `shared_ptr`, cycles с `weak_ptr`

### STL (`hashmap` … `stl_algorithms`)
- [x] vector — API + O-notation (`arrays.cpp`)
- [x] string — API (`strings.cpp`)
- [ ] iterator invalidation, remove-erase idiom
- [ ] `priority_queue` = max-heap по умолчанию

---

# Level 2 — Middle+ (`basics/` + смежные каталоги)

> ~3–4 недели после Foundation. Фокус: **написать и защитить решение**.

## 2.1 Concurrency — `multithreading/`

| Файл | Тема | Middle+ must |
|------|------|--------------|
| `thread.cpp` | `std::thread`, join/detach | [ ] |
| `mutex.cpp` | `lock_guard`, scope lock | [ ] |
| `unique_lock.cpp` | defer, try, adopt | [ ] |
| `condition_variable.cpp` | wait/notify, spurious wakeup | [ ] |
| `deadlock.cpp` | порядок захвата, `std::lock` | [ ] |
| `race_condition.cpp` | data race, UB | [ ] |
| `future.cpp` / `promise.cpp` | async tasks | [ ] |
| `async.cpp` | `std::async` policies | [ ] |
| `threadpool.cpp` | pool + queue | [ ] |
| `pubsub.cpp` | pub/sub между потоками | [ ] |
| `exception.cpp` | exception через thread boundary | [ ] |

**Задания руками (без готового кода):**
- [ ] Producer-consumer на `condition_variable`
- [ ] Thread-safe LRU / cache с лимитом
- [ ] Reader-writer lock (или обосновать почему не нужен)

## 2.2 Exception safety — `exception_safety/`

| Файл | Guarantee | Middle+ must |
|------|-----------|--------------|
| `simple_vector_v1.cpp` … `v3.cpp` | эволия safe vector | [ ] |
| `exception_safe_vector.cpp` | strong guarantee | [ ] |

- [ ] basic / strong / noexcept — дать определение и пример нарушения
- [ ] ctor throws halfway — что с уже выделенной памятью
- [ ] `noexcept` на move — зачем для `vector::push_back`

## 2.3 Interview drills — `questions/`

Прогон **всех** файлов; для Middle+ критичны:

| Файл | Тема |
|------|------|
| `myunique_ptr.cpp` | свой smart pointer |
| `myshared_ptr.cpp` | control block, ref count |
| `circle_ref_shared_ptr.cpp` | weak_ptr |
| `move_forward.cpp`, `perfect_forward.cpp` | forwarding |
| `sfinae.cpp`, `variadic_templates.cpp` | templates |
| `crtp.cpp` | static polymorphism |
| `pimpl.cpp` | compilation firewall |
| `vtable.cpp`, `virtual_dctor.cpp` | polymorphism |
| `pure_virtual_function_call.cpp` | ctor/dtor + virtual |
| `const.cpp` | const correctness |
| `bind.cpp`, `lambdas.cpp` | callable |
| `unordered_set_requirements.cpp` | hashable key |
| `intrusive_container.cpp` | intrusive list (Senior border) |
| `sber_devices_multitreading.cpp` | combined task |

## 2.4 Design — `software_design/`

| Каталог | Тема | Middle+ must |
|---------|------|--------------|
| `solid/` | S/O/L/I/D — bad vs good | [ ] каждый принцип на примере |
| `ood/` | connect_four, blackjack, bank | [ ] моделирование домена |
| `clean_arch/` | слои, DI, Conan+CMake | [ ] собрать и объяснить границы |
| `pubsub.cpp`, `ste.h` | Observer / pub-sub | [ ] |

## 2.5 Language depth — `cpp11/` `cpp14/` `cpp17/`

- [ ] move, lambda, `auto`, smart pointers (cpp11)
- [ ] generic lambdas, `make_unique` (cpp14)
- [ ] `string_view`, `optional`, structured bindings, `if constexpr` (cpp17)

---

# Level 3 — Senior

> После Middle+. Не новые синтаксические трюки, а **инженерные решения**.

## 3.1 Memory model & lock-free (теория + код)

- [ ] happens-before, synchronizes-with
- [ ] `memory_order_acquire/release/relaxed` — когда что
- [ ] почему `mutex` часто лучше naive lock-free
- [ ] false sharing, cache line (~64 bytes)
- [ ] ABA problem (обзор)

**Практика:** перечитать `multithreading/race_condition.cpp`, прогнать с TSan:
```bash
g++ -std=c++17 -fsanitize=thread -g race_condition.cpp -o race && ./race
```

## 3.2 Performance & observability

| Каталог | Тема |
|---------|------|
| `profiling/` | perf, flamegraph mindset |
| `gbenchmark/` | microbenchmark pitfalls |
| `bloaty/` | binary size, sections |
| `exception_handler/` | backtrace, core dump, post-mortem |

Senior checklist:
- [ ] найти лишние копии через profiler, не «на глаз»
- [ ] объяснить cost `shared_ptr` vs `unique_ptr` vs raw + RAII
- [ ] SOA vs AOS для hot loop
- [ ] когда `reserve`/`shrink_to_fit`/`small vector optimization`

## 3.3 API & architecture

- [ ] спроектировать thread-safe logger / metrics sink
- [ ] versioning публичного API (additive vs breaking)
- [ ] PIMPL vs interface class — compile time vs runtime
- [ ] error handling: exceptions vs `expected`/`Status` (embedded)
- [ ] dependency injection без framework-мagic

**Референс в репо:** `software_design/clean_arch/`, prod nodes (`adcu_soc_sw/`, `BLIS/`).

## 3.4 Production & embedded (ваш контекст)

- [ ] real-time: no unbounded alloc in hot path
- [ ] Conan package graph, transitive deps
- [ ] static analysis pipeline (`static_analysis/`)
- [ ] MISRA-мышление: init before use, no implicit narrowing
- [ ] message nodes: latency, queue depth, backpressure

## 3.5 Senior «на доске»

Уметь за 20–30 мин:
- [ ] спроектировать message bus между N nodes
- [ ] code review: найти UAF, data race, exception leak, slicing
- [ ] выбрать container под access pattern (read-heavy / insert-heavy)
- [ ] обосновать `virtual` vs std::variant vs CRTP для extension point

---

## Сводный план по неделям

| Нед | Уровень | Фокус |
|-----|---------|-------|
| 1–2 | Foundation | `basics/` — vars … stl_algorithms |
| 3 | Middle+ | move, smart ptrs, inheritance + `questions/myunique_ptr`, `move_forward` |
| 4 | Middle+ | `multithreading/` целиком + producer-consumer руками |
| 5 | Middle+ | `exception_safety/` + `questions/sfinae`, `crtp`, `pimpl` |
| 6 | Middle+ | `software_design/solid/` + `clean_arch/` build |
| 7 | Senior | memory model, TSan, `race_condition`, atomics theory |
| 8 | Senior | `profiling/`, `exception_handler/`, performance trade-offs |
| 9 | Senior | system design drills + разбор prod code (`BLIS/`, nodes) |
| 10 | Review | `questions/` без IDE + 2 live-coding задачи |

---

## Must know — сжатая шпаргалка

### Middle+ (8 пунктов)
1. Pointer / ref / `const&` / value — когда что в API
2. Rule of Five, virtual dtor, slicing
3. Move + forward + copy elision
4. `unique_ptr` vs `shared_ptr` + cycles
5. Iterator invalidation, complexity STL
6. Mutex + condvar + deadlock prevention
7. Exception safety guarantees
8. SFINAE / variadic / lambda — прочитать и дописать

### Senior (+8 пунктов)
1. Memory model: acquire/release, data race definition
2. Lock-free vs mutex — trade-offs
3. Cache locality, false sharing
4. API lifetime (`string_view`, dangling)
5. PIMPL, compilation firewall, ABI
6. Profiling-driven optimization
7. Post-mortem: core, backtrace, sanitizers
8. Subsystem design: boundaries, errors, testing strategy

---

## Быстрые команды

```bash
# один файл basics
g++ -std=c++17 -Wall -Wextra -Wpedantic vars.cpp -o vars && ./vars

# все пройденные basics
for f in vars loops functions if_statements pointers arrays strings classes inheritance; do
  g++ -std=c++17 -Wall -Wextra -Wpedantic ${f}.cpp -o ${f} && ./${f}
done

# Thread Sanitizer (Middle+/Senior)
g++ -std=c++17 -g -fsanitize=thread multithreading/race_condition.cpp -o race && ./race

# Address Sanitizer
g++ -std=c++17 -g -fsanitize=address -fno-omit-frame-pointer file.cpp -o app && ./app
```

---

## Шаблон нового файла

```cpp
#include <iostream>

int main() {
    // пример 1
    std::cout << 42 << std::endl;  // 42

    // Middle+: объясни вслух — почему безопасно / где UB
    // Senior: какой trade-off у этого решения

    return 0;
}
```

---

## Связь с остальным репо

| Путь | Уровень |
|------|---------|
| `programming/cpp/basics/` | Foundation |
| `programming/cpp/multithreading/` | Middle+ |
| `programming/cpp/exception_safety/` | Middle+ |
| `programming/cpp/questions/` | Middle+ → Senior |
| `programming/cpp/software_design/` | Middle+ → Senior |
| `programming/cpp/exception_handler/` | Senior |
| `programming/cpp/profiling/`, `gbenchmark/`, `bloaty/` | Senior |
| `programming/cpp/cpp11|14|17/` | все уровни |
| `programming/cpp/leetcode/` | Foundation → Middle+ (алгоритмы) |
| `adcu_soc_sw/`, `BLIS/` | Senior (production context) |

Каждый пример — 5–20 строк, комментарий с ожидаемым выводом. На Middle+ добавляй «почему». На Senior — «trade-off».
