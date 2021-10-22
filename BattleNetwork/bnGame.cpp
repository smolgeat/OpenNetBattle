#include <time.h>
#include <queue>
#include <atomic>
#include <cmath>
#include <Swoosh/ActivityController.h>
#include <Swoosh/Ease.h>

#include "bnGame.h"
#include "bnGameSession.h"
#include "bnPlayerPackageManager.h"
#include "bnCardPackageManager.h"
#include "bnMobPackageManager.h"
#include "bnBlockPackageManager.h"
#include "bnLuaLibraryPackageManager.h"
#include "bnGameOverScene.h"
#include "bnFakeScene.h"
#include "bnConfigReader.h"
#include "bnFont.h"
#include "bnText.h"
#include "bnQueueMobRegistration.h"
#include "bnQueueNaviRegistration.h"
#include "bnQueueCardRegistration.h"
#include "bindings/bnQueueLuaLibraryRegistration.h"
#include "bnQueueBlockRegistration.h"
#include "bnResourceHandle.h"
#include "bnInputHandle.h"
#include "overworld/bnOverworldHomepage.h"
#include "SFML/System.hpp"

Game::Game(DrawWindow& window) :
  window(window), 
  reader("config.ini"),
  configSettings(),
  netManager(),
  textureManager(),
  audioManager(),
  shaderManager(),
  inputManager(*window.GetRenderWindow()),
  ActivityController(*window.GetRenderWindow()) {
  
  // figure out system endianness
  {
    int n = 1;
    // little endian if true
    if (*(char*)&n == 1) { endian = Endianness::little; }
  }

  // Link the resource handle to use all the manangers created by the game
  ResourceHandle::audio    = &audioManager;
  ResourceHandle::textures = &textureManager;
  ResourceHandle::shaders  = &shaderManager;

#ifdef BN_MOD_SUPPORT
  ResourceHandle::scripts  = &scriptManager;
#endif

  // Link i/o handle to use the input manager created by the game
  InputHandle::input = &inputManager;

  // Link net handle to use manager created by the game
  NetHandle::net = &netManager;

  // Create package managers for rest of game to utilize
  cardPackageManager = new class CardPackageManager;
  playerPackageManager = new class PlayerPackageManager;
  mobPackageManager = new class MobPackageManager;
  blockPackageManager = new class BlockPackageManager;
  luaLibraryPackageManager = new class LuaLibraryPackageManager;

  // Game session object needs to be available to every scene
  session = new GameSession;

  // Use the engine's window settings for this platform to create a properly 
  // sized render surface...
  unsigned int win_x = static_cast<unsigned int>(window.GetView().getSize().x);
  unsigned int win_y = static_cast<unsigned int>(window.GetView().getSize().y);

  renderSurface.create(win_x, win_y, window.GetRenderWindow()->getSettings());

  // Use our external render surface as the game's screen
  window.SetRenderSurface(renderSurface);
  window.GetRenderWindow()->setActive(false);
}

Game::~Game() {
  if (renderThread.joinable()) {
    renderThread.join();
  }

  delete session;
  delete cardPackageManager;
  delete playerPackageManager;
  delete mobPackageManager;
}

void Game::SetCommandLineValues(const cxxopts::ParseResult& values) {
  commandline = values.arguments();

  if (commandline.empty()) return;

  Logger::Log("Command line args provided");
  for (auto&& kv : commandline) {
    Logger::Logf("%s : %s", kv.key().c_str(), kv.value().c_str());
  }

  // Now that we have CLI values, we can configure 
  // other subsystems that need to read from them...
  unsigned int myPort = CommandLineValue<unsigned int>("port");
  uint16_t maxPayloadSize = CommandLineValue<uint16_t>("mtu");
  netManager.BindPort(myPort);

  if (maxPayloadSize != 0) {
    netManager.SetMaxPayloadSize(maxPayloadSize);
  }
}

