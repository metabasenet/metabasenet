### Compile / run unit test

The unit test has passed`./CMakeLists.txt`和`./test/CMakeLists.txt` integrated in metabasenet
in the project，target set to build all or only the specified test_big can build the unit test.

By loading `./test/test_big`to run metabasenet test manually. If the unit test file or the generation being tested
If the code file changes, just rebuild the unit test.

There are two ways to add more metabasenet unit tests. The first is to add more metabasenet unit tests to the existing ones in the `test/` directory Cpp file join `BOOST_AUTO_TEST_CASE` function; The other is to add new ones Cpp file, which implements the new `BOOST_AUTO_TEST_SUITE` section.

### Run separate unit tests

test_big has some built-in command-line parameters. For example, just run uint256_tests and output detailed information:

    test_big --log_level=all --run_test=uint256_tests

... Or just want to run uint256_tests methods subitem in tests:

    test_big --log_level=all --run_test=uint256_tests/methods

Run `test_big --help` You can get detailed help list information.

### Add description of unit test

The source code files in this directory are unit test cases. The test unit contains a boost framework,
Because metabasenet already uses boost, simply use this framework without additional configuration
Some other frameworks make sense, making it easier and faster to create unit tests.

The build system is set to compile an executable program named `test_big`, which runs all unit tests. The main source file name is `test_big.cpp`.  To add a new unit test file to our test suite, you need to add these file names to the list `./test/CMakeLists.txt`. We take a unit test file to include the test of a class or a source code file as a general principle. The naming rule of the file is `<source_filename>_tests.cpp`, these files encapsulate their tests in a file called `<source_filename>_tests` in the test suite `uint256_tests.cpp` can be used as a specific test unit example for reference.