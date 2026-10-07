# FlashRDR2

Experimental RDR2 Story Mode ScriptHookRDR2 ASI mod for Flash-style super speed.

Controls:
- F6: ON/OFF
- W + Shift: accelerate
- release W or Shift: decelerate
- F9: emergency reset

Default target boost: 32 m/s (~115 km/h).

The mod adds:
- smooth acceleration/deceleration
- forward velocity boost
- motion blur
- optional world slow-motion
- optional external custom Flash ped model
- HUD speed readout

The copyrighted Flash model itself is NOT bundled.

To use your own compatible installed model:

UseCustomModel=1
CustomModelName=your_registered_model_name

Build:
1. Put this project under mods/flash-rdr2 in your repository.
2. Put .github/workflows/flash-rdr2-windows.yml in the repository's .github/workflows folder.
3. Commit/push.
4. Open GitHub Actions.
5. Run "Build FlashRDR2 Windows x64".
6. Download artifact "FlashRDR2-Windows-x64-experimental".

Install:
1. Keep ScriptHookRDR2.dll + dinput8.dll beside RDR2.exe.
2. Extract the built artifact.
3. Run INSTALL.cmd.
4. Choose RDR2.exe.
5. Type INSTALL.
6. Steam -> RDR2 -> Story Mode.
7. Press F6.

Do not use ASI mods in Red Dead Online.

Status:
This is an experimental source build and still needs a real Windows compile + in-game test.
