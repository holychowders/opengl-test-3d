#pragma once

namespace shader_sources {

static const char *vs_xyz_uv_rgba = R"glsl(
    #version 330 core

    layout(location=0) in vec3 a_xyz;
    layout(location=1) in vec2 a_uv;
    layout(location=2) in vec4 a_rgba;

    out vec2 v_uv;
    out vec4 v_rgba;

    uniform mat4 u_mvp;

    void main() {
        gl_Position = u_mvp * vec4(a_xyz, 1);
        v_uv = a_uv;
        v_rgba = a_rgba;
    }
)glsl";
static const char *fs_xyz_uv_rgba = R"glsl(
    #version 330 core

    in vec2 v_uv;
    in vec4 v_rgba;

    out vec4 color;

    void main() {
        color = v_rgba;
    }
)glsl";

#if 0
static const char *vs_xyz_n_uv = R"glsl(
    #version 330 core

    layout(location=0) in vec3 a_pxyz; // pos
    layout(location=1) in vec3 a_nxyz; // norm
    layout(location=2) in vec2 a_uv;

    out vec3 v_nxyz;
    out vec2 v_uv;

    uniform mat4 u_mvp;

    void main() {
        gl_Position = u_mvp * vec4(a_pxyz, 1);
        v_nxyz = a_nxyz;
        v_uv = a_uv;
    }
)glsl";
static const char *fs_xyz_n_uv = R"glsl(
    #version 330 core

    in vec2 v_uv;
    in vec3 v_nxyz;

    out vec4 color;

    uniform sampler2D u_texunit;

    void main() {
        color = texture(u_texunit, v_uv);
    }
)glsl";
#endif

static const char *vs_xyz_n_uv = R"glsl(
    #version 330 core

    layout(location=0) in vec3 a_pxyz; // pos
    layout(location=1) in vec3 a_nxyz; // norm
    layout(location=2) in vec2 a_uv;

    out vec3 v_nxyz;
    out vec2 v_uv;

    uniform mat4 u_mvp;

    void main() {
        gl_Position = u_mvp * vec4(a_pxyz, 1);
        v_nxyz = a_nxyz;
        v_uv = a_uv;
    }
)glsl";
static const char *fs_xyz_n_uv = R"glsl(
    #version 330 core

    in vec2 v_uv;
    in vec3 v_nxyz;

    out vec4 color;

    //layout(binding=1) uniform sampler2D u_texunit;
    uniform sampler2D u_texunit;
    uniform vec4 u_base_color_factor;

    void main() {
        //color = texture(u_texunit, v_uv) * u_base_color_factor;
        color = texture(u_texunit, v_uv);
        //color = vec4(v_uv, 0, 1);
    }
)glsl";

}
