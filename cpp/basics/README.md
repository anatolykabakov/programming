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

---

# Интервью — формат, прогресс, критерии

Раньше жило отдельным файлом `programming/INTERVIEW_PLAN.md`, сведено сюда, чтобы
не расходилось с планом изучения выше.

**Материалы для последних дней перед интервью** — восемь тем с теорией, числами
и заданиями на вечер: [interview_prep/](interview_prep/README.md)

## Формат сессии (75 минут)

| Время | Что |
|-------|-----|
| 5 мин | Блиц: 3–4 коротких вопроса по прошлой сессии — проверка, что пробел закрыт |
| 30 мин | Теоретический блок: вопросы плюс ревью кода (своего или подсунутого) |
| 30 мин | Live coding: одна задача целиком или две простых |
| 10 мин | Разбор, запись пробелов в журнал |

Ответ уровня Middle+ — это не определение из книги, а число или механизм: сколько
аллокаций, какая гарантия, что увидит санитайзер. Спорные утверждения проверяются
компилятором и ASan/TSan в сессии, а не обсуждаются на словах.

## Правила live coding

Порядок жёсткий, отклонение считается ошибкой:

1. Уточнить условие: размер входа, диапазон значений, дубликаты, пустой вход,
   можно ли портить вход.
2. Проговорить 2–3 примера вслух, включая крайние.
3. **Назвать сложность по времени и памяти до написания кода.**
4. Писать компилируемый C++, не псевдокод, без автодополнения IDE.
5. Прогнать написанное на примере вручную.
6. Самому назвать edge cases. Подсказка от интервьюера — минус.

## Пройденные блоки

| Блок | Тема | Результат | Что провалилось |
|------|------|-----------|-----------------|
| 1 | Value semantics, move, NRVO | частично | Не считает аллокации; не знал про переиспользование `capacity` в copy-assign (1000 `new` против 0); границу SSO назвал диапазоном |
| 2 | Smart pointers | частично | Копирование одного `shared_ptr` из двух потоков счёл гонкой; свои реализации: copy-assign не освобождал старое, self-check вместо release, move-ctor не компилировался |
| 3 | Data race vs race condition | закрыт | Определение race condition совпадало с определением data race; `atomic` считался достаточным |
| 4 | condition_variable, async | частично | Инвертирована полярность предиката; семантика `wait` наоборот; потерянное уведомление не назвал |
| 5 | Thread pool | частично | Про thread starvation deadlock и обход move-only `packaged_task` не знал |
| 6 | Exception safety | частично | Гарантии сформулированы неполно; оператору поставлена basic вместо «никаких гарантий» (ASan: double-free) |
| 7 | STL и инвалидация | частично | `push_back` без реаллокации инвалидирует `end()`; при реаллокации умирают и ссылки; рехеш `unordered_map` пропущен; про `h1 ^ h2` и выбор `map`/`unordered_map` ответа не было |

Артефакт в этом каталоге: [race_condition.cpp](race_condition.cpp) — data race
против race condition, шесть демонстраций, TSan находит ровно один дефект из шести.

## Оставшиеся блоки

### Блок 8 — Шаблоны и статический полиморфизм

Материал в [../questions](../questions): `sfinae.cpp`, `crtp.cpp`, `pimpl.cpp`,
`variadic_templates.cpp`.

- SFINAE: что это, зачем, чем заменяется в C++17 (`if constexpr`) и C++20 (концепты).
- CRTP: как работает, чем платит, когда предпочтителен `virtual`.
- PIMPL: что скрывает, что происходит с ABI, почему деструктор обязан быть в .cpp.
- Variadic templates, fold expressions, perfect forwarding и зачем именно `std::forward`.
- Ловушка: шаблонный конструктор перехватывает копирование — как ловится и как лечится.
- `virtual` против `std::variant` против CRTP — обосновать выбор точки расширения.

### Блок 9 — Memory model (граница senior)

- happens-before, synchronizes-with — определения.
- `memory_order_acquire/release/relaxed`: почему в счётчике ссылок инкремент
  `relaxed`, а декремент `acq_rel`.
- Почему `mutex` часто быстрее наивного lock-free.
- False sharing, длина кэш-линии, `alignas(64)`.
- ABA problem — обзорно.
- Практика: прогнать свой код под TSan и объяснить каждое предупреждение.

### Блок 10 — Дизайн подсистемы (senior)

