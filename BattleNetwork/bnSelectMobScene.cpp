#include <Swoosh/ActivityController.h>
#include "bnSelectMobScene.h"
#include "battlescene/bnMobBattleScene.h"
#include "Android/bnTouchArea.h"

constexpr float PIXEL_MAX = 50.0f;
constexpr float PIXEL_SPEED = 180.0f;

using namespace swoosh::types;

SelectMobScene::SelectMobScene(swoosh::ActivityController& controller, SelectedNavi navi, CardFolder& selectedFolder, PA& programAdvance) :
  elapsed(0),
  textbox(320, 100, 24, "resources/fonts/NETNAVI_4-6_V3.ttf"),
  selectedFolder(selectedFolder),
  programAdvance(programAdvance),
  swoosh::Activity(&controller)
{
  selectedNavi = navi;

  // Menu name font
  font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  menuLabel = new sf::Text("BATTLE SELECT", *font);
  menuLabel->setCharacterSize(15);
  menuLabel->setPosition(sf::Vector2f(20.f, 5.0f));

  navigator.setTexture(LOAD_TEXTURE(MUG_NAVIGATOR));
  navigator.setScale(2.0f, 2.0f);
  navigator.setPosition(10.0f, 208.0f);

  navigatorAnimator = Animation("resources/ui/navigator.animation");
  navigatorAnimator.Reload();
  navigatorAnimator.SetAnimation("TALK");
  navigatorAnimator << Animator::Mode::Loop;

  cursor.setTexture(LOAD_TEXTURE(FOLDER_CURSOR));
  cursor.setScale(2.f, 2.f);

  // Selection input delays
  maxSelectInputCooldown = 0.5; // half of a second
  selectInputCooldown = maxSelectInputCooldown;

  // MOB UI font
  mobFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  mobLabel = new sf::Text("", *mobFont);
  mobLabel->setPosition(sf::Vector2f(100.f, 45.0f));

  attackLabel = new sf::Text("", *mobFont);
  attackLabel->setPosition(325.f, 30.f);

  speedLabel = new sf::Text("", *mobFont);
  speedLabel->setPosition(325.f, 45.f);

  hpLabel = new sf::Text("", *mobFont);
  hpLabel->setPosition(325.f, 60.f);

  maxNumberCooldown = 0.5;
  numberCooldown = maxNumberCooldown; // half a second

  // select menu graphic
  bg.setTexture(LOAD_TEXTURE(BATTLE_SELECT_BG));
  bg.setScale(2.f, 2.f);

  gotoNextScene = true; 
  doOnce = false; // wait until the scene starts or resumes
  showMob = false;

  shader = LOAD_SHADER(TEXEL_PIXEL_BLUR);
  factor = PIXEL_MAX;

  // Current selection index
  mobSelectionIndex = 0;

  // Text box navigator
  textbox.Stop();
  textbox.setPosition(100, 210);
  textbox.SetTextColor(sf::Color::Black);
  textbox.SetCharactersPerSecond(25);

#ifdef __ANDROID__
  touchPosX = 0;
  canSwipe = false;
#endif

  mob = nullptr;

  setView(sf::Vector2u(480, 320));
}

SelectMobScene::~SelectMobScene() {
  delete mobLabel;
  delete attackLabel;
  delete speedLabel;
  delete menuLabel;
  delete hpLabel;

  if (mob) delete mob;
}

void SelectMobScene::onUpdate(double elapsed) {
  SelectMobScene::elapsed += elapsed;

  // multiplying update by 2 effectively sets playback speed to 200%
  navigatorAnimator.Update(float(elapsed*2.0), navigator.getSprite());

  textbox.Update((float)elapsed);

  int prevSelect = mobSelectionIndex;

#ifndef __ANDROID__
  // Scene keyboard controls
  if (!gotoNextScene) {
    if (INPUTx.Has(EventTypes::PRESSED_UI_LEFT)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to previous mob 
        selectInputCooldown = maxSelectInputCooldown;
        mobSelectionIndex--;

        // Number scramble effect
        numberCooldown = maxNumberCooldown;
      }
    }
    else if (INPUTx.Has(EventTypes::PRESSED_UI_RIGHT)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to next mob 
        selectInputCooldown = maxSelectInputCooldown;
        mobSelectionIndex++;

        // Number scramble effect
        numberCooldown = maxNumberCooldown;
      }
    }
    else {
      selectInputCooldown = 0;
    }

    if (INPUTx.Has(EventTypes::PRESSED_CANCEL)) {
      // Fade out black and go back to the menu
      gotoNextScene = true;
      AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
      using effect = segue<BlackWashFade, milliseconds<500>>;
      getController().pop<effect>();
    }
  }