TaskGroup Game::Boot(const cxxopts::ParseResult& values)
{
  SeedRand((unsigned int)time(0));
  isDebug = CommandLineValue<bool>("debug");
  singlethreaded = CommandLineValue<bool>("singlethreaded");

  // Initialize the engine and log the startup time
  const clock_t begin_time = clock();

  /**
  * TODO
  DrawWindow::WindowMode mode = configSettings.IsFullscreen() ? DrawWindow::WindowMode::fullscreen : DrawWindow::WindowMode::window;

  window.Initialize(windowMode);
  */

  Logger::Logf("Engine initialized: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);

  // does shaders too
  Callback<void()> graphics;
  graphics.Slot(std::bind(&Game::RunGraphicsInit, this, &progress));

  Callback<void()> audio;
  audio.Slot(std::bind(&Game::RunAudioInit, this, &progress));

  Callback<void()> libraries;
  libraries.Slot( std::bind( &Game::RunLuaLibraryInit, this, &progress ) );

  Callback<void()> navis;
  navis.Slot(std::bind(&Game::RunNaviInit, this, &progress));

  Callback<void()> mobs;
  mobs.Slot(std::bind(&Game::RunMobInit, this, &progress));

  Callback<void()> cards;
  cards.Slot(std::bind(&Game::RunCardInit, this, &progress));

  Callback<void()> blocks;
  blocks.Slot(std::bind(&Game::RunBlocksInit, this, &progress));

  Callback<void()> finish;
  finish.Slot([this] {
    // Tell the input event loop how to behave when the app loses and regains focus
    inputManager.BindLoseFocusEvent(std::bind(&Game::LoseFocus, this));
    inputManager.BindRegainFocusEvent(std::bind(&Game::GainFocus, this));
    inputManager.BindResizedEvent(std::bind(&Game::Resize, this, std::placeholders::_1, std::placeholders::_2));

    Font::specialCharLookup.insert(std::make_pair(char(-1), "THICK_SP"));
    Font::specialCharLookup.insert(std::make_pair(char(-2), "THICK_EX"));
    Font::specialCharLookup.insert(std::make_pair(char(-3), "THICK_NM"));

    // See the random generator with current time
    this->SeedRand((unsigned int)time(0));
  });

  this->UpdateConfigSettings(reader.GetConfigSettings());

  TaskGroup tasks;
  tasks.AddTask("Init graphics", std::move(graphics));
  tasks.AddTask("Init audio", std::move(audio));
  tasks.AddTask("Load Libraries", std::move( libraries ) );
  tasks.AddTask("Load Navis", std::move(navis));
  tasks.AddTask("Load mobs", std::move(mobs));
  tasks.AddTask("Load cards", std::move(cards));
  tasks.AddTask("Load prog blocks", std::move(blocks));
  tasks.AddTask("Finishing", std::move(finish));

    // Load font symbols for use across the entire engine...
  textureManager.LoadImmediately(TextureType::FONT);

  mouseTexture = textureManager.LoadTextureFromFile("resources/ui/mouse.png");
  mouse.setTexture(mouseTexture);
  //  mouse.setScale(2.f, 2.f);
  mouseAnimation = Animation("resources/ui/mouse.animation");
  mouseAnimation << "DEFAULT" << Animator::Mode::Loop;
  mouseAlpha = 1.0;

  window.GetRenderWindow()->setMouseCursorVisible(false);

  // set a loading spinner on the bottom-right corner of the screen
  spinner.setTexture(textureManager.LoadTextureFromFile("resources/ui/spinner.png"));
  spinner.setScale(2.f, 2.f);
  spinner.setPosition(float(window.GetView().getSize().x - 64), float(window.GetView().getSize().y - 50));

  spinnerAnimator = Animation("resources/ui/spinner.animation") << "SPIN" << Animator::Mode::Loop;

  if (!singlethreaded) {
    renderThread = std::thread(&Game::ProcessFrame, this);
  }

  return tasks;
}

bool Game::NextFrame()
{
  bool nextFrameKey = inputManager.Has(InputEvents::pressed_advance_frame);
  bool resumeKey = inputManager.Has(InputEvents::pressed_resume_frames);

  if (nextFrameKey && isDebug && !frameByFrame) {
    frameByFrame = true;
  }
  else if (resumeKey && frameByFrame) {
    frameByFrame = false;
  }

  return (frameByFrame && nextFrameKey) || !frameByFrame;
}

void Game::UpdateMouse(double dt)
{
  auto& renderWindow = *window.GetRenderWindow();
  sf::Vector2f mousepos = renderWindow.mapPixelToCoords(sf::Mouse::getPosition(renderWindow));

  mouse.setPosition(mousepos);
  mouse.setColor(sf::Color::White);
  mouseAnimation.Update(dt, mouse.getSprite());
}

void Game::ProcessFrame()
{
  sf::Clock clock;
  float scope_elapsed = 0.0f;
  window.GetRenderWindow()->setActive(true);

  while (!quitting) {
    clock.restart();

    double delta = 1.0 / static_cast<double>(frame_time_t::frames_per_second);
    this->elapsed += from_seconds(delta);

    // Poll net code
    netManager.Update(delta);

    if (NextFrame()) {
      window.Clear(); // clear screen

      inputManager.Update(); // process inputs
      UpdateMouse(delta);
      this->update(delta);  // update game logic
      this->draw();        // draw game
      mouse.draw(*window.GetRenderWindow());
      window.Display(); // display to screen
    }

    scope_elapsed = clock.getElapsedTime().asSeconds();
  }
}

void Game::RunSingleThreaded()
{
  sf::Clock clock;
  float scope_elapsed = 0.0f;
  window.GetRenderWindow()->setActive(true);

  while (window.Running() && !quitting) {
    clock.restart();
    this->SeedRand(time(0));

    // Poll window events
    inputManager.EventPoll();

    // unused images need to be free'd 
    textureManager.HandleExpiredTextureCache();

    double delta = 1.0 / static_cast<double>(frame_time_t::frames_per_second);
    this->elapsed += from_seconds(delta);

    // Poll net code
    netManager.Update(delta);

    if (NextFrame()) {
      window.Clear(); // clear screen

      inputManager.Update(); // process inputs
      UpdateMouse(delta);
      this->update(delta);  // update game logic
      this->draw();        // draw game
      mouse.draw(*window.GetRenderWindow());
      window.Display(); // display to screen
    }

    quitting = getStackSize() == 0;

    scope_elapsed = clock.getElapsedTime().asSeconds();
  }
}

void Game::Exit()
{
  quitting = true;
}

void Game::Run()
{
  if (singlethreaded) {
    RunSingleThreaded();
    return;
  }  

  while (window.Running() && !quitting) {
    this->SeedRand(time(0));

    // Poll window events
    inputManager.EventPoll();

    // unused images need to be free'd 
    textureManager.HandleExpiredTextureCache();

    quitting = getStackSize() == 0;
  }
}

void Game::SetWindowMode(DrawWindow::WindowMode mode)
{
  windowMode = mode;
}

void Game::Postprocess(ShaderType shaderType)
{
  this->postprocess = shaderManager.GetShader(shaderType);
}

void Game::NoPostprocess()
{
  this->postprocess = nullptr;
}

const sf::Vector2f Game::CameraViewOffset(Camera& camera)
{
  return window.GetView().getCenter() - camera.GetView().getCenter();
}

unsigned Game::FrameNumber() const
{
  return this->elapsed.count();
}

const Endianness Game::GetEndianness()
{
  return endian;
}

void Game::LoadConfigSettings()
{
  // try to read the config file
  configSettings = reader.GetConfigSettings();
}

void Game::UpdateConfigSettings(const struct ConfigSettings& new_settings)
{
  if (!new_settings.IsOK())
    return;

  configSettings = new_settings;

  if (configSettings.GetShaderLevel() > 0) {
    window.SupportShaders(true);
    ActivityController::optimizeForPerformance(swoosh::quality::realtime);
    ActivityController::enableShaders(true);
  }
  else {
    window.SupportShaders(false);
    ActivityController::optimizeForPerformance(swoosh::quality::mobile);
    ActivityController::enableShaders(false);
  }

  // If the file is good, use the Audio() and 
  // controller settings from the config
  audioManager.EnableAudio(configSettings.IsAudioEnabled());
  audioManager.SetStreamVolume(((configSettings.GetMusicLevel()-1) / 3.0f) * 100.0f);
  audioManager.SetChannelVolume(((configSettings.GetSFXLevel()-1) / 3.0f) * 100.0f);

  inputManager.SupportConfigSettings(configSettings);
}

void Game::SeedRand(unsigned int seed)
{
  randSeed = seed;
  scriptManager.SeedRand(seed);
}

const unsigned int Game::GetRandSeed() const
{
  return randSeed;
}

bool Game::IsSingleThreaded() const
{
  return singlethreaded;
}

CardPackageManager& Game::CardPackageManager()
{
  return *cardPackageManager;
}

PlayerPackageManager& Game::PlayerPackageManager()
{
  return *playerPackageManager;
}

MobPackageManager& Game::MobPackageManager()
{
  return *mobPackageManager;
}

BlockPackageManager& Game::BlockPackageManager()
{
  return *blockPackageManager;
}

LuaLibraryPackageManager& Game::GetLuaLibraryPackageManager()
{
  return *luaLibraryPackageManager;
}

ConfigSettings& Game::ConfigSettings()
{
  return configSettings;
}

GameSession& Game::Session()
{
  return *session;
}

void Game::RunNaviInit(std::atomic<int>* progress) {
  clock_t begin_time = clock();
  QueuNaviRegistration(*playerPackageManager); // Queues navis to be loaded later

  playerPackageManager->LoadAllPackages(*progress);

  Logger::Logf("Loaded registered navis: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
}

void Game::RunBlocksInit(std::atomic<int>* progress)
{
  clock_t begin_time = clock();
  QueueBlockRegistration(*blockPackageManager); // Queues navis to be loaded later

  blockPackageManager->LoadAllPackages(*progress);

  Logger::Logf("Loaded registered prog blocks: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
}

void Game::RunMobInit(std::atomic<int>* progress) {
  clock_t begin_time = clock();
  QueueMobRegistration(*mobPackageManager); // Queues mobs to be loaded later

  mobPackageManager->LoadAllPackages(*progress);

  Logger::Logf("Loaded registered mobs: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
}

void Game::RunCardInit(std::atomic<int>* progress) {
  clock_t begin_time = clock();
  QueueCardRegistration(*cardPackageManager);

  cardPackageManager->LoadAllPackages(*progress);

  Logger::Logf("Loaded registered cards: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
}

void Game::RunLuaLibraryInit(std::atomic<int>* progress) {
  clock_t begin_time = clock();
  QueueLuaLibraryRegistration(*luaLibraryPackageManager);

  luaLibraryPackageManager->LoadAllPackages(*progress);

  Logger::Logf("Loaded registered libraries: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
}

void Game::RunGraphicsInit(std::atomic<int> * progress) {
  clock_t begin_time = clock();
  textureManager.LoadAllTextures(*progress);

  Logger::Logf("Loaded textures: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);

  if (reader.GetConfigSettings().GetShaderLevel() > 0) {
    ActivityController::optimizeForPerformance(swoosh::quality::realtime);
    begin_time = clock();
    shaderManager.LoadAllShaders(*progress);

    Logger::Logf("Loaded shaders: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
  }
  else {
    ActivityController::optimizeForPerformance(swoosh::quality::mobile);
    Logger::Log("Shader support is disabled");
  }
}

void Game::RunAudioInit(std::atomic<int> * progress) {
  const clock_t begin_time = clock();
  audioManager.LoadAllSources(*progress);

  Logger::Logf("Loaded Audio() sources: %f secs", float(clock() - begin_time) / CLOCKS_PER_SEC);
}

void Game::GainFocus() {
  window.RegainFocus();
  audioManager.Mute(false);

#ifdef __ANDROID__
  // TODO: Reload all graphics and somehow reassign all gl IDs to all allocated sfml graphics structures
  // Textures().RecoverLostGLContext();
  // ENGINE.RecoverLostGLContext(); // <- does the window need recreation too?
#endif
}

void Game::LoseFocus() {
  audioManager.Mute(true);
}

void Game::Resize(int newWidth, int newHeight) {
  float windowRatio = (float)newWidth / (float)newHeight;
  float viewRatio = window.GetView().getSize().x / window.GetView().getSize().y;

  showScreenBars = windowRatio >= viewRatio;
  window.Resize(newWidth, newHeight);
}