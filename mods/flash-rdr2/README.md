# FlashRDR2 v8 — REAL RUN

Removed completely:
- Flight mode
- Coordinate teleport movement
- Raw velocity / launch movement

Flash run now uses only RDR2's own PED::SET_PED_MOVE_RATE_OVERRIDE every frame.
This keeps the game's normal grounded running/sprinting locomotion instead of
making the player fly, T-pose, clip under terrain, or perform a long jump.

Controls:
- F6: Flash mode on/off
- W: super run (7.0 move-rate override)
- W + Shift: maximum turbo run (10.0 move-rate override)
- Space: super jump
- F9: emergency reset

Still included:
- Invincibility
- Infinite stamina
- Explosive bullet impacts
- Motion blur while running
- Optional external custom ped model

Story Mode only.
