#include "bnTornadoCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTornado.h"
#include "bnField.h"

#define FAN_PATH "resources/spells/buster_fan.png"
#define FAN_ANIM "resources/spells/buster_fan.animation"

#define FRAME1 { 1, 0.1  }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3

TornadoCardAction::TornadoCardAction(Character& owner, int damage) : 
  CardAction(owner, "PLAYER_SHOOTING"), 
  attachmentAnim(FAN_ANIM), armIsOut(false) {
  TornadoCardAction::damage = damage;

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");
  attachmentAnim << Animator::Mode::Loop;

  Animation& userAnim = owner.GetFirstComponent<AnimationComponent>()->GetAnimationObject();
  AddAttachment(userAnim, "BUSTER", *attachment).UseAnimation(attachmentAnim);

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

TornadoCardAction::~TornadoCardAction()
{
  delete attachment;
}

void TornadoCardAction::OnExecute() {
  auto owner = GetOwner();
  
  attachmentAnim.Update(0, attachment->getSprite());
  owner->AddNode(attachment);

  auto team = GetOwner()->GetTeam();
  auto tile = GetOwner()->GetTile();
  auto field = GetOwner()->GetField();

  // On shoot frame, drop projectile
  auto onFire = [this, team, tile, field, owner]() -> void {
    Tornado* tornado = new Tornado(team, 8, damage);
    auto props = tornado->GetHitboxProperties();
    props.aggressor = owner;
    tornado->SetHitboxProperties(props);

    int step = team == Team::red ? 2 : -2;
    field->AddEntity(*tornado, tile->GetX() + step, tile->GetY());
  };

  // Spawn a tornado istance 2 tiles in front of the player every x frames 8 times
  AddAnimAction(2, [onFire, owner, this]() {
    Audio().Play(AudioType::WIND);
    armIsOut = true;
    onFire();
  });
}

void TornadoCardAction::OnAnimationEnd()
{
}

void TornadoCardAction::OnUpdate(double _elapsed)
{
  attachment->setPosition(CalculatePointOffset("buster"));

  if (armIsOut) {
    attachmentAnim.Update(_elapsed, attachment->getSprite());
  }

  CardAction::OnUpdate(_elapsed);
}

void TornadoCardAction::OnEndAction()
{
  GetOwner()->RemoveNode(attachment);
  Eject();
}
