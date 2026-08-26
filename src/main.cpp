#include "hc_types.h"
#include "shader_sources.h"
#include <unordered_map>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
//#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>

////////////////////////////////////////////////////////////////////////// Section: TODO

/*
   TODO:
   - track vertex attribute locations
   - glEnable(GL_DEBUG_OUTPUT); glDebugMessageCallback(...); (OpenGL 4.3 in KHR_debug extension)
   - replace GLM with custom data structures and operations
   - custom loader for gl functions
   - hotloading shader sources (move to files)
*/

////////////////////////////////////////////////////////////////////////// Section: Constants

constexpr f32 WINDOW_WIDTH = (int)(2560.0F * 0.75F);
constexpr f32 WINDOW_HEIGHT = (int)(1440.0F * 0.75F);
constexpr f32 IMGUI_FONT_SIZE = 30 * 0.60F;
constexpr u32 TEXTURE_UNIT_DEBUG = 0;
constexpr u32 TEXTURE_UNIT_CAT_BASE_COLOR = 1;

////////////////////////////////////////////////////////////////////////// Section: Globals

static size_t g_cat_model_mesh_idx;

////////////////////////////////////////////////////////////////////////// Section: Macros

#define gl(gl_operation)                                                                                                                             \
    clear_gl_errors();                                                                                                                               \
    gl_operation;                                                                                                                                    \
    assert(!check_gl_errors() && #gl_operation)

//#define STR_BOOL(value) ((value) ? "true" : "false")

////////////////////////////////////////////////////////////////////////// Section: Data Structures

namespace { // Anonymous namespace to prevent ODR violations and improve LTO (in theory)

#if 0
struct V3F32 {
    union {
        struct { f32 x, y, z; };
        f32 e[3];
    };
};
M4F32 translate(M4F32 mat, V3F32& tvec);
#endif

enum class VertexFormat : u8 { xyz_uv_rgba, xyz_n_uv /* pos: xyz, normals: xyz, texcoords: uv */, xyz };

// TODO: Look into whether or not we should separate the pure geometry data (vb, vb_size, etc), and render data (vao, vbo, ibo, etc).
//       Consider a struct Model with Mesh and RenderData.
struct RenderMesh {
    // GL render data
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    GLsizei index_count;

    // Geometry data
    f32 *vb;
    size_t vb_size;

    // Other
    VertexFormat vertex_format;
    //i32 texunit; // Which texture unit to use when rendering
};

struct Transform {
    glm::vec3 pos;
    glm::vec3 scale;
    glm::vec3 ori;
    glm::vec3 angvel;
};

//struct MeshRenderData {
//    RenderMesh &mesh;
//    Transform &transform;
//};

struct ShaderData {
    GLuint prg;
    std::unordered_map<std::string, GLint> ulocs;
};

struct FrameContext {
    GLFWwindow *window;
    double &dt_s;
    //ImGuiIO &imgui_io;
    ShaderData &shader_xyz_uv_rgba;
    ShaderData &shader_xyz_n_uv;
    glm::mat4 view_matrix;
    glm::mat4 proj_matrix;
};

}

////////////////////////////////////////////////////////////////////////// Section: Logging and Error Checking

static void marker() {
    puts("MARKER: ***************************************************************************\n");
    fflush(stdout);
}
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

// Wrappers

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

// Variadic

// NOTE: Do not use directly. Use finfo(...) instead.
__attribute__((format(printf, 2, 0))) static inline void v_info(const char *regarding, const char *fmt_msg, va_list fmt_args) {
    if (regarding) { printf("INFO [%s]: ", regarding); }
    else { printf("INFO: "); }
    vprintf(fmt_msg, fmt_args);
    fputc('\n', stdout);
    fflush(stdout);
}
__attribute__((format(printf, 2, 3))) static inline void finfo(const char *regarding, const char *fmt_msg, ...) {
    va_list args{};
    va_start(args, fmt_msg);
    v_info(regarding, fmt_msg, args);
    va_end(args);
}
__attribute__((format(printf, 1, 2))) static inline void finfo(const char *fmt_msg, ...) {
    va_list args{};
    va_start(args, fmt_msg);
    v_info(nullptr, fmt_msg, args);
    va_end(args);
}

// NOTE: Do not use directly. Use fwarn(...) instead.
__attribute__((format(printf, 2, 0))) static inline void v_warn(const char *regarding, const char *fmt_msg, va_list fmt_args) {
    if (regarding) { fprintf(stderr, "WARN [%s]: ", regarding); }
    else { fprintf(stderr, "WARN: "); }
    vfprintf(stderr, fmt_msg, fmt_args);
    fputc('\n', stderr);
}
__attribute__((format(printf, 1, 2))) static inline void fwarn(const char *fmt_msg, ...) {
    va_list args{};
    va_start(args, fmt_msg);
    v_warn(nullptr, fmt_msg, args);
    va_end(args);
}
__attribute__((format(printf, 2, 3))) static inline void fwarn(const char *regarding, const char *fmt_msg, ...) {
    va_list args{};
    va_start(args, fmt_msg);
    v_warn(regarding, fmt_msg, args);
    va_end(args);
}

// NOTE: Do not use directly. Use ferror(...) instead.
__attribute__((format(printf, 2, 0))) static inline void v_error(const char *regarding, const char *fmt_msg, va_list fmt_args) {
    if (regarding) { fprintf(stderr, "FAIL [%s]: ", regarding); }
    else { fprintf(stderr, "FAIL: "); }
    vfprintf(stderr, fmt_msg, fmt_args);
    fputc('\n', stderr);
}
__attribute__((format(printf, 2, 3))) static inline void ferror(const char *regarding, const char *fmt_msg, ...) {
    va_list args{};
    va_start(args, fmt_msg);
    v_error(regarding, fmt_msg, args);
    va_end(args);
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
        error("OpenGL", message);
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
    finfo("GLEW", "Using GLEW version %s", (const char *)glewGetString(GLEW_VERSION));
    return true;
}

////////////////////////////////////////////////////////////////////////// Section: GLFW

static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) { // NOLINT(misc-unused-parameters)
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window, true); }
    }
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_EQUAL) { g_cat_model_mesh_idx++; }
        else if (key == GLFW_KEY_MINUS) { g_cat_model_mesh_idx--; }
    }
}

