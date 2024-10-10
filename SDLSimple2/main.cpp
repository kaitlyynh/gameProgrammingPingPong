/**
* Author: Kaitlyn Huynh
* Assignment: Simple 2D Scene
* Date due: 2024-09-28, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
enum AppStatus { RUNNING, TERMINATED };
constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 480 * 1.5;
constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;
constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";
constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

constexpr char MARIO_SPRITE_FILEPATH[]    = "assets 15-30-09-747/mario.png",
               GOOMBA_SPRITE_FILEPATH[] = "assets 15-30-09-747/goomba.png";

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix,
          g_mario_matrix,
          g_goomba_matrix,
          g_projection_matrix,
          g_mario_matrix_l,
          g_mario_matrix_r,
          g_goomba_matrix_m,
          g_goomba_ball_matrix,
          g_goomba_ball2_matrix,
          g_goomba_ball3_matrix;
            
        
float g_previous_ticks = 0.0f;
GLuint g_mario_texture_id,
       g_goomba_texture_id;
GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    return textureID;
}
void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("PING PONG!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }
#ifdef _WINDOWS
    glewInit();
#endif
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    g_mario_matrix       = glm::mat4(1.0f);
    g_goomba_matrix     = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    g_mario_matrix_l       = glm::mat4(1.0f);
    g_mario_matrix_r      = glm::mat4(1.0f);
    g_goomba_matrix_m = glm::mat4(1.0f);
    g_goomba_ball_matrix = glm::mat4(1.0f);
    g_goomba_ball2_matrix = glm::mat4(1.0f);
    g_goomba_ball3_matrix = glm::mat4(1.0f);
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    glUseProgram(g_shader_program.get_program_id());
    glClearColor(0.678f, 0.847f, 0.902f, 1.0f); // Pastel blue coloerd background
    g_mario_texture_id   = load_texture(MARIO_SPRITE_FILEPATH);
    g_goomba_texture_id = load_texture(GOOMBA_SPRITE_FILEPATH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
// Starting positions
glm::vec3 g_player1_pos = glm::vec3(-4.0f, 0.0f, 0.0f);
glm::vec3 g_player2_pos = glm::vec3(4.0f, 0.0f, 0.0f);
glm::vec3 middle_line_pos = glm::vec3(0.0f, -0.5f, 0.0f); // The middle bar
glm::vec3 g_goomba_ball_pos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_goomba_ball2_pos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_goomba_ball3_pos = glm::vec3(0.0f, 0.0f, 0.0f);
// Track overall movement triggers
glm::vec3 g_player1_movement = glm::vec3(0.0f);
glm::vec3 g_player2_movement = glm::vec3(0.0f);
glm::vec3 g_goomba_ball_movement = glm::vec3(-1.0f, -0.25f, 0.0f); // have the ball move lower bottom
glm::vec3 g_goomba_ball2_movement = glm::vec3(-1.2f, -0.25f, 0.0f);
glm::vec3 g_goomba_ball3_movement = glm::vec3(-1.4f, -0.25f, 0.0f);

glm::vec3 g_goomba_ball2_movement_prev = glm::vec3(-1.2f, -0.25f, 0.0f);
glm::vec3 g_goomba_ball3_movement_prev = glm::vec3(-1.4f, -0.25f, 0.0f);

// Scaling vectors
glm::vec3 g_paddle_scale = glm::vec3(0.5f, 1.0f, 0.0f);
glm::vec3 g_divider_scale = glm::vec3(2.0f, 6.0f, 0.0f);
glm::vec3 g_ball_scale = glm::vec3(0.5f, 0.5f, 0.0f);




// Read keystroke input
const Uint8 *key_state = SDL_GetKeyboardState(NULL); // if non-NULL, receives the length of the returned array
bool single_player = false; // 2 players by default
bool left_or_right_hit = false;
bool left_or_right_hit2 = false;
bool left_or_right_hit3 = false;
bool ball_move_left = true;
bool ball_move_left2 = true;
bool ball_move_left3 = true;
int ball_count = 1;
void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
//        if (left_or_right_hit) { // ball hit the left or right of the window
//            g_app_status = TERMINATED;
//            left_or_right_hit = true; // delete later
//            break;
//        }
        if (key_state[SDL_SCANCODE_1])
        {
            ball_count = 1;
            g_goomba_ball2_movement_prev = g_goomba_ball2_movement;
            g_goomba_ball3_movement_prev = g_goomba_ball3_movement;
            
        }
        if (key_state[SDL_SCANCODE_2])
        {
            if (ball_count != 2) {
                ball_count = 2;
                g_goomba_ball2_movement = g_goomba_ball2_movement_prev;
                g_goomba_ball3_movement_prev = g_goomba_ball3_movement;
            }
            // otherwise they're spamming 2
        }
        if (key_state[SDL_SCANCODE_3])
        {
            if (ball_count != 3) {
                ball_count = 3;
                g_goomba_ball2_movement = g_goomba_ball2_movement_prev;
                g_goomba_ball3_movement = g_goomba_ball3_movement_prev;
            }
            // otherwise they're spamming 3
            
            
        }
        switch (event.type) {
            case SDL_QUIT: // key being tapped
                
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
            case SDL_KEYUP:
                g_player1_movement.y = 0.0f;
                if (single_player) { 
                    g_player2_movement.y = 1.0f;
                } else {
                    g_player2_movement.y = 0.0f;
                }
                
                break;
            case SDL_KEYDOWN: // key being held down
                switch (event.key.keysym.sym)
                    {
                        case SDLK_q:
                            g_app_status = TERMINATED;
                            break;
                        case SDLK_w:
                            g_player1_movement.y = 1.0f;
                            break;
                        case SDLK_s:
                            g_player1_movement.y = -1.0f;
                            break;
                        case SDLK_t: // toggle 1-2 player
                            single_player = !single_player;
                            break;
                        case SDLK_UP:
                            LOG(single_player);
                            if (!single_player) { // 2 player mode
                                g_player2_movement.y = 1.0f;
                            }
                            break;
                        case SDLK_DOWN:
                            LOG(single_player);
                            if (!single_player) { // 2 player mode
                                g_player2_movement.y = -1.0f;
                            } 
                            break;
                        default:
                            break;
                    }
        }
    }
}
// Initialize incremental variables
int ticks = 0;
int heartbeat_ticks = 0;
constexpr int   G_MAX_FRAME     = 40;
constexpr float G_GROWTH_FACTOR = 1.30f; // Grow by 30%
constexpr float G_SHRINK_FACTOR = 0.70f; // Shrink by 30%
constexpr float ROT_INCREMENT = 1.0f;
bool g_is_growing = true;
float ROT_ANGLE = 0.0f;
float prev_ticks = 0.0f;
float mario_angle = 0.0f;
float goomba_angle = 0.0f;
float mario_radius = 0.75f;
float goomba_radius = 1.5;
glm::vec3 mario_circular_translate = glm::vec3(0.0f);
glm::vec3 goomba_circular_translate = glm::vec3(0.0f);
glm::vec3 rotate_z_plane = glm::vec3(0.0f, 0.0f, 1.0f);
constexpr float g_player_speed = 1.5f;
constexpr float window_upper_bound = 3.75f;
constexpr float window_lower_bound = -3.75f;
constexpr float window_left_bound = -5.0f;
constexpr float window_right_bound = 5.0f;
// object dimensions
constexpr float ball_width = 0.2f;
constexpr float ball_height = 0.2f;
constexpr float paddle_width = 0.2f;
constexpr float paddle_height = 1.0f;
constexpr float ball_speed = 2.0f;
// ball 1
float x_distance_p1;
float y_distance_p1;
float x_distance_p2;
float y_distance_p2;
// ball 2
float x_distance2_p1;
float y_distance2_p1;
float x_distance2_p2;
float y_distance2_p2;
// ball 3
float x_distance3_p1;
float y_distance3_p1;
float x_distance3_p2;
float y_distance3_p2;
void update()
{
    /* Update incremental variables */
    float curr_ticks = (float) SDL_GetTicks() / 1000.0f;
    float delta_time = curr_ticks - prev_ticks;
    prev_ticks = curr_ticks;
    
    /* Transformations */
    ticks++;
    // Normalize
    if (glm::length(g_player1_movement) > 1.0f)
    {
        g_player1_movement = glm::normalize(g_player1_movement);
    }

    
    // Paddle 1 collisions with ball 1
    x_distance_p1 = fabs((g_player1_pos.x + g_paddle_scale.x * 0.025f) - (g_goomba_ball_pos.x + g_ball_scale.x)) - ((g_paddle_scale.x + g_ball_scale.x) / 2.0f);
    y_distance_p1 = fabs(((g_player1_pos.y + g_paddle_scale.y) - (g_goomba_ball_pos.y + g_paddle_scale.y))) - ((g_paddle_scale.y + g_ball_scale.y) / 2.0f);
    if (x_distance_p1 < 0 && y_distance_p1 < 0)
    {
        if (!ball_move_left) { // go right
            g_goomba_ball_movement.x = 1.0f; // sometimes ball glitch inside of the paddle
        } else { // go left
            g_goomba_ball_movement.x = -1.0f;
        }
        ball_move_left = !ball_move_left;
        LOG("Paddle 1 collision");
        LOG(x_distance_p1);
        LOG(y_distance_p1);
        

    }
    
    // Paddle 2 collisions with ball 1
    x_distance_p2 = fabs((g_player2_pos.x + g_paddle_scale.x * 0.025f) - (g_goomba_ball_pos.x + g_ball_scale.x)) - ((g_paddle_scale.x + g_ball_scale.x) / 2.0f);
    y_distance_p2 = fabs(((g_player2_pos.y + g_paddle_scale.y) - (g_goomba_ball_pos.y + g_paddle_scale.y))) - ((g_paddle_scale.y + g_ball_scale.y) / 2.0f);
    if (x_distance_p2 < 0 && y_distance_p2 < 0)
    {
        if (!ball_move_left) {
            g_goomba_ball_movement.x = -1.0f;
            
        } else {
            g_goomba_ball_movement.x = 1.0f;
        }
        ball_move_left = !ball_move_left;
        
        LOG("Paddle 2 collision");
    }
    // Paddle 1 collisions with ball 2
    x_distance2_p1 = fabs((g_player1_pos.x + g_paddle_scale.x * 0.025f) - (g_goomba_ball2_pos.x + g_ball_scale.x)) - ((g_paddle_scale.x + g_ball_scale.x) / 2.0f);
    y_distance2_p1 = fabs(((g_player1_pos.y + g_paddle_scale.y) - (g_goomba_ball2_pos.y + g_paddle_scale.y))) - ((g_paddle_scale.y + g_ball_scale.y) / 2.0f);
    if (x_distance2_p1 < 0 && y_distance2_p1 < 0)
    {
        if (!ball_move_left2) { // go right
            g_goomba_ball2_movement.x = 1.0f; // sometimes ball glitch inside of the paddle
        } else { // go left
            g_goomba_ball2_movement.x = -1.0f;
        }
        ball_move_left2 = !ball_move_left2;
        LOG("Paddle 1 collision 2");
    }
    
    // Paddle 2 collisions with ball 2
    x_distance2_p2 = fabs((g_player2_pos.x + g_paddle_scale.x * 0.025f) - (g_goomba_ball2_pos.x + g_ball_scale.x)) - ((g_paddle_scale.x + g_ball_scale.x) / 2.0f);
    y_distance2_p2 = fabs(((g_player2_pos.y + g_paddle_scale.y) - (g_goomba_ball2_pos.y + g_paddle_scale.y))) - ((g_paddle_scale.y + g_ball_scale.y) / 2.0f);
    if (x_distance2_p2 < 0 && y_distance2_p2 < 0)
    {
        if (!ball_move_left2) {
            g_goomba_ball2_movement.x = -1.0f;
            
        } else {
            g_goomba_ball2_movement.x = 1.0f;
        }
        ball_move_left2 = !ball_move_left2;
        
        LOG("Paddle 2 collision 2");
    }
    
    // Paddle 1 collisions with ball 3
    x_distance3_p1 = fabs((g_player1_pos.x + g_paddle_scale.x * 0.025f) - (g_goomba_ball3_pos.x + g_ball_scale.x)) - ((g_paddle_scale.x + g_ball_scale.x) / 2.0f);
    y_distance3_p1 = fabs(((g_player1_pos.y + g_paddle_scale.y * 0.025f) - (g_goomba_ball3_pos.y + g_paddle_scale.y))) - ((g_paddle_scale.y + g_ball_scale.y) / 2.0f);
    if (x_distance3_p1 < 0 && y_distance3_p1 < 0)
    {
        if (!ball_move_left3) { // go right
            g_goomba_ball3_movement.x = 1.0f; // sometimes ball glitch inside of the paddle
        } else { // go left
            g_goomba_ball3_movement.x = -1.0f;
        }
        ball_move_left3 = !ball_move_left3;
        LOG("Paddle 1 collision 3");
    }
    
    // Paddle 2 collisions with ball 3
    x_distance3_p2 = fabs((g_player2_pos.x + g_paddle_scale.x * 0.025f) - (g_goomba_ball3_pos.x + g_ball_scale.x)) - ((g_paddle_scale.x + g_ball_scale.x) / 2.0f);
    y_distance3_p2 = fabs(((g_player2_pos.y + g_paddle_scale.y * 0.025f) - (g_goomba_ball3_pos.y + g_paddle_scale.y))) - ((g_paddle_scale.y + g_ball_scale.y) / 2.0f);
    if (x_distance3_p2 < 0 && y_distance3_p2 < 0)
    {
        if (!ball_move_left3) {
            g_goomba_ball3_movement.x = -1.0f;
            
        } else {
            g_goomba_ball3_movement.x = 1.0f;
        }
        ball_move_left3 = !ball_move_left3;
        
        LOG("Paddle 2 collision 3");
    }
    
    
    
    
    // goomba ball bounds
    if (g_goomba_ball_pos.y <= window_lower_bound) { // goomba ball upper and lower bounds
        g_goomba_ball_pos.y = window_lower_bound;
        g_goomba_ball_movement.y *= -1.0f;
        LOG("Lower window hit");
    } else if (g_goomba_ball_pos.y >= window_upper_bound) {
        g_goomba_ball_pos.y = window_upper_bound;
        g_goomba_ball_movement.y *= -1.0f;
        left_or_right_hit = true;
        LOG("Upper window hit");
    }
    
    if (g_goomba_ball2_pos.y <= window_lower_bound) {
        g_goomba_ball2_pos.y = window_lower_bound;
        g_goomba_ball2_movement.y *= -1.0f;
        LOG("Lower window hit");
    } else if (g_goomba_ball2_pos.y >= window_upper_bound) {
        g_goomba_ball2_pos.y = window_upper_bound;
        g_goomba_ball2_movement.y *= -1.0f;
        left_or_right_hit2 = true;
        LOG("Upper window hit");
        
    }
    if (g_goomba_ball3_pos.y <= window_lower_bound) {
        g_goomba_ball3_pos.y = window_lower_bound;
        g_goomba_ball3_movement.y *= -1.0f;
        LOG("Lower window hit");
    } else if (g_goomba_ball3_pos.y >= window_upper_bound) {
        g_goomba_ball3_pos.y = window_upper_bound;
        g_goomba_ball3_movement.y *= -1.0f;
        left_or_right_hit3 = true;
        LOG("Upper window hit");
        
    }
    // goomba ball left and right bounds
    if (g_goomba_ball_pos.x <= window_left_bound) { // player 2 wins
        g_goomba_ball_pos.x = window_left_bound;
        g_goomba_ball_movement.x *= -1.0f;
        LOG("Left window hit");
        LOG(g_goomba_ball_pos.x);
        left_or_right_hit = true;
    } else if (g_goomba_ball_pos.x >= window_right_bound) { // player 1 wins
        g_goomba_ball_pos.x = window_right_bound;
        g_goomba_ball_movement.x *= -1.0f;
        LOG("Right window hit");
        LOG(g_goomba_ball_pos.x);
        left_or_right_hit = true;
    }
    
    if (g_goomba_ball2_pos.x <= window_left_bound) { // player 2 wins
        g_goomba_ball2_pos.x = window_left_bound;
        g_goomba_ball2_movement.x *= -1.0f;
        LOG("Left window hit");
        LOG(g_goomba_ball2_pos.x);
        left_or_right_hit2 = true;
    } else if (g_goomba_ball2_pos.x >= window_right_bound) { // player 1 wins
        g_goomba_ball2_pos.x = window_right_bound;
        g_goomba_ball2_movement.x *= -1.0f;
        LOG("Right window hit");
        LOG(g_goomba_ball2_pos.x);
        left_or_right_hit2 = true;
    }
    
    if (g_goomba_ball3_pos.x <= window_left_bound) { // player 2 wins
        g_goomba_ball3_pos.x = window_left_bound;
        g_goomba_ball3_movement.x *= -1.0f;
        LOG("Left window hit");
        LOG(g_goomba_ball3_pos.x);
        left_or_right_hit3 = true;
    } else if (g_goomba_ball3_pos.x >= window_right_bound) { // player 1 wins
        g_goomba_ball3_pos.x = window_right_bound;
        g_goomba_ball3_movement.x *= -1.0f;
        LOG("Right window hit");
        LOG(g_goomba_ball3_pos.x);
        left_or_right_hit3 = true;
    }
    
    // paddle bounds
    if (g_player1_pos.y <= window_lower_bound) { // player 1 paddle bounds
        g_player1_pos.y = window_lower_bound;
    } else if (g_player1_pos.y - g_paddle_scale.y >= window_upper_bound) {
        g_player1_pos.y = window_upper_bound;
        g_player1_movement.y *= -1.0f;
    }
    
    if (g_player2_pos.y <= window_lower_bound) { // player 2 paddle bounds
        g_player2_pos.y = window_lower_bound;
        g_player2_movement.y *= -1.0f;
    } else if (g_player2_pos.y >= window_upper_bound) {
        g_player2_pos.y = window_upper_bound;
        g_player2_movement.y *= -1.0f;
    }
    
    if (glm::length(g_player2_movement) > 1.0f)
    {
        g_player2_movement = glm::normalize(g_player2_movement);
    }
    g_player1_pos += g_player1_movement * (g_player_speed * delta_time); // Gliding behavior? Fixed
    g_player2_pos += g_player2_movement * (g_player_speed * delta_time);
    

    g_goomba_ball_pos += g_goomba_ball_movement * (ball_speed * delta_time);
    
    if (ball_count == 2) {
        g_goomba_ball2_pos += g_goomba_ball2_movement * (ball_speed * delta_time);
    }
    else if (ball_count == 3) {
        g_goomba_ball2_pos += g_goomba_ball2_movement * (ball_speed * delta_time);
        g_goomba_ball3_pos += g_goomba_ball3_movement * (ball_speed * delta_time);
    }
    
    
    g_mario_matrix_l = glm::mat4(1.0f);
    g_mario_matrix_r = glm::mat4(1.0f);
    g_goomba_matrix_m = glm::mat4(1.0f);
    g_goomba_ball_matrix = glm::mat4(1.0f);
    g_goomba_ball2_matrix = glm::mat4(1.0f);
    g_goomba_ball3_matrix = glm::mat4(1.0f);
    
    // move items if game is still running
    g_mario_matrix_l = glm::translate(g_mario_matrix_l, g_player1_pos);
    g_mario_matrix_r = glm::translate(g_mario_matrix_r, g_player2_pos);
    g_goomba_matrix_m = glm::translate(g_goomba_matrix_m, middle_line_pos);
    g_goomba_ball_matrix = glm::translate(g_goomba_ball_matrix, g_goomba_ball_pos);

    
    g_mario_matrix_l = glm::scale(g_mario_matrix_l, g_paddle_scale);
    g_mario_matrix_r = glm::scale(g_mario_matrix_r, g_paddle_scale);
    g_goomba_matrix_m = glm::scale(g_goomba_matrix_m, g_divider_scale);
    g_goomba_ball_matrix = glm::scale(g_goomba_ball_matrix, g_ball_scale);
    
    if (ball_count == 2) {
        g_goomba_ball2_matrix = glm::translate(g_goomba_ball2_matrix, g_goomba_ball2_pos);
        g_goomba_ball2_matrix = glm::scale(g_goomba_ball2_matrix, g_ball_scale);
    }
    if (ball_count == 3) {
        g_goomba_ball2_matrix = glm::translate(g_goomba_ball2_matrix, g_goomba_ball2_pos);
        g_goomba_ball3_matrix = glm::translate(g_goomba_ball3_matrix, g_goomba_ball3_pos);
        
        g_goomba_ball2_matrix = glm::scale(g_goomba_ball2_matrix, g_ball_scale);
        g_goomba_ball3_matrix = glm::scale(g_goomba_ball3_matrix, g_ball_scale);
    }
    

}
void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}
void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    // Vertices
    float vertices[] = // Paddle Vertices , width = 0.2f , height = 1.0f
    {
        -0.1f, -0.5f,   0.1f, -0.5f,   0.1f,  0.5f,   // Object 1 (right half of the paddle)
        -0.1f, -0.5f,   0.1f,  0.5f,  -0.1f,  0.5f    // Object 2 (left half of the paddle)
    };
    
    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // Map Texture 1 to Vertices 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, // set shape to paddle
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    draw_object(g_mario_matrix_l, g_mario_texture_id); // draw the paddles
    draw_object(g_mario_matrix_r, g_mario_texture_id);
    
    
    draw_object(g_goomba_matrix_m, g_goomba_texture_id); // draw the divider
    
    
    if (ball_count == 1) {
        draw_object(g_goomba_ball_matrix, g_goomba_texture_id); // draw the ball
    }
    else if (ball_count == 2) {
        draw_object(g_goomba_ball_matrix, g_goomba_texture_id); // draw the ball
        draw_object(g_goomba_ball2_matrix, g_goomba_texture_id);
    }
    else if (ball_count == 3) {
        draw_object(g_goomba_ball_matrix, g_goomba_texture_id); // draw the ball
        draw_object(g_goomba_ball2_matrix, g_goomba_texture_id);
        draw_object(g_goomba_ball3_matrix, g_goomba_texture_id); // draw the ball
    }
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    SDL_GL_SwapWindow(g_display_window);
}
void shutdown() { SDL_Quit(); }
int main(int argc, char* argv[])
{
    initialise();
    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }
    shutdown();
    return 0;
}
