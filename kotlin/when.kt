package hello

fun testWhen(input: Any) {
    when (input) {
        1 -> println("One")
        in 10..20 -> println("10..20")
        is String -> println("string")
        else -> println("foo")
    }
}

fun main() {
  testWhen(1) // One
  testWhen("Hello") // string
  testWhen(15) // 10..20
  testWhen(1000) // foo
}
