# TODOS

## Main Objs

1. Ready List must be sorted by priority at all times
2. Implement Preemption.

## Normal Scheduler 

### [X] `Thread_Create()`
1. insert **Ordered** in `ready_list`.
2. Compare the `P_new` and `P_Cur`
3. Call `schedule()` if `P_new` > `P_Cur`

### [X] `Thread_unblock()`
- This is used in the thread_create to unblock the thread i.e. put it in the ready Q.
1. Should insert to `ready_list` in P order.


### [X] `Thread_yield()`
1. Should insert to `ready_list` in P order.


### [X] `Thread_set_priority()`
1. Should reorder the `ready_list` after setting P.


### [X] `sema_down()`
1. Sort waiters in P order.


### [X] `cond_wait()`
1. Sort waiters in P order.

--- 

## Priority Donation
### Functions to modify and there logic:
1. lock_acquire()
   1. i guess we need to add donee thread(thread we donated to) to easen the priority recalculations
   2.  for the priority recalcualtions:
       1. save the current priority
       2. check if there's donors to current thread
       3. if there's donors find the maximum among all donors
       4. finally if any donor is higher than original priority use donation
       5. then call the function for the donee if there's one and if our priority increased
2. lock_release()
   1. clean up priority donations by
      1. looping over all threads that donated to this thread
      2. if any donor only donated beacuse of this lock(which will be realssed) break the donor -> donee  link and remove the donation
   2. releaase the lock
   3. reacalculate the priorities
   4. and try to yield

3. sema_up()

4. sema_down()

5. thread_set_priority() Should consider three scenarios:
   1. Current thread hasn't been donated. So you need to set both old_priority and priority.
   2. Current thread has been donated. But we need to set priority now. If the new priority is less than the priority that the thread get by donating. We only need to set old_priority.
   3. Current thread has been donated. But it will be donated again. We needn't to change old_priority.
   4. We should also cap the priority to max and min priority.
6. We should add a function to try yield to reduce redunduncy and allow us to use readly list in synch.c 
7. In sema_down it makes no sense to insert ordered when the priorities will change due to donation so we have to get the max everytime in sema_up any way


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

6. somehow solve nested donation
