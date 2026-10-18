# MultiBound 3 // Starbound Instance Launcher
This is a fork of [zetaPRIME/MultiBound2](https://github.com/zetaPRIME/MultiBound2), and the only purpose is fixing incompatibilities with modern versions of steamcmd and OpenStarbound, there will be no new features. ~~Okay I made it Qt 6 too, it comes with native dark mode, but I won't add a switch for that!~~ This fork is very likely Windows-only, I don't even test if it builds in Linux and macOS. As usual, there is no support.

## To-do list
- [x] Fix workshop download location mismatch error
- [ ] Fix OpenStarbound CI build download (Windows only, osb no longer offers `.zip` for Linux and macOS, and this is not supported by nightly.link)
- [ ] Fix OpenStarbound release version download
- [ ] Fix OpenStarbound asset path, they switched to `assets` since forever, but I might need a fallback incase someone drop-in updates from MultiBound 2

## Original README
Here's the original readme if you want to know how to use MultiBound.
<details>
<summary>Click to expand</summary>

# MultiBound 2 // Starbound Instance Launcher
MultiBound enables you to quickly and easily set up and use multiple separate instances (or profiles) of Starbound with their own mod list and save directory.

## Features
- Easily create, update and launch modpack profiles from Steam Workshop collections
- Automatically installs and updates Workshop mods without requiring you to subscribe to them on Steam (yes, even if your copy of the game is from GOG)
- OpenStarbound integration - handles installing, updating and launching your profiles with OSB (supports both release versions and CI/nightly builds)

## Installing and running
- **Windows:** Unzip anywhere writable and double-click `multibound.exe`.
- **Arch Linux:** Install `multibound-git` from the AUR; "MultiBound 2" will be added to your launcher.
- **Other Linux/\*nix:** Clone from git and compile using QtCreator like you would with any other Qt application.

## How to use
(instructions pending; once you start it up it should be fairly self-explanatory, though!)

## How to compile
Make sure you have:
- Up to date QtCreator

Open up the project and build like you would any other Qt application, configuring with the "Desktop" kit.

</details>