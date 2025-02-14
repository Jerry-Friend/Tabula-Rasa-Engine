#include "trDefs.h"
#include "trApp.h"
#include "trWindow.h"
#include "SDL/include/SDL.h"

trWindow::trWindow() : trModule()
{
	name = "Window";
	window = NULL;
	screen_surface = NULL;
}

// Destructor
trWindow::~trWindow()
{
}

// Called before render is available
bool trWindow::Awake(JSON_Object* config)
{
	TR_LOG("trWindow: Init SDL window & surface");
	bool ret = true;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		TR_LOG("trWindow: SDL_VIDEO could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		if (config != nullptr) {
			this->SetWidth(json_object_get_number(config, "width"));
			this->SetHeight(json_object_get_number(config, "height"));
			this->SetScale(json_object_get_number(config, "scale"));
			this->SetFullscreen(json_object_get_boolean(config, "fullscreen"));
			this->SetResizable(json_object_get_boolean(config, "resizable"));
			this->SetBorderless(json_object_get_boolean(config, "borderless"));
			this->SetFullscreenWindowed(json_object_get_boolean(config, "fullscreen_window"));
		}
		else {
			this->SetWidth(W_WIDTH);
			this->SetHeight(W_HEIGHT);
			this->SetScale(W_SCALE);
			this->SetFullscreen(W_FULLSCREEN);
			this->SetResizable(W_RESIZABLE);
			this->SetBorderless(W_BORDERLESS);
			this->SetFullscreenWindowed(W_FULLSCREEN_DESKTOP);
		}
		
		//Create window
		Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

		//Use OpenGL 2.1
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

		flags |= SDL_WINDOW_MAXIMIZED;

		if (fullscreen)
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		if (resizable)
		{
			flags |= SDL_WINDOW_RESIZABLE;
		}

		if (borderless)
		{
			flags |= SDL_WINDOW_BORDERLESS;
		}

		if (fullscreen_desktop)
		{
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		}

		window = SDL_CreateWindow(App->GetTitle(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
		int w = 0, h = 0;
		SDL_GetWindowSize(window, &w, &h);
		width = w;
		height = h;
		SDL_SetWindowSize(window, width, height);

		if (window == NULL)
		{
			TR_LOG("trWindow: Window could not be created! SDL_Error: %s\n", SDL_GetError());
			ret = false;
		}
		else
		{
			//Get window surface
			screen_surface = SDL_GetWindowSurface(window);
		}
	}

	return ret;
}

// Called before quitting
bool trWindow::CleanUp()
{
	TR_LOG("trWindow: CleanUp");

	//Destroy window
	if (window != NULL)
	{
		SDL_DestroyWindow(window);
	}

	//Quit SDL subsystems
	SDL_Quit();
	return true;
}

bool trWindow::Load(const JSON_Object * config)
{
	if (config != nullptr) {
		this->SetWidth(json_object_get_number(config, "width"));
		this->SetHeight(json_object_get_number(config, "height"));
		this->SetScale(json_object_get_number(config, "scale"));
		this->SetFullscreen(json_object_get_boolean(config, "fullscreen"));
		this->SetResizable(json_object_get_boolean(config, "resizable"));
		this->SetBorderless(json_object_get_boolean(config, "borderless"));
		this->SetFullscreenWindowed(json_object_get_boolean(config, "fullscreen_window"));
	}
	else {
		this->SetWidth(W_WIDTH);
		this->SetHeight(W_HEIGHT);
		this->SetScale(W_SCALE);
		this->SetFullscreen(W_FULLSCREEN);
		this->SetResizable(W_RESIZABLE);
		this->SetBorderless(W_BORDERLESS);
		this->SetFullscreenWindowed(W_FULLSCREEN_DESKTOP);
	}
	return true;
}

bool trWindow::Save(JSON_Object * config) const
{
	json_object_set_number(config, "width", width);
	json_object_set_number(config, "height", height);
	json_object_set_number(config, "scale", scale);
	json_object_set_boolean(config, "fullscreen", fullscreen);
	json_object_set_boolean(config, "borderless", borderless);
	json_object_set_boolean(config, "resizable", resizable);
	json_object_set_boolean(config, "fullscreen_window", fullscreen_desktop);

	return true;
}

void trWindow::SetTitle(const char* title)
{
	SDL_SetWindowTitle(window, title);
	App->SetTitle(title);
}

void trWindow::SetBrightness(float set)
{
	CAP(set);
	if (SDL_SetWindowBrightness(window, set) != 0)
		TR_LOG("trWindow: Could not change window brightness: %s\n", SDL_GetError());
}

float trWindow::GetBrightness() const
{
	return SDL_GetWindowBrightness(window);
}

void trWindow::SetWidth(uint width)
{
	SDL_SetWindowSize(window, width, height);
	this->width = width;
}

void trWindow::SetHeight(uint height)
{
	SDL_SetWindowSize(window, width, height);
	this->height = height;
}

void trWindow::SetScale(uint scale)
{
	this->scale = scale;
}

uint trWindow::GetMonitorRefreshRate() const
{
	SDL_DisplayMode display_mode;
	if (SDL_GetDesktopDisplayMode(0, &display_mode) != 0)
		TR_LOG("trWindow: Error - SDL_GetDesktopDisplayMode %s", SDL_GetError());
	else
		return display_mode.refresh_rate;

	return 0;
}

SDL_Window * trWindow::GetWindow() const
{
	return window;
}

void trWindow::SetFullscreen(bool fullscreen)
{
	this->fullscreen = fullscreen;
	if (fullscreen)
	{
		if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0)
			TR_LOG("trWindow: Error switching to fullscreen: %s\n", SDL_GetError());
	}
	else {
		if (SDL_SetWindowFullscreen(window, SDL_WINDOW_MAXIMIZED) != 0)
			TR_LOG("trWindow: Error switching to fullscreen: %s\n", SDL_GetError());
	}
}

void trWindow::SetResizable(bool resizable)
{
	this->resizable= resizable;
}

void trWindow::SetBorderless(bool borderless)
{
	this->borderless = borderless;
	if(borderless)
		SDL_SetWindowBordered(window, (SDL_bool)borderless);
}

void trWindow::SetFullscreenWindowed(bool fullscreen_desktop)
{
	this->fullscreen_desktop = fullscreen_desktop;
	if (fullscreen_desktop)
	{
		if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
			TR_LOG("trWindow: Error switching to fullscreen desktop: %s\n", SDL_GetError());
	}
	else {
		if (SDL_SetWindowFullscreen(window, SDL_WINDOW_MAXIMIZED) != 0)
			TR_LOG("trWindow: Error switching to fullscreen: %s\n", SDL_GetError());
	}
}

uint trWindow::GetWidth() const
{
	return width * scale;
}

uint trWindow::GetHeight() const
{
	return height * scale;
}

uint trWindow::GetScale() const
{
	return scale;
}