- Thread-safe logger: сколько потоков, что с бэкпрешером, что при переполнении очереди.
- Message bus между N узлами: latency, глубина очереди, backpressure.
- Error handling в embedded: исключения против `expected`/`Status`.
- Code review на доске: найти UAF, data race, утечку при исключении, slicing.

## Критерии оценки

Ответ считается уровнем Middle+, если:

- сложность названа **до** кода и названа верно;
- аллокации посчитаны числом, а не «примерно»;
- различает data race и race condition, знает, что `atomic` убирает только первое;
- может сказать, какую гарантию исключений даёт его код и почему;
- код собирается с первого раза с `-Wall -Wextra`;
- edge cases названы самостоятельно;
- на «почему так, а не иначе» отвечает trade-off, а не «так принято».

Красные флаги:

- уход от вопроса вместо «не знаю»;
- «завернул в `atomic`/`mutex`, значит потокобезопасно»;
- «проверил `this == &other`» вместо освобождения старого ресурса;
- `main`, проверяющий только работающий путь;
- нет раннего выхода, лишний проход, приём по значению вместо `const&`.

## График сессий

| Сессия | Теория | Алгоритмы |
|--------|--------|-----------|
| 1 | Блок 7: STL и инвалидация | 4.1 Tier 0 — сделано |
| 2 | Блок 8: SFINAE, `if constexpr` | 4.2 Tier 0–1 |
| 3 | Блок 8: CRTP, PIMPL | 4.3 Tier 0–1 |
| 4 | Блок 8: variadic, forwarding | 4.4 Tier 0–1 |
| 5 | Повтор блоков 1–2 | 4.5 Tier 0–1 |
| 6 | Повтор блоков 3–6 | 4.6 Tier 0–1 |
| 7 | Блок 9: memory model, TSan | 4.7 Tier 0–1 |
| 8 | Блок 9: false sharing, atomics | 4.8 и 4.9 |
| 9 | Блок 10: дизайн подсистемы | 4.10 и 4.11 |
| 10 | Полный прогон: 45 мин теории и 2 задачи Tier 1 без подсказок | смешанные |

## Журнал пробелов

| Дата | Тема | Пробел | Закрыто |
|------|------|--------|---------|
| 2026-08-23 | Value semantics | Переиспользование `capacity` в copy-assign | да |
| 2026-08-23 | `shared_ptr` | Копирование из двух потоков — не гонка | да |
| 2026-08-23 | condition_variable | Полярность предиката, семантика `wait` | да |
| 2026-08-23 | Thread pool | Starvation deadlock, move-only `packaged_task` | да |
| 2026-08-23 | Exception safety | Гарантия функции — минимум по путям | да |
| 2026-08-24 | `myunique_ptr` | `return *this` в конструкторе перемещения | да |
| 2026-08-24 | `myshared_ptr` | Нет `#include <atomic>`; `use_count()` не собирается с `atomic`; `release_()` — check-then-act, ASan double-free на 8 потоках | **нет** |
| 2026-08-24 | Инвалидация | `end()` при `push_back`; ссылки при реаллокации; рехеш `unordered_map` | да |
| 2026-08-24 | Хеши | `h1 ^ h2` симметричен и обнуляется: 128 различных хешей из 10000 против 10000 у splitmix64 | да |
| 2026-08-24 | Код 4.1 | Двойной поиск, нет раннего выхода (в 100 раз медленнее), приём по значению, сложность после кода | частично |
| 2026-08-24 | Код 4.2 (125) | Приём по значению третий раз подряд (копия 15 мкс на 200 КБ); `isalnum`/`tolower` от `char` — формально UB, нужен `unsigned char`; локаль-зависимость `tolower` | частично |
| 2026-08-25 | Код 4.2 (167) | Инвариант `l <= r` вместо `l < r`: на входе без решения возвращает один элемент дважды (`[2,2]`); `int r = size() - 1` ловится только `-Wconversion` | частично |
| 2026-08-25 | Код 4.2 (26) | Сдано чисто (8/8, совпадает с `std::unique` на 200 входах). Вернулся `-Wsign-compare`; сравнение с `nums[l-1]` вместо `nums[r-1]` требует неочевидного рассуждения | да |
| 2026-08-25 | Код 4.2 (11) | Сдано (9/9, сверка с перебором на 500 входах без расхождений). `l <= r` здесь безвреден в отличие от 167; обоснование жадности («двигаем меньшую стенку») вслух не дано | да |
| 2026-08-25 | Код 4.2 (15) | Опечатка `j = i + i` вместо `i + 1` давала ложные тройки (111 расхождений из 400). Исправлено: 0 расхождений из 2000, 14 ms на n=3000. Остаётся `size() - 2` при size 0 или 1 → SEGV | да |
| 2026-08-25 | Код 4.3 (121, 643) | Оба сданы, 0 расхождений с перебором. Замечания: накопление в `double` точно только благодаря ограничениям; `-Wsign-compare` четвёртый и пятый раз | да |
| 2026-08-25 | Код 4.3 (3) | Сдано, доказано, что необновление maxLength в ветке else безопасно. Но в 25 раз медленнее: 92% времени на `unordered_set` вместо массива `bool[128]` | да |
| 2026-08-27 | Код 4.6 (206, 141) | Оба сданы чисто: 12/12 тестов, ASan и UBSan чисты, ноль варнингов. Первые задачи в серии без замечаний по стилю | да |
| 2026-08-27 | Код 4.4 (20) | Сдано с третьей попытки. Итерация 1: `push` вне условий — закрывающие попадали в стек, ломалась вложенность. Итерация 2: `return valid` отдавал внешнюю переменную, затенённую внутренней (`-Wshadow` показывает прямо). Итерация 3: чисто, 0 расхождений на 200000 строк | да |

