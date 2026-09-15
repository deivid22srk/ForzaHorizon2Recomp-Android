package com.deivid22srk.fh2recomp.gamepad;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import com.deivid22srk.fh2recomp.NativeBridge;

/**
 * Maps an Xbox-360-compatible Android gamepad to the same driving model as
 * the touch HUD: steering (-1000..1000), throttle/brake (0..1000 from
 * analog triggers), discrete buttons bitmask.
 */
public final class PadInputBridge {
    private PadInputBridge() {}

    public static boolean isGamepad(int source) {
        return (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }

    public static void onMotion(MotionEvent e) {
        if (!isGamepad(e.getSource())) return;
        float steer = e.getAxisValue(MotionEvent.AXIS_X);
        float rt = trigger(e, MotionEvent.AXIS_RTRIGGER, MotionEvent.AXIS_RZ);
        float lt = trigger(e, MotionEvent.AXIS_LTRIGGER, MotionEvent.AXIS_Z);
        // Right stick Y as fallback throttle/brake for pads without analog triggers.
        if (rt < 0.02f && lt < 0.02f) {
            float ry = e.getAxisValue(MotionEvent.AXIS_RZ);
            if (ry < 0) rt = -ry; else lt = ry;
        }
        NativeBridge.nativePushDrivingInput(
                Math.round(clamp(steer) * 1000f),
                Math.round(clamp01(rt) * 1000f),
                Math.round(clamp01(lt) * 1000f),
                currentButtons);
    }

    private static int currentButtons = 0;

    public static boolean onKey(boolean down, int keyCode) {
        int flag = 0;
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A: flag = PadButtons.HANDBRAKE; break;
            case KeyEvent.KEYCODE_BUTTON_X: flag = PadButtons.CAMERA; break;
            case KeyEvent.KEYCODE_BUTTON_Y: flag = PadButtons.GEAR_UP; break;
            case KeyEvent.KEYCODE_BUTTON_L1: flag = PadButtons.GEAR_DOWN; break;
            case KeyEvent.KEYCODE_BUTTON_START: flag = PadButtons.PAUSE; break;
            case KeyEvent.KEYCODE_BUTTON_SELECT: flag = PadButtons.RADIO; break;
            default: return false;
        }
        if (down) currentButtons |= flag; else currentButtons &= ~flag;
        // Re-push with last known analog values (neutral if unknown).
        NativeBridge.nativePushDrivingInput(0, 0, 0, currentButtons);
        return true;
    }

    private static float trigger(MotionEvent e, int axis, int fallback) {
        float v = e.getAxisValue(axis);
        if (v <= 0.02f) v = e.getAxisValue(fallback);
        return clamp01(v);
    }

    private static float clamp(float v) { return Math.max(-1f, Math.min(1f, v)); }
    private static float clamp01(float v) { return Math.max(0f, Math.min(1f, v)); }
}
