#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class VulcanCardAction : public CardAction {
private:
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  VulcanCardAction(Character& owner, int damage);
  ~VulcanCardAction();
  void OnEndAction() override;
  void OnExecute() override;
};