---

# Задачи LeetCode по паттернам

Норматив: **Tier 0** — 10–15 минут на задачу без запинки, включая тесты.
**Tier 1** — 20–25 минут, типичная задача интервью Middle+. **Tier 2** — запас.

Структуры и алгоритмы разобраны в [../../dsa/cpp](../../dsa/cpp): бинарный поиск,
скользящее окно двух видов, хеш-таблица, куча, BFS/DFS, backtracking, trie, DP 1d.
Решения — в [../leetcode](../leetcode).

## 4.1 Хеш-таблицы и счётчики

| № | Название | Tier | Статус |
|---|---|---|---|
| 1 | Two Sum | 0 | сдано |
| 217 | Contains Duplicate | 0 | сдано |
| 242 | Valid Anagram | 0 | сдано |
| 49 | Group Anagrams | 1 | |
| 347 | Top K Frequent Elements | 1 | |
| 560 | Subarray Sum Equals K | 1 | |
| 128 | Longest Consecutive Sequence | 2 | |

Ловушки: `operator[]` вставляет элемент при промахе — в проверке наличия нужен
`find`/`count`/`contains`; `reserve` при известном N; для ключа `pair<int,int>` нет
`std::hash` — нужен свой, и комбинировать надо перемешиванием битов, а не xor;
при рехеше итераторы инвалидируются, а ссылки и указатели нет.

## 4.2 Два указателя

| № | Название | Tier | Статус |
|---|---|---|---|
| 125 | Valid Palindrome | 0 | сдано |
| 167 | Two Sum II — Input Array Is Sorted | 0 | сдано |
| 26 | Remove Duplicates from Sorted Array | 0 | сдано |
| 15 | 3Sum | 1 | сдано (после правки `j = i + 1`) |
| 11 | Container With Most Water | 1 | сдано |
| 42 | Trapping Rain Water | 2 | |

Ловушки: idiom erase-remove; `std::unique` работает только на отсортированном;
сортировка превращает O(n) в O(n log n) — сказать это до кода; пропуск дубликатов
в 3Sum. Не путать 125 (строка) с 9 (число, уже решено в `2_palindrome.cpp`).

## 4.3 Скользящее окно

| № | Название | Tier | Статус |
|---|---|---|---|
| 121 | Best Time to Buy and Sell Stock | 0 | сдано |
| 643 | Maximum Average Subarray I | 0 | сдано |
| 3 | Longest Substring Without Repeating Characters | 1 | сдано |
| 424 | Longest Repeating Character Replacement | 1 | |
| 76 | Minimum Window Substring | 2 | |
| 239 | Sliding Window Maximum | 2 | |

Каркас брать из
[../../dsa/cpp/sliding_window_variable_size.cpp](../../dsa/cpp/sliding_window_variable_size.cpp).
Окно фиксированного и переменного размера — разные каркасы, не путать.

## 4.4 Стек и монотонный стек

