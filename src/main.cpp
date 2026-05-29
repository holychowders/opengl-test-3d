#include "hc_types.h"
#include "shader_sources.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
//#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>

////////////////////////////////////////////////////////////////////////// Section: Constants

#define WINDOW_WIDTH 2560.0F
#define WINDOW_HEIGHT 1440.0F

////////////////////////////////////////////////////////////////////////// Section: Macros

#define gl(gl_operation)                                                                                                                             \
    clear_gl_errors();                                                                                                                               \
    gl_operation;                                                                                                                                    \
    assert(!check_gl_errors() && #gl_operation)

//#define STR_BOOL(value) ((value) ? "true" : "false")

////////////////////////////////////////////////////////////////////////// Section: Data Structures

namespace { // Anonymous namespace to prevent ODR violations and improve LTO (in theory)

#if 0
// TODO: Replace GLM with custom data structures and operations
struct V3F32 {
    union {
        struct { f32 x, y, z; };
        f32 e[3];
    };
};
M4F32 translate(M4F32 mat, V3F32& tvec);
#endif

struct Mesh {
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    GLsizei index_count;
};

struct Transform {
    glm::vec3 pos;
    glm::vec3 scale;
    glm::vec3 ori;
    glm::vec3 angvel;
};

struct MeshRenderData {
    Mesh &mesh;
    f32 *vb;
    u32 vb_size;
    Transform &transform;
};

struct FrameContext {
    GLFWwindow *window;
    double &dt_s;
    //ImGuiIO &imgui_io;
    GLuint shader_program;
    GLint uloc_u_mvp;
    glm::mat4 view_matrix;
    glm::mat4 proj_matrix;
};

}

////////////////////////////////////////////////////////////////////////// Section: Logging and Error Checking

static inline void info(const char *regarding, const char *description) {
    if (regarding) { fprintf(stdout, "INFO [%s]: %s\n", regarding, description); }
    else { fprintf(stdout, "INFO: %s\n", description); }
    fflush(stdout);
}
static inline void info_stderr(const char *regarding, const char *description) {
    if (regarding) { fprintf(stderr, "INFO [%s]: %s\n", regarding, description); }
    else { fprintf(stderr, "INFO: %s\n", description); }
}
static inline void warn(const char *regarding, const char *description) {
    if (regarding) { fprintf(stderr, "WARN [%s]: %s\n", regarding, description); }
    else { fprintf(stderr, "WARN: %s\n", description); }
}
static inline void error(const char *regarding, const char *description) {
    if (regarding) { fprintf(stderr, "FAIL [%s]: %s\n", regarding, description); }
    else { fprintf(stderr, "FAIL: %s\n", description); }
}
static inline void error(const char *regarding, const char *description, const char *fpath, int lineno) {
    fprintf(stderr, "FAIL [%s:%d] [%s]: %s\n", fpath, lineno, regarding, description);
}
static inline void error(const char *description, const char *fpath, int lineno) {
    fprintf(stderr, "FAIL [%s:%d]: %s\n", fpath, lineno, description);
}

//FAIL [main.cpp:420] [GLEW]: There was an error
//FAIL [main.cpp:420 | GLEW]: There was an error

static void info(const char *description) {
    info(nullptr, description);
}
static void info_stderr(const char *description) {
    info_stderr(nullptr, description);
}
static void warn(const char *description) {
    warn(nullptr, description);
}
static void error(const char *description) {
    error(nullptr, description);
}

static void clear_gl_errors() {
    while (glGetError() != GL_NO_ERROR) {}
}

static bool check_gl_errors() {
    bool has_error = false;
    GLenum gl_error = {};
    while ((gl_error = glGetError()) != GL_NO_ERROR) {
        has_error = true;
        const char *message = "";
        switch (gl_error) {
            case GL_INVALID_ENUM: message = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: message = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: message = "GL_INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: message = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_OUT_OF_MEMORY: message = "GL_OUT_OF_MEMORY"; break;
            case GL_STACK_UNDERFLOW: message = "GL_STACK_UNDERFLOW"; break;
            case GL_STACK_OVERFLOW: message = "GL_STACK_OVERFLOW"; break;
            default: {
                char fmsg[128];
                snprintf(fmsg, sizeof(fmsg), "Unknown error: 0x%x", gl_error);
                message = fmsg;
            } break;
        }
        error("OpenGL Operation", message);
    }
    return has_error;
}

////////////////////////////////////////////////////////////////////////// Section: GLEW

static bool glew_init() {
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        error("GLEW", (const char *)glewGetErrorString(err));
        return false;
    }
    info("GLEW", (const char *)glewGetString(GLEW_VERSION));
    return true;
}

