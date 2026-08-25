#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define GLFW_INCLUDE_NONE          // prevent GLFW from including system GL headers
#include <GLFW/glfw3.h>
#include <glad/glad.h>             // must come after GLFW when using GLFW_INCLUDE_NONE

#define PI 3.14159265358979323846f
#define DEG2RAD (PI / 180.0f)

// ---------- Simple matrix helpers (column-major) ----------
typedef struct { float m[16]; } Mat4;

static Mat4 mat4_identity(void) {
    Mat4 r = {{
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    }};
    return r;
}

static Mat4 mat4_perspective(float fovy_deg, float aspect, float near, float far) {
    float f = 1.0f / tanf(fovy_deg * DEG2RAD * 0.5f);
    Mat4 r = {{0}};
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
}

static Mat4 mat4_look_at(float eye_x, float eye_y, float eye_z,
                         float cen_x, float cen_y, float cen_z,
                         float up_x,  float up_y,  float up_z) {
    float fx = cen_x - eye_x, fy = cen_y - eye_y, fz = cen_z - eye_z;
    float len = sqrtf(fx*fx + fy*fy + fz*fz);
    fx /= len; fy /= len; fz /= len;

    float sx = fy*up_z - fz*up_y;
    float sy = fz*up_x - fx*up_z;
    float sz = fx*up_y - fy*up_x;
    len = sqrtf(sx*sx + sy*sy + sz*sz);
    sx /= len; sy /= len; sz /= len;

    float ux = sy*fz - sz*fy;
    float uy = sz*fx - sx*fz;
    float uz = sx*fy - sy*fx;

    Mat4 r = {{
        sx, ux, -fx, 0,
        sy, uy, -fy, 0,
        sz, uz, -fz, 0,
        -(sx*eye_x + sy*eye_y + sz*eye_z),
        -(ux*eye_x + uy*eye_y + uz*eye_z),
        -(-fx*eye_x - fy*eye_y - fz*eye_z),
        1
    }};
    return r;
}

static Mat4 mat4_rotate_y(float angle_rad) {
    float c = cosf(angle_rad), s = sinf(angle_rad);
    Mat4 r = {{
        c, 0, -s, 0,
        0, 1,  0, 0,
        s, 0,  c, 0,
        0, 0,  0, 1
    }};
    return r;
}

static Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r = {{0}};
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k)
                r.m[col*4 + row] += a.m[k*4 + row] * b.m[col*4 + k];
    return r;
}

// ---------- Cylinder generation ----------
typedef struct {
    float *vertices;   // pos(3) + normal(3)
    unsigned int *indices;
    int vertex_count;
    int index_count;
} Mesh;

static Mesh create_cylinder(int slices, float radius, float height) {
    Mesh m = {0};

    // Side: (slices+1)*2 vertices
    // Top cap: 1 center + (slices+1) rim
    // Bottom cap: 1 center + (slices+1) rim
    int side_verts   = (slices + 1) * 2;
    int cap_verts    = 1 + (slices + 1);          // per cap
    m.vertex_count   = side_verts + 2 * cap_verts;

    // Side: slices * 2 tris
    // Each cap: slices tris
    m.index_count    = (slices * 2 + slices * 2) * 3;

    m.vertices = malloc((size_t)m.vertex_count * 6 * sizeof(float));
    m.indices  = malloc((size_t)m.index_count * sizeof(unsigned int));
    if (!m.vertices || !m.indices) {
        fprintf(stderr, "Failed to allocate cylinder\n");
        exit(1);
    }

    float half_h = height * 0.5f;
    int vi = 0;

    // ----- Side wall -----
    for (int i = 0; i <= 1; ++i) {                 // bottom (0) then top (1)
        float y = (i == 0) ? -half_h : half_h;
        for (int j = 0; j <= slices; ++j) {
            float theta = (float)j / slices * 2.0f * PI;
            float x = cosf(theta);
            float z = sinf(theta);

            // position
            m.vertices[vi++] = x * radius;
            m.vertices[vi++] = y;
            m.vertices[vi++] = z * radius;
            // normal (pointing outward)
            m.vertices[vi++] = x;
            m.vertices[vi++] = 0.0f;
            m.vertices[vi++] = z;
        }
    }

    // ----- Top cap -----
    int top_center = vi / 6;
    // center
    m.vertices[vi++] = 0.0f; m.vertices[vi++] =  half_h; m.vertices[vi++] = 0.0f;
    m.vertices[vi++] = 0.0f; m.vertices[vi++] =  1.0f;   m.vertices[vi++] = 0.0f;
    // rim
    for (int j = 0; j <= slices; ++j) {
        float theta = (float)j / slices * 2.0f * PI;
        float x = cosf(theta);
        float z = sinf(theta);
        m.vertices[vi++] = x * radius;
        m.vertices[vi++] = half_h;
        m.vertices[vi++] = z * radius;
        m.vertices[vi++] = 0.0f;
        m.vertices[vi++] = 1.0f;
        m.vertices[vi++] = 0.0f;
    }

    // ----- Bottom cap -----
    int bot_center = vi / 6;
    // center
    m.vertices[vi++] = 0.0f; m.vertices[vi++] = -half_h; m.vertices[vi++] = 0.0f;
    m.vertices[vi++] = 0.0f; m.vertices[vi++] = -1.0f;   m.vertices[vi++] = 0.0f;
    // rim
    for (int j = 0; j <= slices; ++j) {
        float theta = (float)j / slices * 2.0f * PI;
        float x = cosf(theta);
        float z = sinf(theta);
        m.vertices[vi++] = x * radius;
        m.vertices[vi++] = -half_h;
        m.vertices[vi++] = z * radius;
        m.vertices[vi++] = 0.0f;
        m.vertices[vi++] = -1.0f;
        m.vertices[vi++] = 0.0f;
    }

    // ----- Indices -----
    int ii = 0;

    // Side
    for (int j = 0; j < slices; ++j) {
        int bottom = j;
        int top    = (slices + 1) + j;

        m.indices[ii++] = bottom;
        m.indices[ii++] = top;
        m.indices[ii++] = bottom + 1;

        m.indices[ii++] = top;
        m.indices[ii++] = top + 1;
        m.indices[ii++] = bottom + 1;
    }

    // Top cap (center -> rim)
    int top_rim = top_center + 1;
    for (int j = 0; j < slices; ++j) {
        m.indices[ii++] = top_center;
        m.indices[ii++] = top_rim + j;
        m.indices[ii++] = top_rim + j + 1;
    }

    // Bottom cap (note winding for correct normal)
    int bot_rim = bot_center + 1;
    for (int j = 0; j < slices; ++j) {
        m.indices[ii++] = bot_center;
        m.indices[ii++] = bot_rim + j + 1;
        m.indices[ii++] = bot_rim + j;
    }

    return m;
}

