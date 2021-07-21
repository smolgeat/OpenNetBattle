#pragma once

#include <SFML/Graphics.hpp>
#include <time.h>
#include <future>
#include <optional>
#include <Swoosh/Activity.h>
#include <Swoosh/ActivityController.h>

#include "../bnScene.h"
#include "../bnCamera.h"
#include "../bnInputManager.h"
#include "../bnAudioResourceManager.h"
#include "../bnShaderResourceManager.h"
#include "../bnTextureResourceManager.h"
#include "../bnNaviRegistration.h"
#include "../bnPA.h"
#include "../bnDrawWindow.h"
#include "../bnAnimation.h"
#include "../bnCardFolderCollection.h"
#include "../bnKeyItemScene.h"
#include "../bnInbox.h"

// overworld
#include "bnOverworldInteraction.h"
#include "bnOverworldCameraController.h"
#include "bnOverworldSprite.h"
#include "bnOverworldActor.h"
#include "bnOverworldPlayerController.h"
#include "bnOverworldPathController.h"
#include "bnOverworldTeleportController.h"
#include "bnOverworldSpatialMap.h"
#include "bnOverworldMap.h"
#include "bnOverworldPersonalMenu.h"
#include "bnOverworldMenuSystem.h"
#include "bnEmotes.h"
#include "bnXML.h"
#include "bnMinimap.h"

class Background; // forward decl

namespace Overworld {
  struct PlayerSession {
    int health{};
    Emotion emotion{};
    Inbox inbox;
  };

  class SceneBase : public Scene {
  private:
    PlayerSession playerSession;
    std::shared_ptr<Actor> playerActor;
    std::shared_ptr<sf::Texture> customEmotesTexture;
    Overworld::EmoteWidget emote;
    Overworld::EmoteNode emoteNode;
    Overworld::TeleportController teleportController{};
    Overworld::PlayerController playerController{};
    Overworld::SpatialMap spatialMap{};
    std::vector<std::shared_ptr<Overworld::Actor>> actors;
    std::vector<KeyItemScene::Item> items;

    double animElapsed{};
    bool showMinimap{ false };
    bool inputLocked{ false };
    bool cameraLocked{ false };
    bool teleportedOut{ false }; /*!< We may return to this area*/
    bool lastIsConnectedState; /*!< Set different animations if the connection has changed */
    bool gotoNextScene{ false }; /*!< If true, player cannot interact with screen yet */

    Camera camera;
    CameraController cameraController; /*!< camera in scene follows player */
    Text time;

    sf::Vector3f returnPoint{};
    sf::Vector3f cameraTrackPoint{}; // used for smooth cameras
    PersonalMenu personalMenu;
    Minimap minimap;
    SpriteProxyNode webAccountIcon; /*!< Status icon if connected to web server*/
    Animation webAccountAnimator; /*!< Use animator to represent different statuses */

    // Bunch of sprites and their attachments
    std::shared_ptr<Background> bg{ nullptr }; /*!< Background image pointer */
    Overworld::Map map; /*!< Overworld map */
    std::vector<std::shared_ptr<WorldSprite>> sprites;
    std::vector<std::vector<std::shared_ptr<WorldSprite>>> spriteLayers;
    Overworld::MenuSystem menuSystem;

    /*!< Current navi selection index */
    SelectedNavi currentNavi{},
      lastSelectedNavi{ std::numeric_limits<SelectedNavi>::max() };


    CardFolderCollection folders; /*!< Collection of folders */
    PA programAdvance;

    std::future<WebAccounts::AccountState> accountCommandResponse; /*!< Response object that will wait for data from web server*/
    WebAccounts::AccountState webAccount;

    void HandleCamera(float elapsed);
    void HandleInput();
    void LoadBackground(const Map& map, const std::string& value);
    void DrawWorld(sf::RenderTarget& target, sf::RenderStates states);
    void DrawMapLayer(sf::RenderTarget& target, sf::RenderStates states, size_t layer, size_t maxLayers);
    void DrawSpriteLayer(sf::RenderTarget& target, sf::RenderStates states, size_t layer);
    void PrintTime(sf::RenderTarget& target);

#ifdef __ANDROID__
    void StartupTouchControls();
    void ShutdownTouchControls();
#endif

  protected:
    const bool IsMouseHovering(const sf::Vector2f& mouse, const WorldSprite& src);

  public:

    SceneBase() = delete;
    SceneBase(const SceneBase&) = delete;

    /**
     * @brief Loads the player's library data and loads graphics
     */
    SceneBase(swoosh::ActivityController&);

