# MultiBound 3 // Starbound Instance Launcher
This is a fork of [zetaPRIME/MultiBound2](https://github.com/zetaPRIME/MultiBound2), and the only purpose is fixing incompatibilities with modern versions of steamcmd and OpenStarbound, there will be no new features. ~~Okay I made it Qt 6 too, it comes with native dark mode, but I won't add a switch for that!~~ **This fork is Windows-only**, I don't even test if it builds in Linux and macOS, and the corresponding parts are left untouched. As usual, there is no support.

## Bugs fixed
- **Workshop download location mismatch**
<br />An update of steamcmd around December 2025 changed steamcmd's behaviour that it no longer allows the install path being the same as its install folder. steamcmd would show the error message `Please set the game install path to something other than the Steam install folder` and fallback to default path, users have to use symlink as workaround. MultiBound 3 fixes it by using that default path directly.

- **OpenStarbound CI build download**
<br />The original workflow name check no longer matches what MultiBound expects, removed it and added some new checks like only use a successful build. Also OpenStarbound [now uses Deflate64 to compress their artifacts](https://github.com/OpenStarbound/OpenStarbound/pull/534), zip archive named differently, and put all the files inside `client_distribution` folder. Updated the extraction part to handle all these changes. Requires the bundled `7za.exe`, or have [7-Zip](https://7-zip.org/download.html)/[PowerShell 7](https://learn.microsoft.com/en-us/powershell/scripting/install/install-powershell-on-windows) installed.

- **OpenStarbound release version download**
<br />Same zip structural changes as above, so also have the same extractor requirements.

## Non bug-fix changes
- **Upgraded to Qt 6**
<br />Because I happened to only have Qt 6 installed, and didn't bother to add Qt 5. Replaced some deprecated code. It has native dark mode.

- **OpenStarbound asset path**
<br />Not really a bug, but they switched to `assets` since forever, so I changed it too. Added a fallback so that if an OpenStarbound install created by MultiBound 2 has `data` in it, MultiBound 3 will use it instead.

- **Warning messages when a selected OpenStarbound variant doesn't exist**
<br />MultiBound 2 would already fallback to other possible choice, I just added a warning message when that happens.


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