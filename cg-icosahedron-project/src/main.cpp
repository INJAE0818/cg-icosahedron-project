#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "InitShader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <iostream>
#include <cmath>

// ---------- 윈도우 ----------
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

// ---------- 전역 (회전/스케일) ----------
float g_theta[3] = { 0.f, 0.f, 0.f };
float g_scale = 1.0f;
float g_min_scale = 0.5f;
float g_max_scale = 1.5f;
bool  g_spin = false;
int   g_axis = 1; // 0:X 1:Y 2:Z

// ---------- GL 리소스 ----------
GLuint gProgram = 0;
GLint  uMVP = -1;

struct Mesh {
    GLuint vao = 0, vbo = 0;
    GLsizei count = 0; // vertices 개수
};
Mesh g_planes, g_ico_wire, g_ico_solid;
Mesh g_trunc_fill, g_trunc_outline, g_trunc_pent_fill; // ← 추가

// 공통 버텍스: pos(3), color(3)
struct Vtx { glm::vec3 p, c; };
static inline void pushTri(std::vector<Vtx>& dst,
    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
    const glm::vec3& col) {
    dst.push_back({ a,col }); dst.push_back({ b,col }); dst.push_back({ c,col });
}

// ---------- 기하 데이터 ----------
static inline float PHI() { return (1.f + std::sqrt(5.f)) * 0.5f; }

// 정20면체 12정점
static const std::array<glm::vec3, 12> ICO_V = {
    glm::vec3(-1,  PHI(), 0), glm::vec3(1,  PHI(), 0),
    glm::vec3(-1, -PHI(), 0), glm::vec3(1, -PHI(), 0),
    glm::vec3(0, -1,  PHI()), glm::vec3(0,  1,  PHI()),
    glm::vec3(0, -1, -PHI()), glm::vec3(0,  1, -PHI()),
    glm::vec3(PHI(), 0, -1),  glm::vec3(PHI(), 0,  1),
    glm::vec3(-PHI(), 0, -1), glm::vec3(-PHI(), 0,  1)
};
// 정20면체 20면
static const int ICO_F[20][3] = {
    {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
    {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
    {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
    {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
};

// ---------- VAO/VBO 유틸 ----------
static Mesh makeMesh(const std::vector<Vtx>& verts) {
    Mesh m;
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vtx), verts.data(), GL_STATIC_DRAW);

    GLint locPos = glGetAttribLocation(gProgram, "vPosition");
    GLint locCol = glGetAttribLocation(gProgram, "vColor");
    glEnableVertexAttribArray(locPos);
    glEnableVertexAttribArray(locCol);
    glVertexAttribPointer(locPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vtx), (void*)0);
    glVertexAttribPointer(locCol, 3, GL_FLOAT, GL_FALSE, sizeof(Vtx), (void*)offsetof(Vtx, c));

    glBindVertexArray(0);
    m.count = (GLsizei)verts.size();
    return m;
}

// ---------- Step 1: 평면 ----------
static Mesh build_planes() {
    const float p = PHI();
    std::vector<Vtx> out;
    auto quad = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 col) {
        pushTri(out, a, b, c, col);
        pushTri(out, a, c, d, col);
        };
    quad({ 0, 1, p }, { 0, 1,-p }, { 0,-1,-p }, { 0,-1, p }, { 0,1,0 });   // YZ
    quad({ p,0, 1 }, { p,0,-1 }, { -p,0,-1 }, { -p,0, 1 }, { 0,0,1 });   // ZX
    quad({ 1, p,0 }, { 1,-p,0 }, { -1,-p,0 }, { -1, p,0 }, { 1,0,0 });   // XY
    return makeMesh(out);
}

// ---------- Step 2: 정20면체 ----------
static Mesh build_ico(bool solid) {
    std::vector<Vtx> out;
    const glm::vec3 cols[6] = {
        {0.95f,0.30f,0.30f},{0.30f,0.70f,0.95f},{0.30f,0.85f,0.55f},
        {0.95f,0.80f,0.35f},{0.75f,0.55f,0.95f},{0.40f,0.40f,0.90f}
    };
    glm::vec3 col = solid ? cols[0] : glm::vec3(0, 0, 0);
    for (int f = 0; f < 20; ++f) {
        glm::vec3 a = ICO_V[ICO_F[f][0]];
        glm::vec3 b = ICO_V[ICO_F[f][1]];
        glm::vec3 c = ICO_V[ICO_F[f][2]];
        if (solid) {
            col = cols[f % 6];
            pushTri(out, a, b, c, col);
        }
        else {
            pushTri(out, a, b, c, col);
        }
    }
    return makeMesh(out);
}

// ---------- Step 3: 축구공 ----------
struct PolyFace { std::vector<int> idx; };
static std::vector<glm::vec3> g_tr_vtx;
static std::vector<PolyFace>  g_tr_faces;



