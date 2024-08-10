package hello

interface IFoo {
    fun foo()
    fun bar(){
        println("IFoo baz")
    }
}

class Child : IFoo {
    override fun foo() {
        println("Child foo")
    }
}

fun main() {
	//   val i = IFoo() # Interface 'interface IFoo : Any' does not have constructors
    val c = Child()
    c.foo() // Child foo
    c.bar() // IFoo baz
}
