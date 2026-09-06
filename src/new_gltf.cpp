#define CGLTF_IMPLEMENTATION
#include "new_gltf.hpp"

#include "hc_assert.h"
#include "hc_log.hpp"
#include "hc_types.h"

#include <string.h> // memcpy

////////////////////////////////////////////////////////////////////////// Section: Structures

namespace {} // namespace

////////////////////////////////////////////////////////////////////////// Section: cgltf Structure Helpers

const char *cgltf_result_to_str(cgltf_result result) {
    switch (result) {
        case cgltf_result_success: return "Success";
        case cgltf_result_data_too_short: return "Data too short";
        case cgltf_result_unknown_format: return "Unknown format";
        case cgltf_result_invalid_json: return "Invalid JSON";
        case cgltf_result_invalid_gltf: return "Invalid glTF";
        case cgltf_result_invalid_options: return "Invalid options";
        case cgltf_result_file_not_found: return "File not found";
        case cgltf_result_io_error: return "I/O error";
        case cgltf_result_out_of_memory: return "Out of memory";
        case cgltf_result_legacy_gltf: return "Legacy glTF";
        default: return "Unknown error";
    }
}

////////////////////////////////////////////////////////////////////////// Section: cgltf File Loading

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

static inline Texture process_gltf_pbr_texture(cgltf_texture_view texture_view) {
    Texture res_tex{};
    if (texture_view.texture) {
        // Texture View
        // ------------
        if (texture_view.has_transform) {
            texture_view.transform.has_texcoord;
            texture_view.transform.texcoord;

            texture_view.transform.offset;
            texture_view.transform.rotation;
            texture_view.transform.scale;
        }

        // Texture
        // -------
        cgltf_texture *texture = texture_view.texture;
        const char *texture_name = texture->name;
        cgltf_image *texture_image = texture->image;
        if (texture_image) {
            //texture_image->buffer_view;
            //texture_image->name;
        }
    }
    else {}
    return res_tex;
}

static inline Material process_cgltf_material(cgltf_material material) {
    Material res{};

    //material.normal_texture->texture;
    //material.occlusion_texture->texture;
    //material.emissive_texture->texture;
    //material.emissive_factor;
    //material.has_emissive_strength;
    //material.emissive_strength.emissive_strength;

    // PBR Metallic-Roughness
    // ----------------------
    if (material.has_pbr_metallic_roughness) {
        cgltf_pbr_metallic_roughness pbr = material.pbr_metallic_roughness;

        memcpy(res.base_color_factor, pbr.base_color_factor, sizeof(res.base_color_factor));
        res.metallic_factor = pbr.metallic_factor;
        res.roughness_factor = pbr.roughness_factor;

        *res.base_color_texture = process_gltf_pbr_texture(pbr.base_color_texture);
        *res.metallic_roughness_texture = process_gltf_pbr_texture(pbr.metallic_roughness_texture);
    }
    else { cont(4, "...mesh specifies no PBR Metallic-Roughness -- use defaults"); }

    return res;
}