static void glfw_error_callback(int error_code, const char *description) {
    char re[32];
    snprintf(re, sizeof(re), "GLFW: code %d", error_code);
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

static void clear_background(f32 r, f32 g, f32 b, f32 a) {
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
    if (location == -1) { fwarn(nullptr, "Failed to get uniform location: %s", name); }
    return location;
}

static bool shader_verify(GLuint shader, GLenum shader_type) {
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
    bool vs_ok = shader_verify(vs, GL_VERTEX_SHADER);

    // Fragment Shader
    gl(GLuint fs = glCreateShader(GL_FRAGMENT_SHADER));
    gl(glShaderSource(fs, 1, &fs_src, nullptr));
    gl(glCompileShader(fs));
    bool fs_ok = shader_verify(fs, GL_FRAGMENT_SHADER);

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

////////////////////////////////////////////////////////////////////////// Section: Texture

static GLuint texture_create_and_upload_from_rgba(u32 texture_unit_index, const u32 rgba) {
    u8 color[] = { (u8)((rgba >> 24) & 0xFF), (u8)((rgba >> 16) & 0xFF), (u8)((rgba >> 8) & 0xFF), (u8)((rgba >> 0) & 0xFF) };

    GLuint texture{};
    gl(glCreateTextures(GL_TEXTURE_2D, 1, &texture));

    gl(glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR)); // Linearly resample on minification (will not snap to pixel)
    gl(glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR)); // Linearly resample on magnification (stretch to fill)

    gl(glTextureStorage2D(texture, 1, GL_RGBA8, 1, 1));
    gl(glTextureSubImage2D(texture, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color));

    gl(glBindTextureUnit(texture_unit_index, texture));

    return texture;
}

