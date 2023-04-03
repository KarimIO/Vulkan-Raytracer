# Raytracer Test

A test project using Vulkan to build a compute-shader-based raytracer.

## How to Build

### Libraries and APIs used

 * [GLFW](https://www.glfw.org/)
 * [GLM](https://github.com/g-truc/glm)
 * [STB Image](https://github.com/nothings/stb/blob/master/stb_image.h)
 * [Vulkan](https://vulkan.lunarg.com/)

### Build Shaders

Run the following commands with `...` replaced with relevant paths and `x.x.x.x` replaced with relevant version:
```
...\VulkanSDK/x.x.x.x/Bin/glslc.exe -fshader-stage=vertex ParticleSystem.vert.glsl -o vert.spv
...\VulkanSDK/x.x.x.x/Bin/glslc.exe -fshader-stage=fragment ParticleSystem.frag.glsl -o frag.spv
...\VulkanSDK/x.x.x.x/Bin/glslc.exe -fshader-stage=compute ParticleSystem.comp.glsl -o comp.spv
```

### Build Project
 1. Download the libraries specified above. 
 2. Create a project that uses c++20 for modules.
 3. Add `Libraries/bin` folder, GLFW installation's relevant `lib` folder, and Vulkan installation's `Lib` folder to library directories.
 4. Add `vulkan-1.lib;glfw3.lib` to libraries.
 5. Add `Libraries/include`, GLFW installation's `include` folder, GLM's `glm` directory, and `stb_image.h` your Vulkan installation's `Include` directory to include directories.
 6. Add `GLFW_INCLUDE_VULKAN` to preprocessor definitions.
 7. Build and run the project

## Special Thanks

Thanks to `https://vulkan-tutorial.com` which provides the base of this Vulkan framework.
