
# Autocept: The Automatic Interceptor

Autocept is an advanced interposition module within Dice that automatically
intercepts functions without requiring explicit knowledge of their signatures.
It leverages a set of assembly macros to transparently interpose function calls
at runtime, allowing for flexible and easy monitoring of system calls and
library functions.


## What is Autocept?

Autocept is designed to automatically intercept functions by using a combination
of low-level assembly macros and runtime event publishing. It is especially
useful when you want to monitor system calls without needing to explicitly
define each intercepted function’s signature or create a separate interception
function for each one. Autocept operates with a small performance overhead and
integrates seamlessly into Dice’s event-driven architecture.


## Why Use Autocept?

Autocept serves several key purposes in the Dice framework:

1. Function Transparency: Autocept allows the interception of functions without
   requiring prior knowledge of their signatures or the need to manually
   implement each interceptor. This simplifies the process of adding new
   functions to the interposition list.

2. Reduced Boilerplate: Rather than writing a custom interceptor for each system
   function (e.g., sleep, open, close), Autocept allows you to declare the
   desired functions and let the system handle the interception, reducing code
   duplication.

3. Fine-grained Event Publishing: With Autocept, you can publish events for
   arbitrary system calls with a minimal amount of code, making it easier to
   implement custom debugging, tracing, or monitoring tools.

4. Cross-architecture Support: Autocept can be used on different architectures
   (e.g., amd64 and arm64), and it provides a mechanism that adapts to the
   calling conventions and register usage on each platform.


## How Autocept Works

Autocept uses a combination of assembly macros and runtime hooks to interpose
functions. The general process is as follows:

1. Assembly Macros: Autocept provides a set of macros that allow you to declare
   the symbols (e.g., sleep, open, close) you wish to intercept. These macros
   handle saving the argument registers according to the calling conventions
   for the target architecture (e.g., amd64 or arm64).

2. Intercept-Before: Once a function call is detected, the intercept-before
   function is triggered. This publishes an event (e.g., AUTOCEPT) to the Pubsub
   system, allowing subscribers to handle the event as needed.

3. Find Real Function Pointer: After the event is published, Autocept
   dynamically locates the pointer to the real function (the actual system
   function or library function) that is being interposed. This is done through
   a runtime lookup mechanism, ensuring that the original function can still be
   invoked.

4. Register Restoration: Before calling the real function, Autocept restores
   the registers that were saved earlier. This ensures that the calling
   conventions are preserved, and the function receives its arguments as
   expected.

5. Function Execution: The real function is called with the restored registers
   and arguments. This allows the function to execute as if it were not
   interposed, ensuring normal system behavior.

6. Intercept-After: After the real function has executed, Autocept calls the
   intercept-after function. This allows for post-processing of the results or
   any additional logging or monitoring that may be needed. The registers are
   carefully saved and restored to ensure the integrity of the return values.

7. Return to Caller: Finally, Autocept returns to the caller, with the return
   value from the interposed function passed through.


## Event Publishing in Autocept

Autocept publishes a single event type: AUTOCEPT. This event is published before
and after the interposed function is called, providing a simple mechanism for
logging, tracing, or monitoring system calls.

- Event: AUTOCEPT: This event is triggered whenever a function is intercepted.
The event carries a null argument since Autocept is agnostic to the specific
arguments of the function being interposed. This ensures that the interception
mechanism remains lightweight and efficient.


## Example: Intercepting `sleep`

Here is a high-level example of how Autocept could be used to intercept the
sleep function:

1. Declare the Symbol: You specify that you want to intercept sleep using the
   Autocept macros.

```c
AUTOCEPT_SYMBOL(sleep);
```

2. Assembly Macros: The macros save the registers used for arguments according
   to the calling convention for amd64 or arm64 and then call intercept-before.

3. Publishing the Event: The intercept-before function publishes the event
   `AUTOCEPT` before calling the real sleep function.

4. Function Execution: The real sleep function is found dynamically, and its
   execution is triggered with the proper arguments.

5. Post-Execution: After the function finishes, the intercept-after function is
   called to publish additional events or perform necessary actions, and then
   the return value is passed back.


## Advantages of Autocept

1. No Signature Required: Unlike traditional interception, where you must define
   each function’s signature, Autocept dynamically intercepts any function by
   simply listing its symbol.