| № | Название | Tier | Статус |
|---|---|---|---|
| 20 | Valid Parentheses | 0 | сдано |
| 155 | Min Stack | 1 | |
| 150 | Evaluate Reverse Polish Notation | 1 | |
| 739 | Daily Temperatures | 1 | |
| 84 | Largest Rectangle in Histogram | 2 | |
| 853 | Car Fleet | 2 | |

Ловушки: `top()`/`pop()` на пустом `std::stack` — UB, не исключение; `std::stack`
по умолчанию поверх `deque`; в монотонном стеке хранить индексы, а не значения.

## 4.5 Бинарный поиск

| № | Название | Tier |
|---|---|---|
| 704 | Binary Search | 0 |
| 35 | Search Insert Position | 0 |
| 34 | Find First and Last Position of Element in Sorted Array | 1 |
| 33 | Search in Rotated Sorted Array | 1 |
| 153 | Find Minimum in Rotated Sorted Array | 1 |
| 875 | Koko Eating Bananas | 1 |
| 4 | Median of Two Sorted Arrays | 2 |

Ловушки: `mid = l + (r - l) / 2` против переполнения; выбрать инвариант `[l, r]`
или `[l, r)` и не менять его посреди задачи; `lower_bound` против `upper_bound` —
что возвращают при промахе; бинарный поиск по ответу, а не по массиву (875).

## 4.6 Связные списки

| № | Название | Tier | Статус |
|---|---|---|---|
| 206 | Reverse Linked List | 0 | сдано |
| 21 | Merge Two Sorted Lists | 0 | |
| 141 | Linked List Cycle | 1 | сдано |
| 142 | Linked List Cycle II | 1 | |
| 19 | Remove Nth Node From End of List | 1 | решено |
| 143 | Reorder List | 1 | решено |
| 25 | Reverse Nodes in k-Group | 2 | |
| 23 | Merge k Sorted Lists | 2 | |

Ловушки: dummy head убирает половину ветвлений; `unique_ptr` в узлах даёт
рекурсивное разрушение и переполнение стека на длинном списке; Флойд против
`unordered_set` — сравнить по памяти. Задачи 19 и 143 перерешать без подглядывания.

## 4.7 Деревья

| № | Название | Tier |
|---|---|---|
| 104 | Maximum Depth of Binary Tree | 0 |
| 226 | Invert Binary Tree | 0 |
| 100 | Same Tree | 0 |
| 98 | Validate Binary Search Tree | 1 |
| 102 | Binary Tree Level Order Traversal | 1 |
| 235 | Lowest Common Ancestor of a Binary Search Tree | 1 |
| 230 | Kth Smallest Element in a BST | 1 |
| 105 | Construct Binary Tree from Preorder and Inorder Traversal | 2 |
| 297 | Serialize and Deserialize Binary Tree | 2 |
| 124 | Binary Tree Maximum Path Sum | 2 |

Ловушки: в 98 границы не влезают в `int` — нужен `long long` или `optional`;
рекурсия против явного стека и когда рекурсия сломает стек; в обходе по уровням
размер очереди фиксируется до цикла.

## 4.8 Куча и top-K

| № | Название | Tier |
|---|---|---|
| 215 | Kth Largest Element in an Array | 1 |
| 621 | Task Scheduler | 1 |
| 295 | Find Median from Data Stream | 2 |
| 23 | Merge k Sorted Lists | 2 |

Ловушки: `priority_queue` — max-heap по умолчанию, min-heap через
`priority_queue<T, vector<T>, greater<T>>`; для одного k-го элемента `nth_element`
за O(n) вместо сортировки; куча размера k против сортировки всего массива.

## 4.9 Графы

| № | Название | Tier |
|---|---|---|
| 200 | Number of Islands | 1 |
| 994 | Rotting Oranges | 1 |
| 133 | Clone Graph | 1 |
| 207 | Course Schedule | 2 |
| 417 | Pacific Atlantic Water Flow | 2 |

Ловушки: BFS даёт кратчайший путь в невзвешенном графе, DFS нет; `visited` как
`vector<bool>` против `unordered_set`; рекурсивный DFS на сетке 1000×1000
переполнит стек.

## 4.10 Префиксные суммы и интервалы

| № | Название | Tier |
|---|---|---|
| 238 | Product of Array Except Self | 1 |
| 56 | Merge Intervals | 1 |
| 560 | Subarray Sum Equals K | 1 |
| 57 | Insert Interval | 2 |

## 4.11 Спрашивают именно у C++-разработчиков

