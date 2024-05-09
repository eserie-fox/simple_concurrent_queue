# Simple Concurrent Queue
A simple header only concurrent queue using std::mutex and std::condition_variable in C++11.

This concurrent queue utilizes locks (std::mutex) and sleep mechanisms (std::condition_variable).
The lock protects internal data, and if the queue is full during `Push` or empty during `Pop`, it uses sleep to wait. It decouples the circular queue and other internal data structures from concurrency control logic, making the logic simple and easy to understand.

Its design primarily targets blocking requirements; that is, its `Pop` operation will wait for another thread to `Push` in an element or mark the queue as having no subsequent elements.
However, it also provides `TryPop` non-blocking operations, which immediately return the current result without waiting.

## Use and not to Use
This queue is suitable for requirements where data exchange frequency is not very high; its automatic sleep during blocking significantly reduces CPU consumption from busy waiting. For high-frequency data exchange requirements, it is recommended to use lock-free queues.



# Setup
Just put the only header `concurrent_queue.h` in your project and include it.


# Usage

## Declaration
Limited size queue (circular buffer)
```
/*
 * q1 will reserve space for eight elements.
 * When attempting to push more than 8 elements into it,
 * the thread will wait until there is a consumer
 * to make space in the queue.
 */
ConcurrentQueue<T, 8> q1;
```
Unlimited size queue
```
/*
 * q2 will allocate memory when needed.
 * There won't be any waiting during element pushing.
 */
ConcurrentQueue<T> q2;
```
Notice that void type are supported.
The main difference from normal types is that you cannot specify instances 
during Push or Pop; it can only act as a counter and serves as a semaphore.
```
ConcurrentQueue<void> semaphore;
```

## Operation
They support the following operations regardless of the type.
### Push
```
// Push a default constructed new item into back of the queue
void ConcurrentQueue<T>::Push()

// Move and push `item` into back of the queue
// Enabled when T != void
void ConcurrentQueue<T>::Push(T&& item)

// Copy and push `item` into back of of the queue
// Enabled when T != void
void ConcurrentQueue<T>::Push(const T& item)
```
### Pop
```
// Pop out and discard the front element, will wait for element to push. (blocking, may wait other thread to push new element)
// Return true on success.
// Return false on failure (trying to
// pop from a finished and empty queue).
bool ConcurrentQueue<T>::Pop()

// Pop out the front element to `result`, will wait for element to push. (blocking, may wait other thread to push new element)
// Return true on success.
// Return false on failure (trying to
// pop from a finished and empty queue).
// Enabled when T != void
bool ConcurrentQueue<T>::Pop(T& result)
```
### TryPop
```
// Pop out and discard the front element. (non-blocking, return immediately)
// Return true on success.
// Return false on failure (trying to
// pop from an empty queue).
bool ConcurrentQueue<T>::TryPop()

// Pop out the front element to `result`. (non-blocking, return immediately)
// Return true on success.
// Return false on failure (trying to
// pop from an empty queue).
// Enabled when T != void
bool ConcurrentQueue<T>::TryPop(T& result)
```
### Others
```
// Return number of element in the queue
std::size_t Size() const

// Also, copy and move are supported.
```

## Example

```
ConcurrentQueue<std::string> q;
```
Only one producer and only consumer for simplicity.
Multiple producer and consumer are supported.

### Thread producer
```
q.Push("7");
q.Push("123");
q.Push("456");
// No more input.
// If not set finish, consumer's third `Pop` will wait producer's another `Push` forever.
q.SetFinish();
```
### Thread consumer
```
bool suc;
std::string s;

while(!q.TryPop(s)) {
  // busy waiting
};
assert(s == "7");

suc = q.Pop(s);
assert(suc);
assert(s == "123");

suc = q.Pop(s);
assert(suc);
assert(s == "456");

suc = q.Pop(s);
assert(!suc);
```

For more details, please refer to `example/example1.cc` and `example/example2.cc`.

# Benchmarks

Performance tests can be referenced at this link [moodycamel](https://moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++.htm#benchmarks), where std::queue + std::mutex and the non-blocking part of this concurrent queue (i.e., the `TryPop` interface) share the same implementation principles and exhibit similar performance. Under high-frequency data exchange, its performance is inferior to lock-free implementations. This concurrent queue should primarily be used for blocking requirements, providing a simple and usable implementation for such needs.



