# Input — driving-first touch + 360-pattern gamepad

## Touch HUD (`VirtualPadView`)

- Left: steering stick (X-dominant analog, -1000..1000).
- Right: vertical THROTTLE / BRAKE pedals (0..1000 each, multi-touch).
- Buttons: HB (handbrake/A), CAM, G+/G-, pause, radio.
- Resolution-independent, opacity pref (`pad_opacity`).

Deliberately **not** a brawler layout: no D-pad movement cluster; pedals
replace jump/attack buttons.

## Gamepad (`PadInputBridge`)

Same driving model: left stick X = steering (0.08 deadzone, rescaled),
LT/RT (RZ/Z fallback, right-stick-Y last resort) = brake/throttle,
A or B = handbrake (FH2 has no nitro — `BTN_NITRO` is reserved, unmapped),
X = camera, Y/RB = gear up, LB = gear down,
Start = pause — matching the Xbox 360 mapping. USB + Bluetooth via
`InputDevice.SOURCE_GAMEPAD/JOYSTICK`; triggers stay analog end-to-end.

## Native contract

Both paths converge on `Input_PushSource(slot, steer, thr, brk, buttons)`
(`SRC_TOUCH` / `SRC_PAD`) — slots merge by recency with buttons OR'd, so a
gamepad button tap never zeroes touch steering mid-corner (and vice-versa).
Consumed by recompiled code via `Input_*01()`.
