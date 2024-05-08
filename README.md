# Simple Concurrent Queue
A simple header only concurrent queue using std::mutex and std::condition_variable in C++11

In theory, it is cross-platform, but it hasn't been tested.



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
// Pop out the front element to `result`, will wait for element to push.
// Return true on success.
// Return false on failure (trying to
// pop from a finished and empty queue).
// Enabled when T != void
bool ConcurrentQueue<T>::Pop(T& result)

// Pop out and discard the front element, will wait for element to push.
// Return true on success.
// Return false on failure (trying to
// pop from a finished and empty queue).
bool ConcurrentQueue<T>::Pop()
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
q.Push("123");
q.Push("456");
// No more input.
// If not set finish, consumer's third `Pop` will wait producer's another `Push` forever.
q.SetFinish();
```
### Thread consumer
```
std::string s;
bool suc = q.Pop(s);
assert(suc);
assert(s == "123");

suc = q.Pop(s);
assert(suc);
assert(s == "456");

suc = q.Pop(s);
assert(!suc);
```

For more details, please refer to example/example1.cc and example/example2.cc.




