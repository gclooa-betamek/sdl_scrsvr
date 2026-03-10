/** SCRSVR
 *  @file   scrsvr.c
 *  @author looa.dev
 *  @date   2026-03-04
 *  @brief  A simple SDL3 screensaver.
 */

#define SDL_MAIN_USE_CALLBACKS 1  /* Use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define WINDOW_WIDTH_DEFAULT 640
#define WINDOW_HEIGHT_DEFAULT 480

/* Pointer to display bounds and arrays */
static SDL_Rect *bounds = NULL;
static SDL_Window **windows = NULL;
static SDL_Renderer **renderers = NULL;
static SDL_Texture **textures = NULL;

static int texture_width = 0;
static int texture_height = 0;
static int display_count = 0;

/* Cursor state */
static Uint32 last_input_ticks = 0;
static bool cursor_hidden = false;
static bool window_focused = true;

SDL_AppResult SDL_AppFailure(const char *error)
{
    SDL_Log(error, SDL_GetError());
    return SDL_APP_FAILURE;
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("scrsvr", "1.0", "dev.looa.screensaver");

    if (!SDL_Init(SDL_INIT_VIDEO)) SDL_AppFailure("Failed to initialize SDL: %s");

    /* Get display dimensions */
    SDL_DisplayID *displays = SDL_GetDisplays(&display_count);

    /* Allocate arrays for windows, renderers, textures */
    bounds = (SDL_Rect*)malloc(display_count * sizeof(SDL_Rect));
    windows = (SDL_Window**)malloc(display_count * sizeof(SDL_Window*));
    renderers = (SDL_Renderer**)malloc(display_count * sizeof(SDL_Renderer*));
    textures = (SDL_Texture**)malloc(display_count * sizeof(SDL_Texture*));

    for (int i = 0; i < display_count; i++) {
        SDL_GetDisplayBounds(displays[i], &bounds[i]);

        windows[i] = SDL_CreateWindow( 
            "SCRSVR", 
            bounds[i].w, 
            bounds[i].h, 
            SDL_WINDOW_BORDERLESS
        );
        SDL_SetWindowPosition(windows[i], bounds[i].x, bounds[i].y);

        renderers[i] = SDL_CreateRenderer(windows[i], NULL);
        SDL_SetRenderLogicalPresentation(
            renderers[i],
            bounds[i].w,
            bounds[i].h,
            SDL_LOGICAL_PRESENTATION_LETTERBOX
        );
        SDL_SetRenderVSync(renderers[i], 1);

        SDL_Surface *surface = NULL;
        char *png_path = NULL;
        SDL_asprintf(&png_path, "%simage.png", SDL_GetBasePath());
        surface = SDL_LoadPNG(png_path);
        if (!surface) SDL_AppFailure("Failed to load image: %s");

        textures[i] = SDL_CreateTextureFromSurface(renderers[i], surface);
        if (!textures[i]) SDL_AppFailure("Failed to create texture: %s");
        texture_width = surface->w;
        texture_height = surface->h;
        
        SDL_DestroySurface(surface);
        SDL_free(displays);
        SDL_free(png_path);
    }
    /* Initialize cursor/input timers */
    last_input_ticks = SDL_GetTicks();
    cursor_hidden = false;
    window_focused = true;
    SDL_ShowCursor();
    
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    switch (event->type) {
    case SDL_EVENT_KEY_DOWN:
        if (event->key.scancode == SDL_SCANCODE_ESCAPE) return SDL_APP_SUCCESS;
        last_input_ticks = SDL_GetTicks();
        if (cursor_hidden && window_focused) {
            SDL_ShowCursor();
            cursor_hidden = false;
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_WHEEL:
        last_input_ticks = SDL_GetTicks();
        if (cursor_hidden && window_focused) {
            SDL_ShowCursor();
            cursor_hidden = false;
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        window_focused = true;
        last_input_ticks = SDL_GetTicks();
        if (cursor_hidden) {
            SDL_ShowCursor();
            cursor_hidden = false;
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        window_focused = false;
        if (cursor_hidden) {
            SDL_ShowCursor();
            cursor_hidden = false;
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_FRect image_frame;

    for (int i = 0; i < display_count; i++) {
        SDL_RenderClear(renderers[i]);
        image_frame.x = ((float) (bounds[i].w - texture_width)) / 2.0f;
        image_frame.y = ((float) (bounds[i].h - texture_height)) / 2.0f;
        image_frame.w = (float) texture_width;
        image_frame.h = (float) texture_height;
        SDL_RenderTexture(renderers[i], textures[i], NULL, &image_frame);
        SDL_RenderPresent(renderers[i]);
    }
    /* Hide cursor if idle for 1s and window is focused */
    bool cursor_idle = (SDL_GetTicks() - last_input_ticks) >= 1000;
    if (window_focused && !cursor_hidden && cursor_idle) {
        SDL_HideCursor();
        cursor_hidden = true;
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    for (int i = 0; i < display_count; i++) {
        SDL_DestroyTexture(textures[i]);
        SDL_DestroyRenderer(renderers[i]);
        SDL_DestroyWindow(windows[i]);
    }
    SDL_free(bounds);
    SDL_free(windows);
    SDL_free(renderers);
    SDL_free(textures);

    SDL_Quit();
}