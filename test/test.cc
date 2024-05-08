/**
 * @author Hanwen Zheng
 * @email eserinc.z@outlook.com
 * @create date 2024-05-08 09:12:24
 * @modify date 2024-05-08 09:12:24
 * @desc Some tests for concurrent queue.
 */
#include <atomic>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <thread>

#include "../concurrent_queue.h"
#define CATCH_CONFIG_MAIN
#include "../third_party/catch.hpp"

using namespace fox_cq;

int destrct_cnt;

struct MoveOnlyStruct {
  MoveOnlyStruct(int v) {
    if (v >= 0) {
      value = std::make_unique<int>(v);
    }
  };
  MoveOnlyStruct(const MoveOnlyStruct& other) = delete;
  MoveOnlyStruct(MoveOnlyStruct&& other) = default;
  MoveOnlyStruct& operator=(MoveOnlyStruct&& other) = default;
  ~MoveOnlyStruct() {
    if (value) {
      // std::cout << "MoveOnlyStruct#" << *value << " destructed.\n";
      ++destrct_cnt;
    }
  }
  std::unique_ptr<int> value;
};

TEST_CASE("std::unique_ptr in limited sized concurrent queue",
          "<std::unique_ptr, LimitedSize>") {
  using Unique = std::unique_ptr<int>;
  ConcurrentQueue<Unique, 5> c;
  c.Push(std::make_unique<int>(1));
  Unique v;
  c.Pop(v);
  REQUIRE(v);
  REQUIRE(*v == 1);
}

TEST_CASE(
    "MoveOnlyStruct in limited sized concurrent queue destruct at the right "
    "time",
    "<MoveOnlyStruct, LimitedSize>") {
  destrct_cnt = 0;
  ConcurrentQueue<MoveOnlyStruct, 5> c;
  c.Push(MoveOnlyStruct(0));
  REQUIRE(destrct_cnt == 0);
  {
    MoveOnlyStruct tmp(-1);
    c.Pop(tmp);
  }
  REQUIRE(destrct_cnt == 1);
  c.Push(MoveOnlyStruct(1));
  c.Push(MoveOnlyStruct(2));
  c.Push(MoveOnlyStruct(3));
  c.Push(MoveOnlyStruct(4));
  REQUIRE(destrct_cnt == 1);
  {
    MoveOnlyStruct tmp(-1);
    c.Pop(tmp);
  }
  REQUIRE(destrct_cnt == 2);
  {
    MoveOnlyStruct tmp(-1);
    c.Pop(tmp);
  }
  REQUIRE(destrct_cnt == 3);
  {
    MoveOnlyStruct tmp(-1);
    c.Pop(tmp);
  }
  REQUIRE(destrct_cnt == 4);
  {
    MoveOnlyStruct tmp(-1);
    c.Pop(tmp);
  }
  REQUIRE(destrct_cnt == 5);
}

TEST_CASE("std::unique_ptr in unlimited sized concurrent queue",
          "<std::unique_ptr, UnlimitedSize>") {
  using Unique = std::unique_ptr<int>;
  ConcurrentQueue<Unique> c;
  c.Push(std::make_unique<int>(1));
  Unique v;
  c.Pop(v);
  assert(v && *v == 1);
}

TEST_CASE(
    "MoveOnlyStruct in unlimited sized concurrent queue destruct at the right "
    "time",
    "<MoveOnlyStruct, UnlimitedSize>") {
  destrct_cnt = 0;
  ConcurrentQueue<MoveOnlyStruct> c;
  c.Push(MoveOnlyStruct(0));
  REQUIRE(destrct_cnt == 0);
  {
    MoveOnlyStruct tmp(-1);
    c.Pop(tmp);
  }
  REQUIRE(destrct_cnt == 1);
  c.Push(MoveOnlyStruct(1));
  c.Push(MoveOnlyStruct(2));
  c.Push(MoveOnlyStruct(3));
  c.Push(MoveOnlyStruct(4));
  REQUIRE(destrct_cnt == 1);
  {
    MoveOnlyStruct tmp(-1);
    c.Pop(tmp);
  }
  REQUIRE(destrct_cnt == 2);
  {
    MoveOnlyStruct tmp(-1);
    c.Pop(tmp);
  }
  REQUIRE(destrct_cnt == 3);
  {
    MoveOnlyStruct tmp(-1);
    c.Pop(tmp);
  }
  REQUIRE(destrct_cnt == 4);
  {
    MoveOnlyStruct tmp(-1);
    c.Pop(tmp);
  }
  REQUIRE(destrct_cnt == 5);
}

