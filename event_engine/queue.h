#ifndef CDDD_EVENT_ENGINE_QUEUE_H__
#define CDDD_EVENT_ENGINE_QUEUE_H__

#include <queue>

namespace cddd {
namespace event_engine {

enum class queue_op_status {
   success,
   full,
   busy,
};

template<class T, class QueueType=std::queue<T>>
class queue {
public:
   typedef typename QueueType::value_type value_type;
   typedef typename QueueType::reference reference;
   typedef typename QueueType::const_reference const_reference;
   typedef typename QueueType::size_type size_type;
   typedef QueueType container_type;

   explicit queue(const QueueType &que) :
      q(que),
      lock()
   {
   }

   explicit queue(QueueType &&que=QueueType()) :
      q(std::move(que)),
      lock()
   {
   }

   inline bool empty() const {
      lock_type l(lock);
      return empty(l);
   }

   inline size_type size() const {
      lock_type l(lock);
      return size(l);
   }

   inline void push(T &&x) {
      lock_type l(lock);
      push(std::forward<T>(x), l);
   }

   inline T value_pop() {
      lock_type l(lock);
      return value_pop(l);
   }

private:
   typedef std::unique_lock<std::mutex> lock_type;

   QueueType q;
   std::mutex lock;

   inline bool empty(lock_type &) const {
      return q.empty();
   }

   inline size_type size(lock_type &) const {
      return q.size();
   }

   inline void push(const_reference x, lock_type &) {
      q.push(x);
   }

   inline void push(T &&x, lock_type &) {
      q.emplace(std::move(x));
   }

   inline T value_pop(lock_type &) {
      T result = q.front();
      q.pop();
      return std::move(result);
   }
};

}
}

#endif
