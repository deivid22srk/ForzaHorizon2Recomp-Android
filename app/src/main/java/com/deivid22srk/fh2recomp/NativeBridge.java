package com.deivid22srk.fh2recomp;

/** Thin JNI bridge to libfh2recomp.so. All heavy logic lives in C++. */
public final class NativeBridge {
    public static final int SRC_TOUCH = 0;
    public static final int SRC_PAD = 1;

    static {
        System.loadLibrary("fh2recomp");
    }

    private NativeBridge() {}

    public static native void nativeInit(String gamePath, String cacheDir, int apiPreference);
    public static native void nativeShutdown();
    public static native void nativeSetResolutionScale(float scale);
    public static native void nativeSetTargetFps(int fps);
    public static native void nativeSetDrawDistance(float v);
    public static native void nativeSetShadowQuality(int q);
    /** Marks a content:// tree as validated by Java (DocumentsContract). */
    public static native void nativeSetAssetValidated(boolean ok, String label);
    /** Bulk-imports "<fnv-hex>.spv" files built offline (XenosRecomp+DXC). */
    public static native int nativeImportShaderCache(String dir);
    public static native void nativeOnSurfaceCreated(Object surface);
    public static native void nativeOnSurfaceChanged(int w, int h);
    public static native void nativeOnSurfaceDestroyed();
    public static native void nativeOnPause();
    public static native void nativeOnResume();
    public static native void nativeOnTrimMemory(int level);

    // Driving input state: -1000..1000 steering/throttle/brake, buttons bitmask.
    public static native void nativePushDrivingInput(int steering, int throttle, int brake, int buttons, int source);
    public static native String nativeGetStatus();
}
