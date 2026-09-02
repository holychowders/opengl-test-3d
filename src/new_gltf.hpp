#pragma once

#include "cgltf.h"
#include "hc_types.h"

/*
    cgltf Data
    __________

    glTF file -> scenes -> root nodes -> child nodes -> mesh -> primitives

    Multiple nodes can reference the same mesh for instancing

    Scene: A collection of root nodes defining one renderable scene/world
      -> Node: An object in the scene hierarchy with a transform and optional children, mesh, camera, etc
        -> Mesh: A collection of primitives representing one logical piece of geometry/object
          -> Primitive: One piece of drawable geometry (a submesh): vertex attributes + optional indices + material + topology
*/

struct Transform {
    f32 scale[3]{ 1, 1, 1 };
    f32 rotation[4]{ 0, 0, 0, 1 };
    f32 translation[3]{};
};

enum VertexAttributeTypes : u8 { VAPosition = 1 << 0, VANormal = 1 << 1, VATangent = 1 << 2, VATexcoord = 1 << 3, VAColor = 1 << 4 };
typedef u8 VertexFormat;

struct VertexAttributes {
    union {
        struct {
            f32 position[3];
            f32 normal[3];
            f32 tangent[3];
            f32 texcoord[2];
            f32 color[4];
        };
        f32 e[15];
    };
};

struct Texture {};

struct Material {
    // PBR Metallic-Roughness
    f32 base_color_factor[4]{ 1, 1, 1, 1 };
    f32 metallic_factor{ 1 };
    f32 roughness_factor{ 1 };

    // PBR Textures
    Texture *base_color_texture{};
    Texture *metallic_roughness_texture{};

    // Additional
    Texture *normal_texture{};
    Texture *occlusion_texture{};

    Texture *emissive_texture{};
    f32 emissive_factor{ 1 };

    bool has_emissive_strength{};
    f32 emissive_strength{ 1 };
};

struct Mesh {
    u32 *indices{};
    f32* vertices{};
    Material *material{};

    //VertexAttributes vertex_attributes{};
};

struct AssetSceneNode {
    Transform transform{};
    Mesh *meshes{};
};

/// Root node
struct AssetScene {
    AssetSceneNode *nodes{};
};

struct Asset {
    AssetScene *scenes{};
    AssetSceneNode *nodes{};
    Mesh *meshes{};
};

void read_and_process_gltf_file(const char *gltf_path, Asset *out_asset);

const char *cgltf_result_to_str(cgltf_result result);
