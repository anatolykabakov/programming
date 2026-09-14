// race_condition.cpp — два РАЗНЫХ дефекта, которые часто путают.
//
//   DATA RACE (гонка данных)
//     Два потока трогают одну переменную без синхронизации, хотя бы один пишет.
//     Это UB: стандарт не обещает вообще ничего. Ловится ThreadSanitizer'ом.
//     Лечится так: сделать доступ атомарным или закрыть мьютексом.
//
//   RACE CONDITION (состояние гонки)
//     Каждая отдельная операция корректна и синхронизирована, но РЕШЕНИЕ
//     размазано на несколько шагов, и между шагами вклинивается другой поток.
//     Это НЕ UB — программа просто считает неправильно.
//     TSan этого НЕ видит. Лечится тем, что группа шагов делается неделимой:
//     CAS-цикл или мьютекс на всё решение целиком.
//
//   Главный вывод: atomic убирает data race, но НЕ убирает race condition.
//
// Сборка и запуск:
//   g++ -std=c++17 -O2 -Wall -Wextra -pthread race_condition.cpp -o rc && ./rc
//
//   ThreadSanitizer (найдёт ровно ОДИН дефект из четырёх — data race):
//   g++ -std=c++17 -g -pthread -fsanitize=thread race_condition.cpp -o rc_tsan
//   setarch $(uname -m) -R ./rc_tsan        # setarch нужен из-за ASLR на новых ядрах

#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

namespace {

constexpr int kThreads = 8;
constexpr int kIters = 20000;

// Порог заведомо недостижим — он здесь только чтобы был шаг "проверка".
constexpr int kLimit = 1000000;

void runThreads(void (*fn)())
{
  std::vector<std::thread> ts;
  ts.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    ts.emplace_back(fn);
  }
  for (auto& t : ts) {
    t.join();
  }
}

// ---------------------------------------------------------------------------
// 1. DATA RACE — обычный int из нескольких потоков.
//
// "plainCounter = plainCounter + 1" — это ТРИ шага процессора:
// прочитать из памяти, прибавить 1, записать назад. Два потока читают
// одно и то же старое значение и записывают одно и то же новое —
// один инкремент потерян. Плюс формально это UB.
// ---------------------------------------------------------------------------
int plainCounter = 0;

void plainWorker()
{
  for (int i = 0; i < kIters; ++i) {
    if (plainCounter < kLimit) {
      plainCounter = plainCounter + 1;
    }
  }
}

// ---------------------------------------------------------------------------
// 2. RACE CONDITION без data race — atomic, но "проверил, потом сделал".
//
// Каждая из трёх операций атомарна, data race отсутствует, TSan молчит.
// Но между load() и store() значение успевает изменить другой поток,
// и его инкремент затирается. Это называют check-then-act или TOCTOU
// (time of check to time of use).
// ---------------------------------------------------------------------------
std::atomic<int> atomicCounter{0};

void atomicCheckThenActWorker()
{
  for (int i = 0; i < kIters; ++i) {
    if (atomicCounter.load() < kLimit) {              // шаг 1: проверили
      atomicCounter.store(atomicCounter.load() + 1);  // шаг 2: прочитали, шаг 3: записали
    }
  }
}

// ---------------------------------------------------------------------------
// 3. ПРАВИЛЬНО через CAS — проверка и инкремент одной неделимой транзакцией.
//
// compare_exchange_weak(cur, cur + 1) означает: "если в переменной всё ещё
// лежит cur — положи cur + 1 и верни true; иначе запиши в cur актуальное
// значение и верни false". Отсюда цикл: при неудаче условие перепроверяется
// на свежем значении.
//
// memory_order:
//   relaxed на чтении  — нужна только атомарность, ничего не публикуем;
//   acq_rel на успехе  — release, чтобы наши записи увидел следующий владелец,
//                        acquire, чтобы мы увидели записи предыдущего.
// ---------------------------------------------------------------------------
std::atomic<int> casCounter{0};

