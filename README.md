# ManyEmu

An emulator that can run many games and systems at the same time.

## Disclaimer

This is a project I am working on in my spare time just for the fun of writing an emulator. It's in its early stages. For now, you won't have any nice UI or command line options and many games won't work correctly.

The project is still missing a lot of features that you would expect from an emulator. You might want to take a look at it again later once it's a little more mature.

## Features

* Support for more than one system type. For now, only NES games can be played. Gameboy Color support is under way.
* Battery support.
* Automatically save your game when exiting and reload your session when restarting the game.
* Capture and replay inputs from the controller.
* Rewind your games back in time.

## Build instructions

For the moment, this project will only run on Windows. You will need Visual Studio 2013 to build it but you can easily change MakeSolution.bat to support older versions of Visual Studio.

To build the game, you must first run MakeSolution.bat. This will generate a solution for you under a directory named _build_. You can then open the solution, build it and run the game.

## Usage

Currently, the emulator has no UI and doesn't handle any command line option. You will need to change the source code to specify the game you want to launch and the options to use.

To select the game to run and the options to apply, you will need to edit the main() function in ManyEmu\Main.cpp. To configure the game, just set the properties of the config object with whatever you want. Refer to the Application::Config structure for the available options.

## Controls

| Input  | Keyboard      | Gamepad          |
| ------ | ------------- | ---------------- |
| Exit   | Esc           |                  |
| Rewind | Backspace     |                  |
| D-Pad  | Arrows/Keypad | D-Pad/Left stick |
| A      | Z             | A/Y              |
| B      | X             | B/X              |
| Select | A             | Back             |
| Start  | S             | Start            |

## Roadmap

I am planning to add the following features in no specific order:

* Basic UI functionality
* Command line support
* Inputs customization
* Support for more NES mappers
* Gameboy color emulation
* Support for other operating systems
* Allow many games to run at the same time in the UI

