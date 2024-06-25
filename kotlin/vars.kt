package helllo

fun main() {
  // const
  val immutable : String = "Hello, world!"
  // v = null Error: val cannot be reassigned
  println(immutable)

  var mutable : String = immutable
  println(mutable)
  mutable = "1234"
  println(mutable)
  // mutable = 1 error: the integer literal does not conform to the expected type String

  println("val is $immutable")
  println("concatination is ${immutable + mutable}")
}
