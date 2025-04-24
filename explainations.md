# Code Explaination
# Table of Contents

1. [`timer.c`](#timer-c)
   - [Comparator: `wakeup_cmp`](#wakeup_cmp)
   - [Putting a Thread to Sleep: `timer_sleep`](#timer_sleep)
   - [Waking Threads on Each Tick: `timer_interrupt`](#timer_interrupt) 
   - [Summary](#summary)
  
2. [Advanced Scheduler](#3-advanced-schedular)
     - [Nice Value](#1-nice-value)
     - [Recent CPU](#2-recent-cpu)
     - [Load Average](#3-load-average)
     - [Calculating Priority](#4-calculating-priority)
      
---

# 1. `timer.c`
## `wakeup_cmp`
```c
// Comparator: earlier wakeup_tick ⇒ comes first
static bool
wakeup_cmp(const struct list_elem *a,
           const struct list_elem *b)
{
  const sleep_element *ea = list_entry(a, sleep_element, element);
  const sleep_element *eb = list_entry(b, sleep_element, element);
  return ea->wakeup_tick < eb->wakeup_tick;
}
```
This function, `wakeup_cmp`, is a comparator used to determine the relative order of two elements in a list based on their `wakeup_tick` values. It is designed to work with a linked list structure, likely part of a custom implementation of a list (e.g., from a library like Pintos). The purpose of this comparator is to ensure that elements with earlier `wakeup_tick` values are ordered before those with later values.

The function takes three parameters: two pointers to `list_elem` structures (`a` and `b`).

Inside the function, the `list_entry` macro is used to retrieve the parent structure (`sleep_element`) that contains the `list_elem` being compared. This macro essentially performs a type-safe cast, allowing the function to access the `wakeup_tick` field of the `sleep_element` structure. The `list_entry` macro typically works by calculating the offset of the `element` field within the `sleep_element` structure and *using pointer arithmetic* to retrieve the base address of the parent structure.

Finally, the function compares the `wakeup_tick` values of the two `sleep_element` structures (`ea` and `eb`). If the `wakeup_tick` of `ea` is **less** than that of `eb`, the function returns `true`, indicating that `ea` should come before `eb` in the list. Otherwise, it returns `false`. This logic ensures that the list is sorted in ascending order of `wakeup_tick`, with the earliest wake-up times appearing first.

The `wakeup_cmp` function is used as a comparator for ordering elements in the `sleep_list`. While the function itself only compares two elements (`a` and `b`), the entire list is ordered by using this comparator in conjunction with list operations that maintain sorted order

--- 

## `timer_sleep`
```c
/* Sleeps for approximately TICKS timer ticks. Interrupts must
  be turned on. */
void timer_sleep(int64_t ticks_to_sleep)
{
  ASSERT(intr_get_level() == INTR_ON); // Ensure interrupts are enabled.
  /* Build our sleeper node. */
  sleep_element sleeper; // Create a sleep_element to represent the sleeping thread.
  int64_t start = timer_ticks(); // Get the current tick count.
  sleeper.wakeup_tick = start + ticks_to_sleep; // Calculate the tick at which the thread should wake up.
  sema_init(&sleeper.sema, 0); // Initialize the semaphore to block the thread.
  enum intr_level old_level = intr_disable(); // Disable interrupts to modify the shared sleep_list safely.
  /* Insert in sorted order by wakeup_tick. */
  list_insert_ordered(&sleep_list, &sleeper.element, wakeup_cmp, NULL); // Add the sleeper to the sleep_list in sorted order.
  intr_set_level(old_level); // Restore the previous interrupt level.
  /* Block until timer_interrupt ups our semaphore. */
  sema_down(&sleeper.sema); // Block the thread until it is woken up by the timer interrupt.
}
```
The `timer_sleep` function is designed to put the current thread to sleep for a specified number of timer ticks (`ticks_to_sleep`). It ensures that the thread will not resume execution until the specified duration has elapsed.

The function begins by asserting that interrupts are enabled (`ASSERT(intr_get_level() == INTR_ON)`). This is important because the function relies on interrupt-driven mechanisms, such as a timer interrupt, to wake the thread after the sleep duration.

Next, a `sleep_element` structure is created to represent the sleeping thread. This structure contains a `wakeup_tick` field, which is calculated as **the current tick count (`timer_ticks()`) plus the number of ticks to sleep**. This value represents the tick at which the thread should be **woken up**. Additionally, a semaphore (`sleeper.sema`) is initialized to 0. This semaphore will be used to **block the thread until it is explicitly woken up**.

To safely modify the shared `sleep_list` (a global list of sleeping threads), interrupts are temporarily **disabled** using `intr_disable()`. The `sleep_element` is then inserted into the `sleep_list` in sorted order based on its `wakeup_tick` value. This ensures that the list remains ordered, allowing the timer interrupt handler to efficiently wake threads in the correct order. After the insertion, the previous interrupt level is restored using `intr_set_level()`.

Finally, the thread is blocked by calling `sema_down(&sleeper.sema)`. This causes the thread to wait until the semaphore is incremented (or "upped") by the timer interrupt handler. When the timer interrupt determines that the thread's `wakeup_tick` has been reached, it will signal the semaphore, allowing the thread to resume execution.

**Disabling interrupts** can indeed be problematic if done improperly or for extended periods, as it can lead to issues such as missed interrupts, reduced system responsiveness, or even deadlocks in certain scenarios. However, in the context of the `timer_sleep` function, disabling interrupts is a *necessary* and *controlled* operation to ensure the integrity of shared data structures like the sleep_list.

Why Disabling Interrupts is Necessary Here:
1. Prevent Race Conditions
2. Atomic Operations
3. Short Duration

Why It's Acceptable in This Case:
- **Controlled Environment:**
The function is part of a low-level kernel or operating system code, where such practices are more common and acceptable. The kernel is designed to handle these situations carefully.

- **Minimal Impact:**
The critical section (the part of the code where interrupts are disabled) is *very small* and involves only a *single* operation on the `sleep_list`. This minimizes the risk of missed interrupts or other issues.

- **Interrupt-Driven Wakeup:**
The system relies on the timer interrupt to wake up sleeping threads. Disabling interrupts briefly during the insertion process does not interfere with the overall functionality of the timer or other critical interrupts, **as long as the duration is kept short.**

---

## `timer_interrupt`
```c
static void
timer_interrupt(struct intr_frame *args UNUSED)
{
  ticks++; // Increment the global tick count.
  thread_tick(); // Notify the scheduler that a timer tick has occurred.

  /* Wake up as many sleepers as are due. */
  while (!list_empty(&sleep_list)) // Check if there are any sleeping threads in the sleep_list.
  {
    struct list_elem *e = list_front(&sleep_list); // Get the first element in the sleep_list.
    sleep_element *s = list_entry(e, sleep_element, element); // Retrieve the sleep_element structure from the list element.

    if (s->wakeup_tick > ticks) // If the wakeup_tick of the first sleeper is in the future, stop processing.
      break;

    list_pop_front(&sleep_list); // Remove the sleeper from the sleep_list.
    sema_up(&s->sema); // Wake up the thread by signaling its semaphore.
  }
}
```
The `timer_interrupt` function is a timer interrupt handler that is invoked periodically by the system's timer hardware. Its primary responsibilities are to increment the global tick count, perform per-tick thread management, and wake up any threads whose sleep duration has expired.

The function begins by incrementing the global `ticks` variable, which tracks the number of timer ticks since the system started. It then calls `thread_tick()`, which likely performs bookkeeping tasks related to thread scheduling, such as updating thread statistics or triggering a context switch if necessary.

The core functionality of the function is to **wake up threads** that are due to resume execution. It iterates through the global `sleep_list`, which contains threads that are currently sleeping. The list is sorted by the `wakeup_tick` of each thread, ensuring that the **earliest** wake-up times are at the **front** of the list.

For each thread in the `sleep_list`, the function retrieves the **first** element using `list_front()` and converts it into a `sleep_element` structure using the `list_entry` macro. It then checks whether the thread's `wakeup_tick `is **greater** than the current `ticks`. If so, the loop breaks because no other threads in the list are ready to wake up (thanks to the sorted order).

If the `wakeup_tick` is **less** than or **equal** to the current `ticks`, the thread's `sleep_element` is **removed** from the list using `list_pop_front()`. The function then calls `sema_up(&s->sema)` to increment the thread's semaphore, effectively unblocking it and allowing it to resume execution.

In summary, this function ensures that sleeping threads are woken up at the correct time by checking the `wakeup_tick` against the current tick count. By maintaining the `sleep_list` in **sorted** order, the function efficiently processes only the threads that are ready to wake up, minimizing unnecessary work during the interrupt. This design is critical for maintaining the responsiveness and efficiency of the system's timer and thread scheduling mechanisms.

---

## Summary
### 1. The Comparator (`wakeup_cmp`)  
- **Purpose:** Keeps the `sleep‑list` sorted so the thread with the **earliest** wake‑up time is always at the **front**.  
- **How it works:**  
  1. Takes two list nodes, each representing a sleeping thread.  
  2. Retrieves each node’s `wakeup_tick` (the tick count when it wants to wake).  
  3. Returns “true” if the first thread’s wake‑up time is **before** the second’s—so it stays ahead in the list.

---

### 2. Putting a Thread to Sleep (`timer_sleep`)  
- **Goal:** Park (block) the current thread until a specified number of ticks have passed.  
- **Key steps:**  
  1. **Assert interrupts are on** (we need the timer interrupt to wake us).  
  2. Create a small record (`sleeper`) holding:  
     - `wakeup_tick` = current ticks + desired sleep length  
     - a **semaphore** initialized to 0 (so `sema_down` will block).  
  3. **Disable interrupts briefly** to safely insert that record into the global `sleep_list` in sorted order (using our comparator).  
  4. **Restore interrupts** immediately.  
  5. Call `sema_down` on the sleeper’s semaphore—this blocks the thread until the timer interrupt does a matching `sema_up`.

---

### 3. Waking Threads on Each Tick (`timer_interrupt`)  
- **Runs every timer interrupt** (~1000 times/sec).  
- **Responsibilities:**  
  1. Increment the global `ticks` count.  
  2. Do any per‑tick bookkeeping (`thread_tick()`).  
  3. **Check the front of `sleep_list`:**  
     - As long as there’s a record whose `wakeup_tick` ≤ current `ticks`, pop it off the list and call `sema_up` on its semaphore.  
     - That unblocks exactly those threads whose sleep time has arrived—no busy‑waiting, no needless work on later sleepers.

---

### Why this matters  
- **No busy‑waiting:** Threads don’t spin in a loop checking the clock; they’re blocked until the right moment.  
- **Efficiency:** The list stays sorted, so the interrupt handler only ever looks at the front element(s)—constant‑time per wakeup.  
- **Safety:** Briefly turning off interrupts prevents two CPUs (or an interrupt and a normal context) from corrupting the shared list.

--- 

# 3. Advanced schedular
## 1. Nice value
- The *initial thread* starts with a `nice` value of **zero** (which we change in the `init_thread()` function).
- Other threads start with a nice value **inherited** from their *parent* thread (which we change in the `create_thread()` function).
- `thread_get_nice()`: just returns the nice value of the current thread.
  
**Steps to Implement `thread_set_nice(int new_nice)`:**
1. **Update** the `nice` value of the current thread.
2. **Recalculate** the thread's priority based on the new nice value.
3. **Yield the CPU** if the thread's priority changes and it is no longer the *highest-priority* thread.

A **multilevel feedback queue scheduler (MLFQS)** is a dynamic CPU‐scheduling algorithm that partitions ready processes into several queues of differing priorities and time-quantum lengths, and then dynamically promotes or demotes processes between these queues based on their observed CPU and I/O behavior.


## 2. Recent CPU

### Initialization
- Each thread's `recent_cpu` starts at 0
- Initialized in `init_thread()` using `INT_TO_FP(0)`
- Inherited from parent thread when creating new threads

### Updates
1. **Every Timer Tick:**
   - Running thread's `recent_cpu` incremented by 1
   - Except for idle thread which is skipped
   - Code: `t->recent_cpu = FP_ADD_INT(t->recent_cpu, 1)`

2. **Every Second (TIMER_FREQ ticks):**
   - Updates `recent_cpu` for all threads using `calculate_recent_cpu()`
   - Updates happen after load average calculation
   - Updates are done using `thread_foreach()`

### Calculation Formula
```c
recent_cpu = (2*load_avg)/(2*load_avg + 1) * recent_cpu + nice
```
- Uses fixed-point arithmetic for precise calculations
- Components:
  - Coefficient: `(2*load_avg)/(2*load_avg + 1)`
  - Weighted previous value: `coefficient * recent_cpu`
  - Nice adjustment: `+ nice`
  
### Usage
- Used in priority calculations: `priority = PRI_MAX - (recent_cpu/4) - (nice*2)`
- Exposed via `thread_get_recent_cpu()` which returns value * 100
- Affects thread scheduling through priority system
  
### Key Functions
1. `calculate_recent_cpu()`: Updates a thread's recent_cpu value
2. `thread_get_recent_cpu()`: Returns current thread's recent_cpu * 100
3. `thread_tick()`: Handles periodic updates
   
### Important Notes
- All calculations done in fixed-point arithmetic
- Updates happen at different frequencies (every tick vs. every second)
- Idle thread is excluded from recent_cpu updates
- System-wide load_avg affects all threads' recent_cpu values
## 3. Load Average


## 4. Calculating Priority
The priority is calculated using the following formula:
```c
priority = PRI_MAX - (recent_cpu / 4) - (nice * 2) 
``` 
**Steps in `calculate_priority`:**
  1. **Assert MLFQS is enabled**
      - Check that `thread_mlfqs` is true before calculating

  2. **Calculate Components**
      - Use the thread's `recent_cpu` value
      - Divide `recent_cpu` by 4
      - Multiply `nice` value by 2
      - Subtract both from `PRI_MAX`

  3. **Clamp the Result**
      - Ensure priority stays within valid range
      - If `priority > PRI_MAX`, set to `PRI_MAX`
      - If `priority < PRI_MIN`, set to `PRI_MIN`

  4. **Return Final Priority**
      - The resulting value becomes the thread's **new priority**
  
  5. **Update thread scheduling**
  ```c
   if (t == thread_current() && !list_empty(&ready_list) &&
      t->priority < list_entry(list_front(&ready_list),
                               struct thread, elem)
                        ->priority)
    thread_yield();
  ```
  The condition has **three** parts that must all be **true** for the thread to **yield**:

 1. `t == thread_current()` - Checks if we're dealing with the currently running thread
 2. `!list_empty(&ready_list)` - Verifies there are other threads waiting to run
 3. `t->priority < list_entry(...)` - Compares current thread's priority with the highest priority waiting thread

The most complex part is the priority comparison using `list_entry()`. This macro (1) converts the front element of the ready list into a thread structure and (2) accesses its priority. The front of the ready list is used because in a priority scheduler, the **highest** priority thread should always be at the **front**.

Advantages of this:
- **It implements preemptive scheduling:** a thread will give up the CPU immediately if it becomes *lower* priority
- **It's self-balancing:** no external scheduler is needed to force the context switch
- It maintains **priority scheduling** invariants by ensuring **higher** priority threads always **run first**

## Summary
Here's a comprehensive explanation of the key components in the Multi-Level Feedback Queue Scheduler (MLFQS) implementation:

### 1. Important Variables

- `nice`: Integer value between -20 and 20 that affects thread priority
- `recent_cpu`: Fixed-point number tracking CPU usage
- `load_avg`: System-wide load average (fixed-point)
- `priority`: Thread priority (0-63)

### 2. Core Functions and Their Purpose

#### Priority Calculation

1. `calculate_priority(struct thread *t)`
   - Formula: `priority = PRI_MAX - (recent_cpu/4) - (nice*2)`
   - Ensures priority stays within [PRI_MIN, PRI_MAX]
   - Called when nice value changes or periodically

#### Nice Value Management

1. `thread_set_nice(int new_nice)`
   - Sets current thread's nice value
   - Recalculates priority if MLFQS is enabled
   - Yields CPU if needed

2. `thread_get_nice(void)`
   - Returns current thread's nice value

#### Recent CPU Calculation

1. `calculate_recent_cpu(struct thread *t)`
   - Formula: `recent_cpu = (2*load_avg)/(2*load_avg + 1) * recent_cpu + nice`
   - Uses fixed-point arithmetic
   - Skips calculation for idle thread

2. `thread_get_recent_cpu(void)`
   - Returns current thread's recent_cpu * 100
   - Provides 2 decimal places of precision

#### Load Average Management

1. `calculate_load_avg(void)`
   - Formula: `load_avg = (59/60)*load_avg + (1/60)*ready_threads`
   - Counts ready threads plus running thread (if not idle)
   - Uses fixed-point arithmetic

2. `thread_get_load_avg(void)`
   - Returns system load average * 100

### 3. Update Mechanisms

#### Periodic Updates

1. Every Timer Tick:
   - Increment `recent_cpu` for running thread
   - Update priorities if needed
2. Every fourth tick:
   - Recalculate priorities.
3. Every Second (TIMER_FREQ ticks):
   - Recalculate `load_avg`
   - Update `recent_cpu` for all threads
   - Recalculate priorities for all threads

### 4. Thread Creation and Initialization

1. Initial Thread:
   - Starts with `nice` = 0
   - `recent_cpu` = 0
   - Default priority

2. New Threads:
   - Inherit `nice` value from parent
   - Start with `recent_cpu` = 0
   - Priority calculated based on `nice` value

### 5. Fixed-Point Arithmetic

- Used for precise calculations without floating-point operations
- Implements `recent_cpu` and `load_avg` calculations
- Converts between fixed-point and integer representations

This implementation ensures fair CPU distribution while considering both long-term CPU usage (`recent_cpu`) and user-specified preferences (`nice` values).