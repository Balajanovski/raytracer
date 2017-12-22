// 
// Created by Balajanovski on 15/12/2017.
//

#include "Screen_Stream.h"
#include "Pixel_Stream_Base.h"

#include <memory>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <iostream>

#include <cassert>


void Screen_Stream::generate_shaders(const std::string &vertex_shader_src, const std::string &frag_shader_src) {
    m_shader = std::make_shared<Shader>(vertex_shader_src.c_str(), frag_shader_src.c_str());
    m_shader->use();
}



Screen_Stream::Screen_Stream(std::shared_ptr<Screen<int>>& screen_dimensions, const std::string& vertex_shader_src, const std::string& frag_shader_src) :
Pixel_Stream_Base(screen_dimensions) {
    init_glfw();

    m_window = create_glfw_window("Raymarcher", screen_dimensions->get_x_max() - screen_dimensions->get_x_min(), screen_dimensions->get_y_max() - screen_dimensions->get_y_min());
    glfwMakeContextCurrent(m_window);

    init_glad();

// Set the viewport size and resize function
    glViewport(0, 0, screen_dimensions->get_x_max() - screen_dimensions->get_x_min(), screen_dimensions->get_y_max() - screen_dimensions->get_y_min());
    glfwSetFramebufferSizeCallback(m_window,
                                   [](GLFWwindow* window, int width, int height) {
                                       glViewport(0, 0, width, height);
                                   }
    );

    // Clear buffer
    glClearColor(0, 0, 0, 1.0f);
    assert(glGetError() == GL_NO_ERROR);

    glClear(GL_COLOR_BUFFER_BIT);
    assert(glGetError() == GL_NO_ERROR);

    generate_shaders(vertex_shader_src, frag_shader_src);

// Create VAO
    glGenVertexArrays(1, &m_vao);
    assert(glGetError() == GL_NO_ERROR);

    glBindVertexArray(m_vao);
    assert(glGetError() == GL_NO_ERROR);

// Create vertex and element buffers
    const static GLfloat vertices[] = {
            // Position   Tex-coords
            -1.0f,  1.0f, 0.0f, 0.0f, // Top-left
             1.0f,  1.0f, 1.0f, 0.0f, // Top-right
             1.0f, -1.0f, 1.0f, 1.0f, // Bottom-right
            -1.0f, -1.0f, 0.0f, 1.0f  // Bottom-left
    };

    glGenBuffers(1, &m_vbo);
    assert(glGetError() == GL_NO_ERROR);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    assert(glGetError() == GL_NO_ERROR);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    assert(glGetError() == GL_NO_ERROR);

    const static GLuint elements[] = {
            0, 1, 2,
            2, 3, 0
    };

    glGenBuffers(1, &m_ebo);
    assert(glGetError() == GL_NO_ERROR);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    assert(glGetError() == GL_NO_ERROR);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
    assert(glGetError() == GL_NO_ERROR);

// Set shader attributes
    GLint pos_attrib = glGetAttribLocation(m_shader->ID(), "position");
    assert(glGetError() == GL_NO_ERROR);

    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    assert(glGetError() == GL_NO_ERROR);

    glEnableVertexAttribArray(pos_attrib);
    assert(glGetError() == GL_NO_ERROR);

    GLint tex_coord_attrib = glGetAttribLocation(m_shader->ID(), "tex_coord");
    assert(glGetError() == GL_NO_ERROR);

    glEnableVertexAttribArray(tex_coord_attrib);
    assert(glGetError() == GL_NO_ERROR);

    glVertexAttribPointer(tex_coord_attrib, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    assert(glGetError() == GL_NO_ERROR);

// Generate texture
    glGenTextures(1, &m_screen_tex);
    assert(glGetError() == GL_NO_ERROR);

// Bind the texture information
    glActiveTexture(GL_TEXTURE0);
    assert(glGetError() == GL_NO_ERROR);

    glBindTexture(GL_TEXTURE_2D, m_screen_tex);
    m_shader->set_int("tex", 0);
}




Screen_Stream::~Screen_Stream() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}




void Screen_Stream::flush() {



    // Reset texture
    glBindTexture(GL_TEXTURE_2D, m_screen_tex);
    assert(glGetError() == GL_NO_ERROR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bounds->width(), bounds->height(), 0, GL_RGB, GL_FLOAT, &get_buffer()[0]);
    assert(glGetError() == GL_NO_ERROR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    assert(glGetError() == GL_NO_ERROR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    assert(glGetError() == GL_NO_ERROR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    assert(glGetError() == GL_NO_ERROR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    assert(glGetError() == GL_NO_ERROR);

    glBindVertexArray(m_vao);

    // Draw rectangle
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    assert(glGetError() == GL_NO_ERROR);

    // Used to swap buffers here

    clear();
}

void Screen_Stream::clear() {
    // Reset iterator
    pos_iter = get_buffer().begin();
}

bool Screen_Stream::should_window_close() const {
    return glfwWindowShouldClose(m_window);
}

bool Screen_Stream::key_pressed(int key) const {
    return (glfwGetKey(m_window, key) == GLFW_PRESS);
}

void Screen_Stream::set_window_should_close(bool value) {
    glfwSetWindowShouldClose(m_window, value);
}

void Screen_Stream::swap_buffers() const {
    glfwSwapBuffers(m_window);
}

void Screen_Stream::poll_events() const {
    glfwPollEvents();
}

void Screen_Stream::init_glfw() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

GLFWwindow* Screen_Stream::create_glfw_window(const std::string &window_title, int window_width, int window_height) {
    GLFWwindow* window;
    window = glfwCreateWindow(window_width, window_height, window_title.c_str(), NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW m_window");
    }
    return window;
}

void Screen_Stream::init_glad() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
}