TEST_CASE("void typed limited sized concurrent queue", "<void, LimitedSize>") {
  ConcurrentQueue<void, 5> q;
  q.Push();
  REQUIRE(q.Pop() == 1);
  q.Push();
  q.Push();
  q.Push();
  q.Push();
  REQUIRE(q.Pop() == 1);
  REQUIRE(q.Pop() == 1);
  REQUIRE(q.Pop() == 1);
  REQUIRE(q.Pop() == 1);
}

TEST_CASE("void typed unlimited sized concurrent queue",
          "<void, UnlimitedSize>") {
  ConcurrentQueue<void> q;
  q.Push();
  assert(q.Pop() == 1);
  q.Push();
  q.Push();
  q.Push();
  q.Push();
  REQUIRE(q.Pop() == 1);
  REQUIRE(q.Pop() == 1);
  REQUIRE(q.Pop() == 1);
  REQUIRE(q.Pop() == 1);
}

TEST_CASE("Basic parallel test for limited size concurrent queue",
          "<int, LimitedSize>[Parallel]") {
  const int size = 1000;
  srand(time(nullptr));
  std::vector<int> buf;
  for (int i = 0; i < size; i++) {
    buf.push_back(i);
  }

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(buf.begin(), buf.end(), g);

  ConcurrentQueue<int, size> q;
  const int nthreadput = 3;
  const int nthreadget = 2;

  std::vector<std::thread> threads;

  std::atomic<int> completed_put{0};
  for (int i = 0; i < nthreadput; i++) {
    int l = i * size / nthreadput;
    int r = (i + 1) * size / nthreadput;
    threads.emplace_back(
        [&](int l, int r) {
          for (int i = l; i < r; i++) {
            q.Push(buf[i]);
          }
          if (++completed_put == nthreadput) {
            q.SetFinish();
          }
        },
        l, r);
  }
  std::vector<std::vector<int>> collections(nthreadget);
  for (int i = 0; i < nthreadget; i++) {
    threads.emplace_back(
        [&](int id) {
          int x;
          while (true) {
            bool suc = q.Pop(x);
            if (suc) {
              collections[id].push_back(x);
            } else {
              break;
            }
          }
        },
        i);
  }
  for (auto& thread : threads) {
    thread.join();
  }
  std::multiset<int> in(buf.begin(), buf.end());
  std::multiset<int> out;
  for (int i = 0; i < nthreadget; i++) {
    out.insert(collections[i].begin(), collections[i].end());
  }
  REQUIRE(in == out);
}

TEST_CASE("Basic parallel test for unlimited size concurrent queue",
          "<int, UnlimitedSize>[Parallel]") {
  const int size = 1000;
  srand(time(nullptr));
  std::vector<int> buf;
  for (int i = 0; i < size; i++) {
    buf.push_back(i);
  }

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(buf.begin(), buf.end(), g);

  ConcurrentQueue<int> q;
  const int nthreadput = 3;
  const int nthreadget = 2;

  std::vector<std::thread> threads;

  std::atomic<int> completed_put{0};
  for (int i = 0; i < nthreadput; i++) {
    int l = i * size / nthreadput;
    int r = (i + 1) * size / nthreadput;
    threads.emplace_back(
        [&](int l, int r) {
          for (int i = l; i < r; i++) {
            q.Push(buf[i]);
          }
          if (++completed_put == nthreadput) {
            q.SetFinish();
          }
        },
        l, r);
  }
  std::vector<std::vector<int>> collections(nthreadget);
  for (int i = 0; i < nthreadget; i++) {
    threads.emplace_back(
        [&](int id) {
          int x;
          while (true) {
            bool suc = q.Pop(x);
            if (suc) {
              collections[id].push_back(x);
            } else {
              break;
            }
          }
        },
        i);
  }
  for (auto& thread : threads) {
    thread.join();
  }
  std::multiset<int> in(buf.begin(), buf.end());
  std::multiset<int> out;
  for (int i = 0; i < nthreadget; i++) {
    out.insert(collections[i].begin(), collections[i].end());
  }
  REQUIRE(in == out);
}

