# Рекомендации решения задачи лидкода

взято из ролика youtube https://www.youtube.com/watch?v=xF554Tlzo-c

1. Разделить обсудумывание задачи и кодирование. Не приступать к кодированию,
   пока решение не станет совершенно понятным.
2. Написать тесткейсы до кода, чтобы лучше понять условие
3. Если не получается решить задачу за 45 минут, то стоит посмотреть решения и
   обьяснения. Нужно постараться понять решение и воспроизвести его
   самостоятельно
4. Попробовать оптимизировать решение. Убрать вложенность

# Рекомендации решения задачи лидкода (составлено самостоятельно)

Background: Сформулировать условие задачи. Задать уточняющие вопросы. Привести
тестовые примеры( f(input) = output1. ) входных данных, выходных данных,
обьяснение для лучшего понимания. Какие ограничения. Условие задачи должно быть
конкретным и понятным.

Intuition: Визуализировать задачу. Сформулировать интуитивное решение задачи.

Algorithm: Сформулировать алгоритм решения задачи. Сформулировать тесткейсы для
алгоритма. Особые случаи. Прогнать тесткейсы через алгоритм.

Complexity Analysis: Оценить сложность алгоритма Big O. Time. Space.

Code. Не приступать к кодированию, пока решение не станет совершенно понятным.
Написать чистый код. Optimization: предложить оптимизацию алгоритма

# Метод четыре шага

https://www.freecodecamp.org/news/how-to-solve-coding-problems/

1. Понять проблему:

- Прочитать условие задачи.
- Что на входе? (базовый, граничный и невалидный случаи).
- Что на выходе? (базовый случай, граничный и невалидный).
- Составить простые и сложные тесткейсы для базового, граничного и невалидного
  случая. Например, f(input) = output.
- Для сложных проблем требуется составить функциональные и нефункциональные
  требования. Составить варианты использования.

2. Составить план решения.

- Для простых задач достаточно составить последовательность действий для решения
  проблемы.
- Для сложных задач требуется абстрагироваться от кода и составить план, как бы
  человек решал задачу.

3. Выполнить план.

- Написать простое решение, переведя план решения в код.
- Если решение части проблемы в текущий момент не найти, то стоит
  сосредоточиться на части, которая понятна. Решение сложной части может прийти
  в процессе.

4. Оптимизация:

- Есть ли другие подходы к решению задачи?
- Можно ли с первого раза понять смысл кода?
- Можно ли этот код переиспользовать?
- Как улучшить вычислительную сложность?
- Нужен ли рефакторинг?
- Как другие люди решают эту проблему?

# Рекомендации как проходить алгоритмическое интервью с сайта LeetCode

Most algorithmic interview rounds are between 45 - 60 minutes. The interviews
can be broken down into stages, and at each stage, there are multiple things you
should do to maximize your chances of success. Let's break it down.

## Introductions

At the start of the interview, most of the time your interviewer will briefly
introduce themselves and their role at the company, then ask you to introduce
yourself.

- Prepare and rehearse a brief introduction of yourself. It should summarize
  your education, work experience, and interests in 30-60 seconds.
- Smile and speak with a confident voice.
- When the interviewer is talking about their work at the company, pay
  attention - it will help to ask questions about it later.
- If the interviewer mentioned anything that you are also interested in, whether
  it be their work or a hobby, mention it.

## Problem statement

After introductions, your interviewer will give you a problem statement. If
you're working in a shared text editor, they will most likely paste the problem
description along with a test case into the editor, and then read the question
to you.

1. Make sure you fully understand the problem. After the interviewer has read
   the problem over, confirm what the problem is asking by paraphrasing it back
   to them.
2. Ask clarifying questions regarding the input, for example:

- Will the input only have integers, or could there be other types?
- Will the input be sorted or unsorted?
- Is the input guaranteed to have elements or could it be empty?
- What should I do if an invalid input is given?

3. Ask about the expected input size. Sometimes, the interviewer will be vague,
   but if they do give you a range, it can be a clue. For example, if n is very
   small, it is likely backtracking. If n is around 100 - 1000, an O(n2)
   solution might be optimal. If n is very large, then you might need to do
   better than O(n).

The interviewer will likely give you one or two example test cases. Quickly walk
through one to confirm that you understand the problem.

```
Asking clarifying questions not only helps you better understand the problem but also shows attention to detail and being considerate of things like edge cases.
```

## Brainstorming DS&A

Try to figure out what data structure or algorithm is applicable. Break the
problem down and try to find common patterns that you've learned. Figure out
what the problem needs you to do, and think about what data structure or
algorithm can accomplish it with a good time complexity.

Think out loud. It will show your interviewer that you are good at considering
tradeoffs. If the problem involves looking at subarrays, then be vocal about
considering a sliding window because every window represents a subarray. Even if
you're wrong, the interviewer will still appreciate your thought process.

The best way to train this skill is to practice LeetCode problems.

```
By thinking out loud, you also open the door for the interviewer to give you hints and point you in the right direction.
```

