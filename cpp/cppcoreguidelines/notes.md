# P: Philosophy

## P.1: Express ideas directly in code

## P.2: Write in ISO Standard C++

## P.3: Express intent

## P.4: Ideally, a program should be statically type safe

## P.5: Prefer compile-time checking to run-time checking

## P.6: What cannot be checked at compile time should be checkable at run time

## P.7: Catch run-time errors early

## P.8: Don’t leak any resources

## P.9: Don’t waste time or space

## P.10: Prefer immutable data to mutable data

## P.11: Encapsulate messy constructs, rather than spreading ## through the code

## P.12: Use supporting tools as appropriate

## P.13: Use support libraries as appropriate

# I: Interfaces

# F: Functions

# C: Classes and class hierarchies

# Enum: Enumerations

# R: Resource management

# ES: Expressions and statements

# Per: Performance

# CP: Concurrency and parallelism

# E: Error handling

## E.1: Develop an error-handling strategy early in a design

## E.2: Throw an exception to signal that a function can’t perform its assigned task

Note It is often said that exceptions are meant to signal exceptional events and
failures. However, that’s a bit circular because “what is exceptional?”
Examples:

- A precondition that cannot be met
- A constructor that cannot construct an object (failure to establish its
  class’s invariant)
- An out-of-range error (e.g., v[v.size()] = 7)
- Inability to acquire a resource (e.g., the network is down)

## E.3: Use exceptions for error handling only

## E.4: Design your error-handling strategy around invariants

## E.5: Let a constructor establish an invariant, and throw if it cannot

## E.6: Use RAII to prevent leaks

## E.7: State your preconditions

## E.8: State your postconditions

## E.12: Use noexcept when exiting a function because of a throw is impossible or unacceptable

## E.13: Never throw while being the direct owner of an object

## E.14: Use purpose-designed user-defined types as exceptions (not built-in types)

## E.15: Throw by value, catch exceptions from a hierarchy by reference

## E.16: Destructors, deallocation, swap, and exception type copy/move construction must never fail

## E.17: Don’t try to catch every exception in every function

## E.18: Minimize the use of explicit try/catch

## E.19: Use a final_action object to express cleanup if no suitable resource handle is available

## E.25: If you can’t throw exceptions, simulate RAII for ## resource management

## E.26: If you can’t throw exceptions, consider failing fast

## E.27: If you can’t throw exceptions, use error codes ## systematically

## E.28: Avoid error handling based on global state (e.g. errno)

## E.30: Don’t use exception specifications

## E.31: Properly order your catch-clauses

# Con: Constants and immutability

# T: Templates and generic programming

# CPL: C-style programming

# SF: Source files

# SL: The Standard Library
