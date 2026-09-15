package com.deivid22srk.fh2recomp.gamepad;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import com.deivid22srk.fh2recomp.NativeBridge;

/**
 * Maps an Xbox-360-compatible Android gamepad to the same driving model as
 * the touch HUD: steering (-1000..1000), throttle/brake (0..1000 from
 * analog triggers), discrete buttons bitmask.
 *
 * Analog values are preserved across button events (a handbrake tap must not
 * zero the steering mid-corner); the native layer merges touch/pad slots.
 */
public final class PadInputBridge {
    private static final float DEADZONE = 0.08f;

    private PadInputBridge() {}

    public static boolean isGamepad(int source) {
        return (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }

    // Last known analog state, re-sent on every event (buttons included).
    private static float lastSteer;
    private static float lastThrottle;
    private static float lastBrake;
    private static int currentButtons = 0;

    public static void onMotion(MotionEvent e) {
        if (!isGamepad(e.getSource())) return;
        float steer = applyDeadzone(e.getAxisValue(MotionEvent.AXIS_X));
        // Triggers: prefer dedicated axes; RZ/Z carry trigger data on pads
        // without analog-trigger axes (never double-use the same axis).
        float rt = e.getAxisValue(MotionEvent.AXIS_RTRIGGER);
        float lt = e.getAxisValue(MotionEvent.AXIS_LTRIGGER);
        if (rt < 0.02f && lt < 0.02f) {
            float rz = e.getAxisValue(MotionEvent.AXIS_RZ);
            float z = e.getAxisValue(MotionEvent.AXIS_Z);
            if (rz != 0f || z != 0f) {
                rt = Math.max(0f, rz);
                lt = Math.max(0f, z);
            } else {
                // Final fallback: right stick Y (up = throttle, down = brake).
                float ry = e.getAxisValue(MotionEvent.AXIS_Y);
                if (ry < -DEADZONE) rt = -ry; else if (ry > DEADZONE) lt = ry;
            }
        }
        lastSteer = steer;
        lastThrottle = clamp01(rt);
        lastBrake = clamp01(lt);
        push();
    }

    public static boolean onKey(boolean down, int keyCode) {
        int flag = 0;
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A:
            case KeyEvent.KEYCODE_BUTTON_B: // FH2 has no nitro; B = handbrake alt
                flag = PadButtons.HANDBRAKE; break;
            case KeyEvent.KEYCODE_BUTTON_X: flag = PadButtons.CAMERA; break;
            case KeyEvent.KEYCODE_BUTTON_Y: flag = PadButtons.GEAR_UP; break;
            case KeyEvent.KEYCODE_BUTTON_R1: flag = PadButtons.GEAR_UP; break;
            case KeyEvent.KEYCODE_BUTTON_L1: flag = PadButtons.GEAR_DOWN; break;
            case KeyEvent.KEYCODE_BUTTON_START: flag = PadButtons.PAUSE; break;
            case KeyEvent.KEYCODE_BUTTON_SELECT: flag = PadButtons.RADIO; break;
            default: return false;
        }
        if (down) currentButtons |= flag; else currentButtons &= ~flag;
        push(); // keeps last analog values — never zeroes steering
        return true;
    }

    private static void push() {
        NativeBridge.nativePushDrivingInput(
                Math.round(lastSteer * 1000f),
                Math.round(lastThrottle * 1000f),
                Math.round(lastBrake * 1000f),
                currentButtons,
                NativeBridge.SRC_PAD);
    }

    private static float applyDeadzone(float v) {
        if (Math.abs(v) < DEADZONE) return 0f;
        float s = Math.signum(v);
        return s * (Math.abs(v) - DEADZONE) / (1f - DEADZONE);
    }

    private static float clamp01(float v) { return Math.max(0f, Math.min(1f, v)); }
}
