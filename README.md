![Build pass](https://github.com/marqdevx/mm-cs2-scrim/actions/workflows/compile-metamod-addon.yaml/badge.svg)

## IMPORTANT
This repository is based on https://github.com/Source2ZE/CS2Fixes/
Removing "unnecessary" features from the original addon.
Thanks for the work!

```
DISCLAIMER: *not* responsible for any issues or breakages caused by the plugin. 
```

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

## Roadmap
- Scrim
  - [X] Pauses
  - [X] Medic (round backup)
  - [X] Coach
  - [X] gotv .record
    - [X] include date and time
    - [ ] include map name
- Practice
  - Maps
    - [X] Map
    - [X] Spawns
      - [X] Only show competitive spawns
  - Utility
    - [ ] Remove smokes
    - [ ] Rethrow
  - Misc
    - [ ] Move to spec
    - [ ] Noclip

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