2. Performance: Since the interception mechanism is lightweight and involves
   minimal assembly-level manipulation, Autocept introduces minimal overhead.

3. Flexibility: You can easily add new system or library functions to the
   interception list by simply declaring their symbols, making Autocept a highly
   flexible tool for event-driven monitoring and testing.

4. Cross-platform: Autocept works seamlessly across different architectures like
   amd64 and arm64, with the assembly macros automatically adapting to the
   calling conventions of each platform.


## Limitations

- No Argument Handling: Autocept does not inspect or handle the function
  arguments directly, as the event published is null. However, this can be
  mitigated by having subscribers use other mechanisms (e.g., memory access or
  indirect tracking) to infer function arguments if necessary.

- Limited Customization: Since Autocept automatically intercepts functions, it
  is less customizable than manual interception methods. However, this tradeoff
  is typically acceptable for general-purpose monitoring or debugging.


# Switcher: A Mechanism for Systematic Concurrency Testing

Switcher is a key component within the Dice framework that facilitates
systematic concurrency testing by providing precise control over thread
execution. It allows threads to yield and wake up in a controlled manner,
enabling deterministic testing scenarios for multithreaded programs.


## What is Switcher?

The Switcher component is designed to pause and resume threads in a
deterministic order, which is essential for systematic testing and reproducing
specific execution paths. This is achieved through the yield() and wake()
functions, which control thread scheduling.


## Why Use Switcher?

Switcher is especially useful in the context of systematic concurrency testing,
where the goal is to explore a wide range of thread interleavings to detect
subtle concurrency bugs, such as data races, deadlocks, and order-dependent
errors.

1. Deterministic Scheduling: Switcher ensures that threads execute in a
   well-defined order, making it possible to reproduce specific interleavings
   for debugging or testing purposes.

2. Control Over Execution Flow: By calling yield() and wake(), developers can
   force threads to pause and resume at specific points in their execution,
   which is essential for testing various timing scenarios.

3. Concurrency Bug Detection: The ability to control thread scheduling helps to
   trigger complex concurrency issues that are hard to reproduce in an
   uncontrolled execution environment.

4. Replayability: By controlling thread execution, you can record specific
   interleavings and replay them to diagnose issues, making it easier to
   identify and fix bugs that may not be detectable in regular testing.


## How Switcher Works

Switcher operates based on the interaction between the yield() and wake()
functions. Here’s how they work:

1. yield(my_id, any): This function blocks the calling thread until it is
   explicitly woken up. The my_id argument identifies the current thread, and
   the any argument can control the conditions under which the thread will
   resume. If any is set to true, any thread that has called yield() can return,
   but only one thread will actually be allowed to continue at a time.

2. wake(id): This function wakes up a specific thread, identified by id. It
   allows a thread to continue execution after it has previously called yield().
   If the argument is set to ANY_THREAD, any thread that has previously called
   yield() will be allowed to wake up, but only one thread will return.

   - wake(ANY_THREAD): Allows any thread to resume execution, based on the
     interleaving logic defined by the system.
   - wake(id): Wakes up a specific thread identified by id, providing more
     fine-grained control over thread execution.

3. Sequencer: The sequencer is a subscriber that coordinates the use of yield()
   and wake() to enforce specific thread execution orders. It subscribes to all
   events and controls the flow of execution based on a probabilistic approach,
   determining when to wake a thread. By throwing a “dice” (i.e., generating a
   random value), the sequencer can decide which thread to wake and when to
   yield.

4. Controlled Thread Execution: With the help of the Switcher, you can ensure
   that only one thread is executing user code at any given time, preventing
   multiple threads from running simultaneously and potentially interfering with
   each other.


## Example Usage

1. Thread Initialization: When a new thread is created, it calls yield() at the
   beginning of its execution, which blocks it from proceeding until it is woken
   up by the sequencer or another thread.

2. Parent-Child Thread Relationship: The parent thread will call wake(id) to
   wake the child thread and then call yield() itself, blocking until the child
   thread is ready to proceed.

3. Controlled Test Scenarios: By using the wake() and yield() functions, you
   can enforce a specific interleaving of threads and test how your system
   behaves under various timing conditions.


# Sequencer Module
