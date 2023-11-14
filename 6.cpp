// dynamic_cast
#include <iostream>
#include <exception>
#include <stdexcept>
#include <vector>

#include "./sqlite_exception.hpp"
#include "./throwable.hpp"

using namespace std;

void f(vector<int> v) {
  cout << "vectorized version" << endl;
  for (auto &x: v) { cout << "x: " << x << endl; }
}

/*
void f(int x);    // forward declaration
void f();
template <class... Ts>
void f(int x, Ts... args)    // recursive case
{
  f(x);
  f(args...);
}
void f(int x) {    // base case
  cout << "x: " << x << endl;  
}
void f() {}    // base final case
*/

template<class T>
void f(T x);    // forward declaration
void f();
template<class T, class... Ts>
void f(T x, Ts... args)
{
  cout << "template parameter version, recursive" << endl;
  f(x);
  f(args...);
}
template<class T>
void f(T x) 
{
  cout << "template parameter version, single" << endl;
  cout << "x: " << x << endl;
}
void f() {}

int g(int a, int b) { return 0; }

using cxx_sqlite3::call_throwable;
using cxx_sqlite3::make_throwable;

int main() 
{
  /* make a rc-based function throwable */
  int rc_ok = 0;
  int rc;

  struct myexception: public std::runtime_error
  {
      myexception(const char *msg) : std::runtime_error(msg) {}
  };


  throw myexception("runtime");
  
  // Method 1
  /*cout << "Method 1: throw wrapper\n";
  rc = call_throwable(g, 0, std::runtime_error("function failed"), 2, 3);
  cout << "rc: " << rc << endl;
  rc = call_throwable(g, 1, std::runtime_error("function failed"), 2, 3);
  cout << "rc: " << rc << endl;
  exit(0);*/

  // Method 2
  /*cout << "Method 2: Macros\n";
  TRY_OR_THROW__ (g, (2, 3), rc, 0, std::runtime_error("TRY_OR_EXCEPT__: function failed"))
  cout << "rc: " << rc << endl;
  TRY_OR_THROW__ (g, (2, 3), rc, 1, std::runtime_error("TRY_OR_EXCEPT__: function failed"))
  cout << "rc: " << rc << endl;
  exit(0);*/
  

  // Method 3
  cout << "Method 3: throw-functor\n";
  std::runtime_error e("function failed");
  make_throwable<int, int, int> g_throw_expects_0(g, 0, e);
  make_throwable<int, int, int> g_throw_expects_1(g, 1, e);
  rc = g_throw_expects_0(2, 3);
  cout << "rc: " << rc << endl;
  rc = g_throw_expects_1(2, 3);
  cout << "rc: " << rc << endl;
  exit(0);

  f({1, 2, 3}); cout << endl;

  // variadic function, version 1
  f(1, 2, 3);
  cout << endl;

  
  return 0;
}