////////////////////////////////////////////////////////////////////////// Section: GLFW

static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) { // NOLINT(misc-unused-parameters)
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window, true); }
    }
}

static void glfw_error_callback(int error_code, const char *description) {
    char re[256];
    int bytes = snprintf(re, sizeof(re), "GLFW: code %d", error_code);
    assert(bytes > 0 && bytes < (int)sizeof(re));
    error(re, description);
}

static GLFWwindow *glfw_init(int window_width, int window_height, const char *window_title) {
    // Init
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) { return nullptr; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    // Create window
    GLFWwindow *window = glfwCreateWindow(window_width, window_height, window_title, nullptr, nullptr);
    if (window) {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // vsync
        glfwSetKeyCallback(window, glfw_key_callback);
    }
    else {
        error("GLFW", "Failed to initialize\n");
        glfwTerminate();
    }

    return window;
}

static void glfw_update(GLFWwindow *window) {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

////////////////////////////////////////////////////////////////////////// Section: Misc

static void clear_bg(f32 r, f32 g, f32 b, f32 a) {
    gl(glClearColor(r, g, b, a));
    gl(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

static void update_vertex_buffer(GLuint vbo, f32 *vb, size_t vb_size) {
    gl(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    gl(glBufferSubData(GL_ARRAY_BUFFER, 0, vb_size, vb));
}

static glm::mat4 calculate_mvp(const Transform &transform, const glm::mat4 &view, const glm::mat4 &projection) {
    glm::mat4 model = glm::mat4(1.0F);
    model = glm::translate(model, transform.pos);

    model = glm::rotate(model, transform.ori.x, glm::vec3(1.0F, 0.F, 0.0F));
    model = glm::rotate(model, transform.ori.y, glm::vec3(0.0F, 1.F, 0.0F));
    model = glm::rotate(model, transform.ori.z, glm::vec3(0.0F, 0.F, 1.0F));

    model = glm::scale(model, transform.scale);

    return projection * view * model;
}

////////////////////////////////////////////////////////////////////////// Section: Mesh

static bool mesh_bind(Mesh &mesh) {
    if (mesh.vao && mesh.vbo && mesh.ibo) {
        gl(glBindVertexArray(mesh.vao));
        gl(glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo));
        gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo));
        return true;
    }
    else {
        char regarding[256];
        char message[256];
        snprintf(regarding, sizeof(regarding), "%s:%d", __FILE__, __LINE__);
        snprintf(message, sizeof(message), "Failed to bind mesh (vao:%u, vbo:%u, ibo:%u)", mesh.vao, mesh.vbo, mesh.ibo);
        error(regarding, message);
        return false;
    }
}

static void mesh_draw(FrameContext &fctx, MeshRenderData &rd) {
    glm::mat4 u_mvp = calculate_mvp(rd.transform, fctx.view_matrix, fctx.proj_matrix);
    gl(glUniformMatrix4fv(fctx.uloc_u_mvp, 1, GL_FALSE, &u_mvp[0][0]));
    mesh_bind(rd.mesh);
    gl(glDrawElements(GL_TRIANGLES, rd.mesh.index_count, GL_UNSIGNED_INT, nullptr));
}

static Mesh mesh_create(f32 *vb, u32 *ib, size_t vb_size, size_t ib_size) {
    GLuint vao{};
    gl(glGenVertexArrays(1, &vao));
    gl(glBindVertexArray(vao));

    GLuint vbo{};
    gl(glGenBuffers(1, &vbo));
    gl(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    gl(glBufferData(GL_ARRAY_BUFFER, vb_size, vb, GL_STATIC_DRAW));

    // VBO attributes: xyz, uv, rgba
    size_t stride = 9;
    gl(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)0));                     // NOLINT(modernize-use-nullptr)
    gl(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)))); // NOLINT(performance-no-int-to-ptr)
    gl(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)(5 * sizeof(GLfloat)))); // NOLINT(performance-no-int-to-ptr)

    gl(glEnableVertexAttribArray(0));
    gl(glEnableVertexAttribArray(1));
    gl(glEnableVertexAttribArray(2));

    GLuint ibo{};
    gl(glGenBuffers(1, &ibo));
    gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    gl(glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib_size, ib, GL_STATIC_DRAW)); // init index buffer data

    // Clean Up (Unbind)
    gl(glBindVertexArray(0)); // unbind this global VAO (only one VAO is active at a time)

    Mesh mesh{ vao, vbo, ibo, 0 };
    size_t index_count = ib_size / sizeof(ib[0]);
    assert(index_count <= (size_t)INT_MAX);
    mesh.index_count = (GLsizei)index_count;

    return mesh;
}

