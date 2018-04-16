## 2018 SNU Operating System -- Project 2: Add rotational Lock for reader and writer

#### Team1: Gao Zhiyuan, Kim Yongchae, Tormod Aase, Dong Xin
---
### Overview of project
* Add a system call.
* Types of list and Types of Locks in the Implementation.
* Set_rotation Implementation.
* Lock assigning policy and Implementation.
* Exit Handler and Implementation.
* Compilation & Testing.
---
### Add a system call.
---
```=clike
    int set_rotation(int degree);               syscall 380
    int rotlock_read(int degree, int range);    syscall 381
    int rotlock_write(int degree, int range);   syscall 382
    int rotunlock_read(int degree, int range);  syscall 383
    int rotunlock_write(int degree, int range); syscall 313
```
### Types of list
---
Our task struct, contains the task status and the list head to waitingwriter or waitingreader list.
```=clikestruct 
r_{  
  struct task_struct *task;  
    struct list_head list;  
  };
```
```=clikestruct 
w_{  
  struct task_struct *task;  
  struct list_head list;  
  };
```
We manage 2 list in the project, for the waitingreader and waitingwriter.
```=clike  
static struct r_ reader_list = {.task = NULL, .list = LIST_HEAD_INIT(reader_list.list)};    
static struct w_ writer_list = {.task = NULL, .list = LIST_HEAD_INIT(writer_list.list)};
```
### Types of Locks we used for our implementation
---
```=clike
Spinlock_lock_irqsave(&(lock), flags);
Spinlock_unlock_irqrestore(&(lock), flags);
```
These provides a safe locking on entire shared sources, that only one task can access shared sources at the moment and basically Spin lock_lock saves the interrupted state before taking the spin lock and enables the interrupt when Spinlock_unlock. This simply solves concurrency issues.

### Setting rotation value
---
A daemon that updates fake device rotation information, called rotd updates the rotation sequence of ```=clike  0, 30, 60 , ... 330, 0, ...``` in fixed frequency.The set_rotation system updates the rotation information in the kernel. The set_rotation allow the processes that are waiting to grab a lock on a range entered by the new rotation should be allowed to grab the lock (making sure readers and writers don't grab the lock at the same time). This is done by broadcasting(waking up) all the waiting readers and signalling one waiting writers waiting for a lock for each rotation value.

### Lock assigning policy
---
Implement the synchronization primitives so that follow a prevention policy which avoids writer starvation. Assume that a writer wants to acquire a lock with a range R, and the current rotation degree is located in R. In such a case, the writer cannot grab the lock because another reader is holding a lock with a range R' which overlaps with R; the writer should wait for the reader to release its lock. Under such circumstances, other readers with ranges that overlap with R should not be allowed to grab locks even though the current degree is located in their target ranges, in order to prevent writer starvation. In short, "If a reader holds a lock and a writer wants to take the lock, no more readers can take the lock - they should wait" is a desired policy for preventing starvation.

**Rotation Based Writers Lock**
---
* Writer first grabs a spinlock, then increases the waitingWriters count by 1.
* Then writeShouldWait() checks if the degree and range is within the range of the rotation and checks whether there is activewriters or activereaders.
* If there is activewriters or activereaders presented. If there is, then the current waitingwriters are put into the waiting writerslist and calls wait function.
* Else waitingwriters are decreased, activewriters are increased, and then spinlock is unlocked.

**Rotation Based Readers Lock**
---
* Reader first grabs a spinlock , then increases the waitingReaders count by 1.
* Then readShouldWait() checks if the degree and range is within the range of the rotation and checks whether there is activewriters or activereaders.
* If there is activewriters or waitingwriters presented. If there is then the current waitingreadersare put into the waiting readers list and calls Wait function.
* Else waitingreaders are decreased, activereaders are increased then unlocks.

**Rotation Based Writers UnLock**
---
* The activewriters first grabs a spinlock, then decreases the activewriters count by 1.
* If there is still a waitingwriters then the function calls signal one waitingwriters currently to grab the next lock, else broadcast many waiting readers to grab the next lock.
* Then release the spinlock.

**Rotation Based Readers UnLock**
---
* The activereaders first grabs a spinlock, then decreases the activereaders count by 1* If there is still a waitingwriters and activereaders then the function calls signal one waitingwriters currently to grab the next lock.* Then release the spinlock.

### Exit Handler
---
In kernel/exit.c, we added exit_rotlock() function to handle unreleased locks. The exit_rotlock() removes every locks, which the process has requested.

The exit_rotlock() basically wakesup all the waitingwriter and waitingreader list, and deallocating by kfree.

Since the exit_rotlock() is always called, we hack into our kernel, added a count variable of the init_task.h. So when exit_rotlock() function is tested if current->count == 0, then the function simply returns. Inside the rotation lock function current->count is set to non zero value and in rotation unlock function current->count is set to 0.
### Compilation & Testing
Building kernel.
On the terminal
```=clike
> ./build --clean
> ./build image.tar
```
On the putty terminal
```=clike
> thordown
```
To flash the image on the Artik10, after flashing the image file, log in and enable sdb
On the putty terminal
```=clike
> direct_set_debug.sh --sdb-set
```
Then compile the selector.c and trial.c and send the binary executable file to the artik10 device
On the main terminal
```=clike
> sdb root on
> arm-linux-gnueabi-gcc -o select selector.c
> arm-linux-gnueabi-gcc -o trial trial.c
> sdb push select /root/select
> sdb push trial /root/trial
```
Then we test our implementation on the terminal, but before that check that sdb root is enabled.
**Firstly one one terminal we test we one selector**
On the main terminal
```=clike
> sdb shell
> cd root
> ./rotd
> select 1
```
**Secondly on two terminal we test with one selector and one trial**
On one of the main terminal
```=clike
> sdb shell
> cd root
> ./rotd
> select 1
```
On another terminal
```=clike
> sdb shell
> cd root
> ./trial
```
**Third on multiple terminal test with n selector and n trial**