#else
    // Scene keyboard controls
    if (!gotoNextScene) {
        if ((touchPosX - touchPosStartX) < -100 && canSwipe) {
            canSwipe = false;

            // Go to previous mob
            selectInputCooldown = maxSelectInputCooldown;
            mobSelectionIndex++;

            // Number scramble effect
            numberCooldown = maxNumberCooldown;
        }
        else if ((touchPosStartX - touchPosX) < -100 && canSwipe) {
            canSwipe = false;

            // Go to next mob
            selectInputCooldown = maxSelectInputCooldown;
            mobSelectionIndex--;

            // Number scramble effect
            numberCooldown = maxNumberCooldown;
        }

        if (INPUTx.Has(PRESSED_B)) {
            // Fade out black and go back to the menu
            gotoNextScene = true;
            AUDIO.Play(AudioType::CHIP_DESC_CLOSE);
            using segue = swoosh::intent::segue<BlackWashFade, swoosh::intent::milli<500>>;
            getController().pop<segue>();
        }
    }
#endif

  // Keep our mob index in range
  mobSelectionIndex = std::max(0, mobSelectionIndex);
  mobSelectionIndex = std::min((int)MOBS.Size()-1, mobSelectionIndex);

  // Grab the mob info object from this index
  auto& mobinfo = MOBS.At(mobSelectionIndex);

  mobLabel->setString(mobinfo.GetName());
  hpLabel->setString(mobinfo.GetHPString());
  speedLabel->setString(mobinfo.GetSpeedString());
  attackLabel->setString(mobinfo.GetAttackString());

#ifdef __ANDROID__
  if(canSwipe) {
      if (sf::Touch::isDown(0)) {
          sf::Vector2i touchPosition = sf::Touch::getPosition(0, *ENGINE.GetWindow());
          sf::Vector2f coords = ENGINE.GetWindow()->mapPixelToCoords(touchPosition,
                                                                     ENGINE.GetDefaultView());
          sf::Vector2i iCoords = sf::Vector2i((int) coords.x, (int) coords.y);
          touchPosition = iCoords;

          canSwipe = false;

          if(touchPosition.y < 400 && touchPosition.x < 290) {
            if (!touchStart) {
              touchStart = true;
              touchPosStartX = touchPosition.x;
            }

            touchPosX = touchPosition.x;
          }

          if(touchPosition.x <= 320) {
            mobSpr.setPosition(110.0f - (touchPosStartX - touchPosX), 130.f);
            canSwipe = true;
          }
      } else {
          canSwipe = false;
          touchStart = false;
      }
  } else {
      if(prevSelect == mobSelectionIndex) {
          auto x = swoosh::ease::interpolate(0.5f, mobSpr.getPosition().x, 110.f);
          auto y = swoosh::ease::interpolate(0.5f, mobSpr.getPosition().y, 130.f);
          mobSpr.setPosition(x, y);

          if (int(x) == 110 && int(y) == 130) {
              mobSpr.setPosition(110.f, 130.f);
              touchPosX = 50; // somewhere in the middle that wont trigger a swipe
              touchPosStartX = 50;
              touchStart = false;
              canSwipe = true;
          }
      }
  }
#else
  mobSpr.setPosition(110.0f, 130.f);
