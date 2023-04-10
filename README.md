# Stealthometer

The Stealthometer is a tool providing real-time statistics tracking for Hitman 3, taking inspiration from [HitmanStatistics](https://github.com/nvillemin/HitmanStatistics) and [Statman](https://github.com/OrfeasZ/Statman/tree/master/Statman) for older Hitman entries.
The statistics can be displayed in their own window and has dark mode support, ideal live streaming gameplay via window capture.

While Hitman 3 has a built-in Silent Assassin tracker already, Stealthometer provides a lot more insight, such as featuring an exact count of bodies found, which can aid development of gameplay strategies by providing details that the game's SA indicator may not.

It also works well with other modes such as **Freelancer** and contract creation (?), as well as supporting offline play, without the need for a server (or server substitute).

Additionally, this tool aims to support a wide range of potential challenge run ideas such as "all zeroes" and provide enhancements to the existing rating system, with new statistics such as a 'tension' stat (increases with loud play; chaos, panics, etc.) and more to come.

## Future

In mind for the future, though not by any means guaranteed.

- Dynamic 'Stealth Rating'.
- Play Style system.
- "Blood Money newspapers" inspired post-mission breakdowns.

## Installation

1. Download the latest version of [ZHMModSDK](https://github.com/OrfeasZ/ZHMModSDK) and install it.
2. Download the latest version of `Stealthometer` and copy it to the ZHMModSDK `mods` folder (e.g. `C:\Games\HITMAN 3\Retail\mods`).
3. Run the game and once in the main menu, press the `~` key (`^` on QWERTZ layouts) and enable `Stealthometer` from the menu at the top of the screen (you may need to restart your game afterwards).
4. Enjoy!

## Building

### 1. Clone this repository locally with all submodules.

You can either use `git clone --recurse-submodules` or run `git submodule update --init --recursive` after cloning.

### 2. Install Visual Studio (any edition).

Make sure you install the C++ and game development workloads.

### 3. Open the project in your IDE of choice.

See instructions for [Visual Studio](https://github.com/OrfeasZ/ZHMModSDK/wiki/Setting-up-Visual-Studio-for-development) or [CLion](https://github.com/OrfeasZ/ZHMModSDK/wiki/Setting-up-CLion-for-development).
