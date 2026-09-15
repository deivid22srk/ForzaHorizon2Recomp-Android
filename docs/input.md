# Input — driving-first touch + 360-pattern gamepad

## Touch HUD (`VirtualPadView`)

- Left: steering stick (X-dominant analog, -1000..1000).
- Right: vertical THROTTLE / BRAKE pedals (0..1000 each, multi-touch).
- Buttons: HB (handbrake/A), CAM, G+/G-, pause, radio.
- Resolution-independent, opacity pref (`pad_opacity`).

Deliberately **not** a brawler layout: no D-pad movement cluster; pedals
replace jump/attack buttons.

## Gamepad (`PadInputBridge`)

Same driving model: left stick X = steering, LT/RT (or right-stick fallback)
= brake/throttle, A = handbrake, X = camera, Y/RB = gear up, LB = gear down,
Start = pause — matching the Xbox 360 mapping. USB + Bluetooth via
`InputDevice.SOURCE_GAMEPAD/JOYSTICK`; triggers stay analog end-to-end.

## Native contract

Both paths converge on `Input_Push(steer, thr, brk, buttons)` -> JNI
`nativePushDrivingInput`, consumed by recompiled code via `Input_*01()`.
