#pragma once
#include "bnEntity.h"
#include "bnCounterHitPublisher.h"
#include "bnHitPublisher.h"
#include "bnDefenseFrameStateJudge.h"
#include "bnDefenseRule.h"
#include "bnHitProperties.h"
#include "bnTile.h"

#include <string>
#include <queue>
#include <variant>
#include <functional>

using std::string;

class DefenseRule;
class Spell;
class CardAction;

enum class ActionPriority : short {
  immediate = 0,
  card_chain = 1, // !< For cards used in successive animations under special conditions
  trigger = 2,
  forced,
  voluntary
};

struct ActionComparitor {
  bool operator()(const ActionEvent& a, const ActionEvent& b) {
    return a.value < b.value;
  }
};

struct MoveEvent {
  frame_time_t deltaFrames{}; //!< Frames between tile A and B. If 0, teleport. Else, we could be sliding
  frame_time_t delayFrames{}; //!< Startup lag to be used with animations
  frame_time_t endlagFrames{}; //!< Wait period before action is complete
  float height{}; //!< If this is non-zero with delta frames, the character will effectively jump
  Battle::Tile* dest{ nullptr };
};

struct BusterEvent {
  frame_time_t deltaFrames{}; //!< e.g. how long it animates
  frame_time_t endlagFrames{}; //!< Wait period after completition

  // Default is false which is shoot-then-move
  bool blocking{}; //!< If true, blocks incoming move events for auto-fire behavior
};

//!< Used to 1) keep the variant well-formed 2) represent an empty event
struct Nonce {};

struct ActionEvent {
  ActionPriority value{};
  long long timestamp{}; //!< When was this action created

  //!< NOTE: Nonce is first so it's index is always ZERO
  std::variant<Nonce, MoveEvent, BusterEvent, CardAction*> data{ Nonce{} };
};

// overload design pattern for variant visits
template <class ...Fs>
struct overload : Fs... {
  overload(Fs const&... fs) : Fs{ fs }...
  {}

  using Fs::operator()...;
};

template <class ...Ts>
overload(Ts&&...)->overload<std::remove_reference_t<Ts>...>;

/**
 * @class Character
 * @author mav
 * @date 05/05/19
 * @brief Characters are mobs, enemies, and players. They have health and can take hits.
 */
class Character : public virtual Entity, public CounterHitPublisher, public HitPublisher {
  friend class Field;
  friend class AnimationComponent;
protected:
  using StatusCallback = std::function<void()>;

private:
  bool canShareTile; /*!< Some characters can share tiles with others */
  bool slideFromDrag; /*!< In combat, slides from tiles are cancellable. Slide via drag is not. This flag denotes which one we're in. */
  std::vector<DefenseRule*> defenses; /*<! All defense rules sorted by the lowest priority level */
  std::vector<Character*> shareHit; /*!< All characters to share hit damage. Useful for enemies that share hit boxes like stunt doubles */
  std::vector<Component::ID_t> attacks;

  // Statuses are resolved one property at a time
  // until the entire Flag object is equal to 0x00 None
  // Then we process the next status
  // This continues until all statuses are processed
  std::queue<Hit::Properties> statusQueue;

  sf::Shader* whiteout; /*!< Flash white when hit */
  sf::Shader* stun;     /*!< Flicker yellow with luminance values when stun */
  ActionEvent currentAction{}; /*!< Allow actions to take place through a trusted state */
  std::priority_queue<ActionEvent, std::vector<ActionEvent>, ActionComparitor> actionQueue;

  bool hit; /*!< Was hit this frame */
  std::map<Hit::Flags, StatusCallback> statusCallbackHash;
public:

  /**
   * @class Rank
   * @breif A single definition of a character can have adjustments based on rank
   */
  enum class Rank : const int {
    _1,
    _2,
    _3,
    SP,
    EX,
    Rare1,
    Rare2,
    SIZE
  };

  Character(Rank _rank = Rank::_1);
  virtual ~Character();
  
  /**
   * @brief Describe what happens to this character when they are hit by any attack
   * @note for specific responses to status effects from hitboxes, use RegisterStatusCallback(...)
   * @see Character::RegisterStatusCallback
   */
  virtual void OnHit();

  /**
  * The hit routine that happens for every character. Queues status properties and damage
  * to resolve at the end of the battle step.
  * @param props
  * @return Returns false  if IsPassthrough() is true (i-frames), otherwise true
  */
  const bool Hit(Hit::Properties props = Hit::DefaultProperties);

  virtual const bool UnknownTeamResolveCollision(const Spell& other) const;

