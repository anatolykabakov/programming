package hello

fun main() {
  var a: String = "Hello"
  //   a = null # Error: Null cannot be a value of a non-null type 'kotlin.String'.
  println(a.length)

  var b: String? = "Test"
  b = null

  // println(b.length) # Error: Only safe (?.) or non-null asserted (!!.) calls are allowed on a nullable receiver of type 'kotlin.Nothing?
  // Safe call (value or null)
  println(b?.length)

  // Elvis operator - ?:
  var l: Int = 0
  if (b == null) {
      l = -1
  } else {
      l = b.length
  }
  println(l) // -1

  var c = b?.length ?: -1
  println(c) // -1
  println(c == l) // true

  // !! -- Raises Null pointer exception if var contains null

  //   val t = b!!.length Error : null pointer exception

}
