/**
 * @author Hanwen Zheng
 * @email eserinc.z@outlook.com
 * @create date 2024-05-08 09:10:15
 * @modify date 2024-05-08 09:10:15
 * @desc A simple concurrent queue supporting multiple producer and multiple
 * consumer using std::mutex and std::condition_variable.
 */
#pragma once
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

namespace fox_cq {

static const std::size_t ConcurrentQueueUnlimitedSize =
    static_cast<std::size_t>(-1);

namespace internal {

template <typename T, std::size_t MaxSize>
class ConcurrentQueueContainer {
 public:
  ConcurrentQueueContainer() : head_(0), tail_(0), size_(0) {
    data_.reserve(MaxSize);
  }
  ConcurrentQueueContainer(const ConcurrentQueueContainer&) = default;
  ConcurrentQueueContainer(ConcurrentQueueContainer&&) = default;
  ConcurrentQueueContainer& operator=(const ConcurrentQueueContainer&) =
      default;
  ConcurrentQueueContainer& operator=(ConcurrentQueueContainer&&) = default;

  template <typename U>
  void Push(U&& value) {
    assert(size_ < MaxSize);
    if (data_.size() <= tail_) [[unlikely]] {
      data_.emplace_back(std::forward<U>(value));
    } else {
      new (&data_[tail_]) T(std::forward<U>(value));
    }
    if (tail_ < MaxSize - 1) {
      ++tail_;
    } else {
      tail_ = 0;
    }
    ++size_;
  }

  void Push() {
    assert(size_ < MaxSize);
    if (data_.size() <= tail_) [[unlikely]] {
      data_.emplace_back();
    } else {
      new (&data_[tail_]) T();
    }
    if (tail_ < MaxSize - 1) {
      ++tail_;
    } else {
      tail_ = 0;
    }
    ++size_;
  }

  void Pop(T& value) {
    assert(size_ > 0);
    value = std::move(data_[head_]);
    data_[head_].~T();
    if (head_ < MaxSize - 1) {
      ++head_;
    } else {
      head_ = 0;
    }
    --size_;
  }

  void Pop() {
    assert(size_ > 0);
    data_[head_].~T();
    if (head_ < MaxSize - 1) {
      ++head_;
    } else {
      head_ = 0;
    }
    --size_;
  }

  std::size_t Size() const { return size_; }

  std::size_t Capacity() const { return MaxSize; }

  bool Empty() const { return size_ == 0; }

  bool Full() const { return size_ == MaxSize; }

 private:
  std::vector<T> data_;
  std::size_t head_;
  std::size_t tail_;
  std::size_t size_;
};

template <typename T>
class ConcurrentQueueContainer<T, ConcurrentQueueUnlimitedSize> {
 public:
  ConcurrentQueueContainer() = default;
  ConcurrentQueueContainer(const ConcurrentQueueContainer&) = default;
  ConcurrentQueueContainer(ConcurrentQueueContainer&&) = default;
  ConcurrentQueueContainer& operator=(const ConcurrentQueueContainer&) =
      default;
  ConcurrentQueueContainer& operator=(ConcurrentQueueContainer&&) = default;

  template <typename TValue>
  void Push(TValue&& value) {
    data_.push(std::forward<TValue>(value));
  }

  void Push() { data_.emplace(); }

  void Pop(T& value) {
    assert(!data_.empty());
    value = std::move(data_.front());
    data_.pop();
  }

  void Pop() {
    assert(!data_.empty());
    data_.pop();
  }

  std::size_t Size() const { return data_.size(); }

  bool Empty() const { return data_.empty(); }

  bool Full() const { return false; }

 private:
  std::queue<T> data_;
};

template <std::size_t MaxSize>
class ConcurrentQueueContainer<void, MaxSize> {
 public:
  ConcurrentQueueContainer() : size_(0) {}
  ConcurrentQueueContainer(const ConcurrentQueueContainer&) = default;
  ConcurrentQueueContainer(ConcurrentQueueContainer&&) = default;
  ConcurrentQueueContainer& operator=(const ConcurrentQueueContainer&) =
      default;
  ConcurrentQueueContainer& operator=(ConcurrentQueueContainer&&) = default;

