# TrafficCarsRDR2

Experimental Story Mode traffic conversion.

What it does:
- Scans nearby NPC riders.
- Converts a configurable percentage of mounted NPCs into visual car traffic.
- The original horse remains invisible and supplies Rockstar's road/path AI.
- An addon car object (`ironroadster` by default) is attached over the hidden mount.
- This avoids trying to teach RDR2's horse AI how to drive a completely new physics vehicle.

Requirements:
- ScriptHookRDR2
- LML
- The external `ironroadster` model assets from the RDR2AddonVehicles pack (or another compatible object model configured in INI).

Control:
- F10: traffic cars ON/OFF. Turning OFF restores visible horses and deletes attached car shells.

Important:
This is a practical visual traffic hack, not GTA V vehicle AI. NPC route-following comes from the hidden horse. It is far more stable than deleting every horse and attempting to replace the entire RDR2 navigation system.