| № | Название | Почему спрашивают | Статус |
|---|---|---|---|
| 146 | LRU Cache | хеш-таблица плюс двусвязный список, это же и системный вопрос | |
| 380 | Insert Delete GetRandom O(1) | выбор структуры под три требования сразу | |
| 622 | Design Circular Queue | кольцевой буфер, дальше — потокобезопасный вариант | |
| 8 | String to Integer (atoi) | переполнение, знаки, мусор во входе | |
| 394 | Decode String | стек | решено в `../leetcode/1_two_sum.cpp` |
| 9 | Palindrome Number | не путать с 125 | решено в `../leetcode/2_palindrome.cpp` |

Без номера, но спрашивают часто: своя хеш-таблица (цепочки против открытой
адресации, рехеш, load factor); свой `unique_ptr`/`shared_ptr`; `split` строки
по разделителю; дедупликация вектора без лишних аллокаций.

## 4.12 Backtracking

| № | Название | Tier |
|---|---|---|
| 78 | Subsets | 1 |
| 46 | Permutations | 1 |
| 39 | Combination Sum | 1 |
| 51 | N-Queens | 2 |
| 37 | Sudoku Solver | 2 |

Каркас в [../../dsa/cpp/backtracking.cpp](../../dsa/cpp/backtracking.cpp).

Ловушки: состояние вести **одним** вектором с `push_back`/`pop_back` вокруг рекурсивного
вызова, а не копировать вектор на каждый уровень — иначе к факториальной сложности
добавляется копирование; результат и состояние передавать по ссылке, а не возвращать
по значению; дубликаты в 39 и 40 отсекаются сортировкой плюс пропуском равных элементов
**на одном уровне** рекурсии; следить за глубиной — на больших входах рекурсия кладёт стек.

## 4.13 Dynamic Programming

| № | Название | Tier |
|---|---|---|
| 70 | Climbing Stairs | 0 |
| 322 | Coin Change | 1 |
| 1143 | Longest Common Subsequence | 1 |
| 300 | Longest Increasing Subsequence | 1 |
| 72 | Edit Distance | 2 |

Каркасы в [../../dsa/cpp](../../dsa/cpp): `dynamic_programming_1d.cpp`, `memoization.cpp`.

Ловушки: на интервью проговаривать переход «рекурсия с мемоизацией → таблица», а не
писать таблицу сразу; значение «недостижимо» нельзя брать `INT_MAX` — `INT_MAX + 1`
переполнится, берут `n + 1` или `INT_MAX / 2`; таблицу `n × m` почти всегда сворачивают
до одной строки (память O(min(n, m)) вместо O(n·m)); в 300 есть решение за O(n log n)
через `lower_bound` — про него спрашивают после наивного O(n²); при свёртке до одной
строки следить за направлением обхода, иначе элемент используется дважды.

## 4.14 Bit Manipulation

| № | Название | Tier |
|---|---|---|
| 191 | Number of 1 Bits | 0 |
| 190 | Reverse Bits | 0 |
| 268 | Missing Number | 0 |
| 338 | Counting Bits | 1 |
| 371 | Sum of Two Integers | 1 |

Для embedded это не экзотика, а рабочий инструмент: упаковка флагов, CAN-фреймы,
регистры — то же, что в `adcu_soc_sw` и `BLIS`.

Ловушки C++, которые здесь и проверяют: сдвиг влево отрицательного числа — UB до C++20,
поэтому работать с `unsigned` / `uint32_t`; `1 << 31` на `int` — UB, нужно `1u << 31`;
сдвиг на количество бит, большее или равное ширине типа, — тоже UB; `x & (x - 1)`
снимает младший установленный бит (основа быстрого подсчёта); есть `__builtin_popcount`,
а в C++20 — `std::popcount`, и на интервью полезно знать оба; в 371 сложение без `+`
делается через xor и carry, а знаковое переполнение обходится через `unsigned`.

**Итого 82 уникальных задач, закрыто 18.** Разделы 4.12–4.14 и задачи 34, 105, 142, 25 добавлены 2026-08-27 после сверки со
списком паттернов blog.codeinmotion.io — там эти три паттерна есть, а у нас их не было.
Обратное тоже верно: у них нет паттерна хеш-таблиц и счётчиков (раздел 4.1) и задач
на проектирование (раздел 4.11), поэтому списки дополняют друг друга, а не заменяют.
