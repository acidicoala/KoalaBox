## 📖 Introduction

### ❓ What is {{ project_name }}?

{{ project_name }} is a tool for {{ store_sdk }} DLC ownership emulation in games that are _legitimately_ owned in {{ store_name }}.
It attempts to fool games that use {{ store_sdk_full }} ({{ store_sdk }}) into thinking that you own the game's DLCs.
However, {{ project_name }} does not modify the rest of the {{ store_sdk }}, hence features like multiplayer, achievements, and so on remain fully functional.

### ❔ Which games are supported?

Only games that use {{ store_sdk_full }} for the DLC ownership verification are supported.
Hence, if a game's installation directory does not contain any {{ sdk_libs_win }} files then it's definitely not supported.
Even if a game uses {{ store_sdk }} DLL, it's not guaranteed to be supported because each game might implement additional custom verification checks.
Therefore, **you have to first research the game's topic**, to see if it supports unlocking.

Additionally, there are several points to bear in mind when it comes to unlocking DLCs with {{ project_name }}:

{% if show_3rd_party_point %}
* {{ project_name }} most definitely will not work with games that use 3rd party DRM, such as games from Ubisoft, Rockstar, etc.{% endif %}
* {{ project_name }} most likely will not work with games that use Denuvo SecureDLC.
* {{ project_name }} is unlikely to unlock anything in Free-To-Play games since they typically store all player data on the corresponding game server and hence all the checks are server-side.
* {{ project_name }} will not work with games that employ additional ownership protection or if the game is using alternative DLC verification mechanism.
* {{ project_name }} is unlikely to work with games that use an anti-cheat, since they typically detect any DLL/EXE that has been tampered with. Sometimes it is possible to disable an anti-cheat, but that typically entails the loss of online capabilities. Search in the respective game topic for more information about how to disable anti-cheat.
* Some games include DLC files in their base game, regardless of whether you own the DLC or not. However, some games download additional DLC files only after a user has bought the corresponding DLC. In this case, not only will you need to install {{ project_name }}, but you also have to get the additional DLC files from somewhere else and put them into the game folder. Up-to-date DLC files often can be found in corresponding game topics.
* Some games don't use any DRM at all, in which case {{ project_name }} is useless. All you need to do is to download the DLC files and place them in the game folder.