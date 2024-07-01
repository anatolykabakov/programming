package hello

open class Shape {
    open fun draw() {
        println("Shape draw")
    }

    fun fill() {
        println("Shape fill")
    }
}

class Circle() : Shape() {
    override fun draw() {
        println("Circle draw")
    }
}

fun main() {
  val s = Shape()
  s.draw() // Shape draw

  val c = Circle()
  c.fill() // Shape fill
  c.draw() // Circle draw
}