const glm::vec3 TRUNC_COLS[6] = {
    {0.95f,0.30f,0.30f},{0.30f,0.70f,0.95f},{0.30f,0.85f,0.55f},
    {0.95f,0.80f,0.35f},{0.75f,0.55f,0.95f},{0.40f,0.40f,0.90f}
};

// geometry 생성
static void build_truncated_geometry() {
    g_tr_vtx.clear(); g_tr_faces.clear();
    std::vector<std::vector<int>> adj(12);
    for (auto& tri : ICO_F) {
        int a = tri[0], b = tri[1], c = tri[2];
        auto add = [&](int u, int v) {
            if (std::find(adj[u].begin(), adj[u].end(), v) == adj[u].end())
                adj[u].push_back(v);
            };
        add(a, b); add(a, c); add(b, a); add(b, c); add(c, a); add(c, b);
    }
    for (auto& v : adj) std::sort(v.begin(), v.end());

    struct Key {
        int u, v; bool operator<(const Key& o)const {
            return (u < o.u) || (u == o.u && v < o.v);
        }
    };
    std::map<Key, int> E;
    auto getE = [&](int u, int v)->int {
        Key k{ u,v };
        auto it = E.find(k); if (it != E.end()) return it->second;
        glm::vec3 P = ICO_V[u] + (ICO_V[v] - ICO_V[u]) * (1.f / 3.f);
        int id = (int)g_tr_vtx.size();
        g_tr_vtx.push_back(P); E[k] = id; return id;
        };

    auto normalize = [](glm::vec3 v) {return v / glm::length(v); };
    // 12 오각형
    for (int u = 0; u < 12; ++u) {
        glm::vec3 n = normalize(ICO_V[u]);
        glm::vec3 tvec = (std::abs(n.x) > 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 U = glm::normalize(glm::cross(n, tvec));
        glm::vec3 V = glm::normalize(glm::cross(n, U));
        std::vector<std::pair<float, int>> ring;
        for (int v : adj[u]) {
            int pi = getE(u, v);
            glm::vec3 P = g_tr_vtx[pi];
            float ang = std::atan2(glm::dot(P, V), glm::dot(P, U));
            ring.push_back({ ang,pi });
        }
        std::sort(ring.begin(), ring.end(), [](auto& a, auto& b) {return a.first < b.first; });
        PolyFace pent;
        for (auto& kv : ring) pent.idx.push_back(kv.second);
        g_tr_faces.push_back(std::move(pent));
    }
    // 20 육각형
    for (auto& tri : ICO_F) {
        int a = tri[0], b = tri[1], c = tri[2];
        PolyFace hex;
        hex.idx = { getE(a,b),getE(b,a),getE(b,c),
                 getE(c,b),getE(c,a),getE(a,c) };
        g_tr_faces.push_back(std::move(hex));
    }
}

// 육각형만 채움
static Mesh build_truncated_mesh() {
    if (g_tr_vtx.empty()) build_truncated_geometry();
    std::vector<Vtx> out; int hexIndex = 0;
    for (const auto& F : g_tr_faces) {
        if (F.idx.size() == 5) continue;
        glm::vec3 col = TRUNC_COLS[hexIndex++ % 6];
        for (size_t i = 1; i + 1 < F.idx.size(); ++i) {
            glm::vec3 a = g_tr_vtx[F.idx[0]];
            glm::vec3 b = g_tr_vtx[F.idx[i]];
            glm::vec3 c = g_tr_vtx[F.idx[i + 1]];
            pushTri(out, a, b, c, col);
        }
    }
    return makeMesh(out);
}

// 오각형 윤곽선
static Mesh build_truncated_outline_mesh() {
    if (g_tr_vtx.empty()) build_truncated_geometry();
    std::vector<Vtx> out; glm::vec3 black(0, 0, 0);
    for (const auto& F : g_tr_faces) {
        if (F.idx.size() != 5) continue;
        for (size_t i = 0; i < F.idx.size(); ++i) {
            int i0 = F.idx[i], i1 = F.idx[(i + 1) % F.idx.size()];
            glm::vec3 a = g_tr_vtx[i0], b = g_tr_vtx[i1];
            out.push_back({ a,black });
            out.push_back({ b,black });
        }
    }
    return makeMesh(out);
}

// draw 함수들
static void drawMesh(const Mesh& m, const glm::mat4& MVP, bool wire = false) {
    glUseProgram(gProgram);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, glm::value_ptr(MVP));
    glBindVertexArray(m.vao);
    if (wire) { glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); glLineWidth(1.0f); }
    else { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); }
    glDrawArrays(GL_TRIANGLES, 0, m.count);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