  void Push() {
    assert(size_ < MaxSize);
    ++size_;
  }

  void Pop() {
    assert(size_ > 0);
    --size_;
  }

  std::size_t Size() const { return size_; }

  std::size_t Capacity() const { return MaxSize; }

  bool Empty() const { return size_ == 0; }

  bool Full() const { return size_ == MaxSize; }

 private:
  std::size_t size_;
};

template <>
class ConcurrentQueueContainer<void, ConcurrentQueueUnlimitedSize> {
 public:
  ConcurrentQueueContainer() : size_(0) {}
  ConcurrentQueueContainer(const ConcurrentQueueContainer&) = default;
  ConcurrentQueueContainer(ConcurrentQueueContainer&&) = default;
  ConcurrentQueueContainer& operator=(const ConcurrentQueueContainer&) =
      default;
  ConcurrentQueueContainer& operator=(ConcurrentQueueContainer&&) = default;

  void Push() { ++size_; }

  void Pop() {
    assert(size_ > 0);
    --size_;
  }

  std::size_t Size() const { return size_; }

  bool Empty() const { return size_ == 0; }

  bool Full() const { return false; }

 private:
  std::size_t size_;
};
}  // namespace internal

template <typename T, std::size_t MaxSize = ConcurrentQueueUnlimitedSize>
class ConcurrentQueue {
 public:
  ConcurrentQueue() = default;
  ConcurrentQueue(const ConcurrentQueue& other) {
    std::lock(lock_, other.lock_);
    std::lock_guard<std::mutex> guard1(lock_, std::adopt_lock);
    std::lock_guard<std::mutex> guard2(other.lock_, std::adopt_lock);
    data_ = other.data_;
    finished_ = other.finished_;
    WakeupAll();
    other.WakeupAll();
  };
  ConcurrentQueue(ConcurrentQueue&& other) {
    std::lock(lock_, other.lock_);
    std::lock_guard<std::mutex> guard1(lock_, std::adopt_lock);
    std::lock_guard<std::mutex> guard2(other.lock_, std::adopt_lock);
    data_ = std::move(other.data_);
    finished_ = other.finished_;
    WakeupAll();
    other.WakeupAll();
  }
  ConcurrentQueue& operator=(const ConcurrentQueue& other) {
    if (this != &other) {
      std::lock(lock_, other.lock_);
      std::lock_guard<std::mutex> guard1(lock_, std::adopt_lock);
      std::lock_guard<std::mutex> guard2(other.lock_, std::adopt_lock);
      data_ = other.data_;
      finished_ = other.finished_;
      WakeupAll();
      other.WakeupAll();
    }
    return *this;
  }
  ConcurrentQueue& operator=(ConcurrentQueue&& other) {
    if (this != &other) {
      std::lock(lock_, other.lock_);
      std::lock_guard<std::mutex> guard1(lock_, std::adopt_lock);
      std::lock_guard<std::mutex> guard2(other.lock_, std::adopt_lock);
      data_ = std::move(other.data_);
      finished_ = other.finished_;
      WakeupAll();
      other.WakeupAll();
    }
    return *this;
  }

  ~ConcurrentQueue() { SetFinish(); }

  // Mark the queue has no more `Push` operation.
  // `Push` operation after `SetFinish` will be ignored.
  // Notice that `Pop` operation still works for remaining elements in the
  // queue.
  void SetFinish() {
    {
      std::lock_guard<std::mutex> guard{lock_};
      finished_ = true;
    }
    WakeupAll();
  }

  // Push a default constructed new item into back of the queue
  void Push() { PushImpl(); }

  // Move and push `item` into back of the queue
  // Enabled when T != void
  template <typename U = T>
  void Push(
      typename std::enable_if<!std::is_same<U, void>::value, U&&>::type item) {
    PushImpl(std::move(item));
  }

