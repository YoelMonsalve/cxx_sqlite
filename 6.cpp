// dynamic_cast
#include <iostream>
#include <exception>
#include <vector>

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

int main() 
{
  f({1, 2, 3}); cout << endl;

  // variadic function, version 1
  f(1, 2, 3);
  cout << endl;

  int x = 2, y = 3;
  int &xp = x, &yp=y;

  vector<int> w{xp, y};
  for (auto &a: w) { cout << ": " << a; }
  cout << endl;
  x = 4;
  y = 5;
  for (auto &a: w) { cout << ": " << a; }
  cout << endl << endl;

  return 0;
}