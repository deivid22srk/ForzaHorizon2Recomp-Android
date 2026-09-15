package com.deivid22srk.fh2recomp;

import android.app.Activity;
import android.content.ComponentCallbacks2;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.TextView;

import com.deivid22srk.fh2recomp.gamepad.PadInputBridge;
import com.deivid22srk.fh2recomp.gamepad.VirtualPadView;

/**
 * Fullscreen game activity: owns the render SurfaceView + driving HUD overlay.
 * Handles Android lifecycle rigorously (pause/resume/context loss), which has
 * no equivalent on desktop ports.
 */
public class MainActivity extends Activity implements SurfaceHolder.Callback {
    private SurfaceView surfaceView;
    private VirtualPadView padView;
    private TextView statusView;
    private boolean surfaceReady;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        FrameLayout root = new FrameLayout(this);

        surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(this);
        root.addView(surfaceView,
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.MATCH_PARENT,
                        FrameLayout.LayoutParams.MATCH_PARENT));

        padView = new VirtualPadView(this);
        root.addView(padView,
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.MATCH_PARENT,
                        FrameLayout.LayoutParams.MATCH_PARENT));

        statusView = new TextView(this);
        statusView.setTextColor(0xFFFFFFFF);
        statusView.setBackgroundColor(0x88000000);
        statusView.setPadding(16, 8, 16, 8);
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = 16; lp.leftMargin = 16;
        root.addView(statusView, lp);

        setContentView(root);
        hideSystemUi();

        String gamePath = GameFiles.getGamePath(this);
        String treeUri = GameFiles.getTreeUri(this);
        String path = (gamePath != null && !gamePath.isEmpty()) ? gamePath
                : (treeUri != null ? treeUri : "");
        try {
            NativeBridge.nativeInit(path, getCacheDir().getAbsolutePath(), GameFiles.getApi(this));
            NativeBridge.nativeSetResolutionScale(GameFiles.getResScale(this));
            NativeBridge.nativeSetTargetFps(GameFiles.getTargetFps(this));
        } catch (UnsatisfiedLinkError e) {
            statusView.setText("Native lib ausente: " + e.getMessage());
        }
        refreshStatus();
    }

    private void refreshStatus() {
        try {
            statusView.setText(NativeBridge.nativeGetStatus());
        } catch (Throwable t) {
            statusView.setText("status indisponível");
        }
        statusView.postDelayed(this::refreshStatus, 3000);
    }

    private void hideSystemUi() {
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }

    @Override public void surfaceCreated(SurfaceHolder holder) {
        try { NativeBridge.nativeOnSurfaceCreated(holder.getSurface()); } catch (Throwable ignored) {}
        surfaceReady = true;
    }

    @Override public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        try { NativeBridge.nativeOnSurfaceChanged(w, h); } catch (Throwable ignored) {}
    }

    @Override public void surfaceDestroyed(SurfaceHolder holder) {
        surfaceReady = false;
        try { NativeBridge.nativeOnSurfaceDestroyed(); } catch (Throwable ignored) {}
    }

    @Override protected void onPause() {
        super.onPause();
        try { NativeBridge.nativeOnPause(); } catch (Throwable ignored) {}
    }

    @Override protected void onResume() {
        super.onResume();
        hideSystemUi();
        try { NativeBridge.nativeOnResume(); } catch (Throwable ignored) {}
        if (padView != null) padView.reset();
    }

    @Override public void onTrimMemory(int level) {
        super.onTrimMemory(level);
        try { NativeBridge.nativeOnTrimMemory(level); } catch (Throwable ignored) {}
        if (level >= ComponentCallbacks2.TRIM_MEMORY_RUNNING_CRITICAL && padView != null) {
            // Keep input responsive under memory pressure.
            padView.reset();
        }
    }

    @Override public boolean onGenericMotionEvent(MotionEvent e) {
        if (PadInputBridge.isGamepad(e.getSource())) {
            PadInputBridge.onMotion(e);
            return true;
        }
        return super.onGenericMotionEvent(e);
    }

    @Override public boolean onKeyDown(int keyCode, KeyEvent e) {
        if (PadInputBridge.onKey(true, keyCode)) return true;
        return super.onKeyDown(keyCode, e);
    }

    @Override public boolean onKeyUp(int keyCode, KeyEvent e) {
        if (PadInputBridge.onKey(false, keyCode)) return true;
        return super.onKeyUp(keyCode, e);
    }
}
