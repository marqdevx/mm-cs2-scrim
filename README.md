![Build pass](https://github.com/marqdevx/mm-cs2-scrim/actions/workflows/compile-metamod-addon.yaml/badge.svg)

## IMPORTANT
This repository is based on https://github.com/Source2ZE/CS2Fixes/
Removing "unnecessary" features from the original addon.
Thanks for the work!

```
DISCLAIMER: *not* responsible for any issues or breakages caused by the plugin. 
```

### Fix server crashes
In case you have non-expected server crashes, it might be due the `addons\cs2scrim\gamedata\cs2fixes.games.txt` is not up-to-date.
I might not update that text until a next "official" release of the addon, so in order to fix that file:
Copy the content of the file https://github.com/marqdevx/mm-cs2-scrim/blob/main/gamedata/cs2fixes.games.txt onto your `addons\cs2scrim\gamedata\cs2fixes.games.txt`

# Addon information
This plugin adds the minimum requirements for competitive teams that need a more handy way of managing a few commands from the server side, like config management or replay recording.

## Available Commands

* Config and server management (Admin only)  
  `.pracc`: launch the practice config  
  `.scrim`: start the competitive match  
  `.map <map>`: change the map (example: `.map ancient`)  

* Scrim commands  
  `.pause`: pauses the match  
  `.unpause`: request the unpause  
  * Coach  
    `.coach`: switch to coach  
    `.uncoach`: return as a player  
  * Admin only  
    `.record`: start the demo recording  
    `.stoprecord`: finish the recording and save it under `gotv/<date>.dem`  
    `.restore <round>`: load the desired round's backup  
    `.forceunpause`: force the unpause  

* Practice commands  
    `.spawn`: move to the desired competitive spawn i.e `.spawn 2`  

* Leveraged from [CS2Fixes](https://github.com/Source2ZE/CS2Fixes/)  
  `c_reload_admins`: console command to reload admins.cfg  
  `.ban <player> <duration/0 (permanent)>`: ban a player  
  `.unban <player>`: remove ban of player  
  `.kick <player>`: kick a player out of the server  
  `.gag <player> <duration>`: forbids chat to player  
  `.ungag <player>`: allow back a player that was gagged to chat  
  `.slay <player>`: kills a player  
  `.noclip`: toggles noclip to current player (practice mode)  

## Roadmap
- Scrim
  - [X] Pauses
  - [X] Medic (round backup)
  - [X] Coach
  - [X] gotv .record
    - [X] include date and time
    - [ ] include map name
  - [ ] Damage print on round-end
- Practice
  - Maps
    - [X] Map
    - [X] Spawns
      - [X] Only show competitive spawns
  - Utility
    - [ ] Remove smokes
    - [ ] Rethrow
    - [ ] Throw information (air-time, bounces...)
    - [ ] Save throws and lineups
  - Misc
    - [ ] Move to spec
    - [X] Noclip
    - [ ] No flash effect
    - [ ] Create a "menu" workaround, using shoot and reload feedback for example, to confirm or cancel
  - BOTs management
      - [ ] Place
      - [ ] Boost
      - [ ] Util and damage feedback

> I'll try to documentate everything as better as possible, feel free to leave suggestions!

## Compilation

### Requirements

- [Metamod:Source](https://www.sourcemm.net/downloads.php/?branch=master) (build 1219 or higher)
- [AMBuild](https://wiki.alliedmods.net/Ambuild)

### Instructions
[![Set up video](https://img.youtube.com/vi/thk78MDsQnc/0.jpg)]([https://www.youtube.com/watch?v=YOUTUBE_VIDEO_ID_HERE](https://www.youtube.com/watch?v=thk78MDsQnc))  
Set up video: https://www.youtube.com/watch?v=thk78MDsQnc

## Authors from the [original repository](https://github.com/Source2ZE/CS2Fixes/)
- [@xen-000](https://github.com/xen-000)
- [@poggicek](https://github.com/poggicek)