// TODO: Image format arg?
static GLuint texture_create_and_upload_from_image(u32 texture_unit_index, const char *fpath) {
    // Load image
    stbi_set_flip_vertically_on_load(true);
    int width{}, height{}, channels{};
    unsigned char *idata = stbi_load(fpath, &width, &height, &channels, 4);
    if (!idata) {
        error("stbi_load", stbi_failure_reason());
        return 0;
    }

    GLuint texture{};
    gl(glCreateTextures(GL_TEXTURE_2D, 1, &texture));

    gl(glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    gl(glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    gl(glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    gl(glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    gl(glTextureStorage2D(texture, 1, GL_RGBA8, width, height));
    gl(glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, idata));

    gl(glBindTextureUnit(texture_unit_index, texture));

    if (idata) { stbi_image_free(idata); }

    return texture;
}

// Takes PNG/JPG-encoded bytes, not raw image RGBA bytes
static GLuint texture_create_and_upload_from_image(u32 texture_unit_index, const size_t image_size, const uchar *image_bytes) {
    // Load image
    //stbi_set_flip_vertically_on_load(true);
    int width{}, height{}, channels{};
    unsigned char *idata = stbi_load_from_memory(image_bytes, (i32)image_size, &width, &height, &channels, 4);
    if (!idata) {
        error("stbi_load_from_memory", stbi_failure_reason());
        return 0;
    }

    // Create texture object
    GLuint texture{};
    gl(glCreateTextures(GL_TEXTURE_2D, 1, &texture));

    // Allocate and upload image data to texture object
    GLsizei mipmap_levels = 1;                                                                  // 1 + (GLsizei)floor(log2(fmax(width, height)));
    gl(glTextureStorage2D(texture, mipmap_levels, GL_RGBA8, width, height));                    // Allocate
    gl(glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, idata)); // Upload
    //gl(glGenerateTextureMipmap(texture));

    // Set texture parameters
    gl(glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR));    // Linearly resample on minification (will not snap to pixel)
    gl(glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR));    // Linearly resample on magnification (stretch to fill)
    gl(glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)); // Horizonal wrap behavior: clamp, don't wrap
    gl(glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)); // Vertical wrap behavior: clamp, don't wrap

    // Bind texture object and select it into texture unit
    gl(glBindTextureUnit(texture_unit_index, texture));

    // Cleanup
    if (idata) { stbi_image_free(idata); }

    return texture;
}

#if 0
static void texture_bind(GLint uloc_texture_unit_index, GLuint texture_unit_index, GLuint texture_object) {
    gl(glBindTextureUnit(texture_unit_index, texture_object));
    gl(glUniform1i(uloc_texture_unit_index, texture_unit_index));
}
#endif

////////////////////////////////////////////////////////////////////////// Section: Mesh

static bool rendermesh_bind(RenderMesh &rmesh) {
    if (rmesh.vao && rmesh.vbo && rmesh.ibo) {
        gl(glBindVertexArray(rmesh.vao));
        gl(glBindBuffer(GL_ARRAY_BUFFER, rmesh.vbo));
        gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rmesh.ibo));
        return true;
    }
    else {
        char regarding[256];
        char message[256];
        snprintf(regarding, sizeof(regarding), "%s:%d", __FILE__, __LINE__);
        snprintf(message, sizeof(message), "Failed to bind render mesh (vao:%u, vbo:%u, ibo:%u)", rmesh.vao, rmesh.vbo, rmesh.ibo);
        error(regarding, message);
        return false;
    }
}

static void rendermesh_draw(FrameContext &fctx, ShaderData &shader, RenderMesh &rmesh, Transform &transform) {
    if (!shader_bind(shader.prg)) { return; }
    glm::mat4 u_mvp = calculate_mvp(transform, fctx.view_matrix, fctx.proj_matrix);
    if (shader.ulocs.contains("u_texunit")) {
        gl(glUniform1i(shader.ulocs["u_texunit"], TEXTURE_UNIT_CAT_BASE_COLOR));
    } // FIXME: use the appropriate texture unit for each mesh
    gl(glUniformMatrix4fv(shader.ulocs["u_mvp"], 1, GL_FALSE, &u_mvp[0][0]));
    rendermesh_bind(rmesh);
    gl(glDrawElements(GL_LINES, rmesh.index_count, GL_UNSIGNED_INT, nullptr));
}

