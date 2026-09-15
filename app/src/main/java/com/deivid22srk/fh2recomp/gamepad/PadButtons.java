package com.deivid22srk.fh2recomp.gamepad;

/** Bitmask for discrete driving buttons sent to native code. */
public final class PadButtons {
    public static final int HANDBRAKE = 1 << 0; // A (ou B, alt) / freio de mão
    public static final int NITRO     = 1 << 1; // RESERVADO: FH2 não tem nitro (não mapear)
    public static final int CAMERA    = 1 << 2; // X
    public static final int GEAR_UP   = 1 << 3; // Y / RB
    public static final int GEAR_DOWN = 1 << 4; // LB
    public static final int PAUSE     = 1 << 5; // Start
    public static final int RADIO     = 1 << 6; // D-pad

    private PadButtons() {}
}
