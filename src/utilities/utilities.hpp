#ifndef UTILITIES_HPP_INCLUDED
#define UTILITIES_HPP_INCLUDED
#include <chrono>
#include <vector>
#include <vector>
#include <iostream>
#include <assert.h>
#include <random>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <thread>
#include <mutex>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "useful_macros.hpp"
#define MIN_TICK_TIME 0.012 //min time it takes to measure time in microsecs, couldn't observe anything below 14 ns

double normal_lnp(double x, double mean, double variance);

namespace boat {
    extern std::mt19937 generator;

template <int... Is>
struct seq {};

template <int N, int... Is>
struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

template <int... Is>
struct gen_seq<0, Is...> : seq<Is...> {};

class Timer {
 public:
  Timer() : start_(std::chrono::high_resolution_clock::now()) {}

  void stop() { end_ = std::chrono::high_resolution_clock::now(); }

  double time_interval_nanoseconds() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end_ - start_)
        .count();
  }
  double time_interval_microseconds() {
    return time_interval_nanoseconds() / 1000.0;
  }
  double time_interval_miliseconds() {
    return time_interval_nanoseconds() / 1000000.0;
  }
  double time_interval_seconds() {
    return time_interval_nanoseconds() / 1000000000.0;
  }

  double time_since_start_nanoseconds() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::high_resolution_clock::now() - start_).count();
  }
  double time_since_start_microseconds() {
    return time_since_start_nanoseconds() / 1000.0;
  }
  double time_since_start_miliseconds() {
    return time_since_start_nanoseconds() / 1000000.0;
  }
  double time_since_start_seconds() {
    return time_since_start_nanoseconds() / 1000000000.0;
  }

 private:
  std::chrono::high_resolution_clock::time_point start_;
  std::chrono::high_resolution_clock::time_point end_;
};

template <class T>
double time_func(T func) {
  Timer t;
  func();
  return t.time_since_start_microseconds();
}
template <class T>
double time_func_seconds(T func) {
  Timer t;
  func();
  return t.time_since_start_seconds();
}

template <class T>
void print_vec(const std::vector<T>& v) {
  for (auto e : v) {
    std::cout << e << ", ";
  }
  std::cout << std::endl;
}

template <class T, class F>
double vector_average(const std::vector<T>& in, F func) {
  double sum = 0.0;
  for (const auto& e : in) {
    sum += func(e);
  }
  return sum / in.size();
}


extern std::string python_lib;

double os_time_backwards_sample(double time);
double average_vec(const std::vector<double>& v);
double stdev_vec(const std::vector<double>& v);
void add_indent(std::string& code, int indent);
std::string exec(std::string instruction, bool flush_stderr = true, bool printing = false);
std::string python_exec(std::string python_command, bool flush_stderr = true);
double weighted_average(double left, double right, double proportion_right);
std::string python_exec_cmd(std::string python_command);
std::string get_time(const std::string& format);
void os_time_init();
void os_time_store();
void os_time_load();

class AsyncExec {
public:
    AsyncExec() = delete;
    AsyncExec(const AsyncExec&) = delete;
    AsyncExec(AsyncExec&&) = delete;

    AsyncExec(const std::string& instruction,
              const std::string& machine = "",
              const std::string& password = "") {
      //Should use a mutex for static_id_
      terminated_ = false;
      {
        std::lock_guard<std::mutex> lock(mtx_);
        id_ = "sbo_server" + std::to_string(next_id_);
        next_id_++;
      }
      machine_ = machine;
      password_ = password;
      std::string cmd = "screen -S " + id_ + " -dm bash -c 'source ~/.bashrc; " +
                        instruction + "; exec sh'";

      add_ssh(cmd, machine_, password_);
      exec(cmd);
    }

    ~AsyncExec() {
      if(!terminated_) {
        terminate();
      }
    }

    void terminate() {
      assert(!terminated_);
      std::string cmd = "screen -X -S " + id_ + " quit";
      add_ssh(cmd, machine_, password_);
      exec(cmd);
      terminated_ = true;
    }

    static void terminate_all(const std::string& machine, const std::string& password = "") {
      std::string cmd = "screen -ls | grep sbo_server* | cut -d. -f1 | awk '{print $1}' | xargs kill";
      add_ssh(cmd, machine, password);
      exec(cmd);
    }


    static void add_ssh(std::string& cmd, const std::string& machine, const std::string& password) {
      if (machine != "") {
        //Running remotely
        cmd = "ssh -o \"StrictHostKeyChecking no\" " + machine +
              " \" " + cmd + " \" ";
        add_password(cmd, password);
      }
    }

    static void add_password(std::string& cmd, const std::string& password) {
      if(password != "" ){
        cmd = "sshpass -p \"" + password + "\" " + cmd;
      }
    }

private:


    std::string id_;
    bool terminated_;
    std::string machine_;
    std::string password_;
    static int next_id_;
    static std::mutex mtx_;
};
template <class IndividualResult>
struct ResultVector;

template <>
struct ResultVector<double> {
    // Has a utility function and hence useful for aggregating results
    ResultVector(){}
    ResultVector(std::vector<double> individual_results) : individual_results_(std::move(individual_results)){}
    double utility() {
      return average_vec(individual_results_);
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& individual_results_;
    }

    std::vector<double> individual_results_;
};

template <class IndividualResult>
struct ResultVector {
    // Has a utility function and hence useful for aggregating results
    ResultVector(){}
    ResultVector(std::vector<IndividualResult> individual_results) : individual_results_(std::move(individual_results)){}
    double utility() {
      double sum = 0.0;
      for (auto& r : individual_results_) {
        sum += r.utility();
      }
      sum = sum / individual_results_.size();
      return sum;
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& individual_results_;
    }

    std::vector<IndividualResult> individual_results_;
};

template<class T>
void boost_store(const T& val, const std::string& filename) {
  std::ofstream ofs (filename);
  boost::archive::text_oarchive oa(ofs);
  oa << val;
}

template<class T>
void boost_load(T& val, const std::string& filename) {
  std::ifstream ifs (filename);
  boost::archive::text_iarchive ia(ifs);
  ia >> val;
}

template <class T, class Func>
std::vector<T> filter(const std::vector<T>& input, Func filter){
  std::vector<T> res;
  for(const auto& e: input){
    if(filter(e)){
      res.push_back(e);
    }
  }
  return res;
}

template <class T>
std::vector<T> concat(const std::vector<T>& a, const std::vector<T>& b) {
  std::vector<T> res;
  res.reserve(a.size() + b.size());
  res.insert(res.end(), a.begin(), a.end());
  res.insert(res.end(), b.begin(), b.end());
  return res;
}

//  Func double->double is a monotonic function from 0 to +inf
template <class Func>
double continous_binary_search(Func f, double target, double initial_guess = 1.0, double precision = 1e-6) {
  double x = initial_guess;
  double y = f(x);
  bool going_up = y < target;
  bool found_interval = false;
  double next = x; //Setting next to avoid warnings
  while(!found_interval) {
    double next = going_up ? x*2.0 : x/2.0;
    double y_next = f(next);
    found_interval = going_up ^ !(y_next > target);
    x = found_interval ? x : next;
  }

  double low = std::min(x, next);
  double high = std::max(x, next);
  double mid = (low + high) / 2.0;
  while((high-low)/mid > precision){
    double y_mid = f(mid);
    if(y_mid > target) {
      high = mid;
    } else {
      low = mid;
    }
    mid = (low + high) / 2.0;
  }
  return mid;
}

} // end boat namespace

#endif  // UTILITIES_HPP_INCLUDED
