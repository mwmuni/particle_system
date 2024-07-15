#define SDL_MAIN_HANDLED

// if you are using SDL2, you should include SDL2/SDL.h on linux, and SDL.h on windows
#ifdef _WIN32
#include <SDL.h>
#endif
#ifdef __linux__
#include <SDL2/SDL.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SIMULATION_RANGE 5.0f
#define NUM_PARTICLES 200
// #define WINDOW_WIDTH 800
// #define WINDOW_HEIGHT 600
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define REPULSION_STRENGTH 0.3f
#define MIN_DISTANCE 0.1f
#define BASE_PARTICLE_SIZE 1.0f


typedef struct {
    float x, y;
    float vx, vy;
    float radius;
    SDL_Color color;
} Particle;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

void init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    
    window = SDL_CreateWindow("Particle System", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
}

// Update the particle initialization
void init_particles(Particle* particles) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x = ((float)rand() / RAND_MAX) * SIMULATION_RANGE * 2 - SIMULATION_RANGE;
        particles[i].y = ((float)rand() / RAND_MAX) * SIMULATION_RANGE * 2 - SIMULATION_RANGE;
        particles[i].vx = ((float)rand() / RAND_MAX) * 1 - 0.5;  // Reduced velocity range
        particles[i].vy = ((float)rand() / RAND_MAX) * 1 - 0.5;  // Reduced velocity range
        particles[i].radius = BASE_PARTICLE_SIZE;  // Increased particle size for visibility
        particles[i].color.r = rand() % 256;
        particles[i].color.g = rand() % 256;
        particles[i].color.b = rand() % 256;
        particles[i].color.a = 255;
    }
}

// Update the particle update function
void update_particles(Particle* particles, float dt) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        float total_force_x = 0;
        float total_force_y = 0;

        // Calculate repulsion forces from other particles
        for (int j = 0; j < NUM_PARTICLES; j++) {
            if (i != j) {
                float dx = particles[i].x - particles[j].x;
                float dy = particles[i].y - particles[j].y;
                float distance = sqrt(dx*dx + dy*dy);

                if (distance < MIN_DISTANCE) {
                    distance = MIN_DISTANCE;  // Prevent division by zero
                }

                float force = REPULSION_STRENGTH / (distance * distance);
                total_force_x += force * dx / distance;
                total_force_y += force * dy / distance;
            }
        }

        // Update velocity based on repulsion forces
        particles[i].vx += total_force_x * dt;
        particles[i].vy += total_force_y * dt;

        // Update position
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;

        // Bounce off walls (keep this part from the original function)
        if (particles[i].x < -SIMULATION_RANGE || particles[i].x > SIMULATION_RANGE) {
            particles[i].vx *= -1;
            particles[i].x = particles[i].x < 0 ? -SIMULATION_RANGE : SIMULATION_RANGE;
        }
        if (particles[i].y < -SIMULATION_RANGE || particles[i].y > SIMULATION_RANGE) {
            particles[i].vy *= -1;
            particles[i].y = particles[i].y < 0 ? -SIMULATION_RANGE : SIMULATION_RANGE;
        }
    }
}


// Update the rendering function to map simulation coordinates to screen coordinates
void render_particles(SDL_Renderer* renderer, Particle* particles) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    for (int i = 0; i < NUM_PARTICLES; i++) {
        SDL_SetRenderDrawColor(renderer, particles[i].color.r, particles[i].color.g, particles[i].color.b, particles[i].color.a);
        
        // Map simulation coordinates to screen coordinates
        int screen_x = (int)((particles[i].x + SIMULATION_RANGE) / (2 * SIMULATION_RANGE) * WINDOW_WIDTH);
        int screen_y = (int)((particles[i].y + SIMULATION_RANGE) / (2 * SIMULATION_RANGE) * WINDOW_HEIGHT);
        
        SDL_Rect rect = {
            screen_x - (int)particles[i].radius,
            screen_y - (int)particles[i].radius,
            (int)(particles[i].radius * 2),
            (int)(particles[i].radius * 2)
        };
        SDL_RenderFillRect(renderer, &rect);
    }
    
    SDL_RenderPresent(renderer);
}

int main(int argc, char* args[]) {
    srand(time(NULL));
    init_sdl();
    
    Particle particles[NUM_PARTICLES];
    init_particles(particles);
    
    SDL_Event e;
    int quit = 0;
    Uint32 last_time = SDL_GetTicks();
    
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }
        
        Uint32 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        
        update_particles(particles, dt);
        render_particles(renderer, particles);
        
        Uint32 frame_time = SDL_GetTicks() - last_time;
        const Uint32 target_frame_time = 1000 / 144; // Target frame time in milliseconds

        if (frame_time < target_frame_time) {
            Uint32 delay_time = target_frame_time - frame_time;
            SDL_Delay(delay_time);
        }

        float actual_frame_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        update_particles(particles, actual_frame_time);
        render_particles(renderer, particles);
        if (frame_time < target_frame_time) {
            SDL_Delay(target_frame_time - frame_time);
        }
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
