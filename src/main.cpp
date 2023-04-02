#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

void renderQuad();
unsigned int loadTexture(char const * path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool hdr = true;
bool bloom = true;
bool hdrKeyPressed = false;
bool bloomKeyPressed = false;
float exposure = 1.0f;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    glm::vec3 backpackRotation = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        <<backpackPosition.x<<'\n'
        <<backpackPosition.y<<'\n'
        <<backpackPosition.z<<'\n'
        <<backpackRotation.x<<'\n'
        <<backpackRotation.y<<'\n'
        <<backpackRotation.z<<'\n'
        ;
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >>backpackPosition.x
           >>backpackPosition.y
           >>backpackPosition.z
           >>backpackRotation.x
           >>backpackRotation.y
           >>backpackRotation.z
           ;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glCullFace(GL_FRONT);

    float cubeVertices[] = {
            // positions                     // normals                        // texture coords

            // FRONT FACE
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,

            // BACK FACE
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

            // LEFT FACE
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            // RIGHT FACE
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,

            // BOTTOM FACE
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

            // TOP FACE
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f
    };



    //  BLOOM

    Shader lightCubeShader("resources/shaders/light_source.vs", "resources/shaders/light_source.fs");
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader blurShader("resources/shaders/7.blur.vs", "resources/shaders/7.blur.fs");
    Shader bloom_finalShader("resources/shaders/7.bloom_final.vs", "resources/shaders/7.bloom_final.fs");
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs","resources/shaders/skybox.fs");

    //----------------------------------
    // configure floating point framebuffer
    // ------------------------------------
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }
    // ------------------------------------------------



    //configure the cube's VAO (and VBO)
    //glFrontFace(GL_CW);

    unsigned int cubeVBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // second, configure the light's VAO (VBO stays the same; the cubeVertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    glm::vec3 pointLightPositions[] = {
            glm::vec3(-4.0f,2.7f,-1.6f),
            glm::vec3(-1.0f,1.847f,-0.3f),
            glm::vec3(-0.3f,1.847f,0.366f)
    };

    //blendingShader.use();
    //blendingShader.setInt("texture1", 0);

    blurShader.use();
    blurShader.setInt("image", 0);

    bloom_finalShader.use();
    bloom_finalShader.setInt("scene", 0);
    bloom_finalShader.setInt("bloomBlur", 1);
    bloom_finalShader.setFloat("bloomThreshold", 0.5f);

    glm::vec3 lightColor;


    //load skybox

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };




    stbi_set_flip_vertically_on_load(false);

    vector<std::string> faces =
    {
                "resources/objects/skybox/right.jpg",
                "resources/objects/skybox/left.jpg",
                "resources/objects/skybox/top.jpg",
                "resources/objects/skybox/bottom.jpg",
                "resources/objects/skybox/front.jpg",
                "resources/objects/skybox/back.jpg"
    };
    unsigned int cubemapTexture = loadCubemap(faces);



    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);



    // TEXTURE TO BLEND
    unsigned int aquarium = loadTexture(FileSystem::getPath("resources/textures/tex.jpeg").c_str());

    // load models
    // -----------
    Model tobogan("resources/objects/pool/parque.obj");
    tobogan.SetShaderTextureNamePrefix("material.");

    Model cocoTree("resources/objects/coconutTree/coconutTreeBended.obj");
    cocoTree.SetShaderTextureNamePrefix("material.");

    Model bush("resources/objects/bush/hedge.obj");
    bush.SetShaderTextureNamePrefix("material.");

    Model ocean("resources/objects/realPool/round-swimming-pool.obj");
    ocean.SetShaderTextureNamePrefix("material.");

    Model sand("resources/objects/sand/sand.obj");
    sand.SetShaderTextureNamePrefix("material.");

    Model swing("resources/objects/swing/child_swing.obj");
    swing.SetShaderTextureNamePrefix("material.");

    Model lamp("resources/objects/lamp/candelabre.obj");
    lamp.SetShaderTextureNamePrefix("material.");



    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(-4.0f,2.7f,-1.6f);
    pointLight.ambient = glm::vec3(0.5, 0.1, 0.1);
    //pointLight.ambient = glm::vec3(1,1,1);
    pointLight.diffuse = glm::vec3(0.95f, 1 ,1);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    float pointLight_constant = 1.0f;
    float pointLight_linear = 7.0f;
    float pointLight_quadratic = 16.08;

    glm::vec3 ambient(0,0,0);


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);



        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



        // don't forget to enable shader before setting uniforms
        ourShader.use();

        // FIRST LIGHT SOURCE -------------------------------------------------------
        pointLight.position = glm::vec3(glm::vec3(-4.0f,2.5f,0.3f));
        //pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        ourShader.setVec3("pointLight[0].position",pointLightPositions[0]);
        ourShader.setVec3("pointLight[0].ambient", ambient);
        ourShader.setVec3("pointLight[0].diffuse", glm::vec3(0.0f,1.0f,1.0f));
        ourShader.setVec3("pointLight[0].specular", glm::vec3(0.0f,2.5f,2.5f));
        ourShader.setFloat("pointLight[0].constant", pointLight_constant);
        ourShader.setFloat("pointLight[0].linear", pointLight_linear);
        ourShader.setFloat("pointLight[0].quadratic", pointLight_quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        ourShader.setVec3("spotLight[0].position", pointLightPositions[0]);
        ourShader.setVec3("spotLight[0].direction", 0.0f, -1.0f, 0.0f);
        ourShader.setVec3("spotLight[0].ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight[0].diffuse", 0.0f, 3.0f, 3.0f);
        ourShader.setVec3("spotLight[0].specular", 0.0f, 3.0f, 3.0f);
        ourShader.setFloat("spotLight[0].constant", pointLight_constant);
        ourShader.setFloat("spotLight[0].linear", pointLight_linear);
        ourShader.setFloat("spotLight[0].quadratic", pointLight_quadratic);
        ourShader.setFloat("spotLight[0].cutOff", glm::cos(glm::radians(16.0f)));
        ourShader.setFloat("spotLight[0].outerCutOff", glm::cos(glm::radians(20.0f)));


        //SECOND LIGHT SOURCE -------------------------

        ourShader.setVec3("pointLight[1].position", pointLightPositions[1]);
        ourShader.setVec3("pointLight[1].ambient", ambient);
        ourShader.setVec3("pointLight[1].diffuse", 5,5,5);
        ourShader.setVec3("pointLight[1].specular", 5,5,5);
        ourShader.setFloat("pointLight[1].constant", pointLight_constant);
        ourShader.setFloat("pointLight[1].linear", pointLight_linear);
        ourShader.setFloat("pointLight[1].quadratic", pointLight_quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        ourShader.setVec3("spotLight[1].position", pointLightPositions[1]);
        ourShader.setVec3("spotLight[1].direction", 0.0f, -1.0f, 0.0f);
        ourShader.setVec3("spotLight[1].ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight[1].diffuse", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight[1].specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight[1].constant", 1.0f);
        ourShader.setFloat("spotLight[1].linear", 3.4);
        ourShader.setFloat("spotLight[1].quadratic", 0.932);
        ourShader.setFloat("spotLight[1].cutOff", glm::cos(glm::radians(2.0f)));
        ourShader.setFloat("spotLight[1].outerCutOff", glm::cos(glm::radians(24.0f)));

        //THIRD LIGHT SOURCE------------------------------------------
        ourShader.setVec3("pointLight[2].position", pointLightPositions[2]);
        ourShader.setVec3("pointLight[2].ambient", ambient);
        ourShader.setVec3("pointLight[2].diffuse", 5,0,5);
        ourShader.setVec3("pointLight[2].specular", 5,0,5);
        ourShader.setFloat("pointLight[2].constant", pointLight_constant);
        ourShader.setFloat("pointLight[2].linear", pointLight_linear);
        ourShader.setFloat("pointLight[2].quadratic", pointLight_quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        ourShader.setVec3("spotLight[2].position", pointLightPositions[2]);
        ourShader.setVec3("spotLight[2].direction", 0.0f, -1.0f, 0.0f);
        ourShader.setVec3("spotLight[2].ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight[2].diffuse", 1.0f, 0.0f, 1.0f);
        ourShader.setVec3("spotLight[2].specular", 1.0f, 0.0f, 1.0f);
        ourShader.setFloat("spotLight[2].constant", 1.0f);
        ourShader.setFloat("spotLight[2].linear", 3.4);
        ourShader.setFloat("spotLight[2].quadratic", 0.932);
        ourShader.setFloat("spotLight[2].cutOff", glm::cos(glm::radians(2.0f)));
        ourShader.setFloat("spotLight[2].outerCutOff", glm::cos(glm::radians(24.0f)));


        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);





        // render the loaded model

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f,-24.0f,0.0f)
                /*programState->backpackPosition*/); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(15.0f)/*glm::vec3(programState->backpackScale)*/);    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        ourShader.setFloat("material.shininess", 1.0f);
        sand.Draw(ourShader);

        ourShader.setFloat("material.shininess", 32.0f);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-4.0f,0.3f,-2.0f)
                /*programState->backpackPosition*/); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.0035f)/*glm::vec3(programState->backpackScale)*/);    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        lamp.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-3.0f,-0.3f,-1.0f)
                /*programState->backpackPosition*/); // translate it down so it's at the center of the scene
        //model = glm::rotate(model, glm::radians(currentFrame), glm::vec3(0,0,1));
        model = glm::scale(model, glm::vec3(0.1f)/*glm::vec3(programState->backpackScale)*/);    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        swing.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.4f,0,-2.0f)
                /*programState->backpackPosition*/); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.009)/*glm::vec3(programState->backpackScale)*/);    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        ocean.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.4f,0,2.2f)
                /*programState->backpackPosition*/); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.005)/*glm::vec3(programState->backpackScale)*/);    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        bush.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.5f,0,-2.0f)
                /*programState->backpackPosition*/); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.005)/*glm::vec3(programState->backpackScale)*/);    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        bush.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.5f,0,-2.0f)
                /*programState->backpackPosition*/); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.005)/*glm::vec3(programState->backpackScale)*/);    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        cocoTree.Draw(ourShader);


        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.4f,0,2.2f)
                               /*programState->backpackPosition*/); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.005)/*glm::vec3(programState->backpackScale)*/);    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        cocoTree.Draw(ourShader);


        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(0,0,-0.6)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0,1,0));
        model = glm::scale(model, glm::vec3(0.01));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        tobogan.Draw(ourShader);

        //FIRST LIGHT-----------------------------------
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        glBindVertexArray(cubeVAO);

        // we now draw as many light bulbs as we have point lights.
        model = glm::mat4(1.0f);


        model = glm::mat4(1.0f);
        model = glm::translate(model, pointLightPositions[0]/*programState->backpackPosition*/);
        model = glm::scale(model, glm::vec3(0.1f)); // Make it a smaller cube
        lightCubeShader.setVec3("lightColor", glm::vec3(0.0f,2.5f,2.5f));
        lightCubeShader.setMat4("model", model);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        //glDisable(GL_CULL_FACE);
        glBindVertexArray(0);

        //SECOND LIGHT-----------------------------
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        glBindVertexArray(cubeVAO);

        // we now draw as many light bulbs as we have point lights.
        //model = glm::mat4(1.0f);


        model = glm::mat4(1.0f);
        model = glm::translate(model, pointLightPositions[1]/*programState->backpackPosition*/);
        model = glm::scale(model, glm::vec3(0.1f)); // Make it a smaller cube
        lightCubeShader.setVec3("lightColor", glm::vec3(5.0f,5.0f,5.0f));
        lightCubeShader.setMat4("model", model);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        //glDisable(GL_CULL_FACE);
        glBindVertexArray(0);
        //THIRD LIGHT-------------------------------
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        glBindVertexArray(cubeVAO);

        // we now draw as many light bulbs as we have point lights.
        //model = glm::mat4(1.0f);


        model = glm::mat4(1.0f);
        model = glm::translate(model, pointLightPositions[2] /*programState->backpackPosition*/);
        model = glm::scale(model, glm::vec3(0.1f)); // Make it a smaller cube
        lightCubeShader.setVec3("lightColor", glm::vec3(3.0f,0.0f,3.0f));
        lightCubeShader.setMat4("model", model);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        //glDisable(GL_CULL_FACE);
        glBindVertexArray(0);


        //SKYBOX1
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        /*projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);*/
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
        //------END OF SKYBOX------
        //AQUARIUM
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, aquarium);
         view = programState->camera.GetViewMatrix();
        blendingShader.use();
        blendingShader.setInt("texture1", 0);
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);

        model = glm::mat4(1.0f);
        model = glm::translate(model, /*programState->backpackPosition*/glm::vec3(0,0,0));
        model = glm::scale(model, glm::vec3(15.0f)); // Make it a smaller cube
        blendingShader.setMat4("model", model);



        // render the cube
        glBindVertexArray(cubeVAO);


        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);


        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. blur bright fragments with two-pass Gaussian Blur
        // --------------------------------------------------
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 20;
        blurShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 3. now render floating point color buffer to 2D plane and tonemap HDR colors to default framebuffer's (clamped) color range
        // --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloom_finalShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        bloom_finalShader.setInt("bloom", bloom);
        bloom_finalShader.setFloat("exposure", exposure);
        renderQuad();



        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat3("Backpack rotation", (float*)&programState->backpackRotation);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


unsigned int quadVAO = 0;
unsigned int quadVBO;

void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


