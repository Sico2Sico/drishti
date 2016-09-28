/*!
  @file   test-drishti.cpp
  @author David Hirvonen
  @brief  Google test for public drishti API.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <gtest/gtest.h>

extern const char* modelFilename;
extern const char* imageFilename;
extern const char* truthFilename;
extern bool isTextArchive;

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    assert(argc >= 4);
    modelFilename = argv[1];
    imageFilename = argv[2];
    truthFilename = argv[3];
    isTextArchive = (argc > 4) ? (std::atoi(argv[4]) > 0) : false;
    return RUN_ALL_TESTS();
}