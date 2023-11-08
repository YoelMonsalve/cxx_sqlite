#include <type_traits>
#include <iostream>
#include <string>
#include <tuple>

class A
{
    public:
    void foo(int) { std::cout << "A::foo(int)\n"; };
    void foo(float) { std::cout << "A::foo(float)\n"; };
};

class B : public A
{
    public:
    void foo(std::string) { std::cout << "B::foo(string)\n"; };
    using A::foo;
    using value_type = int;

    B() { foo(10.5f); }
};

template<class T, T v>
struct integral_constant2
{
    static constexpr T value = v;
    using value_type = T;
    using type = integral_constant2; // using injected-class-name
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; } // since c++14
};

/*template <class T>
void test(T value)
{
    int a[std::tuple_size_v<T>]; // can be used at compile time
 
    std::cout << std::tuple_size<T>{} << ' ' // or at run time
              << sizeof a << ' ' 
              << sizeof value << '\n';
}*/

template <class Ts>
size_t get_size(Ts t) {
    //std::cout << t << std::endl;
    return std::tuple_size<decltype(t)>::value;
}

/** variadic function, variable number of arguments */
#if 1   
/** forward reference*/
template <class T>
void print_argv(T last);

template <class T, class... Ts>
void print_argv(T first, Ts... argv) 
{
    std::cout << "print_argv: " << first << std::endl;
    // invoke recursively
    print_argv<Ts...>(argv...);
}
// base class
template <class T>
void print_argv(T last)
{
    std::cout << "print_argv (last argument): " << last << std::endl;
}
/*void print_argv()
{
    std::cout << "print_argv (no arguments left): " << std::endl;
}*/
#endif

template <int I, class... Ts>
void print_tuple(std::tuple<Ts...> t) 
{
    std::cout << "tuple(" << I << "): " << std::get<I>(t) << std::endl;
    if constexpr (I > 0) { //< std::tuple_size<Ts...>::value) {
        print_tuple<I-1, Ts...>(t);
    }
}

int main()
{
    /*B b;
    b.foo(12); // won't work!
    B::value_type x = 3;
    decltype(x) y = 4;
    std::cout << "x: " << x << std::endl;*/
    
    typedef integral_constant2<int, 2> two_t;
    /*
    two_t K2;
    two_t::value_type K3 = 3;
    std::cout << "value of two_t(): " << two_t::value << std::endl;
    std::cout << "constant K2: " << (size_t)K2 << std::endl;
    std::cout << "constant K3: " << K3 << std::endl;
    */

    // heterogeneous tuple construction
    auto T = std::make_tuple(10, "Test", 3.14, 1.5f);
    print_tuple<3>(T);
    //print_tuple<1>(T);

    //print_argv(1, 'a', "foo", 1.34);
    exit(0);

    std::tuple<int, char, float> t{3, 'a', 1.5};
    std::tuple<int, char, float, double> t2{3, 'a', 1.5f, 1.5};
    typedef decltype(t) mytype;
    std::cout << "sizeof t:" << std::tuple_size<std::tuple<int, char, float, int>>{} << std::endl;
    std::cout << "sizeof t:" << std::tuple_size<decltype(t)>{} << std::endl;
    std::cout << "sizeof t:" << get_size(t2) << std::endl;

    exit(0);

    typedef std::integral_constant<int, 4> four_t;
 
    static_assert(not std::is_same<two_t, four_t>::value,
                  "two_t and four_t are equal!");
    std::cout << "four_t: " << four_t() << std::endl;
 
    static_assert(two_t::value * 2 == four_t::value, "2*2 != 4");
 
    enum class my_e { e1, e2 };
 
    typedef std::integral_constant<my_e, my_e::e1> my_e_e1;
    typedef std::integral_constant<my_e, my_e::e2> my_e_e2;
 
    static_assert(my_e_e1() == my_e::e1);
 
    static_assert(my_e_e1::value != my_e::e2,
                 "my_e_e1::value == my_e::e2");
 
    static_assert(std::is_same<my_e_e2, my_e_e2>::value,
                  "my_e_e2 != my_e_e2");
}