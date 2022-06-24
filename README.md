# FreeAnimHelpers
Unreal Engine 5 plugin with some animation helpers
Currenty, it conteins only animation helper to snap character feet to ground.

## Usage

1. Copy *FreeAnimHelpers* folder to [your project]/Plugins or Engine/Plugins and enable it in Project Settings -> Plugins window.

2. Open Skeletal Mesh used in animation you need to modify.

3. For both legs, add tip sockets:
- attached to foot bone;
- located at ball and at the ground, orientation doesn't matter.

![Tip socket placement](readme_tip.jpg)

4. Open Animation Data Modifiers window (via Windows menu) and add **SnapFootToGround** modifier.

5. Set *Snap Foot Orientation* checkbox, if you want to make feet horizontal.

6. Fill names of feet bones and tip sockets, then right click and select *Apply Modifier*.

7. Save animation sequence.