    /**
     * @brief Checks input events and listens for select buttons. Segues to new screens.
     * @param elapsed in seconds
     */
    virtual void onUpdate(double elapsed) override;

    /**
     * @brief Draws the UI
     * @param surface
     */
    virtual void onDraw(sf::RenderTexture& surface) override;

    /**
     * @brief Stops music, plays new track, reset the camera
     */
    virtual void onStart() override;

    /**
     * @brief Does nothing
     */
    virtual void onLeave() override;

    /**
     * @brief Does nothing
     */
    virtual void onExit() override;

    /**
     * @brief Checks the selected navi if changed and loads the new images
     */
    virtual void onEnter() override;

    /**
     * @brief Resets the camera
     */
    virtual void onResume() override;

    /**
     * @brief Stops the music
     */
    virtual void onEnd() override;

    /**
    * @brief Update's the player sprites according to the most recent selection
    */
    void RefreshNaviSprite();

    /**
    * @brief Equip a folder for the navi that was last used when playing as them
    */
    void NaviEquipSelectedFolder();

    /**
    * @brief Effectively sets the scene
    */
    void LoadMap(const std::string& data);

    void TeleportUponReturn(const sf::Vector3f& position);
    const bool HasTeleportedAway() const;

    void SetBackground(const std::shared_ptr<Background>&);

    void SetCustomEmotesTexture(const std::shared_ptr<sf::Texture>&);

    void AddItem(const std::string& id, const std::string& name, const std::string& description);
    void RemoveItem(const std::string& id);

    /**
     * @brief Add a sprite
     * @param sprite
     */
    void AddSprite(const std::shared_ptr<WorldSprite>& sprite);

    /**
     * @brief Remove a sprite
     * @param sprite
     */
    void RemoveSprite(const std::shared_ptr<WorldSprite>& sprite);

    /**
     * @brief Adds Actor for updates and rendering. (Calls AddSprite)
     * @param actor
     */
    void AddActor(const std::shared_ptr<Actor>& actor);


    /**
     * @brief Removes the actor for updates and rendering (Calls RemoveSprite)
     * @param actor
     */
    void RemoveActor(const std::shared_ptr<Actor>& actor);


    /**
     * @brief Block the player from moving and interacting
     * @param actor
     */
    void LockInput();


    /**
     * @brief Unblock the player from moving and interacting. Unlock is not guaranteed as the player may be in a menu
     * @param actor
     */
    void UnlockInput();

    /**
     * @brief Stop the camera from following the player
     * @param actor
     */
    void LockCamera();

    /**
     * @brief Allow the camera to follow the player
     * @param actor
     */
    void UnlockCamera();

    //
    // Menu selection callbacks
    //

    void GotoChipFolder();
    void GotoNaviSelect();
    void GotoConfig();
    void GotoMobSelect();
    void GotoPVP();
    void GotoMail();
    void GotoKeyItems();

    //
    // Getters
    //
    PersonalMenu& GetPersonalMenu();
    Minimap& GetMinimap();
    SpatialMap& GetSpatialMap();
    std::vector<std::shared_ptr<Actor>>& GetActors();
    Camera& GetCamera();
    Map& GetMap();
    PlayerSession& GetPlayerSession();
    std::shared_ptr<Actor> GetPlayer();
    PlayerController& GetPlayerController();
    TeleportController& GetTeleportController();
    SelectedNavi& GetCurrentNavi();
    const std::shared_ptr<sf::Texture>& GetCustomEmotesTexture() const;
    EmoteNode& GetEmoteNode();
    EmoteWidget& GetEmoteWidget();
    std::shared_ptr<Background> GetBackground();
    PA& GetProgramAdvance();
    std::optional<CardFolder*> GetSelectedFolder();
    Overworld::MenuSystem& GetMenuSystem();
    bool IsInputLocked();
    bool IsInFocus() const;
    virtual std::string GetPath(const std::string& path);
    virtual std::string GetText(const std::string& path);
    virtual std::shared_ptr<sf::Texture> GetTexture(const std::string& path);
    virtual std::shared_ptr<sf::SoundBuffer> GetAudio(const std::string& path);

    //
    // Helpers
    //
    std::pair<unsigned, unsigned> PixelToRowCol(const sf::Vector2i& px, const sf::RenderWindow& window) const;

    //
    // Optional events that can be decorated further
    //
    virtual void OnEmoteSelected(Emotes emote);
    virtual void OnCustomEmoteSelected(unsigned emote);

    //
    // Required implementations
    //
    virtual void OnTileCollision() = 0;
    virtual void OnInteract(Interaction type) = 0;
  };
}