Once you have decided on what data structures/algorithms to use, you now need to
construct your actual algorithm. Before coding, you should think of the rough
steps of the algorithm, explain them to the interviewer, and make sure they
understand and agree that it is a reasonable approach. Usually, if you are on
the wrong path, they will subtly hint at it.

```
It is extremely important that you are receptive to what the interviewer says at this stage. Remember: they know the optimal solution. If they are giving you a hint, it is because they want you to succeed. Don't be stubborn and be ready to explore the ideas they give you.
```

## Implementation

Once you have come up with an algorithm and gotten the interviewer on board, it
is time to start writing code.

- If you intend on using a library or module like Python's collections for
  example, make sure the interviewer is ok with it before importing it.
- As you implement, explain your decision-making. For example, if you are
  solving a graph problem, when you declare a set seen, explain that it is to
  prevent visiting the same node more than once, thus also preventing cycles.
- Write clean code. Every major programming language has a convention for how
  code should be written. Make sure you know the basics of the language that you
  plan to be using. Google provides a
  summary(https://google.github.io/styleguide/) for all major languages. The
  most important sections are case conventions, indentations, spacing, and
  global variables.
- Avoid duplicated code. For example, if you are doing a DFS on a matrix, loop
  over a directions array [(0, 1), (1, 0), (0, -1), (-1, 0)] instead of writing
  the same logic 4 times for each direction. If you find yourself writing
  similar code in multiple places, consider creating a function or simplifying
  it with a loop.
- Don't be scared of using helper functions. They make your code more modular,
  which is very important in real software engineering. It might also make
  potential follow-ups easier.

Don't panic if you get stuck or realize that your original plan might not work.
Communicate your concerns with your interviewer. It makes their life a lot
harder if you are struggling in silence.

One strategy is to first implement a brute force solution while acknowledging
that it is a suboptimal solution. Once it is completed, analyze each part of the
algorithm, figure out what steps are "slow", and try to think about how it can
be sped up. Engage your interviewer and include them in the discussion - they
want to help.

## Testing & debugging

Once you have finished coding, your interviewer will likely want to test your
code. Depending on the company, there are a few different environments your
interview might be taking place in:

1. Built-in test cases, code is run

- These platforms are similar to LeetCode. There will be a wide variety of test
  cases - small inputs, huge inputs, inputs that test edge cases.

- This environment puts the most stress on your code because a non-perfect
  solution will be exposed.

- However, it also puts the least stress on creating your own tests, since they
  are already built-in.

2. Write your own test cases, code is run

- These platforms are usually shared text editors that support running code. The
  interviewer will want you to write your own test cases.

- To actually test the code, you should write in the outermost scope of the
  code, wherever the code will get run first. Assuming you solved the problem in
  a function (like on LeetCode), you can call your function with the test cases
  you wrote and print the results to the console.

- When writing your own tests, make sure to try a variety. Include edge cases,
  intuitive inputs, and possibly invalid inputs (if the interviewer wants you to
  handle that case).

3. Write your own test cases, code is not run

- These platforms will just be shared text editors that do not support running
  code. The interviewer will want you to write your own test cases and walk
  through them manually.

- To "test" the code, you will have to go through the algorithm manually with
  each test case. Try to condense trivial parts - for example, if you're
  creating a prefix sum, don't literally walk through the for loop with every
  element. Say something along the lines of "after this for loop, we will have a
  prefix sum which will look like ...".

- As you are walking through the code, write (in the editor, outside your
  function somewhere) the variables used in the function and continuously update
  them.

Regardless of the scenario, if it turns out your code has an error, don't panic!
If the environment supports running code, put print statements in relevant
locations to try to identify the issue. Walk through a test case manually (as
you would if you have an environment without runtime support) with a small test
case. As you do it, talk about what the expected values of the variables should
be and compare them with what they actually are. Again, the more vocal you are,
the easier it is for the interviewer to help you.

## Explanations and follow-ups

After coding the algorithm and running through test cases, be prepared to answer
questions regarding your algorithm. Questions you should always expect and be
ready for include:

1. What is the time and space complexity of the algorithm?

- You should speak in terms of the worst-case scenario. However, if the worst
  case is rare and the average case has a significantly faster runtime, you
  should also mention this.

2. Why did you choose to do ...? This could be your choice of data structure,
   choice of algorithm, choice of for loop configurations, etc. Be prepared to
   explain your thought process.

3. Do you think that the algorithm could be improved in terms of time or space
   complexity?

- If the problem needs to look at every element in the input (let's say the
  input isn't sorted and you needed to find the max element), then you probably
  can't do better than O(n). Otherwise, you probably can't do better than
  O(logn).

- If the interviewer asks this, the answer is usually yes. Be careful about
  asserting that your algorithm is optimal - it's ok to be wrong, but it's not
  ok to be confidently wrong.

If there is time remaining in the interview, you may be asked an entirely new
question. In that case, restart from step 2 (Problem statement). However, you
may also be asked follow-ups to the question you already solved. The interviewer
might introduce new constraints, ask for an improved space complexity, or any
other number of things.

```
This section is why it is important to actually understand solutions and not just memorize them.
```

## Outro

The interviewer will usually reserve a few minutes at the end of the interview
to allow you to ask questions about them or the company. You will rarely be able
to improve the outcome of the interview at this point, but you can certainly
make it worse.

Interviews are a two-way street. You should use this time as an opportunity to
also get to know the company and see if you would like to work there. You should
prepare some questions before the interview, such as:

- What does an average day look like?
- Why did you decide to join this company instead of another one?
- What is your favorite and least favorite thing about the job?
- What kind of work can I expect to work on?

All big companies will have their own tech blog. A great way to demonstrate your
interest in the company is to read some blog posts and compile a list of
questions regarding why the company makes the decisions they do.

Be interested, keep smiling, listen to the interviewer's responses, and ask
follow-up questions to show that you understand their answers.

If you don't have quality questions or appear bored/uninterested, it could give
a bad signal to the interviewer. It doesn't matter how well you did on the
technical portion if the interviewer doesn't like you in the end.

## Interview stages cheat sheet

The following will be a summary of the "Stages of an interview" article. If you
have a remote interview, you can print this condensed version and keep it in
front of you during the interview.

### Stage 1: Introductions

- Have a rehearsed 30-60 second introduction regarding your education, work
  experience, and interests prepared.
- Smile and speak with confidence.
- Pay attention when the interviewer talks about themselves and incorporate
  their work into your questions later.

### Stage 2: Problem statement

- Paraphrase the problem back to the interviewer after they have read it to you.
- Ask clarifying questions about the input such as the expected input size, edge
  cases, and invalid inputs.
- Quickly walk through an example test case to confirm you understand the
  problem.

### Stage 3: Brainstorming DS&A

- Always be thinking out loud.
- Break the problem down: figure out what you need to do, and think about what
  data structure or algorithm can accomplish it with a good time complexity.
- Be receptive to any comments or feedback from the interviewer, they are
  probably trying to hint you towards the correct solution.
- Once you have an idea, before coding, explain your idea to the interviewer and
  make sure they understand and agree that it is a reasonable approach.

### Stage 4: Implementation

- Explain your decision-making as you implement. When you declare things like
  sets, explain what the purpose is.
- Write clean code that conforms to your programming language's conventions.
- Avoid writing duplicate code - use a helper function or for loop if you are
  writing similar code multiple times.
- If you are stuck, don't panic - communicate your concerns with your
  interviewer.
- Don't be scared to start with a brute force solution (while acknowledging that
  it is brute force), then improve it by optimizing the "slow" parts.
- Keep thinking out loud and talk with your interviewer. It makes it easier for
  them to give you hints.

### Stage 5: Testing & debugging

- When walking through test cases, keep track of the variables by writing at the
  bottom of the file, and continuously update them. Condense trivial parts like
  creating a prefix sum to save time.
- If there are errors and the environment supports running code, put print
  statements in your algorithm and walk through a small test case, comparing the
  expected value of variables and the actual values.
- Be vocal and keep talking with your interviewer if you run into any problems.

### Stage 6: Explanations and follow-ups

Questions you should be prepared to answer:

- Time and space complexity, average and worst case.
- Why did you choose this data structure, algorithm, or logic?
- Do you think the algorithm could be improved in terms of complexity? If they
  ask you this, then the answer is usually yes, especially if your algorithm is
  slower than O(n).

### Stage 7: Outro

- Have questions regarding the company prepared.
- Be interested, smile, and ask follow-up questions to your interviewer's
  responses.

# Google рекомендации

Вот по каким критериям оценивает Google (от ex googler):

- Алгоритмы. В начале, не написав ни строчки кода, вы должны объяснить, как вы
  будете решать эту задачу, какие алгоритмы и структуры данных будете
  использовать, какая сложность по времени и памяти у них. Сначала лучше
  предложить брут форс решение, если есть несколько оптимальных, предложить все.

- Кодинг. Вы должны озвученные ранее мысли по решению задачи изложить в коде. В
  идеале вы должны написать хорошо читаемый код с хорошо именованными
  переменными, методами и т.д. Ну и, конечно, написанный код должен работать и
  покрывать все кейсы, даже если вы его не запустите, что, скорее всего, будет
  так, так как вы будете все писать на пустом листке, без всяких подсвечиваний
  синтаксиса (лучше готовится к этому, если вам предложат писать в каком-то
  онлайн-редакторе кода, будем считать, что вам повезло).

- Коммуникация. Это самое сложная часть для всех не англоговорящих или просто
  интровертов. Вы должны комментировать каждую написанную вами строчку, почему
  вы ее пишете и к чему она приведет. Также вы должны разговаривать на общие и
  профессиональные темы по ходу интервью, здесь могут проверять ваши софт
  скиллы, даже не дойдя до этапа behavioral.

- Способ решения проблемы. Вы с самого начала должны по полочкам расставить
  условия задачи и задавать уточняющие вопросы и выяснить, какие edge cases
  могут быть. По мере написания кода вы должны разделять задачи на подзадачи,
  если это нужно, не писать сумбурно и все переписывать, ваш алгоритм должен
  быть понятен собеседующему на 100%.
