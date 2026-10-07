OPTIONAL McLAREN / SUPERCAR SETUP

A real McLaren cannot be created by FlashRDR2.asi alone because RDR2 has no McLaren asset.

Use the open-source RDR2 Addon Vehicles framework:
https://github.com/Silonugget/RDR2AddonVehicles

Requirements:
- Lenny's Mod Loader (LML)
- AB ScriptHookRDR2
- Vulkan advanced graphics
- RDR2AddonVehicles.asi + vehicleconfig.json
- A compatible converted RDR2 vehicle model (YDR/YTD/etc.)

The framework supports real driveable addon cars and exposes top-speed/acceleration config.
Once you have a McLaren-compatible RDR2 vehicle asset, add it to that framework and tune it
as a hypercar. This is the correct architecture for 'McLaren instead of horses'; trying to
turn a horse entity into a McLaren inside FlashRDR2 would just produce broken collision and
animation.

Suggested hypercar tuning starting point in the vehicle framework:
- very high acceleration
- high top speed
- low suspension travel
- low center of gravity

FlashRDR2 v10 and RDR2AddonVehicles can run side by side.
