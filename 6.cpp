// dynamic_cast
#include <iostream>
#include <exception>
using namespace std;

class Base { virtual void dummy() {} };
class Derived: public Base { int a; };

int main () {
  try {
    Base * pba = new Derived;
    Base * pbb = new Base;
    Derived * pd;

    pd = dynamic_cast<Derived*>(pba);
    if (pd==0) cout << "pba couldn't be converted to (Derived *).\n";

    pd = dynamic_cast<Derived*>(pbb);
    if (pd==0) cout << "pbb couldn't be converted to (Derived *).\n";

  } catch (exception& e) {cout << "Exception: " << e.what();}
  return 0;
}
