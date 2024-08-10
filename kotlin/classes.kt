package hello

class Person(val firstName: String, val lastName: String, var age: Int) {
    var children: MutableList<Person> = mutableListOf() // ArrayList

    // init called after constructor
    init {
        println("Person $firstName created")
    }

    // secondary constructor
    constructor(firstName: String, lastName: String, age: Int, child: Person): this(firstName, lastName, age) {
        children.add(child)
    }

    constructor(): this("firstName", "lastName", -1) {}
}

// data prefix creates methods equals, hashcode, toString and etc
data class Rectangle(var height: Double, var length: Double) {
    var perimeter = (height + length) * 2
    var test = 1
    	get() = field + 1 // field == this.test
    	set(value) {
            if (value < 0) println("Negative value")
        }
    fun area() = height * length

}

fun main() {
 	var p1 = Person("John", "Doe", 29)
    println(p1.firstName)

    var c1 = Person("Child", "Doe", 27)
    var p2 = Person("Wojak", "Doe", 27, c1)
    println(p2.firstName)

    var p3 = Person()
    println(p3.firstName)

    val r1 = Rectangle(2.0, 3.0)
    val r2 = Rectangle(2.0, 3.0)
    println(r1 == r2) // true
}
