#pragma once
#include <SFML\Graphics.hpp>
using sf::Drawable;
using sf::RenderWindow;
using sf::VideoMode;
using sf::Event;
#include <vector>
using std::vector;

#include "bnLayered.h"

class Renderer
{
public:
	static Renderer& GetInstance();
	void Initialise();
	void Draw(Drawable& _drawable);
	void Draw(Drawable* _drawable);
	void Draw(vector<Drawable*> _drawable);
	void Display();
	bool Running();
	void Clear();
	RenderWindow* GetWindow() const;

	void Push(LayeredDrawable* _drawable);
	void DrawLayers();
	void Lay(Drawable* _drawable);
	void Lay(vector<Drawable*> _drawable);
	void DrawOverlay();
	void Pose(Drawable* _drawable);
	void DrawUnderlay();

private:
	Renderer(void);
	~Renderer(void);

	RenderWindow* window;
	Underlay underlay;
	Layers layers;
	Overlay overlay;
};