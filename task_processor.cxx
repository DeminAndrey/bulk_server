#include <iostream>

#include <stdexcept>

#include <boost/asio/io_context.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/thread.hpp>

namespace detail {

template <typename T>
struct task_wrapped {

private:
  T task_unwrapped_;

public:
  explicit task_wrapped(const T &f)
    : task_unwrapped_(f) {}

  void operator()() const {
    try {
      boost::this_thread::interruption_point();
    }
    catch (const boost::thread_interrupted &) {
    }

    try {
      task_unwrapped_();
    }
    catch (const std::exception &ex) {
      std::cerr << ex.what() << std::endl;
    }
    catch (const boost::thread_interrupted &) {
      std::cerr << "Interrupted!" << std::endl;
    }
    catch (...) {
      std::cerr << "Unknown!" << std::endl;
    }
  }
};


template<typename T>
task_wrapped<T> make_task_wrapped(const T &task_unrapped) {
  return task_wrapped<T>(task_unrapped);
}
} // namespace detail


class task_processor : private boost::noncopyable {

protected:
  static boost::asio::io_context &get_ioc() {
    static boost::asio::io_context ioc;
    static boost::asio::io_context::work work(ioc);

    return ioc;
  }

public:
  template<typename T>
  static void push_task(const T &task_unwrapped) {
    get_ioc().post(detail::task_wrapped(task_unwrapped));
  }

  static void start() {
    get_ioc().run();
  }

  static void stop() {
    get_ioc().stop();
  }

};

int func_test() {
  static int count = 0;
  ++count;

  std::cout << "thread_id: " << std::this_thread::get_id() << std::endl;

  boost::this_thread::interruption_point();

  switch (count) {
  case 3:
    throw std::logic_error("");
  case 10:
    throw boost::thread_interrupted();
  case 90:
    task_processor::stop();
  }

  return count;
}


int main() {
  for(size_t i = 0; i < 100; ++i) {
    task_processor::push_task(&func_test);
  }

  assert(func_test() == 1);

  int sum = 0;
  task_processor::push_task(
        [&sum]() {
    sum = 2 + 2;
  });

  assert(sum == 0);

  task_processor::start();
  //assert(func_test() == 91);

  return 0;
}