static void drawLines(const Mesh& m, const glm::mat4& MVP, float width = 1.5f) {
    glUseProgram(gProgram);
    glUniformMatrix4fv(uMVP, 1, GL_FALSE, glm::value_ptr(MVP));
    glBindVertexArray(m.vao);
    glLineWidth(width);
    glDrawArrays(GL_LINES, 0, m.count);
    glBindVertexArray(0);
}

// 오각형(검은 라인으로만 그렸던 부분)도 연회색으로 채우기
static Mesh build_truncated_pent_fill_mesh() {
    if (g_tr_vtx.empty()) build_truncated_geometry();
    std::vector<Vtx> out;
    const glm::vec3 pentCol = glm::vec3(0.92f, 0.92f, 0.92f); // 연한 회색

    for (const auto& F : g_tr_faces) {
        if (F.idx.size() != 5) continue; // 오각형만
        // 팬 삼각분할 (idx[0] 고정)
        for (size_t i = 1; i + 1 < F.idx.size(); ++i) {
            glm::vec3 a = g_tr_vtx[F.idx[0]];
            glm::vec3 b = g_tr_vtx[F.idx[i]];
            glm::vec3 c = g_tr_vtx[F.idx[i + 1]];
            pushTri(out, a, b, c, pentCol);
        }
    }
    return makeMesh(out);
}


// ---------- 콜백 ----------
static void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }
static void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
}
static void key_callback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    switch (key) {
    case GLFW_KEY_SPACE:g_spin = !g_spin; break;
    case GLFW_KEY_I: g_theta[0] = g_theta[1] = g_theta[2] = 0.f; break;
    case GLFW_KEY_Z: g_scale = std::max(g_min_scale, g_scale * 0.95f); break;
    case GLFW_KEY_X: g_scale = std::min(g_max_scale, g_scale * 1.05f); break;
    }
}
static void mouse_button_callback(GLFWwindow*, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        g_axis = (g_axis + 1) % 3;
}

// ---------- 초기화 ----------
static void init_scene() {
    gProgram = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(gProgram);
    uMVP = glGetUniformLocation(gProgram, "uMVP");

    glEnable(GL_DEPTH_TEST);
    glClearColor(1, 1, 1, 1);

    g_planes = build_planes();
    g_ico_wire = build_ico(false);
    g_ico_solid = build_ico(true);

    // 축구공 구성요소
    g_trunc_fill = build_truncated_mesh();            // 육각형 채움
    g_trunc_pent_fill = build_truncated_pent_fill_mesh();  // ← 오각형 채움(새로)
    g_trunc_outline = build_truncated_outline_mesh();    // 오각형 윤곽선
}


// ---------- 메인 ----------
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Icosahedron Portfolio", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create window\n"; return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n"; return -1;
    }

    init_scene();
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(5, 5, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        if (g_spin) { g_theta[g_axis] += 0.5f; if (g_theta[g_axis] > 360.f) g_theta[g_axis] -= 360.f; }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 rot(1.0f);
        rot = glm::rotate(rot, glm::radians(g_theta[0]), glm::vec3(1, 0, 0));
        rot = glm::rotate(rot, glm::radians(g_theta[1]), glm::vec3(0, 1, 0));
        rot = glm::rotate(rot, glm::radians(g_theta[2]), glm::vec3(0, 0, 1));
        glm::mat4 S = glm::scale(glm::mat4(1), glm::vec3(g_scale));

        // 중앙: 평면+와이어 정20면체
        {
            glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(0.f, 2.f, 0.f)) * rot * S;
            glm::mat4 MVP = proj * view * model;
            drawMesh(g_planes, MVP, false); 
            drawMesh(g_ico_wire, MVP, true);
        }
        // 좌하: 솔리드 정20면체
        {
            glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(-4.f, -2.f, 0.f)) * rot * S;
            glm::mat4 MVP = proj * view * model;
            drawMesh(g_ico_solid, MVP, false);
        }
        
        // 우하: 축구공
        {
            glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(2.f, -2.f, 0.f)) * rot * S;
            glm::mat4 MVP = proj * view * model;

            glEnable(GL_POLYGON_OFFSET_FILL);

            // 먼저 오각형 채움(연회색)
            glPolygonOffset(1.0f, 1.0f);
            drawMesh(g_trunc_pent_fill, MVP, false);

            // 그 위에 육각형 채움(컬러)
            glPolygonOffset(2.0f, 2.0f);
            drawMesh(g_trunc_fill, MVP, false);

            glDisable(GL_POLYGON_OFFSET_FILL);

            // 마지막으로 오각형 윤곽선
            drawLines(g_trunc_outline, MVP, 1.5f);

            /*------------------------------step4------------------------------*/
            char title[128];
            const char* axes[3] = { "X","Y","Z" };
            snprintf(title, sizeof(title),
                "Icosahedron Portfolio | Spin:%s Axis:%s Scale:%.2f",
                g_spin ? "On" : "Off", axes[g_axis], g_scale);
            glfwSetWindowTitle(window, title);
        }


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

}