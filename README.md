# Videos w/ SOUND
#### Branch: master as of 6/01/2018
Click the thumbnail to watch on youtube. 

[![Video of engine 6/01/2018](https://img.youtube.com/vi/D6uHYNMeqxM/1.jpg)](https://www.youtube.com/watch?v=D6uHYNMeqxM) 

[![sword.png](https://s15.postimg.cc/l91tx2baj/sword.png)](https://www.youtube.com/watch?v=D6uHYNMeqxM) 

[![healing.png](https://s15.postimg.cc/ce0zmk9nf/healing.png)](https://www.youtube.com/watch?v=D6uHYNMeqxM) 

# Features

**Keyboard support for MMBN Chrono X Config.ini files**
[![tool.png](https://s15.postimg.cc/hdqmp92i3/tool.png)](https://postimg.cc/image/wmgk30w6f/)

Just copy and paste your `config.ini` file to the same folder as the executable and the engine will read it.
There is no joystick support at this time.

--------

In this demo, you can move Mega around, shoot, charge, and delete enemies on the grid. When the chip cust is full, you can bring up the chip select menu. 

The player can select chips and deselect them in the order they were added.  Return to battle and you can use the chips by pressing Right-Control. 

At this time only 5 chip types are implemented: All heath+ chips, Invsible, Cannon, CrckPanel, and Sword.
`BasicSword` is the default behavior for all the sword chips at the moment. It behaves as wide-sword attacking one panel ahead and one panel below. Each chip plays the appropriate animation.

Rename the file in `resources/database/library.txt - Copy`to `resources/database/library.txt` for a full library while playing the game.

There is 1 Program Advance: XtremeCannon. Can be activated by selecting `Cannon1 A` + `Cannon1 B` + `Cannon1 C` in order. It deals a whopping 600 points of damage, shaking the screen, and attacks the first 3 enemies vertically.
There other other PAs that can be triggered through system but are not implemented and do not do any damage. 
You can write your own PA's and add your own chips by editting the `/database` textfiles.

Mega can also be deleted, ending the demo. If mega wins, the battle results will show up with your time, ranking, and a random chip based on score.

# Controls
```
ARROWS -> Move
SPACE  -> Shoot (hold to charge)
P      -> Pause/Unpause 
Return -> Bring up chip select GUI / Hide / Continue
R CTRL -> Use a chip
```

# Wiki
Care to [contribute](https://github.com/TheMaverickProgrammer/battlenetwork/wiki)? 

# Author TheMaverickProgrammer

## Update 6/01/2018
Added battle results to the end of the match that shows time, ranking, and a new item. Items can be registered with `Mob` objects inside the `MobFactory` specialization. The ranking is authentically calculated from the official strategy guide for MMBN 6. The rewarded item is random but based on rank.

[![preview.png](https://s15.postimg.cc/6cpgwlocr/preview.png)](https://postimg.cc/image/phsq6d30n/)

Added animations for health, sword, and cannon chips. Made the tiles blink yellow for spells that enable this behavior `EnableTileHighlight(true);` in the `Spells` child classes.

## Contributions to the project
Pheelbert wrote the base tile movement code, sprite resource loading, and the rendering pipeline. I've since then added many new features off the foundation. It's becoming something entirely new. 
Here are my changes and contributions in writing:

New: 
* Optimized Shader support in render pipeline
* Pixelated battle intro shader effect
* Pause & pause state shader effect
* Flash white when hit and shader
* Support for stun (yellow) shader
* Cust bar, cust bar progess logic, and cust bar shader
* MMBN identical Mettaur battle AI -> fight in order taking turns, metts do not attack open tiles ahead, metts toggle movement in restricted space
* Entity collision support -> Restrict movement if a tile is occupied
* HP dials in real-time when attacked
* Moving off cracked tiles become broken
* Broken tiles cooldown, flicker, and then restore
* Created audio resource manager with streaming and buffering. Has support for limited channels for an authentic retro experience.
* Provided all audio for the game
* Added Audio priorities 
* Added many new sprites
* Additional keyboard events for more buttons
* Chip library system
* Chip data class
* Chip selection GUI and basic state system 
* Chip select queue and deselect in order 
* Chip select GUI is complete with all the data from the chip
* Exact chip cards are rendered
* Greyscale shader applied on currently selected or invalid chip combos
* Chip UI component for player -> Renders chip name and attack power
* Boss AI ProgsMan (work in progress)
* Throwable Spells
* HP+10 chip works in-battle
* CrckPnl chip works in-battle
* Invsble chip works in-battle
* Chip icons are rendered in battle
* Chip icons are rendered in select
* Chip combo select system is now working
* Enemy AI and state system
* Player AI, state, and integrated keyboard control system
* Artifacts
* Loading screen + logs on loading screen effect
* PA (Program Advance) system, display, loading
* Chip library now loaded from a script
* Mob + MobFactory objects
* Refactored battle scene to accept Mob as input type and animate the battle intro
* Logger now spits out a file `log.txt` for debugging
* Loading screen + loading screen custom graphics
* Camera quakes
* New animation support
* Battle results 
* Post-battle reward system
* Authentic MMBN 6 ranking system 
* Tile highlighting as seen in the games by enemy attacks and some player chips

Changes from original author:

* Fixed bullet bug -> Bullets would never make it to back row (index error)
* Fixed wave bug -> Metts in back row could not spawn spell ^
* Improved animation times for smoother natural mmbn experience
* Fixed mega death bug -> Game no longer freezes on close
* Keyboard names and controls
* Moved origin logic for health UI components into its own function in Entity class Entity::getAnimOffset()
* Upgraded Thor and SFML
* Rewrote AnimationComponent to use latest Thor API
* Rewrote player movement code to use latest AnimationComponent features
* Refactored Logger class to queueue and dequeeue logs for use in multithreaded loading screen
* Rewrote logger to support queueing/dequeueing across threads for loading screen support
* Done away with Thor:: dependencies and wrote my own animation class and animation editor

# Author Pheelbert
Wrote the foundation for the battle engine. He wrote the tile-based movement and update system emulating an authentic mmbn player experience.

## battlenetwork
## =============

https://www.youtube.com/watch?v=GQa0HsVPNE8&feature=youtu.be

A Megaman Battle Network project that I work on occasionally. Made from scratch with SFML and Thor in C++.

'extern/dlls/*(-d).dll' must be copied to the Release/ or Debug/

Be wary of the license, this project was created and I am still working on it for academic reasons. It would be a shame if someone were to steal my project and call it their own!
