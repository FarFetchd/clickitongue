#ifndef CLICKITONGUE_BLOCKING_QUEUE_H_
#define CLICKITONGUE_BLOCKING_QUEUE_H_

#include <atomic>
#include <condition_variable>
#include <optional>
#include <queue>

// Allows, thread-safely, one or more producers to enqueue() items, while a
// consumer consumes any available items with deque(). If the queue is empty,
// deque() blocks until something is enqueued, then returns that new thing.

template<class T>
class BlockingQueue
{
public:
  void enqueue(T obj)
  {
    {
      const std::lock_guard<std::mutex> lock(mu_);
      q_.push(obj);
    }
    pokes_.poke();
  }
  // returns nullopt if the queue has shut down and the consumer should go away.
  std::optional<T> deque()
  {
    pokes_.consumePoke();
    const std::lock_guard<std::mutex> lock(mu_);
    if (q_.empty())
      return std::nullopt;
    T obj = q_.front();
    q_.pop();
    return obj;
  }
  void shutdown()
  {
    pokes_.poke();
  }

private:
  uint64_t last_action_msse_ = 0; // guarded by mu_
  std::queue<T> q_; // guarded by mu_
  std::mutex mu_;

  // Allows an enqueuer to queue up event notifications, and a dequeuer to block
  // until there is one or more notifications enqueued.
  class PokeQueue
  {
  public:
    PokeQueue() : pokes_outstanding_(0) {}

    void poke()
    {
      pokes_outstanding_++;
      cv_.notify_all();
    }
    void consumePoke()
    {
      std::unique_lock<std::mutex> lock(mu_);
      cv_.wait(lock, [&] { return pokes_outstanding_.load() > 0; });
      pokes_outstanding_--;
    }

  private:
    std::condition_variable cv_;
    std::mutex mu_;
    std::atomic<int> pokes_outstanding_;
  };
  PokeQueue pokes_;
};

#endif // CLICKITONGUE_BLOCKING_QUEUE_H_
