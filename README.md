Rough hack of Google Native Client support in d1x-rebirth.

Based on d1x-rebirth v0.55.1 source from <http://www.dxx-rebirth.com/>

Includes slightly patched PhysicsFS (<http://icculus.org/physfs/>)

Includes DosBox OPL3 emulator for nostalgic music (<http://www.dosbox.com>)

To build:

* install native client sdk
* install SDL, libmikmod, libogg, libvorbis and SDL_mixer from naclports
* set NACL_SDK_ROOT enviroment variable
* `cd arch/nacl`
* change NACL_TARGET_PLATFORM in `scons` if not using pepper_16
* `./scons`

To run:

* `ln -s $descent_data_dir/descent.hog $descent_data_dir/descent.pig .`
* `$nacl_sdk/examples/httpy.py`
* open chrome
* enable native client for non-apps with <about:flags>
* restart chrome
* open <http://localhost:5103/d1xnacl.html>

Main things missing:

* full-screen
* network play
* custom levels
* storing player-status/savegames/demos
