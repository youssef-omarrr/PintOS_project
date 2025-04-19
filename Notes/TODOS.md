# TODOS

## Main Objs

1. Ready List must be sorted by priority at all times
2. Implement Preemption.

## Normal Scheduler 

### [ ] `Thread_Create()`
1. insert **Ordered** in `ready_list`.
2. Compare the `P_new` and `P_Cur`
3. Call `schedule()` if `P_new` > `P_Cur`

### [ ] `Thread_unblock()`
- This is used in the thread_create to unblock the thread i.e. put it in the ready Q.
1. Should insert to `ready_list` in P order.


### [ ] `Thread_yield()`
1. Should insert to `ready_list` in P order.


### [ ] `Thread_set_priority()`
1. Should reorder the `ready_list` after setting P.


### [ ] `sema_down()`
1. Sort waiters in P order.


### [ ] `cond_wait()`
1. Sort waiters in P order.

--- 

## Priority Donation

### functions to modify:

1. `init_thread` : init the P donation list.
2. `lock_acquire` : 
    1. If the lock is not available, store the address of the lock.
    2. Store the current Priority and maintian the donated threads on list.
    3. Donate priority.
3. `lock_release`:
    1. When the lock is released, remove the thread that holds the lock on donation list  
    2. set P to old_P if no other waiters o.w set P to highest donor.

4. `thread_set_priority`: Set P considering the donations

5. somehow solve nested donation
