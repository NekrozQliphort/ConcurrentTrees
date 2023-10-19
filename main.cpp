#include <iostream>
#include "src/NatarajanBST/NatarajanBST.h"

int main() {
  NatarajanBST<int> t{};

  std::cout << t.insert(0) << std::endl;
  std::cout << t.insert(1) << std::endl;
  std::cout << t.insert(2) << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << t[0] << std::endl;
  std::cout << t[1] << std::endl;
  std::cout << t[2] << std::endl;
  std::cout << t[3] << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << t.remove(0) << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << t[0] << std::endl;
  std::cout << t[1] << std::endl;
  std::cout << t[2] << std::endl;
  std::cout << t[3] << std::endl;
}
