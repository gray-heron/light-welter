
#include <SDL2/SDL.h>

#include "config.h"
#include "view_opengl.h"

using namespace SDL2pp;
using std::get;

glm::vec3 HTN(glm::vec4 homo_vector)
{
    return glm::vec3(homo_vector.x / homo_vector.w, homo_vector.y / homo_vector.w,
                     homo_vector.z / homo_vector.w);
}

glm::vec4 NTH(glm::vec3 nonhomo_vector)
{
    return glm::vec4(nonhomo_vector.x, nonhomo_vector.y, nonhomo_vector.z, 1.0);
}

ViewOpenGL::ViewOpenGL()
    : rx_(Config::inst().GetOption<int>("resx")),
      ry_(Config::inst().GetOption<int>("resy")),
      window_("OpenGL preview", 10, SDL_WINDOWPOS_CENTERED, rx_, ry_, SDL_WINDOW_OPENGL),
      main_context_(SDL_GL_CreateContext(window_.Get())), camera_pos_(-10, 0, 1),
      fov_(65.0f), pitch_(0.0f), yaw_(0.0f)
{
    SDL_GL_SetSwapInterval(1);
    SDL_GL_ResetAttributes();

    glewExperimental = GL_TRUE;
    ASSERT(glewInit() == GLEW_OK);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    GLuint programID = LoadShaders("shaders/SimpleTransform.vertexshader",
                                   "shaders/SingleColor.fragmentshader");

    mvp_id_ = glGetUniformLocation(programID, "MVP");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glUseProgram(programID);
}

glm::mat4 ViewOpenGL::UpdateCamera()
{
    glm::vec4 lookat_h = glm::vec4(1.0f, 0.0f, 0.0f, 1.0);

    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), pitch_, glm::vec3(0, 0, 1));
    rot = glm::rotate(rot, yaw_, glm::vec3(0, 1, 0));

    lookat_h = rot * lookat_h;
    camera_lookat_ = HTN(lookat_h);

    return glm::lookAt(camera_pos_, camera_pos_ + camera_lookat_, glm::vec3(0, 1, 0));
}

glm::mat4 ViewOpenGL::GetMVP()
{
    glm::mat4 projection =
        glm::perspective(glm::radians(fov_), float(rx_) / float(ry_), 1.0f, 1000.0f);

    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 view = UpdateCamera();

    glm::mat4 mvp = projection * view * model;

    return mvp;
}

glm::vec3 ViewOpenGL::GetCameraPos() { return camera_pos_; }

void ViewOpenGL::Render(const Scene &scene)
{
    SDL_GL_MakeCurrent(window_.Get(), main_context_);

    auto mvp = GetMVP();
    // Clear the screen
    glUniformMatrix4fv(mvp_id_, 1, GL_FALSE, &mvp[0][0]);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto renderable : scene.renderables_)
    {
        renderable->RenderByOpenGL();
    }

    // Swap buffers
    SDL_GL_SwapWindow(window_.Get());

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            HandleKeyDown(event.key);
            break;
        case SDL_MOUSEBUTTONDOWN:
            HandleMouseKeyDown(event.button);
            break;
        }
    }
}

void ViewOpenGL::HandleKeyDown(SDL_KeyboardEvent key)
{
    glm::vec3 camera_lookat_side =
        HTN(glm::rotate(glm::mat4(1.0), glm::half_pi<float>(), glm::vec3(0, 1, 0)) *
            NTH(camera_lookat_));

    switch (key.keysym.sym)
    {
    case SDLK_UP:
        pitch_ += 0.05;
        break;
    case SDLK_DOWN:
        pitch_ -= 0.05;
        break;
    case SDLK_LEFT:
        yaw_ += 0.05;
        break;
    case SDLK_RIGHT:
        yaw_ -= 0.05;
        break;
    case SDLK_w:
        camera_pos_ += camera_lookat_;
        break;
    case SDLK_s:
        camera_pos_ -= camera_lookat_;
        break;
    case SDLK_a:
        camera_pos_ += camera_lookat_side;
        break;
    case SDLK_d:
        camera_pos_ -= camera_lookat_side;
        break;
    case SDLK_q:
        camera_pos_ += glm::vec3(0.0, 1.0, 0.0);
        break;
    case SDLK_e:
        camera_pos_ += glm::vec3(0.0, -1.0, 0.0);
        break;
    case SDLK_KP_PLUS:
        fov_ *= 1.1f;
        break;
    case SDLK_KP_MINUS:
        fov_ *= 0.9f;
        break;
    case SDLK_ESCAPE:
        action_queue_.push(Action::Exit);
        break;
    case SDLK_RETURN:
        action_queue_.push(Action::TakePicture);
        break;
    }
}

void ViewOpenGL::HandleMouseKeyDown(SDL_MouseButtonEvent key)
{
    if (key.button != SDL_BUTTON_LEFT)
        return;
}

boost::optional<ViewOpenGL::Action> ViewOpenGL::DequeueAction()
{
    if (action_queue_.empty())
        return boost::none;

    auto ret = action_queue_.front();
    action_queue_.pop();
    return ret;
}