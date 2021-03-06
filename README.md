# FreeAnimHelpers
Unreal Engine 5 plugin with some animation helpers.

Currenty, it contains two useful animation helpers.

## Usage

Copy *FreeAnimHelpers* folder to [your project]/Plugins or Engine/Plugins and enable it in Project Settings -> Plugins window.

### SnapFootToGround (Animation Modifier)

Modifier to snap character feet to ground. I use it for turn-in-place animation like [this](https://www.youtube.com/watch?v=TX2gcdWHLpY).

1. Open Skeletal Mesh used in animation you need to modify.

2. For both legs, add tip sockets:
- attached to foot bone;
- located at ball and at the ground, orientation doesn't matter.

![Tip socket placement](readme_tip.jpg)

3. Open animation sequence asset in Animation Editor, then Animation Data Modifiers window (via Windows menu) and add **SnapFootToGround** modifier.

4. Set *Snap Foot Orientation* checkbox, if you want to make feet horizontal.

5. Fill names of feet bones and tip sockets, then right click and select *Apply Modifier*.

6. Save animation sequence.

### PrepareTurnInPlaceAsset (Animation Modifier)

For my personal specific puropses. You don't need it.

### ResetBonesTranslation (Animation Modifier)

For all bones with "Translation Retargeting Option" = "Skeleton" in the skeleton hierarchy, this modifier changes local translation in animation sequence to skeleton-default. In other words, after thes modifier you can reset "Translation Retargeting Option" for all bones back to "Animation". Useful if you want to export to FBX animation sequence retargeted from another skeleton.

Usage: add modifier to animation sequence, select desired skeletal mesh (to get local translations of bones) and apply it.


## To Do

- add/remove root motion
- insert T-pose/reference pose in first frame of animation sequence
