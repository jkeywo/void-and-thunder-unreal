# Playing and hosting

Start the packaged Windows executable or Play the Menu map in Unreal. Choose
hull, broadside, battery and special equipment. Extra modules must fit the hull's
mount limits; crew operates surplus mounts in the authored catalogue order.
Preferred equipment and solo career statistics are stored independently from worlds.

Create World starts a named listen-server campaign. Shared sandbox defaults to 500
mortal NPC ships across ten systems; Authored preserves the source population counts.
Continue World loads the same name, with no offline advancement. The host's profile
and reconnect tokens must remain available alongside the campaign.

Guests use Find LAN worlds / Join world, or Join address with the host's IP and
port (default `7777`). Permit the game through Windows Firewall for your LAN.
The world accepts four captains including its host. Each installation has a local
profile GUID; do not copy one personal profile to simultaneously active guests.
Closing or leaving the host ends everyone's connection. There is no host migration.
Guests return to Menu and can rejoin the same world with their saved identity/token.

## Controls

| Action | Keyboard / mouse | Controller |
|---|---|---|
| Speed / turn | Tap W/S: reverse → halt → half → full; A/D steer | Left stick (analogue) |
| Aim | Mouse | Right stick, while aiming a device |
| Port / starboard broadside | Left / right mouse | Left / right trigger |
| EMP disruptor | Q | X / square |
| Torpedoes, hold to lock; release to launch | Left Ctrl | Left shoulder |
| Microwarp, hold to mark; release to jump | Left Shift | Right shoulder |
| Boost | Space | A / cross |
| Brace | C | Y / triangle |
| Board or charge a jump | B | B / circle |
| Mines | M | D-pad left |
| Point defence | X | D-pad right |
| Menu | Escape | Start |
| Toggle AI pilot | T or menu button | Left stick press or menu button |
| Recover a disabled sandbox ship | R or menu recovery button | D-pad down |

Aim by steering to bring a broadside onto the target. Hold its mouse button or
trigger to aim; move the mouse left/right (or use the right stick) to adjust within the bank's firing arc, then release to fire. A single press starts aiming. Loaded aim beams are amber and reloading beams
are dim red. Shields have directional
banks. EMP affects systems; point defence intercepts hostile shots; torpedoes
launch from the current hull pose and arc above/below the plane. Equipment shares
battery, ammunition and authoritative cooldowns.

Remain near the station surface for three seconds to dock. Its menu offers free
repair, local heat payment, refit and undocking. Hold interact near a linked-system
gate for six seconds to jump; releasing freezes progress, leaving the gate resets
it. Only your ship travels. The chart shows global populations even where no captain
is present. Shared-faction captains can still damage and board each other. A prize
is awarded once. Destroyed hulls recover at the nearest reachable station with the
selected hull and fit; credits and standings are retained without a new penalty.

Solo Skirmish has the authored waves and final bastion. Test Range has an inert,
invulnerable target. Solo aiming, hit-stop and menus affect game time; hosted worlds
always continue in real time, including while docked or while the host is alone.

Worlds autosave every 60 simulation seconds, on guest disconnect and orderly host
exit. Campaign snapshots use `.vts` files in the game's `Saved/SaveGames` directory,
with a validated last-good backup. Personal/profile saves and Career saves are
separate Unreal SaveGame files. Legacy Rust save files are unsupported.

Development-only console helpers include `VTHost`, `VTJoin <address>`, `VTScale
<count>`, `VTJump <linked-system>`, `VTSave` and `VTLoad`. Normal charged travel and
menu flows are available in Shipping; bypass travel and validation probes are not.

Command-line hosting/joining works in Development and Shipping: `-VTHostWorld=Campaign` (add `-VTContinueWorld` to load it), or `-VTJoinAddress=192.168.1.10:7777`. These invoke the same native session flows as the menu.

The menu supports controller focus and D-pad navigation. Performance, Balanced and
High graphics buttons save personal Unreal settings independently from campaign
saves. The frontend scales to fit smaller windows; station actions appear while
docked. No physical-controller or clean-machine installer validation is claimed
by the automated keyboard/controller event probes.

The optional AI pilot flies and operates your fitted ship while you retain the
camera and captain identity. The host makes its decisions. Return to manual control
with the same toggle; restored/reconnected ships start in manual mode.

Nearby lootable ships and jump gates show the current keyboard/controller interaction buttons and hold progress beside the object. Looting prompts follow the host-selected eligible prize and disappear when it is claimed or leaves range. Stations show hold-position docking progress. The reference grid uses concentric distance rings and radial spokes from the nearest star in the current system.

Jump gates are upright rings. Within interaction range, hold the mapped interact
button (B / pad B) to guide the ship to the inner approach, align it and charge
the jump. Keep holding while it flies through the opening. A charged ship parked
near the gate does not travel; releasing cancels guided entry. Only that captain
changes system.

A fitted microwarp uses Left Shift / right shoulder: hold to aim, release to jump.
The HUD displays the active mapped binding. Warp and torpedo aiming suppress
broadside input and cancel an unfinished broadside wind-up. Optional loadout
selectors offer Empty mount explicitly; checked fits survive frontend setup and
replicated ship initialization. Torpedoes use the original projectile size and material.

Gate alignment may reverse when that reaches the staging point faster. Passage through the ring always remains forwards.

Torpedo visuals are restored to the original projectile size and material. Charged gate passage now surges forwards to the ring, flashes white at teleport, then brakes rapidly from the destination ring to the same previous arrival point.

Near a jump ring, a flat arrow marks the staging point and points through the gate. A smaller arrow loops through the acceleration, teleport and braking path. Hold interact to line up and pass through; approach and arrival distances are twice their previous lengths.

During gate acceleration the camera stays aimed at your departure position. At teleport it switches to the final stopping position and remains centred there during braking. Normal camera tracking then resumes.