static inline void process_gltf_mesh_primitives(cgltf_mesh mesh) {
    // Iterate Mesh Primitives
    //   Note: There is up to one material per primitive
    //   Note: `primitive` references the geometry and material needed for a single draw call.
    //         `primitive.attributes` (required) provides an accessor to positions, normals, UVs, etc.
    //         `primitive.material` (optional) references up to one material to be used for the primitive.
    // -----------------------
    cont(3, "...processing node mesh");
    for (cgltf_size prim_idx{}; prim_idx < mesh.primitives_count; prim_idx++) {
        cgltf_primitive prim = mesh.primitives[prim_idx];

        // Material
        // --------
        cgltf_material *mat = prim.material;
        if (mat) { process_cgltf_material(*mat); }
        else { cont(4, "...mesh primitive has no material"); }

        // Unpack Indices
        // --------------
        if (prim.indices) {
            cgltf_accessor *indices = prim.indices;
            // TODO: Add this as a flexible array member on a struct so we don't just malloc this
            u32 *res_indices = (u32 *)malloc(indices->count * sizeof(*res_indices));
            cgltf_size indices_unpacked = cgltf_accessor_unpack_indices(indices, res_indices, sizeof(u32), indices->count);
        }
        else { cont(4, "...mesh primitive is non-indexed"); }

        // Iterate Primitive Attributes
        // ----------------------------
        VertexAttributes *res_vertex = (VertexAttributes *)malloc(sizeof(VertexAttributes));
        for (cgltf_size attr_idx{}; attr_idx < prim.attributes_count; attr_idx++) {
            cgltf_attribute attr = prim.attributes[attr_idx];
            switch (attr.type) {
                // Vertex Attributes
                // -----------------
                case cgltf_attribute_type_position: {
                    f32 *res_positions = (f32 *)malloc(attr.data->count * sizeof(f32));
                    cgltf_accessor_unpack_floats(attr.data, res_positions, attr.data->count);

                } break;
                case cgltf_attribute_type_normal: {
                    f32 *res_normals = (f32 *)malloc(attr.data->count * sizeof(f32));
                    cgltf_accessor_unpack_floats(attr.data, res_normals, attr.data->count);
                } break;
                case cgltf_attribute_type_tangent: {
                    f32 *res_tangents = (f32 *)malloc(attr.data->count * sizeof(f32));
                    cgltf_accessor_unpack_floats(attr.data, res_tangents, attr.data->count);
                } break;
                case cgltf_attribute_type_texcoord: {
                    f32 *res_texcoords = (f32 *)malloc(attr.data->count * sizeof(f32));
                    cgltf_accessor_unpack_floats(attr.data, res_texcoords, attr.data->count);
                } break;
                case cgltf_attribute_type_color: {
                    f32 *res_colors = (f32 *)malloc(attr.data->count * sizeof(f32));
                    cgltf_accessor_unpack_floats(attr.data, res_colors, attr.data->count);
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
        //out_asset->vertices; // interleave into single f32 vbuffer

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

// FIXME: Remove asserts
static inline void process_gltf_scene_node(cgltf_node *node) {
    ASSERT(node); // invalid node provided

    cont(2, "...processing scene node");

    // Node Name
    // ---------
    char *node_name = node->name;
    if (node_name) {
        // print name
    }

    // Node Transforms
    // ---------------
    Transform res_node_xform{};
    if (node->has_scale) { memcpy(res_node_xform.scale, node->scale, sizeof(res_node_xform.scale)); }
    if (node->has_rotation) { memcpy(res_node_xform.rotation, node->rotation, sizeof(res_node_xform.rotation)); }
    if (node->has_translation) { memcpy(res_node_xform.translation, node->translation, sizeof(res_node_xform.translation)); }
    //node->has_matrix;

    if (node->mesh) { process_gltf_mesh_primitives(*node->mesh); }
    else if (node->camera) { /* Ignore camera */
    }

    // Recurse Through Child Nodes
    // ---------------------------
    for (cgltf_size chnode_idx{}; chnode_idx < node->children_count; chnode_idx++) {
        cgltf_node *chnode = node->children[chnode_idx];
        process_gltf_scene_node(chnode); // recursive call
    }
}

// FIXME: Remove asserts
static inline void process_gltf_scene(cgltf_data *gltf_data, AssetScene *out_scenes) {
    // Verification
    // ------------
    if (!gltf_data) { return; }               // invalid gltf_data
    if (!gltf_data->scenes_count) { return; } // no scenes present
    if (!gltf_data->scenes) { return; }       // no scenes present

    // Iterate Scenes
    // --------------
    for (cgltf_size scene_idx{}; scene_idx < gltf_data->scenes_count; scene_idx++) {
        cgltf_scene scene = gltf_data->scenes[scene_idx];
        char *scene_name = scene.name;
        if (scene_name) {
            char msg[128]{};
            snprintf(msg, sizeof(msg), "Processing glTF scene: %s", scene_name);
            info(msg);
        }
        else { info("Processing glTF scene"); }

        // Iterate Nodes
        // -------------
        for (cgltf_size node_idx{}; node_idx < scene.nodes_count; node_idx++) {
            cgltf_node *node = scene.nodes[node_idx];
            cont(1, "...about to process a parent node");
            process_gltf_scene_node(node);
        }
    }
}

// TODO: Remove asserts
cgltf_data *read_gltf_file(const char *gltf_path) {
    finfo(nullptr, "Reading glTF file (cgltf): %s", gltf_path);

    // glTF Configuration and Data
    // ---------------------------
    cgltf_options gltf_options{};
    cgltf_data *gltf_data{};

    // Parse glTF File
    // ---------------
    cgltf_result parse_result = cgltf_parse_file(&gltf_options, gltf_path, &gltf_data);
    if (parse_result == cgltf_result_success) { cont("...successfully parsed file"); }
    else {
        char emsg[256]{};
        const char *cgltf_emsg = cgltf_result_to_str(parse_result);
        snprintf(emsg, sizeof(emsg), "Failed to parse glTF file (cgltf_parse_file)\n      File: %s\n      Reason: %s", gltf_path, cgltf_emsg);
        error(emsg);
        return gltf_data;
    }

    // Load glTF Content
    // -----------------
    cgltf_result load_result = cgltf_load_buffers(&gltf_options, gltf_data, "assets/");
    if (load_result == cgltf_result_success) { cont("...successfully loaded buffers"); }
    else {
        char emsg[256]{};
        const char *cgltf_emsg = cgltf_result_to_str(parse_result);
        snprintf(emsg, sizeof(emsg), "Failed to load buffers (cgltf_load_buffers)\n      File: %s\n      Reason: %s", gltf_path, cgltf_emsg);
        error(emsg);
        return gltf_data;
    }

    // Verification
    // ------------
    if (gltf_data) { cont("...finished reading glTF file"); }
    else {
        error("glTF data is empty even though it was parsed and loaded successfully");
        return gltf_data;
    }

    return gltf_data;
}

void read_and_process_gltf_file(const char *gltf_path, Asset *out_asset) {
    // Verification and Initialization
    // -------------------------------
    if (!out_asset) { return; }
    *out_asset = Asset{};

    // Read glTF
    // ---------
    cgltf_data *gltf_data = read_gltf_file(gltf_path);

#if 0
    // Determine How to Process the glTF
    // ---------------------------------
    // Process by scene
    if (gltf_data->scenes_count) { process_gltf_scene(gltf_data, out_asset->scenes); }
    // Process by meshes and materials separately
    else {
        for (cgltf_size material_idx{}; material_idx < gltf_data->materials_count; material_idx++) {
            cgltf_material material = gltf_data->materials[material_idx];
        }
        for (cgltf_size mesh_idx{}; mesh_idx < gltf_data->materials_count; mesh_idx++) {
            cgltf_mesh mesh = gltf_data->meshes[mesh_idx];
            mesh.primitives->material;
        }
    }
#else
    for (cgltf_size material_idx{}; material_idx < gltf_data->materials_count; material_idx++) {
        cgltf_material material = gltf_data->materials[material_idx];
    }
    for (cgltf_size mesh_idx{}; mesh_idx < gltf_data->materials_count; mesh_idx++) {
        cgltf_mesh mesh = gltf_data->meshes[mesh_idx];
        process_gltf_mesh_primitives(mesh);
    }
#endif

    // Iterate and find parent-less nodes and store them as scenes.
    // Must beware of parent-less nodes that weren't meant to be drawn for whatever reason (misc/unfinished/unorganized geometry).

    //SubmeshRootNode
    //Submesh
    //SubmeshParent

    // A glTF scene is just a collection of the root nodes the author intended you to render.
    // If a scene doesn't exist, you can still iterate all parent-less nodes to find root nodes.

    // Nodes may, however, exist outside of a scene.
    // These dangling nodes may represent misc geometry that the author didn't care to organize into a scene for whatever reason.

    // Clean Up
    // --------
    cgltf_free(gltf_data);
}