static RenderMesh rendermesh_create(VertexFormat vfmt, f32 *vb, u32 *ib, size_t vb_size, size_t ib_size) {
    GLuint vao{};
    gl(glGenVertexArrays(1, &vao));
    gl(glBindVertexArray(vao));

    GLuint vbo{};
    gl(glGenBuffers(1, &vbo));
    gl(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    gl(glBufferData(GL_ARRAY_BUFFER, vb_size, vb, GL_STATIC_DRAW));

    switch (vfmt) {
        case VertexFormat::xyz_uv_rgba: {
            size_t stride = 9;
            gl(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)0)); // NOLINT(modernize-use-nullptr)
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            gl(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat))));
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            gl(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)(5 * sizeof(GLfloat))));

            gl(glEnableVertexAttribArray(0));
            gl(glEnableVertexAttribArray(1));
            gl(glEnableVertexAttribArray(2));
        } break;
        case VertexFormat::xyz: {
            size_t stride = 3;
            gl(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)0)); // NOLINT(modernize-use-nullptr)
            gl(glEnableVertexAttribArray(0));
        } break;
        case VertexFormat::xyz_n_uv: {
            size_t stride = 8;
            gl(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)0)); // NOLINT(modernize-use-nullptr)
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            gl(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat))));
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            gl(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void *)(6 * sizeof(GLfloat))));

            gl(glEnableVertexAttribArray(0));
            gl(glEnableVertexAttribArray(1));
            gl(glEnableVertexAttribArray(2));
        } break;
        default: {
            error(__FUNCTION__, "Passed an unhandled vertex format");
        } break;
    }

    GLuint ibo{};
    gl(glGenBuffers(1, &ibo));
    gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    gl(glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib_size, ib, GL_STATIC_DRAW)); // init index buffer data

    // Clean Up (Unbind)
    gl(glBindVertexArray(0)); // unbind this global VAO (only one VAO is active at a time)

    RenderMesh rmesh{ vao, vbo, ibo, 0, vb, vb_size, vfmt };
    size_t index_count = ib_size / sizeof(ib[0]);
    assert(index_count <= (size_t)INT_MAX);
    rmesh.index_count = (GLsizei)index_count;

    return rmesh;
}

////////////////////////////////////////////////////////////////////////// Section: ImGui

