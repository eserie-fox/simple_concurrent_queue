/**
 * @author Hanwen Zheng
 * @email eserinc.z@outlook.com
 * @create date 2024-05-08 15:22:09
 * @modify date 2024-05-08 15:22:09
 * @desc Simple code testing basic function in C++11. This source file is
 * supposed to be compile with '-std=c++11'.
 */
#include <iostream>

#include "../concurrent_queue.h"

#define ASSERT(x)                                                        \
  do {                                                                   \
    if (!(x)) {                                                          \
      std::cerr << __FILE__ << ":" << __LINE__ << " " << #x << " failed" \
                << std::endl;                                            \
      abort();                                                           \
    }                                                                    \
  } while (false)

void TestUnlimited() {
  fox_cq::ConcurrentQueue<int> q;

  ASSERT(q.UnlimitedSize());
  ASSERT(!q.LimitedSize());

  q.Push(1);
  q.Push(2);
  q.Push(3);
  q.SetFinish();
  ASSERT(q.Size() == 3);
  int value;
  ASSERT(q.Pop(value));
  ASSERT(value == 1);
  ASSERT(q.Size() == 2);

  ASSERT(q.Pop(value));
  ASSERT(value == 2);
  ASSERT(q.Size() == 1);

  ASSERT(q.Pop(value));
  ASSERT(value == 3);
  ASSERT(q.Size() == 0);

  ASSERT(!q.Pop(value));
}

void TestLimited() {
  fox_cq::ConcurrentQueue<int, 10> q;

  ASSERT(!q.UnlimitedSize());
  ASSERT(q.LimitedSize());

  q.Push(1);
  q.Push(2);
  q.Push(3);
  q.SetFinish();
  ASSERT(q.Size() == 3);
  int value;
  ASSERT(q.Pop(value));
  ASSERT(value == 1);
  ASSERT(q.Size() == 2);

  ASSERT(q.Pop(value));
  ASSERT(value == 2);
  ASSERT(q.Size() == 1);

  ASSERT(q.Pop(value));
  ASSERT(value == 3);
  ASSERT(q.Size() == 0);

  ASSERT(!q.Pop(value));
}

int main() {
  TestUnlimited();
  TestLimited();
  std::cout << "Passed in C++11" << std::endl;
  return 0;
}