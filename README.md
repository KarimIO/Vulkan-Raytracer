# Raytracer Test

A raytracer built with Vulkan and C++20 modules that supports realtime compute-shader-based raytracing with multi-bounce and multi-ray per pixel. We only support spheres for the time being.

You can control the camera using WASD / Arrow keys.

![A demo image](Demo.jpg "A demo image")

## Roadmap

 * Progressive render accumulation to reduce noise
 * Load models from glTF files
 * Depth of Field
 * Textures
   * Texture Infrastructure
   * Albedo Texture
   * Normal Texture
   * Metallic Texture
   * Roughness Texture
 * Importance Sampling
 * Other methods of reducing noise

## How to Build

### Libraries and APIs used

 * [GLFW](https://www.glfw.org/)
 * [GLM](https://github.com/g-truc/glm)
 * [STB Image](https://github.com/nothings/stb/blob/master/stb_image.h)
 * [Vulkan](https://vulkan.lunarg.com/)

### Setting Up C++ For Modules

I decided to use this as an opportunity to practice with C++20 modules as well. They are still experimental in every toolchain, but they're a lot simpler to work with. Intellisense does not play nicely with it though.

To use modules with Visual Studio, use versions 2019 or 2022, I recommend the latter. Make sure to install the "Desktop Development with C++" workload, and add "C++ Modules for v143 build tools (x64/x86 - experimental)" in the individual components section.
Continue to set up the project with [this tutorial.](https://learn.microsoft.com/en-us/cpp/cpp/modules-cpp?view=msvc-170)

For other systems, you can use the following links. Note that I have not built with other toolchains so I can't verify how they work:
 * [GCC](https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Modules.html)
 * [Clang](https://blog.ecosta.dev/en/tech/cpp-modules-with-clang)

### Build Shaders

Run the following commands with `...` replaced with relevant paths and `x.x.x.x` replaced with relevant version:
```
.../VulkanSDK/x.x.x.x/Bin/glslc.exe -fshader-stage=vertex Fullscreen.vert.glsl -o Fullscreen.vert.spv
.../VulkanSDK/x.x.x.x/Bin/glslc.exe -fshader-stage=fragment Fullscreen.frag.glsl -o Fullscreen.frag.spv
.../VulkanSDK/x.x.x.x/Bin/glslc.exe -fshader-stage=compute Raytracing.comp.glsl -o Raytracing.comp.spv
```

### Build Project
 1. Download the libraries specified above. 
 2. Create a project that uses c++20 for modules.
 3. Add GLFW installation's relevant `lib` folder, and Vulkan installation's `Lib` folder to library directories.
 4. Add `vulkan-1.lib;glfw3.lib` to libraries.
 5. Add GLFW installation's `include` folder, GLM's `glm` directory, a directory including `stb_image.h`, and your Vulkan installation's `Include` directory to include directories.
 6. Add `MAX_FRAMES_IN_FLIGHT=2;GLFW_INCLUDE_VULKAN` to preprocessor definitions.
 7. Build and run the project

## References and Thanks

### PBR and Raytracing
 * [LearnOpengl.com's PBR Theory Article](https://learnopengl.com/PBR/Theory) by Joey de Vries
 * [Ray Tracing in One Weekend](https://raytracing.github.io/) by Peter Shirley
 * [Physically Based Rendering: From Theory To Implementation](https://pbr-book.org/) by Matt Pharr, Wenzel Jakob, and Greg Humphreys
 * [Sebastian Lague's YouTube Video](https://www.youtube.com/watch?v=Qz0KTGYJtUk) which provides an excellent summary over the concept of Raytracing

### Vulkan
 * [Vulkan-Tutorial.com](https://vulkan-tutorial.com) by Alexander Overvoorde and Sascha Willems
 * [This Vulkan Github Repository](https://github.com/SaschaWillems/Vulkan) by Sascha Willems