////////////////////////////////////////////////////////////////////////// Section: Shaders

//#define strfmt(str, fmt) ({ \
//                           \
//})

//static char* strfmt(

static bool shader_bind(GLuint prg) {
    if (!prg) {
        error("Failed to bind shader program (null shader program provided)", __FILE__, __LINE__);
        return false;
    }
    gl(glUseProgram(prg));
    return true;
}

static void shader_unbind() {
    gl(glUseProgram(0));
}

static GLint shader_get_uniform_location(GLuint shader_program, const char *name) {
    gl(GLint location = glGetUniformLocation(shader_program, name));
    if (location == -1) {
        char fmsg[128];
        snprintf(fmsg, sizeof(fmsg), "Failed to get uniform location: %s", name);
        warn(fmsg);
    }
    return location;
}

static bool shader_object_verify(GLuint shader, GLenum shader_type) {
    GLint compile_success = GL_FALSE;
    gl(glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_success));
    if (!compile_success) {
        GLint log_len{};
        gl(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len));

        char log_message[2048];
        gl(glGetShaderInfoLog(shader, log_len, &log_len, log_message));

        const char *shader_type_str = "vertex";
        if (shader_type == GL_VERTEX_SHADER) { shader_type_str = "vertex"; }
        else if (shader_type == GL_FRAGMENT_SHADER) { shader_type_str = "fragment"; }

        char fmsg[sizeof(log_message) + 128];
        snprintf(fmsg, sizeof(fmsg), "Failed to compile %s shader\n%s", shader_type_str, log_message);
        error(fmsg);
        return false;
    }
    return true;
}

static bool shader_program_verify(GLuint prg) {
    // Check Link Status
    GLint link_success = GL_FALSE;
    gl(glGetProgramiv(prg, GL_LINK_STATUS, &link_success));
    if (!link_success) {
        char log_message[2048]{};
        gl(glGetProgramInfoLog(prg, sizeof(log_message), nullptr, log_message));
        if (log_message[0]) { error("shader program link", log_message); }
        else { error("Failed to link shader program"); }
    }

    GLint validate_success = GL_FALSE;
    if (link_success) {
        // Check Validation Status
        gl(glValidateProgram(prg));
        gl(glGetProgramiv(prg, GL_VALIDATE_STATUS, &validate_success));
        if (!validate_success) {
            char log_message[2048]{};
            gl(glGetProgramInfoLog(prg, sizeof(log_message), nullptr, log_message));
            if (log_message[0]) { error("shader program validation", log_message); }
            else { error("Failed to validate shader program"); }
        }
    }

    return (validate_success && link_success);
}

// TODO: Verify we're binding/unbinding the program properly

