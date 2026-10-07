# FlashRDR2 v13.2 — TRAFFIC FIX

This fixes the reason cars were not appearing in v13.1.

The traffic scanner used to run only after Flash mode (F6) was enabled.
v13.2 moves traffic processing outside the Flash-mode branch, so it runs all the time
in Story Mode.

Changes:
- Traffic works even with Flash powers OFF.
- Scan interval reduced to 500 ms.
- TrafficPercent defaults to 100 for easy testing.
- If `ironroadster` is not installed, the mod uses the known base-game BUGGY01 hash
  (0xB3C45542).
- Vehicle shells are made mission entities and explicitly visible before attachment.
- HUD shows converted traffic count.

Controls:
- F6: Flash powers
- F10: traffic ON/OFF
- F9: Flash reset

Expected test:
1. Enter Story Mode.
2. Do NOT press F6.
3. Ride/walk near NPCs who are already mounted on horses.
4. Top-left HUD should show `TRAFFIC BUGGY01 FALLBACK` and `converted N`.
5. Mounted NPCs should get buggy shells attached while their horses are hidden.

For a modern roadster shell instead of BUGGY01, install the external `ironroadster`
LML assets from RDR2AddonVehicles.
