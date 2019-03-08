/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}
	
	// add stuff here as needed

	lock->held = 0; //noone holds the lock at first
	lock->holding = NULL; //^^	

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	// add stuff here as needed
	lock_release(lock); //release the lock so noone holds it anymore.	

	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	// Write this
		
	assert(lock != NULL); 		//make sure lock is a lock
	assert(in_interrupt==0); 	//make sure you are not in an interrupt
	int spl;
	spl = splhigh(); 		//save priority and set interrupts to off
	assert(lock_do_i_hold(lock) == 0);//make sure you are not currently in possession of lock
	while (1) {
		if(lock->holding == NULL){
			lock->holding = curthread; 	//assign the lock to the current thread
			lock->held = 1; 		//say the lock is held
			splx(spl); 			//reset priority	
			return;
		}
		else{
			thread_sleep(lock); 	//lock things while the lock is held
		}
	}
	splx(spl); 			//reset priority	

}

void
lock_release(struct lock *lock)
{
	int spl;
	assert(lock != NULL);		//make sure lock is a lock
	assert(lock_do_i_hold(lock) == 1);	//make sure the current thread has the lock			
	spl = splhigh();
	if(lock_do_i_hold(lock)){		//save priority and set interrupts to off
		lock->held = 0;			
		lock->holding = NULL;		//say that the lock is open for grabs
		thread_wakeup(lock);		//wake up the lock thread
	}
	splx(spl);			//reset priority

}

int
lock_do_i_hold(struct lock *lock)
{
	assert(lock != NULL);		//make sure lock is a lock
	if (lock->held == 0) return 0;	//if lock is not held noone is holding lock return 0
	if (lock->holding == curthread) return 1; // if I hold the lock return 1
	return 0;			//if I don't hold the lock but it is held, return 0

}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
	
	// add stuff here as needed
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	// add stuff here as needed
	
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	int spl;			
	spl = splhigh();		//go into high priority mode
	lock_release(lock);		//release the lock supplied
	thread_sleep(cv);		//sleep the cv thread
	lock_acquire(lock);		//when you wake up reset priority and reacquire lock
	splx(spl);	
	(void)cv;
	(void)lock;
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	int spl; 
	spl = splhigh();
	thread_wakeone(cv);		//save priority, wake up a single thread in cv then reset priority
	splx(spl);
	(void)cv;
	(void)lock;

}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	int spl; 
	spl = splhigh();
	thread_wakeup(cv);		//save priority, wake up all threads in cv then reset priority
	splx(spl);
	(void)cv;
	(void)lock;

	
}
