#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "devices/pit.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"

// == new == //
#include "lib/kernel/list.h"

/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops(unsigned loops);
static void busy_wait(int64_t loops);
static void real_time_sleep(int64_t num, int32_t denom);
static void real_time_delay(int64_t num, int32_t denom);

////////////////////////=== NEW ===//////////////////////////////////
// Ordered list of sleeping threads.
static struct list sleep_list;

/* One element per sleeping thread.
  This structure is used to store information about a thread that is currently sleeping. */
typedef struct sleep_element
{
  struct list_elem element; // List element to link this structure in the sleep_list.
  int64_t wakeup_tick;      // The tick at which the thread should wake up.
  struct semaphore sema;    // Semaphore used to block and wake up the thread.
} sleep_element;

/* Comparator function for ordering sleep_elements in the sleep_list.
  Threads with earlier wakeup_tick values are placed earlier in the list.
  This ensures that the thread with the earliest wakeup time is at the front of the list. */
static bool
wakeup_cmp(const struct list_elem *a,
           const struct list_elem *b)
{
  // Retrieve the sleep_element structures from the list elements.
  const sleep_element *ea = list_entry(a, sleep_element, element);
  const sleep_element *eb = list_entry(b, sleep_element, element);

  // Compare the wakeup_tick values to determine the order.
  return ea->wakeup_tick < eb->wakeup_tick;
}
////////////////////////=== NEW ===//////////////////////////////////

/* Sets up the timer to interrupt TIMER_FREQ times per second,
   and registers the corresponding interrupt. */
void timer_init(void)
{
  pit_configure_channel(0, 2, TIMER_FREQ);
  intr_register_ext(0x20, timer_interrupt, "8254 Timer");

  // == new == //
  list_init(&sleep_list);
}

/* Calibrates loops_per_tick, used to implement brief delays. */
void timer_calibrate(void)
{
  unsigned high_bit, test_bit;

  ASSERT(intr_get_level() == INTR_ON);
  printf("Calibrating timer...  ");

  /* Approximate loops_per_tick as the largest power-of-two
     still less than one timer tick. */
  loops_per_tick = 1u << 10;
  while (!too_many_loops(loops_per_tick << 1))
  {
    loops_per_tick <<= 1;
    ASSERT(loops_per_tick != 0);
  }

  /* Refine the next 8 bits of loops_per_tick. */
  high_bit = loops_per_tick;
  for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
    if (!too_many_loops(loops_per_tick | test_bit))
      loops_per_tick |= test_bit;

  printf("%'" PRIu64 " loops/s.\n", (uint64_t)loops_per_tick * TIMER_FREQ);
}

