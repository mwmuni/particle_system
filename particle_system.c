#define SDL_MAIN_HANDLED

// Include the appropriate SDL header based on the platform
#ifdef _WIN32
#include <SDL.h>
#include <wtypes.h>
#include <windows.h>
#include <process.h>
#else
#include <SDL2/SDL.h>
#include <pthread.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SIMULATION_RANGE 5.0f
#define NUM_PARTICLES 2000
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define REPULSION_STRENGTH 0.3f
#define MIN_DISTANCE 0.1f
#define BASE_PARTICLE_SIZE 1.0f
#define NUM_THREADS 32

typedef struct {
    float x, y;
    float vx, vy;
    float radius;
    SDL_Color color;
} Particle;

typedef struct {
    Particle* particles;
    int start;
    int end;
    float dt;
} ThreadData;

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

void init_particles(Particle* particles) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x = ((float)rand() / RAND_MAX) * SIMULATION_RANGE * 2 - SIMULATION_RANGE;
        particles[i].y = ((float)rand() / RAND_MAX) * SIMULATION_RANGE * 2 - SIMULATION_RANGE;
        particles[i].vx = ((float)rand() / RAND_MAX) * 1 - 0.5;
        particles[i].vy = ((float)rand() / RAND_MAX) * 1 - 0.5;
        particles[i].radius = BASE_PARTICLE_SIZE;
        particles[i].color.r = rand() % 256;
        particles[i].color.g = rand() % 256;
        particles[i].color.b = rand() % 256;
        particles[i].color.a = 255;
    }
}

void* update_particles_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    Particle* particles = data->particles;
    int start = data->start;
    int end = data->end;
    float dt = data->dt;

    for (int i = start; i < end; i++) {
        float total_force_x = 0;
        float total_force_y = 0;

        for (int j = 0; j < NUM_PARTICLES; j++) {
            if (i != j) {
                float dx = particles[i].x - particles[j].x;
                float dy = particles[i].y - particles[j].y;
                float distance = sqrt(dx * dx + dy * dy);

                if (distance < MIN_DISTANCE) {
                    distance = MIN_DISTANCE;
                }

                float force = REPULSION_STRENGTH / (distance * distance);
                total_force_x += force * dx / distance;
                total_force_y += force * dy / distance;
            }
        }

        particles[i].vx += total_force_x * dt;
        particles[i].vy += total_force_y * dt;
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;

        if (particles[i].x < -SIMULATION_RANGE || particles[i].x > SIMULATION_RANGE) {
            particles[i].vx *= -1;
            particles[i].x = particles[i].x < 0 ? -SIMULATION_RANGE : SIMULATION_RANGE;
        }
        if (particles[i].y < -SIMULATION_RANGE || particles[i].y > SIMULATION_RANGE) {
            particles[i].vy *= -1;
            particles[i].y = particles[i].y < 0 ? -SIMULATION_RANGE : SIMULATION_RANGE;
        }
    }

    return NULL;
}

void update_particles(Particle* particles, float dt) {
#ifdef _WIN32
    HANDLE threads[NUM_THREADS];
#else
    pthread_t threads[NUM_THREADS];
#endif
    ThreadData thread_data[NUM_THREADS];

    int particles_per_thread = NUM_PARTICLES / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].particles = particles;
        thread_data[i].start = i * particles_per_thread;
        thread_data[i].end = (i == NUM_THREADS - 1) ? NUM_PARTICLES : (i + 1) * particles_per_thread;
        thread_data[i].dt = dt;

#ifdef _WIN32
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall*)(void*))update_particles_thread, &thread_data[i], 0, NULL);
#else
        pthread_create(&threads[i], NULL, update_particles_thread, &thread_data[i]);
#endif
    }

    for (int i = 0; i < NUM_THREADS; i++) {
#ifdef _WIN32
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
#else
        pthread_join(threads[i], NULL);
#endif
    }
}

void render_particles(SDL_Renderer* renderer, Particle* particles) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    for (int i = 0; i < NUM_PARTICLES; i++) {
        SDL_SetRenderDrawColor(renderer, particles[i].color.r, particles[i].color.g, particles[i].color.b, particles[i].color.a);
        
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

#ifdef _WIN32
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char* args[])
#endif
{
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
        const Uint32 target_frame_time = 1000 / 144;

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