  // Copy and push `item` into back of of the queue
  // Enabled when T != void
  template <typename U = T>
  void Push(typename std::enable_if<!std::is_same<U, void>::value,
                                    const U&>::type item) {
    PushImpl(item);
  }

  // Pop out and discard the front element. (non-blocking, return immediately)
  // Return true on success.
  // Return false on failure (trying to
  // pop from an empty queue).
  bool TryPop() { return TryPopImpl(); }

  // Pop out the front element to `result`. (non-blocking, return immediately)
  // Return true on success.
  // Return false on failure (trying to
  // pop from an empty queue).
  // Enabled when T != void
  template <typename U = T>
  bool TryPop(
      typename std::enable_if<!std::is_same<U, void>::value, U&>::type result) {
    return TryPopImpl(result);
  }

  // Pop out and discard the front element, will wait for element to push. (blocking, may wait other thread to push new element)
  // Return true on success.
  // Return false on failure (trying to
  // pop from a finished and empty queue).
  bool Pop() { return PopImpl(); }

  // Pop out the front element to `result`, will wait for element to push. (blocking, may wait other thread to push new element)
  // Return true on success.
  // Return false on failure (trying to
  // pop from a finished and empty queue).
  // Enabled when T != void
  template <typename U = T>
  bool Pop(
      typename std::enable_if<!std::is_same<U, void>::value, U&>::type result) {
    return PopImpl(result);
  }

  // Return number of element in the queue
  std::size_t Size() const {
    std::lock_guard<std::mutex> guard{lock_};
    return data_.Size();
  }

  // Return true iff this queue has no limit.
  bool UnlimitedSize() const { return MaxSize == ConcurrentQueueUnlimitedSize; }

  // Return true iff this queue has limit.
  bool LimitedSize() const { return MaxSize != ConcurrentQueueUnlimitedSize; }

 private:
  void WakeupAll() const {
    empty_cond_.notify_all();
    // Full waiting only happens in limited size.
    if (LimitedSize()) full_cond_.notify_all();
  }

  template <typename... Args>
  void PushImpl(Args&&... item) {
    std::unique_lock<std::mutex> lk{lock_};
    if (LimitedSize() && data_.Full() && !finished_) {
      full_cond_.wait(lk, [this] { return !data_.Full() || finished_; });
    }
    if (finished_) {
      // finished, should notify other threads to stop waiting.
      WakeupAll();
      return;
    }
    data_.Push(std::forward<Args>(item)...);
    if (LimitedSize() && !data_.Full()) {
      full_cond_.notify_one();
    }
    empty_cond_.notify_one();
  }

  template <typename... Args>
  bool PopImpl(Args&&... result) {
    std::unique_lock<std::mutex> lk{lock_};
    empty_cond_.wait(lk, [this] { return !data_.Empty() || finished_; });
    if (!data_.Empty()) {
      data_.Pop(std::forward<Args>(result)...);
      if (!data_.Empty()) {
        empty_cond_.notify_one();
      } else if (finished_) {
        // finished, should notify other threads to stop waiting.
        WakeupAll();
        return true;
      }
      if (LimitedSize()) full_cond_.notify_one();

      return true;
    }

    assert(finished_);
    // finished, should notify other threads to stop waiting.
    WakeupAll();
    return false;
  }

  template <typename... Args>
  bool TryPopImpl(Args&&... result) {
    std::unique_lock<std::mutex> lk{lock_};
    if (!data_.Empty()) {
      data_.Pop(std::forward<Args>(result)...);

      if (LimitedSize()) full_cond_.notify_one();
      return true;
    }
    return false;
  }

  mutable std::mutex lock_;
  mutable std::condition_variable empty_cond_;
  mutable std::condition_variable full_cond_;
  internal::ConcurrentQueueContainer<T, MaxSize> data_;
  bool finished_ = false;
};

}  // namespace fox_cq