/// Returns created shader program object. Returns 0 on failure.
static GLuint shader_program_create(const char *vs_src, const char *fs_src) {
    assert(vs_src);
    assert(fs_src);

    // Vertex Shader
    gl(GLuint vs = glCreateShader(GL_VERTEX_SHADER));
    gl(glShaderSource(vs, 1, &vs_src, nullptr));
    gl(glCompileShader(vs));
    bool vs_ok = shader_object_verify(vs, GL_VERTEX_SHADER);

    // Fragment Shader
    gl(GLuint fs = glCreateShader(GL_FRAGMENT_SHADER));
    gl(glShaderSource(fs, 1, &fs_src, nullptr));
    gl(glCompileShader(fs));
    bool fs_ok = shader_object_verify(fs, GL_FRAGMENT_SHADER);

    // Program
    GLuint prg = 0;
    if (vs_ok && fs_ok) {
        gl(prg = glCreateProgram());
        gl(glAttachShader(prg, vs));
        gl(glAttachShader(prg, fs));
        gl(glLinkProgram(prg));
        bool prg_ok = shader_program_verify(prg);
        if (!prg_ok) {
            error("Failed to create shader program");
            gl(glDeleteProgram(prg));
        }
    }

    // Delete Intermediate Shader Objects
    gl(glDeleteShader(vs));
    gl(glDeleteShader(fs));
    vs = 0;
    fs = 0;

    return prg;
}

////////////////////////////////////////////////////////////////////////// Section: ImGui

static ImGuiIO &imgui_init(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::GetStyle().FontSizeBase = 30;
    ImGui::GetStyle().ScaleAllSizes(1);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();

    return io;
}

/// Create window and begin frame
static void imgui_start(const char *title) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin(title);
}

static void imgui_end() {
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static void imgui_framerate(ImGuiIO &imgui_io) {
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0 / (double)imgui_io.Framerate, (double)imgui_io.Framerate);
}

static void imgui_section(const char *name) {
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();
    ImGui::Text("%s", name);
}

static void imgui_render(ImGuiIO &imgui_io, MeshRenderData &cube_rd) {
    imgui_start("Debug Menu");
    imgui_framerate(imgui_io);
    {
        imgui_section("Cube");
        ImGui::DragFloat3("Translation##cube", &cube_rd.transform.pos.x, 1);
        ImGui::DragFloat3("Angular Velocity##cube", &cube_rd.transform.angvel.x, 0.005F);
        ImGui::DragFloat3("Orientation##cube", &cube_rd.transform.ori.x, 0.005F);
    }
    imgui_end();
}

////////////////////////////////////////////////////////////////////////// Section: Main

static void update(FrameContext &fctx, MeshRenderData &bg_rd, MeshRenderData &cube_rd) {
    bg_rd.transform.ori += (float)fctx.dt_s * bg_rd.transform.angvel;
    cube_rd.transform.ori += (float)fctx.dt_s * cube_rd.transform.angvel;
}

static void render(FrameContext &fctx, MeshRenderData &bg_rd, MeshRenderData &cube_rd) {
    if (!shader_bind(fctx.shader_program)) { return; }
    clear_bg(0.1F, 0.1F, 0.1F, 0.1F);
    mesh_draw(fctx, bg_rd);
    mesh_draw(fctx, cube_rd);
}

