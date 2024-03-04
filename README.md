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

The plugin has been developed with these ideas in mind:
1. Minimum impact on the server performance
2. Built for competitive teams to practice (not for tournaments)

## Available Commands

* Config and server management (Admin only)  
  `.pracc`: launch the practice config  
  `.scrim`: start the competitive match  
  `.map <map>`: change the map (example: `.map ancient`)  
  `.rcon <command>`: workaround to be used as the old rcon, execute commands from server side. Usage on the console `c_rcon <comnand>`  

* Scrim commands  
  `.pause`: pauses the match  
  `.unpause`: request the unpause  
  * Coach  
    `.coach`: switch to coach, also available `.coach <ct/t>`  
    `.uncoach`: return as a player  
  * Admin only  
    `.record`: start the demo recording  
    `.stoprecord`: finish the recording and save it under `gotv/<date_time_map>.dem` example: `gotv/03March_20-08_de_anubis.dem`  
    `.restore <round>`: load the desired round's backup  
    `.forceunpause`: force the unpause  

* Practice commands  
    `.spawn`: move to the desired competitive spawn i.e `.spawn 2` or `.spawn 1 ct`  
    `.last`: teleports to your latest thrown grenade lineup  
    `.noflash`: toggle flash effect  
    `.ct`, `.t`, `.spec`, `.side`: switch team side  

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
  - [X] Medic (round backup/restore)
  - [X] Coach
  - [X] gotv .record
    - [X] include date and time
    - [X] include map name
      - [X] Automatically
  - [ ] Damage print on round-end
- Practice
  - Maps
    - [X] Map
    - [X] Spawns
      - [X] Only show competitive spawns
  - Utility (grenades)
    - Flashes
      - [X] No flash effect
      - [X] Flashed targets and time flashed
    - [ ] Remove smokes
    - [ ] Rethrow
    - [ ] Throw information (air-time, bounces...)
      - [X] Flash target and time
    - [ ] Save throws and lineups
      - [X] Last
  - Misc
    - [X] Move to spec or team side
    - [X] Noclip
    - [ ] Create a "menu" workaround, using shoot and reload feedback for example, to confirm or cancel
    - [X] Damage information
  - BOTs management
      - [ ] Place
      - [ ] Boost
- General
  - [X] rcon

> I'll try to documentate everything as better as possible, feel free to leave suggestions!


## Installation instructions

[![Set up video](https://img.youtube.com/vi/thk78MDsQnc/0.jpg)]([https://www.youtube.com/watch?v=YOUTUBE_VIDEO_ID_HERE](https://www.youtube.com/watch?v=thk78MDsQnc))  
Set up video: https://www.youtube.com/watch?v=thk78MDsQnc

### cs2scrim config
You can enable/disable the features of the plugin by changing the values at `<server>/game/csgo/cfg/cs2scrim/cs2scrim.cfg`

## Compilation

### Requirements

- [Metamod:Source](https://www.sourcemm.net/downloads.php/?branch=master) (build 1282 or higher)
- [AMBuild](https://wiki.alliedmods.net/Ambuild)

## Credits

### Authors from CS2Fixes
[original repository](https://github.com/Source2ZE/CS2Fixes/)
- [@xen-000](https://github.com/xen-000)
- [@poggicek](https://github.com/poggicek)

This people did the hard work, and keep doing it adding features to CS2Fixes, check them out (they also made the zombie reborn innit).
I contacted Xen through discord before releasing this repository and he gave me some suggestions and approval to release it within their license, thanks!

### Splewis
Almost all of the features are originally from the CS:GO plugin https://github.com/splewis/csgo-practice-mode, using it as reference. Thanks @splewis! (hope to see your work on CS2)

### CKRAS Hosting
[CKRAS Hosting Team](https://www.ckras.com/en) They liked the addon, they contacted me and helped trying out the best way to create the coach, find bugs and give feedback.

### Cierzo eSports team
My teammates are being supportive by bringing feedback, ideas and testing, I mean, server crashes, toooo many server crashes ♥