package hello

fun foo42(): Int {
  return 42
}

fun fooSame(): Int = 42

fun sum(x: Int, y: Int): Int = x + y

fun toString(x: Int): String {
  return "String is $x"
}

fun toList(x: Int, y: Int, z: Int): List<Int> {
  return listOf(x, y, z)
}

fun defaultArgs(x: Int, y: Int = 2) {
  println(x + y)
}

fun foo(name: String, number: Int = 42, toUpperCase: Boolean = false): String {
  return (if (toUpperCase) name.toUpperCase() else name) + number
}

fun main() {
  println(foo42())
  println(fooSame())
  println(sum(1, 2))
  println(toString(42))
  println(toList(x = 1, y = 2, z = 3))
  defaultArgs(1)
  defaultArgs(1, 3)

  println(foo("foo"))
  println(foo("foo", 31, true))
}
