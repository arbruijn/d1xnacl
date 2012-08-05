Rough hack of Google Native Client support in d1x-rebirth.

Based on d1x-rebirth v0.55.1 source from <http://www.dxx-rebirth.com/>

Includes slightly patched PhysicsFS (<http://icculus.org/physfs/>)

Includes DosBox OPL3 emulator for nostalgic music (<http://www.dosbox.com>)

To build:

* install native client sdk (<https://developers.google.com/native-client/sdk/download>)
* set NACL_SDK_ROOT to ...naclsdk/pepper_20
* install SDL, libmikmod, libogg, libvorbis and SDL_mixer from naclports:

  * mkdir naclports
  * cd naclports
  * svn checkout http://naclports.googlecode.com/svn/trunk/src
    (this directory must be named `src`!)
  * cd src
  * export NACL_PACKAGES_BITSIZE=32
  * make sdl mikmod ogg vorbis sdl_mixer
  * export NACL_PACKAGES_BITSIZE=64
  * make sdl mikmod ogg vorbis sdl_mixer

* `cd d1xnacl/arch/nacl`
* `./scons`

To run:

* `ln -s $descent_data_dir/descent.hog $descent_data_dir/descent.pig .`
* `$NACL_SDK_ROOT/examples/httpy.py --no_dir_check`
* open chrome
* enable native client for non-apps with about:flags
* restart chrome
* open <http://localhost:5103/d1xnacl.html>

Main things missing:

* full-screen
* network play
* custom levels
* storing player-status/savegames/demos