void casWorker()
{
  for (int i = 0; i < kIters; ++i) {
    int cur = casCounter.load(std::memory_order_relaxed);
    while (cur < kLimit) {
      if (casCounter.compare_exchange_weak(cur, cur + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
        break;  // успех
      }
      // провал: cur уже перезаписан актуальным значением, пробуем снова
    }
  }
}

// ---------------------------------------------------------------------------
// 4. ПРАВИЛЬНО через мьютекс — проверка и изменение внутри одной критической
// секции. Медленнее CAS, но читается тривиально и масштабируется на любую
// сложность инварианта. Для всего, что сложнее счётчика, выбирайте это.
// ---------------------------------------------------------------------------
int mutexCounter = 0;
std::mutex counterMutex;

void mutexWorker()
{
  for (int i = 0; i < kIters; ++i) {
    std::lock_guard<std::mutex> lk(counterMutex);
    if (mutexCounter < kLimit) {
      mutexCounter = mutexCounter + 1;
    }
  }
}

// ---------------------------------------------------------------------------
// 5. Почему это не абстрактная придирка: банковский счёт.
//
// balance атомарный, снятие атомарное — а счёт всё равно уходит в минус,
// потому что "проверить, что денег хватает" и "снять" — два шага.
// Пауза между проверкой и снятием просто расширяет окно, чтобы дефект был
// виден на каждом прогоне, а не раз в сто. Без неё баг не исчезает — он
// становится редким, то есть его труднее поймать тестами.
//
// В варианте с мьютексом пауза стоит ВНУТРИ критической секции. В проде так
// делать нельзя (держим лок и спим), здесь это специально: даже с широким
// окном инвариант "остаток >= 0" не нарушается.
// ---------------------------------------------------------------------------
constexpr int kStartBalance = 100;
constexpr int kWithdraw = 10;
constexpr int kAttempts = 20;
constexpr int kRounds = 200;
constexpr auto kWindow = std::chrono::microseconds(20);

std::atomic<int> atomicBalance{kStartBalance};

void atomicWithdrawWorker()
{
  for (int i = 0; i < kAttempts; ++i) {
    if (atomicBalance.load() >= kWithdraw) {  // "на счету хватает"
      std::this_thread::sleep_for(kWindow);   // окно для другого потока
      atomicBalance.fetch_sub(kWithdraw);     // снимаем
    }
  }
}

int mutexBalance = kStartBalance;
std::mutex balanceMutex;

void mutexWithdrawWorker()
{
  for (int i = 0; i < kAttempts; ++i) {
    std::lock_guard<std::mutex> lk(balanceMutex);  // проверка и снятие — одно целое
    if (mutexBalance >= kWithdraw) {
      std::this_thread::sleep_for(kWindow);
      mutexBalance -= kWithdraw;
    }
  }
}

}  // namespace

int main()
{
  const int expected = kThreads * kIters;

  printf("== счётчик: ожидаем %d ==\n", expected);

  runThreads(plainWorker);
  printf("1. int, без синхронизации   : %7d  потеряно %6d   <- DATA RACE, TSan ловит\n", plainCounter,
         expected - plainCounter);

  runThreads(atomicCheckThenActWorker);
  printf("2. atomic, check-then-act   : %7d  потеряно %6d   <- RACE CONDITION, TSan молчит\n", atomicCounter.load(),
         expected - atomicCounter.load());

  runThreads(casWorker);
  printf("3. atomic + CAS-цикл        : %7d  потеряно %6d   <- верно\n", casCounter.load(),
         expected - casCounter.load());

  runThreads(mutexWorker);
  printf("4. mutex                    : %7d  потеряно %6d   <- верно\n", mutexCounter, expected - mutexCounter);

  printf("\n== счёт: %d на счету, %d потоков по %d попыток снять %d, %d раундов ==\n", kStartBalance, kThreads,
         kAttempts, kWithdraw, kRounds);

  int atomicWorst = 0;
  int atomicNegativeRounds = 0;
  for (int r = 0; r < kRounds; ++r) {
    atomicBalance.store(kStartBalance);
    runThreads(atomicWithdrawWorker);
    const int left = atomicBalance.load();
    if (left < 0)
      ++atomicNegativeRounds;
    if (left < atomicWorst)
      atomicWorst = left;
  }
  printf("5. atomic, проверил-снял    : раундов с минусом %3d/%d, худший остаток %5d  <- RACE CONDITION\n",
         atomicNegativeRounds, kRounds, atomicWorst);

  int mutexWorst = 0;
  int mutexNegativeRounds = 0;
  for (int r = 0; r < kRounds; ++r) {
    {
      std::lock_guard<std::mutex> lk(balanceMutex);
      mutexBalance = kStartBalance;
    }
    runThreads(mutexWithdrawWorker);
    if (mutexBalance < 0)
      ++mutexNegativeRounds;
    if (mutexBalance < mutexWorst)
      mutexWorst = mutexBalance;
  }
  printf("6. mutex на всё решение     : раундов с минусом %3d/%d, худший остаток %5d  <- верно\n", mutexNegativeRounds,
         kRounds, mutexWorst);
}
