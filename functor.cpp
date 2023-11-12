#include <iostream>

using namespace std; 

template<class T>
struct functor
{
    void operator()(T x) { cout << "empty functor\n"; }
};

template<>
struct functor<int> 
{
    void operator()(int x) { cout << "functor<int>(" << x << ")\n"; }
};

template<>
struct functor<float> 
{
    void operator()(float x) { cout << "functor<float>(" << x << ")\n"; }
};


template<class T>
void fn(T x)
{
    functor<T>{}(x);
}

int main() {

    functor<int> fi{};
    functor<float> ff{};
    
    fi(3);
    functor<float>{}(4.5);

    cout << endl;

    fn(3);
    fn(4.5f);
    fn((char) 'A');
    fn(nullptr);

    return 0;
}