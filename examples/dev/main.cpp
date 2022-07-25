//
//  main.cpp
//  u
//
//  Created by Alexandre Arsenault on 2022-06-29.
//

#include <iostream>
#include "nano/nano.h"
//#include "nano/graphic.h"

struct hh {
  int X, Y, Width, Height;
};

struct fhh {
  int left, top, bottom, right;
};
//
// class my_component : public nano::extension_base<my_component> {
// public:
//   my_component(int v) : extension_base(*this), value(v) {}
//     virtual ~my_component() override
//     {
//
//     }
//
//   int value;
// };
//
// class my_component2 : public nano::extension_base<my_component2> {
// public:
//  my_component2(int v) : extension_base(this), value(v) {}
//    virtual ~my_component2() override = default;
//  int value;
//};
//
// class my_component3 : public nano::extension_base<my_component3> {
// public:
//    my_component3(int v) : extension_base(*this), value(v) {}
//    virtual ~my_component3() override = default;
//  int value;
//};

int main(int argc, const char* argv[]) {

  nano::point pt0 = { 0, 1 };
  nano::size st0 = { 0, 1 };
  st0 *= 2;
  std::cout << pt0 << " " << st0 << std::endl;
  nano::color c0 = 0xFF0000FF;
  nano::color c2 = nano::color::from_argb(0xFF00FF00);
  float rjk[4] = { 0, 1, 0, 1 };
  //    const float* rrrrrr = &rjk[0];

  //    nano::color c1 = nano::color(rrrrrr, 4);

  //    nano::color c1 = nano::color(rjk);

  //    nano::color c1 = nano::color(nano::memory_range(rjk));
  //
  ////    nano::color c1 = nano::color(rjk);
  //    std::cout << c0 << " " << c1 << " " << c2 << std::endl;
  //
  //    nano::color::float_rgba<double> frgb = c0.f_rgba<double>();
  //    nano::color::float_rgba<float> frgbd = c0.f_rgba();
  //    float fffff = c0.red<float>();
  //
  //    nano::extension_list clist;
  //
  //    clist.add<my_component>(45);
  //    clist.add<my_component2>(7);
  //
  //    std::cout << frgb.r << "-- " << clist.get<my_component>()->value << std::endl;
  //    std::cout << "-- " << clist.get<my_component2>()->value << std::endl;
  //
  //    nano::point<int> ot = {8, 9};
  //
  //    int gg = 42;
  //    long klklk = gg;
  //    gg = klklk;
  //
  // insert code here...
  hh h{ 0, 9, 9, 9 };
  nano::rect<int> r = { 0, 0, 1, 2 };
  nano::rect<double> r2 = { 0, 0, 1, 2.0 };
  nano::rect<float> rr2 = h;
  fhh ccc = { 3, 4, 5, 6 };
  nano::rect<float> rr2e = ccc;
  std::cout << "rr2e " << rr2e << std::endl;

  std::cout << std::clamp(0.5, 1.0, 2.0) << " " << r << std::endl;
  std::cout << std::clamp(0.5, 1.0, 2.0) << " " << r2 << std::endl;
  r2 = h;
  std::cout << std::clamp(0.5, 1.0, 2.0) << " " << r2 << std::endl;

  r2 = r;
  std::cout << std::clamp(0.5, 1.0, 2.0) << " " << r2 << std::endl;

  nano::rect<float> r3 = { 0.0, 0.0, 1.0, 2.3 };
  r2 = r3;

  nano::rect<float> r4 = r3;

  //    std::swap(r, r2);
  r.swap(r2);
  std::swap(rr2, rr2e);
  //
  //
  //    std::cout << (r4 == r2) << " " << typeid(decltype(r2.x)).name() << std::endl;
  //
  //
  //    r4.size = r2.size;
  //
  //    auto fct = nano::finally([]() {
  //        std::cout << "Bingo" << std::endl;
  //    });
  //
  //    r2 = {{1, 2}, {3, 4}};
  //    h = static_cast<hh>(r2);
  //    hh j = r2.convert<hh>();
  //    nano::rect<float> k = j;
  //    std::cout << k << std::endl;
  //
  //    k.position = ot;
  //    k.size = {7.0, 6.07};
  //    std::cout << k << std::endl;
  //
  //    if(nano::assign(r2, r)) {
  //
  //    }
  //
  //    std::string_view sv = "alex";
  //    std::cout << sv << std::endl;
  //    bool aaaaa = sv == "bingo";
  //    std::cout << aaaaa << std::endl;
  //
  //    nano::object::lifetime lt;
  //    {
  //        nano::object obj;
  //        lt = obj.get_lifetime();
  //        std::cout << "lt " << lt.is_valid() << std::endl;
  //    }
  //    std::cout << "lt " << lt.is_valid() << std::endl;

  return 0;
}
