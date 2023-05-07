# Building without CMake

If you do not want to, or cannot use CMake, use the following guide.

## Windows with Visual Studio 2022+

 1. Download the libraries specified above. 
 2. Create a C++ Project with the following compiler flags:
    * `General > C++ Language Standard`: `/std:c++latest`
    * `C/C++ > Code Generation > Enable C++ Exceptions`: `/EHsc`
    * `C/C++ > Code Generation > Runtime Library`: `/MD`
    * `C/C++ > Code Generation > Floating Point Model`: Make this empty
    * `C/C++ > Language > Enable Experimental C++ Standard Library Modules`: `/experimental:module`
 3. Add the following paths to additional library directories:
    * GLFW installation's relevant `lib` folder
    * Vulkan installation's `Lib` folder
 4. Add the following dependencies:
    *  `vulkan-1.lib`
    *  `glfw3.lib`
 5. Add the following paths to additional include directories:
    * `libraries/glfw/include`
    * `libraries/glm`
    * `libraries/stb`
    * Your Vulkan installation's `Include` directory
 6. Add `GLFW_INCLUDE_VULKAN` to preprocessor definitions.
 7.  Build and run the project

## Other Systems

 * [GCC](https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Modules.html)
 * [Clang](https://blog.ecosta.dev/en/tech/cpp-modules-with-clang)
