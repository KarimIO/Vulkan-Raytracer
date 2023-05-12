#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// TINYGLTF has been modified so that GetComponentSizeInBytes and
// GetNumComponentsInType are not static, to remove an error that
// is probably related to module support.
#include <tiny_gltf.h>
