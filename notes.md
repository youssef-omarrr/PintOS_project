## 0. installation notes:
### In step 5:
- **DO NOT** go to the repo folder dir, stay where you are.
- copy this command (the one in the repo is not working):
```shell
docker run --platform linux/amd64 --rm -it -v "$(pwd)/CSEx61-dockerized-pintos:/root/pintos " 
```
then paste the **image ID** from docker


## 1. timer.c
- change `timer_sleep()` function (remove the bust waiting "while loop"), one way to do it is using **semaphores**.
- use `sema_down()` in `timer_sleep()` and `sema_up()` in `timer_interrupt()`
- init a **list** using `list.c` and use `list_ordered` where in the list (to reduce overhead)
```c
list = [{thread_id_1, ticks_no}, {thread_id_2, ticks_no}, {thread_id_3, ticks_no}]
```
where the list is ordered by the ticks number
- in `timer_interrupt()` go through the list and `sema_up()` the one with the right ticks number