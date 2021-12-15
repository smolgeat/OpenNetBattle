#pragma once

#include "../bnBattleSceneState.h"
#include "../../bnSpriteProxyNode.h"
#include "../../bnCardUseListener.h"

#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Shader.hpp>

class Mob;
class Player;

namespace Battle {
  class Card;
}

/*
    \brief This state will govern combat rules
*/
struct CombatBattleState final : public BattleSceneState, public CardActionUseListener {
  bool canPause{ true };
  bool isPaused{ false };
  bool hasTimeFreeze{ false };
  bool clearedMob{ false };
  bool skipFrame{ false };
  Mob* mob{ nullptr };
  sf::Sprite pause;
  sf::Sprite doubleDelete;
  sf::Sprite tripleDelete;
  sf::Sprite counterHit;
  sf::Shader* pauseShader; /*!< Dim screen */
  std::vector<const BattleSceneState*> subcombatStates;
  const bool HasTimeFreeze() const;
  const bool PlayerWon() const;
  const bool PlayerLost() const;
  const bool PlayerRequestCardSelect();
  const bool HandleNextRoundSetup(const BattleSceneState* state);
  const bool IsStateCombat(const BattleSceneState* state);

  void EnablePausing(bool enable);
  void SkipFrame();
  void onStart(const BattleSceneState* last) override;
  void onEnd(const BattleSceneState* next) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  void OnCardActionUsed(std::shared_ptr<CardAction> action, uint64_t timestamp) override;

  CombatBattleState(Mob* mob, double customDuration);
};