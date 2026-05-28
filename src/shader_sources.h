#pragma once

namespace shader_sources {

static const char *vs_src = R"glsl(
    #version 330 core
    layout(location=0) in vec3 a_xyz;
    layout(location=1) in vec2 a_uv;
    layout(location=2) in vec4 a_rgba;
    uniform mat4 u_mvp;
    out vec2 v_uv;
    out vec4 v_rgba;
    void main() {
        gl_Position = u_mvp * vec4(a_xyz, 1);
        v_uv = a_uv;
        v_rgba = a_rgba;
    }
)glsl";
static const char *fs_src = R"glsl(
    #version 330 core
    in vec2 v_uv;
    in vec4 v_rgba;
    out vec4 color;
    void main() {
        color = v_rgba;
    }
)glsl";

}
