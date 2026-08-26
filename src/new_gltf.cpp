#include "cgltf.h"
#include <assert.h>

namespace {

struct Scene {};

//struct Material {
//    glm::vec4 base_color = { 1, 1, 1, 1 };
//    f32 metallic = 1.0F;
//    f32 roughness = 1.0F;
//    glm::vec3 emissive = { 0, 0, 0 };
//};

} // namespace

/*
    MATERIALS
    ---------

    Material: How light interacts with the surface of an object.

    Phong and other simple shading models use a material model consisting of a small set of material properties:
      - diffuse color/texture: color of diffusely reflected light
      - specular color: color of specularly reflected light
      - shininess: controls the size and sharpness of specular highlights

    Physically Based Rendering (PBR) is a more realistic approach to shading based on the physical behavior of light
    and surfaces. The common Metallic-Roughness PBR material model describes a surface primarily using:
      - base color: intrinsic color of the surface
      - metallic: controls whether the surface behaves as a metal or dielectric
      - roughness: controls the sharpness or blurriness of reflections

    glTF 2.0 uses Metallic-Roughness PBR and provides:
      - from cgltf_material->pbr_metallic_roughness:
        - base color factor/texture: base surface color
        - metallic factor: degree of metallic behavior
        - roughness factor: degree of surface roughness
        - metallic-roughness texture: per-texel metallic and roughness values

    glTF 2.0 also provides the following material properties that can be used with PBR, but also other shading models:
      - from cgltf_material:
        - normal texture: per-texel surface normal variation
        - occlusion texture: per-texel ambient occlusion
        - emissive factor/texture: light emitted by the surface

    More advanced material features are often implemented on top of PBR as extensions (cgltf).

    Older/alternative material models are supported through extensions (cgltf).
*/

// FIXME: Remove asserts
static inline void process_glb_scene_node(cgltf_node *node) {
    assert(node); // invalid node provided

    // Node Name
    // ---------
    char *node_name = node->name;
    if (node_name) {
        // print name
    }

    // Node Transforms
    // ---------------
    // if present, may access transformation matrix or individual translation, rotation, scale transforms
    //node->has_matrix;
    //node->has_scale;
    //node->has_rotation;
    //node->has_translation;

    // Iterate Mesh Primitives
    // -----------------------
    if (node->mesh) {
        cgltf_mesh *mesh = node->mesh;
        for (cgltf_size prim_idx{}; prim_idx < mesh->primitives_count; prim_idx++) {
            cgltf_primitive prim = mesh->primitives[prim_idx];

            // Material
            // --------
            cgltf_material *mat = prim.material;
            assert(mat); // warn no material. not an error.

            //mat->normal_texture;
            //mat->occlusion_texture;
            //mat->emissive_texture;
            //mat->emissive_factor;
            //mat->has_emissive_strength;
            //mat->emissive_strength;

            // PBR Metallic-Roughness
            // ----------------------
            assert(!mat->has_pbr_metallic_roughness); // warn PBR Metallic-Roughness not specified and that PBR data is cgltf defaults

            cgltf_pbr_metallic_roughness pbr = mat->pbr_metallic_roughness;
            //pbr.base_color_factor;
            //pbr.base_color_texture;
            //pbr.metallic_factor;
            //pbr.roughness_factor;
            //pbr.metallic_roughness_texture;

            // Unpack Indices
            // --------------
            // if prim.indices == nullptr, the mesh is non-indexed: no indices; draw vertices directly with glDrawArrays instead of glDrawElements
            if (prim.indices) {
                cgltf_accessor *indices = prim.indices;

                //indices; // cgltf_accessor_read_... or cgltf_accessor_unpack_...
                //indices->normalized;
                //indices->count; // number of indices
                //indices->is_sparse; // ???
                //indices->offset;
                //indices->stride;
                //indices->type // scalar?
                //indices->component_type // cgltf_component_type_r_8, cgltf_component_type_r_16, etc
            }

            // Iterate Primitive Attributes
            // ----------------------------
            for (cgltf_size attr_idx{}; attr_idx < prim.attributes_count; attr_idx++) {
                cgltf_attribute attr = prim.attributes[attr_idx];
                switch (attr.type) {
                    // Vertex Attributes
                    // -----------------
                    case cgltf_attribute_type_position: {
                    } break;
                    case cgltf_attribute_type_normal: {
                    } break;
                    case cgltf_attribute_type_tangent: {
                    } break;
                    case cgltf_attribute_type_texcoord: {
                    } break;
                    case cgltf_attribute_type_color: {
                    } break;

                    // Skinning
                    // --------
                    case cgltf_attribute_type_joints: {
                    } break;
                    case cgltf_attribute_type_weights: {
                    } break;

                    // Others
                    // ------
                    case cgltf_attribute_type_custom: {
                    } break;

                    case cgltf_attribute_type_invalid: {
                    } break;

                    default: {
                    } break;
                }
            }

            // Primitive Topology
            // ------------------
            switch (prim.type) {
                case cgltf_primitive_type_points: {
                } break;
                case cgltf_primitive_type_lines: {
                } break;
                case cgltf_primitive_type_line_loop: {
                } break;
                case cgltf_primitive_type_line_strip: {
                } break;
                case cgltf_primitive_type_triangles: {
                } break;
                case cgltf_primitive_type_triangle_strip: {
                } break;
                case cgltf_primitive_type_triangle_fan: {
                } break;

                case cgltf_primitive_type_invalid: {
                } break;

                default: {
                } break;
            }
        }
    }
    // Ignore Presence of Camera
    // -------------------------
    else if (node->camera) {
        char *camera_name = node->camera->name;
        if (camera_name) {
            // print name
        }
        // Ignore camera
    }

    // Recurse Through Child Nodes
    // ---------------------------
    for (cgltf_size chnode_idx{}; chnode_idx < node->children_count; chnode_idx++) {
        cgltf_node *chnode = node->children[chnode_idx];
        process_glb_scene_node(chnode); // recursive call
    }
}

