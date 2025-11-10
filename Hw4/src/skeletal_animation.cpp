#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>

#include <iostream>

// function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Models and animations
Model idleModel(FileSystem::getPath("assets/Stepping Forward.dae")); // Use Forward model for Idle (stopped forward)
Animation idleAnimation(FileSystem::getPath("assets/Stepping Forward.dae"), &idleModel);

Model forwardModel(FileSystem::getPath("assets/Stepping Forward.dae"));
Animation forwardAnimation(FileSystem::getPath("assets/Stepping Forward.dae"), &forwardModel);

Model backwardModel(FileSystem::getPath("assets/Stepping Backward.dae"));
Animation backwardAnimation(FileSystem::getPath("assets/Stepping Backward.dae"), &backwardModel);

Model leftModel(FileSystem::getPath("assets/Stepping Left.dae"));
Animation leftAnimation(FileSystem::getPath("assets/Stepping Left.dae"), &leftModel);

// animator pointer and current model pointer
Animator* animator = nullptr;
Model* currentModel = nullptr;
bool mirrorRight = false;

enum MovementState {
    Idle,
    MovingForward,
    MovingBackward,
    MovingLeft,
    MovingRight
};

MovementState currentState = Idle;

void setAnimation(Model* model, Animation* animation, MovementState state, bool mirror = false) {
    if (animator) delete animator;
    animator = new Animator(animation);
    currentModel = model;
    mirrorRight = mirror;
    currentState = state;
}

void processInput(GLFWwindow* window) {
    bool wNow = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool sNow = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool aNow = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool dNow = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    // If no movement keys pressed, idle
    if (!wNow && !sNow && !aNow && !dNow) {
        if (currentState != Idle) {
            setAnimation(&idleModel, &idleAnimation, Idle, false);
        }
    }
    else {
        // Movement keys take priority in order W, S, A, D
        if (wNow && currentState != MovingForward) {
            setAnimation(&forwardModel, &forwardAnimation, MovingForward, false);
        }
        else if (sNow && currentState != MovingBackward) {
            setAnimation(&backwardModel, &backwardAnimation, MovingBackward, false);
        }
        else if (aNow && currentState != MovingLeft) {
            setAnimation(&leftModel, &leftAnimation, MovingLeft, false);
        }
        else if (dNow && currentState != MovingRight) {
            setAnimation(&leftModel, &leftAnimation, MovingRight, true); // mirrored left for right
        }
    }

    if (wNow)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (sNow)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (aNow)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (dNow)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    // glfw initialization and configuration
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    // tell stb_image.h to flip loaded textures on y-axis
    stbi_set_flip_vertically_on_load(true);

    // configure global OpenGL state
    glEnable(GL_DEPTH_TEST);

    // build and compile shader
    Shader ourShader("anim_model.vs", "anim_model.fs");

    // start with idle animation
    setAnimation(&idleModel, &idleAnimation, Idle, false);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        animator->UpdateAnimation(deltaTime);

        // render
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        auto transforms = animator->GetFinalBoneMatrices();
        for (int i = 0; i < transforms.size(); ++i)
            ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.4f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));

        if (mirrorRight) {
            model = glm::scale(model, glm::vec3(-1.0f, 1.0f, 1.0f));
        }

        ourShader.setMat4("model", model);
        currentModel->Draw(ourShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    if (animator)
        delete animator;

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)(xpos - lastX);
    float yoffset = (float)(lastY - ypos); // reversed since y-coordinates range bottom to top

    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll((float)yoffset);
}