TEST_CASE("Basic parallel test for void typed unlimited size concurrent queue",
          "<void, UnlimitedSize>[Parallel]") {
  const int size = 1000;
  srand(time(nullptr));

  ConcurrentQueue<void> q;
  const int nthreadput = 3;
  const int nthreadget = 2;

  std::vector<std::thread> threads;

  std::atomic<int> completed_put{0};
  for (int i = 0; i < nthreadput; i++) {
    int l = i * size / nthreadput;
    int r = (i + 1) * size / nthreadput;
    threads.emplace_back(
        [&](int l, int r) {
          for (int i = l; i < r; i++) {
            q.Push();
          }
          if (++completed_put == nthreadput) {
            q.SetFinish();
          }
        },
        l, r);
  }
  std::vector<int> collections(nthreadget);
  for (int i = 0; i < nthreadget; i++) {
    threads.emplace_back(
        [&](int id) {
          while (true) {
            auto suc = q.Pop();
            if (suc) {
              ++collections[id];
            } else {
              break;
            }
          }
        },
        i);
  }
  for (auto& thread : threads) {
    thread.join();
  }
  int in = size;
  int out = 0;
  for (int i = 0; i < nthreadget; i++) {
    out += collections[i];
  }
  REQUIRE(in == out);
}

TEST_CASE("Medium parallel test for limited size concurrent queue",
          "<int, LimitedSize>[Parallel]") {
  const int size = 100000;
  srand(time(nullptr));
  std::vector<int> buf;
  for (int i = 0; i < size; i++) {
    buf.push_back(i);
  }

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(buf.begin(), buf.end(), g);

  ConcurrentQueue<int, 20> q;
  const int nthreadput = 10;
  const int nthreadget = 10;

  std::vector<std::thread> threads;

  std::atomic<int> completed_put{0};
  for (int i = 0; i < nthreadput; i++) {
    int l = i * size / nthreadput;
    int r = (i + 1) * size / nthreadput;
    threads.emplace_back(
        [&](int l, int r) {
          for (int i = l; i < r; i++) {
            q.Push(buf[i]);
          }
          if (++completed_put == nthreadput) {
            q.SetFinish();
          }
        },
        l, r);
  }
  std::vector<std::vector<int>> collections(nthreadget);
  for (int i = 0; i < nthreadget; i++) {
    threads.emplace_back(
        [&](int id) {
          int x;
          while (true) {
            bool suc = q.Pop(x);
            if (suc) {
              collections[id].push_back(x);
            } else {
              break;
            }
          }
        },
        i);
  }
  for (auto& thread : threads) {
    thread.join();
  }
  std::multiset<int> in(buf.begin(), buf.end());
  std::multiset<int> out;
  for (int i = 0; i < nthreadget; i++) {
    out.insert(collections[i].begin(), collections[i].end());
  }
  REQUIRE(in == out);
}

TEST_CASE("Medium parallel test for unlimited size concurrent queue",
          "<int, UnlimitedSize>[Parallel]") {
  const int size = 100000;
  srand(time(nullptr));
  std::vector<int> buf;
  for (int i = 0; i < size; i++) {
    buf.push_back(i);
  }

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(buf.begin(), buf.end(), g);
 
  ConcurrentQueue<int> q;
  const int nthreadput = 10;
  const int nthreadget = 10;

  std::vector<std::thread> threads;

  std::atomic<int> completed_put{0};
  for (int i = 0; i < nthreadput; i++) {
    int l = i * size / nthreadput;
    int r = (i + 1) * size / nthreadput;
    threads.emplace_back(
        [&](int l, int r) {
          for (int i = l; i < r; i++) {
            q.Push(buf[i]);
          }
          if (++completed_put == nthreadput) {
            q.SetFinish();
          }
        },
        l, r);
  }
  std::vector<std::vector<int>> collections(nthreadget);
  for (int i = 0; i < nthreadget; i++) {
    threads.emplace_back(
        [&](int id) {
          int x;
          while (true) {
            bool suc = q.Pop(x);
            if (suc) {
              collections[id].push_back(x);
            } else {
              break;
            }
          }
        },
        i);
  }
  for (auto& thread : threads) {
    thread.join();
  }
  std::multiset<int> in(buf.begin(), buf.end());
  std::multiset<int> out;
  for (int i = 0; i < nthreadget; i++) {
    out.insert(collections[i].begin(), collections[i].end());
  }
  REQUIRE(in == out);
}
