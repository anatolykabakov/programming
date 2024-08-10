package hello

fun main() {
  // for each

  val immutablelist = listOf("apple", "banana", "orange")
  val mutablelist = mutableListOf("apple", "banana", "orange")

  for (item in immutablelist) {
    println(item)
  }
  for (item in mutablelist) {
    println(item)
  }

  // while
  var index = 0
  while (index < immutablelist.size) {
    println("Item at $index is ${immutablelist[index]}")
    index += 1
  }

  // ranges 3..10

  println(5 in 3..10) // true

  for (i in 1..10) {
    println(i)
  }

  for (i in 1 until 10) { // not include 10
    println(i)
  }

  for (i in 10 downTo 1) {
    println(i)
  }

  for (i in 0..100 step 20) {
    println(i)
  }

}