#endif

  if (prevSelect != mobSelectionIndex || doOnce) {
    doOnce = false;
    factor = PIXEL_MAX;

    // Current mob graphic
    mobSpr.setTexture(mobinfo.GetPlaceholderTexture());
    mobSpr.setScale(2.f, 2.f);
    mobSpr.setOrigin(mobSpr.getLocalBounds().width / 2.f, mobSpr.getLocalBounds().height / 2.f);

    textbox.SetText(mobinfo.GetDescriptionString());
    textbox.Stop();
  
    prevSelect = mobSelectionIndex;
  }

  /**
   * The following code just scrambles the name, health, speed, and attack data
   * Each line does the same thing
   * If it's a number, choose random numbers
   * If it's alphabetical, choose random capital letters
   * The scramble index moves towards the end of the string over time
   * Eventually the original data is unmodified and the effect ends
   */
  if (numberCooldown > 0) {
    numberCooldown -= (float)elapsed;
    std::string newstr;

    for (int i = 0; i < mobLabel->getString().getSize(); i++) {
      double progress = (maxNumberCooldown - numberCooldown) / maxNumberCooldown;
      double index = progress * mobLabel->getString().getSize();

      if (i < (int)index) {
        newstr += mobLabel->getString()[i];
      }
      else {
        if (mobLabel->getString()[i] != ' ') {
          newstr += (char)(((rand() % (90 - 65)) + 65) + 1);
        }
        else {
          newstr += ' ';
        }
      }
    }

    int randAttack = 0;
    int randSpeed = 0;
    int randHP = 0;

    int count = (int)mobinfo.GetHPString().size() - 1;
    while (count >= 0) {
      int index = (int)std::pow(10.0, (double)count);
      index *= (rand() % 9) + 1;

      randHP += index;

      count--;
    }
    
    count = (int)mobinfo.GetAttackString().size() - 1;

    while (count >= 0) {
      int index = (int)std::pow(10.0, (double)count);
      index *= (rand() % 9) + 1;

      randAttack += index;

      count--;
    }
    
    count = (int)mobinfo.GetSpeedString().size() - 1;

    while (count >= 0) {
      int index = (int)std::pow(10.0, (double)count);
      index *= (rand() % 9) + 1;

      randSpeed += index;

      count--;
    }

    attackLabel->setString(std::to_string(randAttack));
    speedLabel->setString(std::to_string(randSpeed));
    hpLabel->setString(std::to_string(randHP));
    mobLabel->setString(sf::String(newstr));
  }

  
  /**
   * End scramble effect 
   */

  factor -= (float)elapsed * PIXEL_SPEED;

  if (factor <= 0.f) {
    factor = 0.f;
  }

  // Progress for data scramble effect
  float progress = (maxNumberCooldown - numberCooldown) / maxNumberCooldown;

  if (progress > 1.f) { 
    progress = 1.f; 
  
  if(!gotoNextScene) {
      textbox.Play();
    }
  }

  // Make a selection
  if (INPUTx.Has(EventTypes::PRESSED_CONFIRM) && !gotoNextScene) {
    
    if (MOBS.Size() != 0) {
      mob = MOBS.At(mobSelectionIndex).GetMob();
    }

    if (!mob) {
      // Play error buzzer if the mob index is out of range or
      // data is invalid
      gotoNextScene = false;

      AUDIO.Play(AudioType::CHIP_ERROR, AudioPriority::lowest);
    }
    else {
      gotoNextScene = true;

      // Play the pre battle rumble sound
      AUDIO.Play(AudioType::PRE_BATTLE, AudioPriority::high);

      // Stop music and go to battle screen 
      AUDIO.StopStream();

      // Get the navi we selected
      Player* player = NAVIS.At(selectedNavi).GetNavi();

      // Shuffle our new folder
      CardFolder* newFolder = selectedFolder.Clone();
      newFolder->Shuffle();

      // Queue screen transition to Battle Scene with a white fade effect
      // just like the game
      MobBattleProperties props{ 
        { *player, programAdvance, newFolder, mob->GetField(), mob->GetBackground() },
        MobBattleProperties::RewardBehavior::take,
        { mob }
      };

      using effect = segue<WhiteWashFade>;
      getController().push<effect::to<MobBattleScene>>(props);
    }
  }

  // Close mouth when there's a space.
  // Overload boolean logic to close mouth whenever the text box is also paused
  bool isEqual = !textbox.IsPlaying() || textbox.GetCurrentCharacter() == '\0';

  if (isEqual && navigatorAnimator.GetAnimationString() != "IDLE") {
    navigatorAnimator.SetAnimation("IDLE");
    navigatorAnimator << Animator::Mode::Loop;
  }
  else if(!isEqual && navigatorAnimator.GetAnimationString() != "TALK") {
    navigatorAnimator.SetAnimation("TALK");
    navigatorAnimator << Animator::Mode::Loop;
  }
}