// FIXME: Remove asserts
static inline void process_glb_scene(cgltf_data *glb_data) {
    // Verification
    // ------------
    if (!glb_data) { return; }               // invalid glb_data
    if (!glb_data->scenes_count) { return; } // no scenes present
    if (!glb_data->scenes) { return; }       // no scenes present

    // Iterate Scenes
    // --------------
    for (cgltf_size scene_idx{}; scene_idx < glb_data->scenes_count; scene_idx++) {
        cgltf_scene scene = glb_data->scenes[scene_idx];
        char *scene_name = scene.name;
        if (scene_name) { /* print name */
        }

        // Iterate Nodes
        // -------------
        for (cgltf_size node_idx{}; node_idx < scene.nodes_count; node_idx++) {
            cgltf_node *node = scene.nodes[node_idx];
            process_glb_scene_node(node);
        }
    }
}

static inline void read_glb(const char *glb_path) {
    // GLB Configuration and Data
    // --------------------------
    cgltf_options glb_options{};
    cgltf_data *glb_data{};

    // Parse GLB File
    // --------------
    cgltf_result parse_result = cgltf_parse_file(&glb_options, glb_path, &glb_data);
    if (parse_result != cgltf_result_success) { return; } // failed to parse file

    // Load GLB Content
    // ----------------
    cgltf_result load_result = cgltf_load_buffers(&glb_options, glb_data, "assets/");
    if (load_result != cgltf_result_success) { return; } // failed to fully load referenced data

    // Verification
    // ------------
    if (!glb_data) { return; } // something strange went wrong, because this should have been covered above

    // Determine How to Process the GLB
    // --------------------------------
    // Process by scene
    if (glb_data->scenes_count) { process_glb_scene(glb_data); }

    // Clean Up
    // --------
    cgltf_free(glb_data);
}
