/**
 * @author Hanwen Zheng
 * @email eserinc.z@outlook.com
 * @create date 2024-05-08 09:13:32
 * @modify date 2024-05-08 09:13:32
 * @desc Another simple example showing that object lifetime is handled well.
 */
#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "../concurrent_queue.h"

const int RangePerProducer = 10000;

struct MemoryLeakTestStruct {
  static std::atomic<int> remaining_count;
  static std::atomic<int> constructed_count;
  MemoryLeakTestStruct() : controller(std::make_unique<CntControl>()) {
    ++constructed_count;
  }
  MemoryLeakTestStruct(MemoryLeakTestStruct&&) = default;
  MemoryLeakTestStruct& operator=(MemoryLeakTestStruct&&) = default;
  struct CntControl {
    CntControl() { ++remaining_count; }
    ~CntControl() { --remaining_count; }
  };
  std::unique_ptr<CntControl> controller;
};

std::atomic<int> MemoryLeakTestStruct::remaining_count;
std::atomic<int> MemoryLeakTestStruct::constructed_count;

using CQueue = fox_cq::ConcurrentQueue<MemoryLeakTestStruct>;
std::atomic<int> ncompleted{0};

void Producer(int id, int total_num, CQueue& queue) {
  for (int i = 0; i < RangePerProducer; i++) {
    queue.Push(MemoryLeakTestStruct{});
  }
  if (++ncompleted == total_num) {
    // last producer that compeleted producing should set the queue to finish
    // since no more production to push into queue.
    queue.SetFinish();
  }
}

void Consumer(int id, CQueue& queue) {
  while (true) {
    if (!queue.Pop()) {
      // return if no more production.
      return;
    }
  }
}

int main(){
  std::cout << " -----Example2 begin----" << std::endl;
  // multiple producer and multiple consumer are allowed
  CQueue q;
  const int NProducer = 5;
  const int NConsumer = 5;
  MemoryLeakTestStruct::remaining_count = 0;
  MemoryLeakTestStruct::constructed_count = 0;

  std::vector<std::thread> threads;
  for (int i = 0; i < NProducer; i++) {
    threads.emplace_back(&Producer, i, NProducer, std::ref(q));
  }
  for(int i=0;i< NConsumer;i++){
    threads.emplace_back(&Consumer, i, std::ref(q));
  }

  for(auto& thread : threads){
    thread.join();
  }

  std::cout << "Total constructed number of object: "
            << MemoryLeakTestStruct::constructed_count << "\n"
            << "Remaining number of object after consuming: "
            << MemoryLeakTestStruct::remaining_count << std::endl;
  if (MemoryLeakTestStruct::remaining_count == 0) {
    std::cout << "Passed" << std::endl;
  } else {
    std::cout << "Failed" << std::endl;
  }
  std::cout << " -----Example2 end----" << std::endl;

  return MemoryLeakTestStruct::remaining_count == 0 ? 0 : 1;
}
