## 0. Installation:

### 1. Install Docker Desktop 
- From [This Link](https://www.docker.com/products/docker-desktop/)

### 2. Pull the PintOs image sajedalmorsy/pintos:1.0 from Docker Hub:
```bash
docker pull sajedalmorsy/pintos:1.0 
```

### 3. Clone the current Repo 

```bash
git clone https://github.com/youssef-omarrr/PintOS_project.git
```

### 4. cd to the repo folder **Not the PintOS folder**

```bash
cd PintOS_project
```

### 5. Copy the Image ID from Docker Desktop Images
![image](https://github.com/user-attachments/assets/3f7dfd41-a2e4-4bd0-b037-8f0f3a512871)



### 6. Run the Docker Container with the image ID
- **Replace `<ImageID>` with your Image ID**
- RUN FROM **THE ROOT** OF THE REPO FOLDER **DONT** CD TO `CSEx61-dockerized-pintos`
```bash
docker run --platform linux/amd64 --rm -it -v "$(pwd)/CSEx61-dockerized-pintos:/root/pintos " <ImageID>
```

### 7. Now you are running the Container in Docker 
![image](https://github.com/user-attachments/assets/072c248c-9237-4c64-bcea-3af8ea5a39a6)

---
## Important Links

- [Introduction - Stanford](https://web.stanford.edu/~ouster/cgi-bin/cs140-spring20/pintos/pintos_1.html#SEC1)
- [Threads - Stanford](https://web.stanford.edu/~ouster/cgi-bin/cs140-spring20/pintos/pintos_2.html#SEC15)
- [Advanced Scheduler - Stanford](https://web.stanford.edu/~ouster/cgi-bin/cs140-spring20/pintos/pintos_7.html#SEC131)



--- 

## 1. timer.c
- change `timer_sleep()` function (remove the bust waiting "while loop"), one way to do it is using **semaphores**.
- use `sema_down()` in `timer_sleep()` and `sema_up()` in `timer_interrupt()`
- init a **list** using `list.c` and use `list_ordered` where in the list (to reduce overhead)
```c
list = [{thread_id_1, ticks_no}, {thread_id_2, ticks_no}, {thread_id_3, ticks_no}]
```
where the list is ordered by the ticks number
- in `timer_interrupt()` go through the list and `sema_up()` the one with the right ticks number

## 2. priority scheduler
- you need to update `sema_up()` found in `synch.c` file (using `thread_yield()`)
- implement `thread_set_priority()` and `thred_get_priority()` functions.
- `thread_set_priority` video solution:
  
    ![alt text](images/image.png)
- `thred_get_priority()` should have **priority donation**, and return the donated priority, by updating `next_thread_to_run()`.
- **priority donation**:
  - if a thread (eg. `thread_2`) of **lower** priority has a lock and another thread (eg. `thread_1`) of **higher** priority is created and it wants the same lock, normally `thread_1` would have the highest priority of execution, but because `thread_2` has the lock and it should give `thread_2` the highest priority to release the lock so that `thread_1` can work.
  - `thread_2` will have the same priority of `thread_1` (*in term of numbers*) until `thread_2` releases the lock then it returns again to its original priority. 
- Based on:
    1. **locks**:
       1. lock_aquire()
       2. lock_release()
    2. **semaphores**:
       1. sema_up()
       2. sema_down()
    3. **condition variables**:
       1. cond_wait()
       2. cond_signal()
       3. cond_broadcast()

## 3. Advanced scheduler
- priority changes using a certain equation.
  
  ![alt text](images/image-3.png)
- after every interrupt update the thread priority.
- **nice values**: They are integers in the range of -20 to 20, where:
  - A lower nice value (e.g., -20) indicates a *higher* priority for the thread.
  - A higher nice value (e.g., 20) indicates a *lower* priority for the thread.
- **recent cpu**: how many ticks has the thread used the cpu for.
  
  ![alt text](images/image-2.png)
- **load_avg**: average number of threads ready to run over the past minute.

    ![alt text](images/image-4.png)

- for **nice values** and **recent cpu**:
  - you need to add them to the thread implementation in `thread.h` (eg. int nice; fixed_t recent_cpu)
  - complete the `thread_get_nice()`, `thread_set_nice()`, `thread_get_recent_cpu()` and `thread_set_recent_cpu()` functions.
  - video solution:
  
    ![alt text](images/image-1.png)