void SelectMobScene::onDraw(sf::RenderTexture & surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);
  ENGINE.Draw(menuLabel);

  // Draw mob name with shadow
  mobLabel->setPosition(sf::Vector2f(30.f, 30.0f));
  mobLabel->setFillColor(sf::Color(138, 138, 138));
  ENGINE.Draw(mobLabel);
  mobLabel->setPosition(sf::Vector2f(30.f, 28.0f));
  mobLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(mobLabel);

  // Draw attack rating with shadow
  attackLabel->setPosition(382.f, 80.f);
  attackLabel->setFillColor(sf::Color(88, 88, 88));
  ENGINE.Draw(attackLabel);
  attackLabel->setPosition(380.f, 78.f);
  attackLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(attackLabel);

  // Draw speed rating with shadow
  speedLabel->setPosition(382.f, 112.f);
  speedLabel->setFillColor(sf::Color(88, 88, 88));
  ENGINE.Draw(speedLabel);
  speedLabel->setPosition(380.f, 110.f);
  speedLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(speedLabel);

  // Draw hp
  hpLabel->setPosition(382.f, 144.f);
  hpLabel->setFillColor(sf::Color(88, 88, 88));
  ENGINE.Draw(hpLabel);
  hpLabel->setPosition(380.f, 142.f);
  hpLabel->setFillColor(sf::Color::White);
  ENGINE.Draw(hpLabel);

  // Pixelate the mob texture
  if (mobSpr.getTexture()) {
    sf::IntRect t = mobSpr.getTextureRect();
    sf::Vector2u size = mobSpr.getTexture()->getSize();
    shader.SetUniform("x", (float)t.left / (float)size.x);
    shader.SetUniform("y", (float)t.top / (float)size.y);
    shader.SetUniform("w", (float)t.width / (float)size.x);
    shader.SetUniform("h", (float)t.height / (float)size.y);
    shader.SetUniform("pixel_threshold", (float)(factor / PIXEL_MAX));

    // Refresh mob graphic origin every frame as it may change
    float scale = (PIXEL_MAX-factor) / PIXEL_MAX;
    scale *= 2.f;

    mobSpr.setOrigin(mobSpr.getTextureRect().width / 2.f, mobSpr.getTextureRect().height / 2.f);
    mobSpr.setScale(scale, scale);
    mobSpr.SetShader(shader);

    ENGINE.Draw(mobSpr);
  }

  ENGINE.Draw(textbox);
  ENGINE.Draw(navigator);

  // Draw the LEFT cursor
  if (mobSelectionIndex > 0) {
    cursor.setColor(sf::Color::White);
  }
  else {
    // If there are no more items to the left, fade the cursor
    cursor.setColor(sf::Color(255, 255, 255, 100));
  }
     
  // Add sine offset to create a bob effect
  auto offset = std::sin(elapsed*10.0) * 5;
  
  // Put the left cursor on the left of the mob
  cursor.setPosition(23.0f + (float)offset, 130.0f);
  
  // Flip the x axis
  cursor.setScale(-2.f, 2.f);
  
  // Draw left cursor
  ENGINE.Draw(cursor);

  // Draw the RIGHT cursor
  if (mobSelectionIndex < (int)(MOBS.Size() - 1)) {
    cursor.setColor(sf::Color::White);
  } else {
    // If there are no more items to the right, fade the cursor
    cursor.setColor(sf::Color(255, 255, 255, 100));
  }

  // Add sine offset to create a bob effect
  offset = -std::sin(elapsed*10.0) * 5;
  
  // Put the right cursor on the right of the mob
  cursor.setPosition(200.0f + (float)offset, 130.0f);
  
  // Flip the x axis
  cursor.setScale(2.f, 2.f);
  
  // Draw the right cursor
  ENGINE.Draw(cursor);
}

void SelectMobScene::onResume() {
  if (mob) {
    delete mob;
    mob = nullptr;
  }

  // Fix camera if offset from battle?
  // ENGINE.SetCamera(camera);

  // Re-play music
  AUDIO.Stream("resources/loops/loop_navi_customizer.ogg", true);

  gotoNextScene = false;
  Logger::Log("SelectMobScene::onResume()");

#ifdef __ANDROID__
  StartupTouchControls();
#endif
}

void SelectMobScene::onStart() {
  textbox.Play();
  doOnce = true;
  showMob = true;
  gotoNextScene = false;
  factor = PIXEL_MAX;

#ifdef __ANDROID__
  StartupTouchControls();
#endif

  Logger::Log("SelectMobScene::onStart()");
}

void SelectMobScene::onLeave() {
  textbox.Stop();

#ifdef __ANDROID__
  ShutdownTouchControls();
#endif

  Logger::Log("SelectMobScene::onLeave()");

}

void SelectMobScene::onExit() {
  //textbox.SetText("");

  Logger::Log("SelectMobScene::onExit()");

}

void SelectMobScene::onEnter() {
  Logger::Log("SelectMobScene::onEnter()");
}

void SelectMobScene::onEnd() {
#ifdef __ANDROID__
  ShutdownTouchControls();
#endif
  Logger::Log("SelectMobScene::onEnd()");

}

#ifdef __ANDROID__
void SelectMobScene::StartupTouchControls() {
  /* Android touch areas*/
  TouchArea& rightSide = TouchArea::create(sf::IntRect(240, 0, 240, 320));

  releasedB = false;

  rightSide.enableExtendedRelease(true);

  rightSide.onTouch([]() {
      INPUTx.VirtualKeyEvent(InputEvent::RELEASED_A);
  });

  rightSide.onRelease([this](sf::Vector2i delta) {
      if(!releasedB) {
        INPUTx.VirtualKeyEvent(InputEvent::PRESSED_A);
      }
  });

  rightSide.onDrag([this](sf::Vector2i delta){
      if(delta.x < -25 && !releasedB) {
        INPUTx.VirtualKeyEvent(InputEvent::PRESSED_B);
        INPUTx.VirtualKeyEvent(InputEvent::RELEASED_B);
        releasedB = true;
      }
  });
}

void SelectMobScene::ShutdownTouchControls() {
  TouchArea::free();
}
#endif