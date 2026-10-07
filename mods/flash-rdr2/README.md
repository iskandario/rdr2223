# FlashRDR2 v9 — FORCE RUN

This version switches to the same core technique used by open-source RDR2 trainers:
`ENTITY::APPLY_FORCE_TO_ENTITY` in local forward direction every frame while W is held.

Why:
- `SET_PED_MOVE_RATE_OVERRIDE` alone did not create enough actual speed in RDR2.
- Raw velocity and coordinate stepping looked like long jumps / sliding / clipping.
- Local forward force lets the game keep its grounded locomotion while adding real forward acceleration.

Controls:
- F6: Flash mode on/off
- W: fast run
- W + Shift: turbo run
- Space: super jump
- F9: emergency reset

Still included:
- Invincibility
- Infinite stamina
- Explosive bullet impacts
- Motion blur
- Optional external ped model

Story Mode only.