int main() {
    GLFWwindow *window = glfw_init(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL 3D Test");
    if (!window) { return -1; }
    if (!glew_init()) {
        glfwTerminate();
        return -1;
    }
    gl(glEnable(GL_DEPTH_TEST));
    gl(glDepthFunc(GL_LESS));
    ImGuiIO &imgui_io = imgui_init(window);

    // Section: Vertex and Index Buffers
    // clang-format off
    f32 bg_quad_vb[] = { 
    //             x    y               z     u  v    r  g  b  a
                 -000, 000,            000,   0, 0,   1, 0, 0, 1, // bot left
         WINDOW_WIDTH, 000,            000,   1, 0,   0, 1, 0, 1, // bot right
         WINDOW_WIDTH, WINDOW_HEIGHT,  000,   0, 1,   0, 0, 1, 1, // top right
                 -000, WINDOW_HEIGHT,  000,   1, 1,   1, 1, 1, 1, // top left
    };
    u32 bg_quad_ib[] = { 0, 1, 2, 2, 3, 0 };

    f32 cube_vb[] = {
        // Back face
        -0.5, -0.5, -0.5,  0, 0,  0.15F, 0.15F, 0.15F, 1,
         0.5, -0.5, -0.5,  1, 0,  0.15F, 0.15F, 0.15F, 1,
         0.5,  0.5, -0.5,  0, 1,  0.15F, 0.15F, 0.15F, 1,
        -0.5,  0.5, -0.5,  1, 1,  0.15F, 0.15F, 0.15F, 1,
        // Front Face
        -0.5, -0.5,  0.5,  0, 0,  0, 0, 0, 1,
         0.5, -0.5,  0.5,  1, 0,  0, 0, 0, 1,
         0.5,  0.5,  0.5,  0, 1,  0, 0, 0, 1,
        -0.5,  0.5,  0.5,  1, 1,  0, 0, 0, 1,
    };
   u32 cube_ib[] = {
        0,1,2, 2,3,0, // back quad
        4,5,6, 6,7,4, // front quad
        4,0,3, 3,7,4, // left quad
        1,5,6, 6,2,1, // right quad
        3,2,6, 6,7,3, // top quad
        4,5,1, 1,0,4, // bottom quad
    };
    // clang-format on

    // Section: Meshes
    Mesh bg_mesh = mesh_create(bg_quad_vb, bg_quad_ib, sizeof(bg_quad_vb), sizeof(bg_quad_ib));
    Mesh cube_mesh = mesh_create(cube_vb, cube_ib, sizeof(cube_vb), sizeof(cube_ib));

    // Section: Mesh Transforms
    Transform bg_transform{ .pos{ 1, 1, -500 }, .scale = glm::vec3(1), .ori = glm::vec3(0), .angvel = glm::vec3(0) };
    Transform cube_transform{ .pos{ 0.5F * WINDOW_WIDTH, 0.5F * WINDOW_HEIGHT, 0.F },
                              .scale{ 500, 500, 500 },
                              .ori{ 0.2, -0.4, 0 },
                              .angvel{ 0, 0.4, 0 } };

    // Section: Shader Program
    GLuint shader_program = shader_program_create(shader_sources::vs_src, shader_sources::fs_src);
    GLint uloc_u_mvp = shader_get_uniform_location(shader_program, "u_mvp");

    // Section: Shared Transforms
    const glm::mat4 view_matrix(1);
    const glm::mat4 proj_matrix = glm::ortho(0.F, WINDOW_WIDTH, 0.F, WINDOW_HEIGHT, -1000.F, 1000.F);

    // Section: Frame Setup
    MeshRenderData bg_mesh_rd{ bg_mesh, bg_quad_vb, sizeof(bg_quad_vb), bg_transform };
    MeshRenderData cube_mesh_rd{ cube_mesh, cube_vb, sizeof(cube_vb), cube_transform };
    double t_now_s{}, t_last_s{}, dt_s{};
    FrameContext frame_ctx = { window, dt_s, shader_program, uloc_u_mvp, view_matrix, proj_matrix };
    while (!glfwWindowShouldClose(window)) {
        t_now_s = glfwGetTime();
        dt_s = t_now_s - t_last_s;
        t_last_s = t_now_s;
        update(frame_ctx, bg_mesh_rd, cube_mesh_rd);
        render(frame_ctx, bg_mesh_rd, cube_mesh_rd);
        imgui_render(imgui_io, cube_mesh_rd);
        glfw_update(window);
    }
    glfwTerminate();
}

////////////////////////////////////////////////////////////////////////// Section: Garbage

#if 0
namespace {

struct VertexData {
    f32 xyz[3];
    f32 uv[2];
    f32 rgba[4];
};

}

static VertexBufferData gen_quad_vbuffer(f32 *out_vbuffer, VertexData &vbottom_left, u32 size) {
    f32 *xyz = vbottom_left.xyz;
    f32 *uv = vbottom_left.uv;
    f32 *rgba = vbottom_left.rgba;

    f32 vb[] = { *xyz, *uv, *rgba };
}

static f32* flatten_vertex_buffer_data(VertexBufferData& vertex_buffer);

//
VertexData vbottomleft = { .xyz{ -100, 100, -100 }, .uv{ 0, 1 }, .rgba{ 1, 0, 1 } };
VertexBufferData vbuffer = gen_quad_vbuffer(vbuffer, vbottomleft, 200);
vb.flatten();
#endif