static void free_mesh(Mesh *m) {
    free(m->vertices);
    free(m->indices);
    m->vertices = NULL;
    m->indices  = NULL;
}

// ---------- Shaders ----------
static const char *vertex_shader_src =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "    Normal  = mat3(transpose(inverse(model))) * aNormal;\n"
    "    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
    "}\n";

static const char *fragment_shader_src =
    "#version 330 core\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "out vec4 FragColor;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 objectColor;\n"
    "void main() {\n"
    "    vec3 norm = normalize(Normal);\n"
    "    vec3 lightDir = normalize(lightPos - FragPos);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 viewDir = normalize(viewPos - FragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, norm);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);\n"
    "    vec3 ambient  = 0.15 * objectColor;\n"
    "    vec3 diffuse  = 0.7 * diff * objectColor;\n"
    "    vec3 specular = 0.4 * spec * vec3(1.0);\n"
    "    FragColor = vec4(ambient + diffuse + specular, 1.0);\n"
    "}\n";

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        fprintf(stderr, "Shader compile error:\n%s\n", log);
        exit(1);
    }
    return s;
}

static GLuint create_program(void) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER,   vertex_shader_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    int ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, NULL, log);
        fprintf(stderr, "Program link error:\n%s\n", log);
        exit(1);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ---------- GLFW callbacks ----------
static void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
}

// ---------- Main ----------
int main(void) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(800, 600, "Sphere", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1); // vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);

    // Cylinder mesh
    Mesh cylinder = create_cylinder(64, 0.6f, 1.6f);   // slices, radius, height

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 cylinder.vertex_count * 6 * sizeof(float),
                 cylinder.vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 cylinder.index_count * sizeof(unsigned int),
                 cylinder.indices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    GLuint program = create_program();
    glUseProgram(program);

    // Uniform locations
    GLint model_loc      = glGetUniformLocation(program, "model");
    GLint view_loc       = glGetUniformLocation(program, "view");
    GLint proj_loc       = glGetUniformLocation(program, "projection");
    GLint light_pos_loc  = glGetUniformLocation(program, "lightPos");
    GLint view_pos_loc   = glGetUniformLocation(program, "viewPos");
    GLint color_loc      = glGetUniformLocation(program, "objectColor");

    // Fixed camera
    float eye_x = 0.0f, eye_y = 0.0f, eye_z = 3.5f;
    Mat4 view = mat4_look_at(eye_x, eye_y, eye_z, 0,0,0, 0,1,0);
    Mat4 proj = mat4_perspective(45.0f, 800.0f/600.0f, 0.1f, 100.0f);

    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.m);
    glUniform3f(light_pos_loc, 2.0f, 2.0f, 3.0f);
    glUniform3f(view_pos_loc, eye_x, eye_y, eye_z);
    glUniform3f(color_loc, 0.2f, 0.6f, 0.9f); // nice blue

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float t = (float)glfwGetTime();
        Mat4 model = mat4_rotate_y(t * 0.8f);

        // slight tilt for nicer look
        Mat4 tilt = mat4_identity();
        float c = cosf(0.4f), s = sinf(0.4f);
        tilt.m[5] = c; tilt.m[6] = s;
        tilt.m[9] = -s; tilt.m[10] = c;
        model = mat4_mul(tilt, model);

        glUseProgram(program);
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.m);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, cylinder.index_count, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(program);
    free_mesh(&cylinder);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}