/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks(void)
{
  enum intr_level old_level = intr_disable();
  int64_t t = ticks;
  intr_set_level(old_level);
  return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed(int64_t then)
{
  return timer_ticks() - then;
}

////////////////////////=== NEW ===//////////////////////////////////
/* Sleeps for approximately TICKS timer ticks. Interrupts must
  be turned on. */
void timer_sleep(int64_t ticks_to_sleep)
{
  ASSERT(intr_get_level() == INTR_ON); // Ensure interrupts are enabled.

  /* Build our sleeper node. */
  sleep_element sleeper;                        // Create a sleep_element to represent the sleeping thread.
  int64_t start = timer_ticks();                // Get the current tick count.
  sleeper.wakeup_tick = start + ticks_to_sleep; // Calculate the tick at which the thread should wake up.
  sema_init(&sleeper.sema, 0);                  // Initialize the semaphore to block the thread.

  enum intr_level old_level = intr_disable(); // Disable interrupts to modify the shared sleep_list safely.
  /* Insert in sorted order by wakeup_tick. */
  list_insert_ordered(&sleep_list, &sleeper.element, wakeup_cmp, NULL); // Add the sleeper to the sleep_list in sorted order.


  intr_set_level(old_level); // Restore the previous interrupt level.

  /* Block until timer_interrupt ups our semaphore. */
  sema_down(&sleeper.sema); // Block the thread until it is woken up by the timer interrupt.
}
////////////////////////=== NEW ===//////////////////////////////////

/* Sleeps for approximately MS milliseconds.  Interrupts must be
   turned on. */
void timer_msleep(int64_t ms)
{
  real_time_sleep(ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts must be
   turned on. */
void timer_usleep(int64_t us)
{
  real_time_sleep(us, 1000 * 1000);
}

/* Sleeps for approximately NS nanoseconds.  Interrupts must be
   turned on. */
void timer_nsleep(int64_t ns)
{
  real_time_sleep(ns, 1000 * 1000 * 1000);
}

/* Busy-waits for approximately MS milliseconds.  Interrupts need
   not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_msleep()
   instead if interrupts are enabled. */
void timer_mdelay(int64_t ms)
{
  real_time_delay(ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts need not
   be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_usleep()
   instead if interrupts are enabled. */
void timer_udelay(int64_t us)
{
  real_time_delay(us, 1000 * 1000);
}

/* Sleeps execution for approximately NS nanoseconds.  Interrupts
   need not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_nsleep()
   instead if interrupts are enabled.*/
void timer_ndelay(int64_t ns)
{
  real_time_delay(ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void timer_print_stats(void)
{
  printf("Timer: %" PRId64 " ticks\n", timer_ticks());
}

////////////////////////=== NEW ===//////////////////////////////////
/* Timer interrupt handler. */
static void
timer_interrupt(struct intr_frame *args UNUSED)
{
  ticks++; // Increment the global tick count.
  thread_tick(); // Notify the scheduler that a timer tick has occurred.

  enum intr_level old_level = intr_disable();
  // Wake up as many sleepers as are due.
  while (!list_empty(&sleep_list)) // Check if there are any sleeping threads in the sleep_list.
  {
    struct list_elem *e = list_front(&sleep_list); // Get the first element in the sleep_list.
    sleep_element *s = list_entry(e, sleep_element, element); // Retrieve the sleep_element structure from the list element.

    if (s->wakeup_tick > ticks) // If the wakeup_tick of the first sleeper is in the future, stop processing.
      break;

    list_pop_front(&sleep_list); // Remove the sleeper from the sleep_list.
    intr_set_level(old_level);  // Re-enable interrupts before waking thread
    sema_up(&s->sema);// Wake up the thread by signaling its semaphore.
    old_level = intr_disable();  // Disable again for next iteration
  }
}
////////////////////////=== NEW ===//////////////////////////////////

/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops(unsigned loops)
{
  /* Wait for a timer tick. */
  int64_t start = ticks;
  while (ticks == start)
    barrier();

  /* Run LOOPS loops. */
  start = ticks;
  busy_wait(loops);

  /* If the tick count changed, we iterated too long. */
  barrier();
  return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.

   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait(int64_t loops)
{
  while (loops-- > 0)
    barrier();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep(int64_t num, int32_t denom)
{
  /* Convert NUM/DENOM seconds into timer ticks, rounding down.

        (NUM / DENOM) s
     ---------------------- = NUM * TIMER_FREQ / DENOM ticks.
     1 s / TIMER_FREQ ticks
  */
  int64_t ticks = num * TIMER_FREQ / denom;

  ASSERT(intr_get_level() == INTR_ON);
  if (ticks > 0)
  {
    /* We're waiting for at least one full timer tick.  Use
       timer_sleep() because it will yield the CPU to other
       processes. */
    timer_sleep(ticks);
  }
  else
  {
    /* Otherwise, use a busy-wait loop for more accurate
       sub-tick timing. */
    real_time_delay(num, denom);
  }
}

/* Busy-wait for approximately NUM/DENOM seconds. */
static void
real_time_delay(int64_t num, int32_t denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  ASSERT(denom % 1000 == 0);
  busy_wait(loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000));
}
