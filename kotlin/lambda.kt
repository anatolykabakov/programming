package hello

fun main() {
  val arr = intArrayOf(1,5,3,2)

  arr.forEach { e -> print("$e ") } // 1 2 3 5
  println("")
  println(arr.map { e -> e * 2 }) // [2, 4, 6, 10]
  println(arr.filter { e -> e % 2 == 0 }) // [2]
  println(arr.reduce { sum, e -> sum + e }) // 1

  val sortedByDescending = arr.sortedByDescending { it }
  println(sortedByDescending) // [5, 3, 2, 1]

  println(arr.any { it > 10 }) // false
  println(arr.any { it < 10 }) // true

  println(arr.sum()) // 11

  val numbers = listOf(1, -1, 2, -2)
  val (positive, negative) = numbers.partition { it > 0 }

  println(positive) // [1, 2]
  println(negative) // [-1, -2]

  val groupedValues = listOf("a", "b", "aaa", "bbb", "bb").groupBy { it.length }

  println(groupedValues) // {1=[a, b], 3=[aaa, bbb], 2=[bb]}
}
