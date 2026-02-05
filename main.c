#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <wayland-client.h>
#include <wlr-layer-shell-unstable-v1-client-protocol.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#include "bubbles.h"

SDL_Window *window;
SDL_Renderer *renderer;

struct {
    /* These are owned by SDL and must not be destroyed! */
    struct wl_display *wl_display;
    struct wl_surface *wl_surface;

    struct zwlr_layer_surface_v1 *layer_surface;
	bool configured;

    /* These are owned by the application and need to be cleaned up on exit. */
    struct wl_registry *wl_registry;
    struct zwlr_layer_shell_v1 *layer_shell;
} wl_state = {0};

struct _app app = {0};

/*
  wayland
*/
static void noop () {}

static void registry_handle_global (void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	if ( strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0 )
		wl_state.layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = (void (*)(void *, struct wl_registry *, uint32_t))noop,
};

static void layer_surface_handle_configure (void *data, struct zwlr_layer_surface_v1 *layer_surface,
		uint32_t serial, uint32_t width, uint32_t height)
{
	(void)data;
	(void)layer_surface;

    SDL_SetWindowSize(window, width, height);

	zwlr_layer_surface_v1_ack_configure(wl_state.layer_surface, serial);
    wl_state.configured = true;
    SDL_Log("layer_surface ack %d:%d", width, height);
}


const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_handle_configure,
	.closed    = (void (*)(void *data, struct zwlr_layer_surface_v1 *layer_surface))noop,
};

/*
  main
*/

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    // https://jackjamison.net/blog/sdl-native-wayland
    for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
        SDL_Log("%s", SDL_GetVideoDriver(i));
        if (strcmp(SDL_GetVideoDriver(i), "wayland") == 0) {
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");
            break;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Video driver must be 'wayland', not '%s'", SDL_GetCurrentVideoDriver());
        return SDL_APP_FAILURE;
    }

    /* Create a window with the custom surface role property set. */
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_WAYLAND_SURFACE_ROLE_CUSTOM_BOOLEAN, true);   /* Roleless surface */
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);                        /* OpenGL enabled */
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_TRANSPARENT_BOOLEAN, true);                   /* Transparent window */
    //SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, WINDOW_WIDTH);                       /* Default width */
    //SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, WINDOW_HEIGHT);                     /* Default height */
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);            /* Handle DPI scaling internally */
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Wayland custom surface role test"); /* Default title */

    window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation failed");
        return SDL_APP_FAILURE;
    }

    /* Create the renderer */
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Renderer creation failed");
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    app.textures[0] = IMG_LoadTexture(renderer, "img/bubble-blue.png");
    app.textures[1] = IMG_LoadTexture(renderer, "img/bubble-purple.png");
    app.textures[2] = IMG_LoadTexture(renderer, "img/bubble-red.png");
    assert(app.textures[0] && app.textures[1] && app.textures[2]);

    /* Get the display object and use it to create a registry object, which will enumerate the xdg_wm_base protocol. */
    wl_state.wl_display = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
    if (!wl_state.wl_display) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to get wl_display");
        return SDL_APP_FAILURE;
    }
    wl_state.wl_registry = wl_display_get_registry(wl_state.wl_display);
    wl_registry_add_listener(wl_state.wl_registry, &registry_listener, NULL);

    /* Roundtrip to enumerate registry objects. */
    wl_display_roundtrip(wl_state.wl_display);

    /* Get the wl_surface object from the SDL_Window */
    wl_state.wl_surface = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);

    /* Create layer_surface from wl_surface */
    wl_state.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
		wl_state.layer_shell,
		wl_state.wl_surface,
		NULL,
		ZWLR_LAYER_SHELL_V1_LAYER_TOP,
		"bubbles"
	);

    zwlr_layer_surface_v1_add_listener(
        wl_state.layer_surface,
        &layer_surface_listener,
        NULL
    );

    zwlr_layer_surface_v1_set_size(
		wl_state.layer_surface,
        0,0
	);
	zwlr_layer_surface_v1_set_margin(
		wl_state.layer_surface,
        0,0,0,0
	);
	zwlr_layer_surface_v1_set_anchor(
		wl_state.layer_surface,
		ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
	);

    zwlr_layer_surface_v1_set_keyboard_interactivity(
        wl_state.layer_surface,
        1
    );

    zwlr_layer_surface_v1_set_exclusive_zone(wl_state.layer_surface, -1);

    wl_surface_commit(wl_state.wl_surface);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e)
{
    switch (e->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
        if (e->key.key == SDLK_ESCAPE || e->key.key == SDLK_SPACE)
            return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        int w, h;
        SDL_GetRenderOutputSize(renderer, &w, &h);
        app.w = (float)w;
        app.h = (float)h;
        break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_Delay(1000/60);

    if (!wl_state.configured)
        return SDL_APP_CONTINUE;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
    SDL_RenderClear(renderer);

    if (app.i < NBUBBLES && (SDL_GetTicks() / 800) > app.i) {
        app.bubbles[app.i].pos.x = 0;
        app.bubbles[app.i].pos.y = app.h - BUBBLE_SIZE;
        app.bubbles[app.i].pos.w = BUBBLE_SIZE;
        app.bubbles[app.i].pos.h = BUBBLE_SIZE;
        app.bubbles[app.i].vx = SDL_randf() * app.h / BUBBLE_SIZE + 0.5;
        app.bubbles[app.i].vy = -(SDL_randf() * app.h / BUBBLE_SIZE  + 0.5);
        app.bubbles[app.i].color = app.textures[rand() % 3];

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderTexture(renderer, app.bubbles[app.i].color, NULL, &app.bubbles[app.i].pos);

        app.i++;
    }


    for (int i = 0; i < app.i; ++i) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderTexture(renderer, app.bubbles[i].color, NULL, &app.bubbles[i].pos);

        if (app.bubbles[i].pos.x < 0 || app.bubbles[i].pos.x + BUBBLE_SIZE > app.w)
            app.bubbles[i].vx *= -1.0;

        if (app.bubbles[i].pos.y < 0 || app.bubbles[i].pos.y + BUBBLE_SIZE > app.h)
            app.bubbles[i].vy *= -1.0;

        if (has_collision(&app, i))
            resolve_collision(&app, i);

        app.bubbles[i].pos.x += app.bubbles[i].vx;
        app.bubbles[i].pos.y += app.bubbles[i].vy;
    }

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (wl_state.layer_surface)
        zwlr_layer_surface_v1_destroy(wl_state.layer_surface);
    
}