static ImGuiIO &imgui_init(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::GetStyle().FontSizeBase = IMGUI_FONT_SIZE;
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

static void imgui_render(ImGuiIO &imgui_io, Transform &transform) {
    imgui_start("Debug Menu");
    imgui_framerate(imgui_io);
    {
        imgui_section("Cat");
        ImGui::DragFloat3("Translation##cat", &transform.pos.x, 1);
        ImGui::DragFloat3("Angular Velocity##cat", &transform.angvel.x, 0.005F);
        ImGui::DragFloat3("Orientation##cat", &transform.ori.x, 0.005F);
    }
    imgui_end();
}

////////////////////////////////////////////////////////////////////////// Section: Main

static const char *cgltf_result_to_str(cgltf_result result) {
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

static const char *cgltf_attribute_type_to_str(cgltf_attribute_type type) {
    switch (type) {
        case cgltf_attribute_type_invalid: return "invalid";
        case cgltf_attribute_type_position: return "position";
        case cgltf_attribute_type_normal: return "normal";
        case cgltf_attribute_type_tangent: return "tangent";
        case cgltf_attribute_type_texcoord: return "texcoord";
        case cgltf_attribute_type_color: return "color";
        case cgltf_attribute_type_joints: return "joints";
        case cgltf_attribute_type_weights: return "weights";
        case cgltf_attribute_type_custom: return "custom";
        default: return "Unknown";
    }
}

// Returns the element count for accessor data of cgltf_type (ie: cgltf_accessor->type)
static cgltf_size cgltf_accessor_type_component_count(cgltf_type accessor_type) {
    switch (accessor_type) {
        case cgltf_type_scalar: return 1;
        case cgltf_type_vec2: return 2;
        case cgltf_type_vec3: return 3;
        case cgltf_type_vec4: return 4;
        case cgltf_type_mat2: return 4;  // 2x2
        case cgltf_type_mat3: return 9;  // 3x3
        case cgltf_type_mat4: return 16; // 4x4
        default: return 0;
    }
}

/*
// commented out line below reads in normals one by one
cgltf_accessor *normal_accessor = current_attribute.data;
for (cgltf_size vrtx_idx = 0; vrtx_idx < normal_accessor->count; vrtx_idx++) {
    f32 next_vnormal[3]{}; // a glTF normal has 3 floats (vec3)
    if (!cgltf_accessor_read_float(normal_accessor, vrtx_idx, next_vnormal, 3)) {
        ferror("cgltf", "Failed to read floats from normal accessor (mesh_idx:%zu, vrtx_idx:%zu)", mesh_idx, vrtx_idx);
    }
}
*/

static std::vector<RenderMesh> load_glb_and_create_rmeshes(const char *glb_path) {
    std::vector<RenderMesh> submeshes{};
    cgltf_options glb_options{};
    cgltf_data *glb_data{};

    // Parse
    // -----
    cgltf_result parse_result = cgltf_parse_file(&glb_options, glb_path, &glb_data);
    if (parse_result == cgltf_result_success) {
        // Fully Load the Data
        // -------------------
        cgltf_result load_result = cgltf_load_buffers(&glb_options, glb_data, "assets/");
        if (load_result == cgltf_result_success) {
            // Iterate Meshes
            // --------------
            for (cgltf_size mesh_idx = 0; mesh_idx < glb_data->meshes_count; mesh_idx++) {
                cgltf_mesh *mesh = &glb_data->meshes[mesh_idx];
                finfo("cgltf", "Mesh %zu: \"%s\"", mesh_idx, mesh->name);
                // Iterate Mesh Primitives
                //   Note: There is up to one material per primitive
                //   Note: `primitive` references the geometry and material needed for a single draw call.
                //         `primitive.attributes` (required) provides an accessor to positions, normals, UVs, etc.
                //         `primitive.material` (optional) references up to one material to be used for the mesh.
                // -----------------------------------
                for (cgltf_size prim_idx = 0; prim_idx < mesh->primitives_count; prim_idx++) {
                    cgltf_primitive *prim = &mesh->primitives[prim_idx];

                    cgltf_size vertex_count{};
                    std::vector<u32> mesh_indices{};
                    std::vector<f32> mesh_normals{};
                    std::vector<f32> mesh_positions{};
                    std::vector<f32> mesh_texcoords{};
                    //std::unordered_map<const char*, u32>

                    // A material describes how 3D surfaces reflect light
                    cgltf_material *mesh_material = prim->material; // TODO: A mesh can have multiple materials across several primitives
                    if (mesh_material) {
                        cgltf_buffer_view *tex_img_buf = mesh_material->pbr_metallic_roughness.base_color_texture.texture->image->buffer_view;
                        cgltf_float *tex_rgba_factor = mesh_material->pbr_metallic_roughness.base_color_factor;
                        uchar *image_data = (uchar *)tex_img_buf->buffer->data + tex_img_buf->offset;
                        finfo(nullptr, "        Loaded mesh texture image \"%s\"", tex_img_buf->name);
                        if (mesh_idx == 0) { // TODO
                            texture_create_and_upload_from_image(TEXTURE_UNIT_CAT_BASE_COLOR, tex_img_buf->size, image_data);
                        }
                    }
                    else { error("        Mesh has no material"); }

                    cgltf_accessor *indices_accessor = prim->indices;
                    if (indices_accessor) {
                        mesh_indices.resize(indices_accessor->count);
                        cgltf_size unpack_result =
                            cgltf_accessor_unpack_indices(indices_accessor, mesh_indices.data(), sizeof(u32), indices_accessor->count);
                        // find out how many indices are required in the output buffer. Returns 0 if the accessor is sparse or if the output component size is less than the accessor's component size.
                        if (unpack_result == 0) {
                            warn("        Indices accessor is sparse or out component size is less than accessor component size: ");
                        }
                        else { finfo("        Indices unpacked: %zu", unpack_result); }
                    }
                    else { error("        Failed to get indices accessor"); }

                    for (cgltf_size attr_idx = 0; attr_idx < prim->attributes_count; attr_idx++) { // Iterate each mesh attribute
                        cgltf_attribute *attr = &prim->attributes[attr_idx];
                        //finfo(nullptr, "Found attribute \"%s\" (mesh_idx:%zu, prim_idx:%zu, attr_idx:%zu)", cgltf_attribute_type_to_str(current_attribute.type), mesh_idx, prim_idx, attr_idx);
                        //cgltf_accessor *normal_accessor{}, *position_accessor{}, *texcoord_accessor{};
                        if (attr->type == cgltf_attribute_type_normal) {
                            cgltf_accessor *normal_accessor = attr->data;
                            cgltf_size float_count = normal_accessor->count * cgltf_accessor_type_component_count(normal_accessor->type);
                            mesh_normals.resize(float_count);
                            // Unpack all components from each normal in the current mesh
                            if (!cgltf_accessor_unpack_floats(normal_accessor, mesh_normals.data(), float_count)) {
                                error("        Failed to unpack floats from normal accessor");
                            }
                        }
                        else if (attr->type == cgltf_attribute_type_position) {
                            cgltf_accessor *position_accessor = attr->data;
                            vertex_count = position_accessor->count;
                            cgltf_size float_count = position_accessor->count * cgltf_accessor_type_component_count(position_accessor->type);
                            mesh_positions.resize(float_count);
                            if (!cgltf_accessor_unpack_floats(position_accessor, mesh_positions.data(), float_count)) {
                                error("        Failed to unpack floats from position accessor");
                            }
                        }
                        else if (attr->type == cgltf_attribute_type_texcoord) {
                            cgltf_accessor *texcoord_accessor = attr->data;
                            cgltf_size float_count = texcoord_accessor->count * cgltf_accessor_type_component_count(texcoord_accessor->type);
                            mesh_texcoords.resize(float_count);
                            if (!cgltf_accessor_unpack_floats(texcoord_accessor, mesh_texcoords.data(), float_count)) {
                                error("        Failed to unpack floats from texcoord accessor");
                            }
                        }
                        // clang-format off
                        else { fwarn(nullptr, "        Unhandled attribute \"%s\"", cgltf_attribute_type_to_str(attr->type)); }
                        // clang-format on

                        // Unify primitive data into structured vertices

                        // Option 1
                        // std::vector<RenderMesh> meshes{};
                        // std::vector<Vertex> vertices{};
                        // for vtx_idx...
                        //     Vertex vertex{};
                        //     vertex.position = { x, y }
                        //     vertices.push_back(vertex);
                        // meshes.push_back(vertices);

                        // Option 2
                        // std::vector<RenderMesh> meshes{};
                        // RenderMesh rmesh{};
                        //
                        //
                    }
                    //if (mesh_indices.size() == 0) { finfo("        Mesh has no indices"); }
                    //if (mesh_normals.size() == 0) { finfo("        Mesh has no normals"); }
                    //if (mesh_positions.size() == 0) { finfo("        Mesh has no positions"); }
                    //if (mesh_texcoords.size() == 0) { finfo("        Mesh has no texcoords"); }
                    assert(vertex_count);
                    assert(mesh_positions.size() == vertex_count * 3);
                    assert(mesh_normals.size() == vertex_count * 3);
                    assert(mesh_texcoords.size() == vertex_count * 2);

                    std::vector<f32> vb;
                    vb.reserve(mesh_positions.size() + mesh_normals.size() + mesh_texcoords.size());
                    for (size_t vrtx_idx{}; vrtx_idx < vertex_count; vrtx_idx++) {
                        size_t p = vrtx_idx * 3;
                        vb.push_back(mesh_positions[p + 0]);
                        vb.push_back(mesh_positions[p + 1]);
                        vb.push_back(mesh_positions[p + 2]);
                        size_t n = vrtx_idx * 3;
                        vb.push_back(mesh_normals[n + 0]);
                        vb.push_back(mesh_normals[n + 1]);
                        vb.push_back(mesh_normals[n + 2]);
                        size_t t = vrtx_idx * 2;
                        vb.push_back(mesh_texcoords[t + 0]);
                        vb.push_back(mesh_texcoords[t + 1]);

                        //finfo(nullptr, "%f %f %f  %f %f %f  %f %f", vb[vrtx_idx+0], vb[vrtx_idx+1],vb[vrtx_idx+2],vb[vrtx_idx+3],vb[vrtx_idx+4],vb[vrtx_idx+5], vb[vrtx_idx+6], vb[vrtx_idx+7]);
                        //finfo(nullptr, "        UV %f %f", vb[vrtx_idx+6], vb[vrtx_idx+7]);
                    }

                    // Positions are the only required mesh data in glTF. Indices, normals, texcoords, etc, are all optional.
                    RenderMesh rmesh = rendermesh_create(VertexFormat::xyz_n_uv,
                                                         vb.data(),
                                                         mesh_indices.data(),
                                                         vb.size() * sizeof(vb[0]),
                                                         mesh_indices.size() * sizeof(mesh_indices[0]));
                    submeshes.push_back(rmesh);
                }
            }
        }
        else { ferror("cgltf", "cgltf_load_buffers failed: %s", cgltf_result_to_str(load_result)); }
    }
    else { ferror("cgltf", "cgltf_parse_file failed: %s", cgltf_result_to_str(parse_result)); }

    return submeshes;
}

#if 0 // NOTE: Contains incorrect comments
static std::vector<RenderMesh> old_load_gltf_and_create_mesh(const char *gltf_path) {
    cgltf_options load_options{}; // optionally force file type, provide memory allocation, provide file operation callbacks
    cgltf_data *gltf_data{};      // allocated and filled by cgltf_parse(); generally mirrors the gltf spec

    std::vector<RenderMesh> meshes{};

    // Parse the .gltf or .glb.
    //  - A .gltf file generally contains asset metadata, referencing .bin files which contain asset payloads (mesh and texture image data).
    //  - A .glb file contains the full asset payload and metadata all in one.
    // Texture images and vertex/index buffers from external files referenced by a .gltf are not loaded.
    if (cgltf_parse_file(&load_options, gltf_path, &gltf_data) == cgltf_result_success) {
        // Will load any external files (relative to assets/) referenced by the gltf and fill in the rest of the gltf_data.
        // If the gltf file is .glb, everything is usually self-contained, so there usually won't be any external files to read from,
        // in which case this won't do anything and will just be successful by default.
        if (cgltf_load_buffers(&load_options, gltf_data, "assets/") == cgltf_result_success) {
            cgltf_validate(gltf_data);

            // gltf_data contains meshes.
            // Each mesh contains primitives.
            // Each primitive contains attributes.
            // Each attribute points to an accessor.
            // Each accessor describes how to read data from a buffer view / buffer.
            //
            // cgltf_accessor pos_attr_accessor* = gltf_data->mesh[0].primitives[0].attributes[pos_attr_idx].data[0]
            //                                                mesh     attr metadata  eg: pos        vertices
            // float *data = cgltf_accessor_read_float(pos_attr_accessor, ...);
            //

            // Access first mesh's primitives data (contains attribute metadata and data for mesh)
            // TODO if (gltf_data->meshes_count) {access;};
            for (cgltf_size mesh_idx = 0; mesh_idx < gltf_data->meshes_count; mesh_idx++) {
                for (cgltf_size prim_idx = 0; prim_idx < gltf_data->meshes_count; prim_idx++) {
                    cgltf_primitive *mesh_primitive_data = &gltf_data->meshes[mesh_idx].primitives[prim_idx];

                    cgltf_accessor *pos_attr_accessor{};
                    // Iterate through the mesh's vertex attributes
                    for (cgltf_size i = 0; i < mesh_primitive_data->attributes_count; i++) {
                        cgltf_attribute *attr = &mesh_primitive_data->attributes[i];
                        if (attr) {
                            if (attr->type == cgltf_attribute_type_position) {
                                pos_attr_accessor = attr->data;
                                break;
                            }
                        }
                        else { warn("cgltf", "Failed to get attribute"); }
                    }
                    //if (!pos_attr_accessor) { error("cgltf", "Failed to get position accessor. Can't attempt to extract vertices."); }

                    if (pos_attr_accessor) {
                        // Extract vertices
                        cgltf_size vertex_count = pos_attr_accessor->count;
                        std::vector<f32> vpoints(vertex_count * 3);
                        for (cgltf_size i = 0; i < vertex_count; i++) {
                            cgltf_accessor_read_float(pos_attr_accessor, i, &vpoints[i * 3], 3); // read the next three floats
                        }

                        // Extract indices
                        cgltf_accessor *idx_attr_accessor = mesh_primitive_data->indices;
                        if (idx_attr_accessor) {
                            std::vector<u32> indices(idx_attr_accessor->count);
                            for (cgltf_size i = 0; i < idx_attr_accessor->count; i++) {
                                indices[i] = (u32)cgltf_accessor_read_index(idx_attr_accessor, i);
                            }

                            // Section: Final GLTF Data
                            f32 *vb = vpoints.data();
                            u32 *ib = indices.data();
                            size_t vb_size = vpoints.size() * sizeof(f32);
                            size_t ib_size = indices.size() * sizeof(u32);

                            meshes.push_back(mesh_create(VertexFormat::xyz, vb, ib, vb_size, ib_size));
                        }
                    }
                }
            }
        }
        else { error("cgltf", "Failed to parse load vertex/index data from parsed gltf data"); }
        cgltf_free(gltf_data);
    }
    else { error("cgltf", "Failed to parse gltf asset"); }
    //assert(false);
    return meshes;
}
#endif

static void update(FrameContext &fctx, Transform &asset_tform, Transform &tbg, Transform &tcube) {
    float dt_s = (f32)fctx.dt_s;
    asset_tform.ori += dt_s * asset_tform.angvel;
    tbg.ori += dt_s * tbg.angvel;
    tcube.ori += dt_s * tcube.angvel;
}

static void render(FrameContext &fctx,
                   std::vector<RenderMesh> &cat_model,
                   RenderMesh &mbg,
                   RenderMesh &mcube,
                   Transform &tasset,
                   Transform &tbg,
                   Transform &tcube) {
    clear_background(0.1F, 0.1F, 0.1F, 0.1F);
    //rendermesh_draw(fctx, fctx.shader_xyz_uv_rgba, mbg, tbg);

    if (g_cat_model_mesh_idx == SIZE_MAX) { g_cat_model_mesh_idx = 0; }
    if (g_cat_model_mesh_idx >= cat_model.size()) { g_cat_model_mesh_idx = cat_model.size() - 1; }
    //rendermesh_draw(fctx, fctx.shader_xyz_n_uv, cat_model[g_cat_model_mesh_idx], tasset);
    for (RenderMesh rmesh : cat_model) {
        rendermesh_draw(fctx, fctx.shader_xyz_n_uv, rmesh, tasset);
    }

    //rendermesh_draw(fctx, mcube, tcube);
}

int main() {
    GLFWwindow *window = glfw_init((int)WINDOW_WIDTH, (int)WINDOW_HEIGHT, "OpenGL 3D Test");
    if (!window) { return -1; }
    if (!glew_init()) {
        glfwTerminate();
        return -1;
    }
    ImGuiIO &imgui_io = imgui_init(window);

    gl(glEnable(GL_DEPTH_TEST));
    gl(glEnable(GL_BLEND));
    gl(glDepthFunc(GL_LESS));
    gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // Section: Load Asset Files
    //GLB_Model model = load_glb_and_create_rmeshes("assets/behemot_cat.glb");
    std::vector<RenderMesh> cat_model = load_glb_and_create_rmeshes("assets/behemot_cat.glb");
    Transform asset_transform{ .pos{ 0.35F * WINDOW_WIDTH, 0.15F * WINDOW_HEIGHT, 0.F },
                               .scale = glm::vec3(9),
                               .ori = glm::vec3(0),
                               .angvel = { 0, 1, 0 } };

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
    RenderMesh bg_mesh = rendermesh_create(VertexFormat::xyz_uv_rgba, bg_quad_vb, bg_quad_ib, sizeof(bg_quad_vb), sizeof(bg_quad_ib));
    RenderMesh cube_mesh = rendermesh_create(VertexFormat::xyz_uv_rgba, cube_vb, cube_ib, sizeof(cube_vb), sizeof(cube_ib));

    // Section: Mesh Transforms
    Transform bg_transform{ .pos{ 1, 1, -500 }, .scale = glm::vec3(1), .ori = glm::vec3(0), .angvel = glm::vec3(0) };
    Transform cube_transform{ .pos{ 0.25F * WINDOW_WIDTH, 0.5F * WINDOW_HEIGHT, 0.F },
                              .scale{ 500, 500, 500 },
                              .ori{ 0.2, -0.4, 0 },
                              .angvel{ 0, 0.4, 0 } };

    // Section: Shader Program
    GLuint prg{};
    // XYZ UV RGBA
    prg = shader_program_create(shader_sources::vs_xyz_uv_rgba, shader_sources::fs_xyz_uv_rgba);
    ShaderData shader_xyz_uv_rgba = { prg, { { "u_mvp", shader_get_uniform_location(prg, "u_mvp") } } };
    // XYZ N UV
    prg = shader_program_create(shader_sources::vs_xyz_n_uv, shader_sources::fs_xyz_n_uv);
    ShaderData shader_xyz_n_uv = { prg,
                                   { { "u_mvp", shader_get_uniform_location(prg, "u_mvp") },
                                     { "u_texunit", shader_get_uniform_location(prg, "u_texunit") } } };

    // Set default textures
    texture_create_and_upload_from_rgba(TEXTURE_UNIT_DEBUG, 0xFF00FF);

    // Section: Shared Transforms
    const glm::mat4 view_matrix(1);
    const glm::mat4 proj_matrix = glm::ortho(0.F, WINDOW_WIDTH, 0.F, WINDOW_HEIGHT, -1000.F, 1000.F);

    // Section: Frame Setup
    //MeshRenderData bg_mesh_rd{ bg_mesh, bg_quad_vb, sizeof(bg_quad_vb), bg_transform };
    //MeshRenderData cube_mesh_rd{ cube_mesh, cube_vb, sizeof(cube_vb), cube_transform };
    double t_now_s{}, t_last_s{}, dt_s{};
    FrameContext frame_ctx = { window, dt_s, shader_xyz_uv_rgba, shader_xyz_n_uv, view_matrix, proj_matrix };
    while (!glfwWindowShouldClose(window)) {
        t_now_s = glfwGetTime();
        dt_s = t_now_s - t_last_s;
        t_last_s = t_now_s;
        update(frame_ctx, asset_transform, bg_transform, cube_transform);
        render(frame_ctx, cat_model, bg_mesh, cube_mesh, asset_transform, bg_transform, cube_transform);
        //imgui_render(imgui_io, cube_transform);
        imgui_render(imgui_io, asset_transform);
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
