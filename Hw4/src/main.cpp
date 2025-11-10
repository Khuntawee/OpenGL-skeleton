#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#include "animator.h"
#include "model_animation.h"
#include "filesystem.h"

#include <iostream>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 1.0f, 5.0f)); // We'll update this per frame to behind the character
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, Animator &animator, glm::vec3 &characterPos, float &characterRotation);

enum class AnimationState
{
    StoppedForward,    // Used as idle
    SteppingForward,
    SteppingBackward,
    SteppingLeft,
    SteppingRight
};

AnimationState g_currentState = AnimationState::StoppedForward;

// Global animation pointers
Animation* stoppedForwardAnim_ptr = nullptr;  // no separate idle
Animation* steppingForwardAnim_ptr = nullptr;
Animation* steppingBackwardAnim_ptr = nullptr;
Animation* steppingLeftAnim_ptr = nullptr;
Animation* steppingRightAnim_ptr = nullptr;  // mirror of left

Animation* g_currentAnimation = nullptr;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Character Animation - WASD + Fixed 3rd Person Camera",NULL,NULL);
    if(!window){
        std::cout<<"Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window,framebuffer_size_callback);
    glfwSetCursorPosCallback(window,mouse_callback);
    glfwSetScrollCallback(window,scroll_callback);

    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout<<"Failed to initialize GLAD\n";
        return -1;
    }

    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);

    // Animation files (make sure these exist!)
    const std::string stoppedForwardPath    = FileSystem::getPath("assets/Stepping Forward.dae"); // used also as stopped forward pose
    const std::string steppingForwardPath   = FileSystem::getPath("assets/Stepping Forward.dae");
    const std::string steppingBackwardPath  = FileSystem::getPath("assets/Stepping Backward.dae");
    const std::string steppingLeftPath      = FileSystem::getPath("assets/Stepping Left.dae");

    std::string shaderVSPath = FileSystem::getPath("shaders/anim_model.vs");
    std::string shaderFSPath = FileSystem::getPath("shaders/anim_model.fs");

    Shader ourShader(shaderVSPath.c_str(), shaderFSPath.c_str());

    Model ourModel(stoppedForwardPath);  // load model from stopped forward animation file

    Animation stoppedForwardAnim(stoppedForwardPath, &ourModel);
    Animation steppingForwardAnim(steppingForwardPath, &ourModel);
    Animation steppingBackwardAnim(steppingBackwardPath, &ourModel);
    Animation steppingLeftAnim(steppingLeftPath, &ourModel);

    // Right step is mirrored left step
    Animation& steppingRightAnim = steppingLeftAnim;

    stoppedForwardAnim_ptr = &stoppedForwardAnim;
    steppingForwardAnim_ptr = &steppingForwardAnim;
    steppingBackwardAnim_ptr = &steppingBackwardAnim;
    steppingLeftAnim_ptr = &steppingLeftAnim;
    steppingRightAnim_ptr = &steppingRightAnim;

    Animator animator(stoppedForwardAnim_ptr);
    g_currentAnimation = stoppedForwardAnim_ptr;
    g_currentState = AnimationState::StoppedForward;

    glm::vec3 characterPos(0.0f, -0.5f, 0.0f);
    float characterRotation = 0.0f;

    while(!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, animator, characterPos, characterRotation);
        animator.UpdateAnimation(deltaTime);

        // Update camera position to fixed offset behind character
        // Fixed offset: 5 units behind, 2 units up
        glm::vec3 offset(0.0f, 2.0f, 5.0f);
        // Calculate camera position relative to characterRotation (rotate offset)
        glm::vec3 camPos = characterPos 
                           + glm::vec3(
                                sin(characterRotation) * offset.z,
                                offset.y,
                                cos(characterRotation) * offset.z);
        camera.Position = camPos;

        // Camera looks at character's position + some height offset (e.g. head height)
        glm::vec3 lookAtTarget = characterPos + glm::vec3(0.0f, 1.0f, 0.0f);
        camera.Front = glm::normalize(lookAtTarget - camera.Position);
        // Recalculate right and up vectors for camera
        camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0.0f,1.0f,0.0f)));
        camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        auto transforms = animator.GetFinalBoneMatrices();
        for(int i=0; i < (int)transforms.size(); ++i)
            ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, characterPos);
        model = glm::rotate(model, characterRotation, glm::vec3(0.0f, 1.0f, 0.0f));

        if(g_currentState == AnimationState::SteppingRight)
        {
            model = glm::scale(model, glm::vec3(-0.5f, 0.5f, 0.5f)); // mirror X for right step
        }
        else
        {
            model = glm::scale(model, glm::vec3(0.5f));
        }

        ourShader.setMat4("model", model);

        ourModel.Draw(ourShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, Animator &animator, glm::vec3 &characterPos, float &characterRotation)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window,true);

    float moveSpeed = 2.0f * deltaTime;
    bool moved = false;
    AnimationState nextState = AnimationState::StoppedForward; // default stopped forward pose

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        characterPos.z -= moveSpeed * cos(characterRotation);
        characterPos.x -= moveSpeed * sin(characterRotation);
        moved = true;
        nextState = AnimationState::SteppingForward;
    }
    else if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        characterPos.z += moveSpeed * cos(characterRotation);
        characterPos.x += moveSpeed * sin(characterRotation);
        moved = true;
        nextState = AnimationState::SteppingBackward;
    }
    else if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        characterPos.z -= moveSpeed * sin(characterRotation);
        characterPos.x += moveSpeed * cos(characterRotation);
        moved = true;
        nextState = AnimationState::SteppingLeft;
    }
    else if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        characterPos.z += moveSpeed * sin(characterRotation);
        characterPos.x -= moveSpeed * cos(characterRotation);
        moved = true;
        nextState = AnimationState::SteppingRight;
    }

    if(nextState != g_currentState)
    {
        Animation* targetAnimation = nullptr;
        switch(nextState)
        {
            case AnimationState::StoppedForward:
                targetAnimation = stoppedForwardAnim_ptr;
                break;
            case AnimationState::SteppingForward:
                targetAnimation = steppingForwardAnim_ptr;
                break;
            case AnimationState::SteppingBackward:
                targetAnimation = steppingBackwardAnim_ptr;
                break;
            case AnimationState::SteppingLeft:
                targetAnimation = steppingLeftAnim_ptr;
                break;
            case AnimationState::SteppingRight:
                targetAnimation = steppingRightAnim_ptr;
                break;
        }

        if(targetAnimation && targetAnimation != g_currentAnimation)
        {
            animator.PlayAnimation(targetAnimation);
            g_currentAnimation = targetAnimation;
            g_currentState = nextState;
        }
    }

    // No free camera movement, only fixed behind character,
    // so comment out or remove camera.ProcessKeyboard calls if you want.

}

void framebuffer_size_callback(GLFWwindow* window,int width,int height)
{
    glViewport(0,0,width,height);
}

void mouse_callback(GLFWwindow* window,double xpos,double ypos)
{
    // Optional: Disable free mouse camera movement if you want
    // For now, no effect on camera since it's fixed behind character
    lastX = xpos;
    lastY = ypos;
}

void scroll_callback(GLFWwindow* window,double xoffset,double yoffset)
{
    // Optional: you can add zoom in/out here by changing camera.Zoom
    // camera.ProcessMouseScroll(yoffset);
}
