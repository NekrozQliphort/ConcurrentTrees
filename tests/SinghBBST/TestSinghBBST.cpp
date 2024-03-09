#include "catch.hpp"
#include "src/SinghBBST/SinghBBST.h"

TEST_CASE("Singh BBST Sanity Check") {
  SinghBBST<int> tree;
  for (int i = 0; i < 100; i++)
    REQUIRE(!tree[i]);
  for (int i = 0; i < 100; i++)
    REQUIRE(tree.insert(i));
  for (int i = 0; i < 100; i++)
    REQUIRE(tree[i]);
}