  const bool HasCollision(const Hit::Properties& props);

  void ResolveFrameBattleDamage();

  virtual void OnUpdate(double elapsed) = 0;

  void QueueAction(const ActionEvent& action);

  // TODO: move tile behavior out of update loop and into its own rule system for customization
  void Update(double elapsed) override;
  
  /**
  * @brief Default characters cannot move onto occupied, broken, or empty tiles
  * @param next
  * @return true if character can move to tile, false otherwise
  */
  virtual bool CanMoveTo(Battle::Tile* next) override;
 
  
  /**
   * @brief Get the character's current health
   * @return 
   */
  virtual int GetHealth() const;
  
  /**
   * @brief Get the character's max (init) health
   * @return 
   */
  const int GetMaxHealth() const;

  const bool CanAttack() const;

  /**
   * @brief Set the health of the character
   * @param _health
   */
  void SetHealth(int _health);
 
  /**
   * @brief Place the character in the tile's character bucket
   * @param tile
   */
  virtual void AdoptTile(Battle::Tile* tile);
  
  /**
   * @brief Sets counter flag on
   * @param on
   * 
   * Used by counter frames. 
   * 
   * NOTE: Should be changed to protected or private access
   */
  void ToggleCounter(bool on = true);
  
  /**
   * @brief Query the character's state is Stunned
   * @return true if character is currently stunned, false otherwise
   */
  bool IsStunned();

  /**
   * @brief Get the rank of this character
   * @return const Rank
   */
  const Rank GetRank() const;

  /**
   * @brief Some characters allow others to move on top of them
   * @param enabled true, characters can share space, false otherwise
   */
  void ShareTileSpace(bool enabled);
  
  /**
   * @brief Query if entity can share space
   * @return true if shareTilespace is enabled, false otherwise
   */
  const bool CanShareTileSpace() const;

  /**
  * @brief Query if entity is pushable by tiles
  * @return true if canTilePush is enabled, false otherwise
  */
  const bool CanTilePush() const;

  /**
   * @brief Some characters can be moved around on the field by tiles
   * @param enabled
   */
  void EnableTilePush(bool enabled);

  /**
   * @brief Characters can have names
   * @param name
   */
  void SetName(std::string name);
  
  /**
   * @brief Get name of character
   * @return const std::string
   */
  const std::string GetName() const;

  /**
   * @brief Add defense rule for combat resolution
   * @param rule
   */
  void AddDefenseRule(DefenseRule* rule);
  
  /**
   * @brief Removes the defense rule from this character's defense checks
   * @param rule
   */
  void RemoveDefenseRule(DefenseRule* rule);

  /**
   * @brief Check if spell passes all defense checks. Updates the DefenseFrameStateJudge.
   * @param in attack
   * @param judge. The frame's current defense object with triggers and block statuses
   * @param in. The attack to test defenses against.
   * @param filter. Filter which types of defenses to check against by DefenseOrder value
   */
  void DefenseCheck(DefenseFrameStateJudge& judge, Spell& in, const DefenseOrder& filter);

  /**
  * @brief Create a combat link between other characters
  * @param to. A Character ptr.
  *
  * When this entity gets hit, it will propagate the hit to this parameter Character ptr
  */
  void SharedHitboxDamage(Character* to);

  /**
  * @brief Sever a combat link from other character 
  * @param to. A Character ptr.
  *
  * If the character exists in this entity's shared hit-list, it will remove it
  */
  void CancelSharedHitboxDamage(Character* to);

  void EndCurrentAction();

private:
  int maxHealth{ std::numeric_limits<int>::max() };
  sf::Vector2f counterSlideOffset; /*!< Used when enemies delete on counter - they slide back */
  float counterSlideDelta;

protected:
  void RegisterStatusCallback(const Hit::Flags& flag, const StatusCallback& callback);

  /**
  * @brief Stun a character for maxCooldown seconds
  * @param maxCooldown
  * Used internally by class
  *
  */
  void Stun(double maxCooldown);

  /**
  * @brief Query if an attack successfully countered a Character
  * @return true if character is currently countered, false otherwise
  * Used internally by class
  */
  bool IsCountered();

  int health{ std::numeric_limits<int>::max() };
  bool counterable{};
  int counterFrameFlag{ 0 };
  bool canTilePush{};
  std::string name;
  double stunCooldown{ 0 }; /*!< Timer until stun is over */
  double invincibilityCooldown{ 0 }; /*!< Timer until invincibility is over */
  Character::Rank rank;
};
