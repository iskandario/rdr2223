# FlashRDR2 v13 — SINGLE ASI + sane scale + integrated traffic

Why v12 looked broken:
- 4.25x PED scale stretches the skeleton/clothes/hair, but RDR2 camera offsets do not scale with it.
- That is why the camera was inside the shoulder/body and hair/clothing looked deformed.

v13 defaults to 1.35x. It is still visibly larger than NPCs without wrecking the camera rig.

Everything is now inside ONE `FlashRDR2.asi`.
There is no second TrafficCars.asi to forget to build/install.

Controls:
- F6: powers on/off
- W: super run
- W + Shift: hyper turbo
- Space: super jump
- F9: reset
- F10: traffic conversion on/off

Traffic behavior:
- Every ~1.5 sec it scans nearby mounted NPCs.
- If `ironroadster` addon assets are installed, it hides the horse and attaches that car shell.
- If the addon car asset is NOT installed, it automatically falls back to the base-game `BUGGY01`
  shell so you can immediately see that traffic conversion is running.

IMPORTANT:
The fallback is an RDR2 buggy, not a modern GTA car. A modern car model physically does not exist
in the base game. For the actual `ironroadster` modern-car shell you still need the LML vehicle
assets from RDR2AddonVehicles:
https://www.nexusmods.com/reddeadredemption2/mods/5285

The Nexus pack is ~478 MB and includes 53 example vehicles.
