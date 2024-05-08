/**
 * @author Hanwen Zheng
 * @email eserinc.z@outlook.com
 * @create date 2024-05-08 09:12:50
 * @modify date 2024-05-08 09:12:50
 * @desc A simple example showing the concurrent queue how to use.
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
using CQueue = fox_cq::ConcurrentQueue<std::unique_ptr<int>>;
std::atomic<int> ncompleted{0};

void Producer(int id, int total_num, CQueue& queue) {
  for (int i = 0; i < RangePerProducer; i++) {
    queue.Push(std::make_unique<int>(i + id * RangePerProducer));
  }
  if (++ncompleted == total_num) {
    // last producer that compeleted producing should set the queue to finish
    // since no more production to push into queue.
    queue.SetFinish();
  }
}

void Consumer(int id, std::vector<std::vector<int>>& collections,
              CQueue& queue) {
  while (true) {
    std::unique_ptr<int> result;
    if (!queue.Pop(result)) {
      // return if no more production.
      return;
    }
    collections[id].push_back(*result);
  }
}

int main(){
  // multiple producer and multiple consumer are allowed
  CQueue q;
  const int NProducer = 5;
  const int NConsumer = 5;
  std::vector<std::vector<int>> collections(NConsumer);
  std::vector<std::thread> threads;
  for (int i = 0; i < NProducer; i++) {
    threads.emplace_back(&Producer, i, NProducer, std::ref(q));
  }
  for(int i=0;i< NConsumer;i++){
    threads.emplace_back(&Consumer, i, std::ref(collections), std::ref(q));
  }

  for(auto& thread : threads){
    thread.join();
  }

  std::vector<int> in;
  for(int i=0;i<NProducer* RangePerProducer;i++){
    in.push_back(i);
  }

  std::vector<int> out;
  for (int i = 0; i < NConsumer; i++) {
    out.insert(out.end(), collections[i].begin(), collections[i].end());
  }
  std::sort(out.begin(),out.end());

  bool suc = (in == out);

  std::cout << "Consumers can catch all the production from producer? "
            << (suc ? "Yes" : "No") << std::endl;

  return 0;
}
