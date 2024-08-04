package hello

import kotlinx.coroutines.*
import java.util.Random

fun main() = runBlocking {
  repeat(100) {
      launch {
          val result = doWork(it.toString())
      	  println(result)
      }
  }
}

suspend fun doWork(name: String): String {
    delay(Random().nextInt(5000).toLong())
    return "Done. $name"
}
