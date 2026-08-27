#pragma once

#include "cgltf.h"

void read_and_process_gltf_file(const char *gltf_path);

cgltf_data *read_gltf_file(const char *gltf_path);

void process_gltf_scene(cgltf_data *gltf_data);
void process_gltf_scene_node(cgltf_node *node);

const char *cgltf_result_to_str(cgltf_result result);
