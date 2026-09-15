package com.deivid22srk.fh2recomp.gamepad;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;

import com.deivid22srk.fh2recomp.NativeBridge;

/**
 * Driving-oriented touch HUD:
 *  - left: analog steering stick (X axis dominant)
 *  - right: THROTTLE (upper) / BRAKE (lower) vertical pedals
 *  - small buttons: handbrake, camera, gear +/-, pause
 *
 * Layout is resolution-independent and configurable via opacity/size prefs.
 */
public class VirtualPadView extends View {
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    // Analog state
    private float stickCx, stickCy, stickR;
    private float stickDx;
    private int stickPointer = -1;

    // Pedals 0..1
    private float throttle, brake;
    private int throttlePointer = -1, brakePointer = -1;
    private RectF throttleRect = new RectF(), brakeRect = new RectF();

    // Buttons
    private int buttons;
    private final RectF[] btnRects = new RectF[6];
    private final String[] btnLabels = {"HB", "CAM", "G+", "G-", "| |", "R"};
    private final int[] btnFlags = {
            PadButtons.HANDBRAKE, PadButtons.CAMERA, PadButtons.GEAR_UP,
            PadButtons.GEAR_DOWN, PadButtons.PAUSE, PadButtons.RADIO };
    private int[] btnPointers = new int[]{-1,-1,-1,-1,-1,-1};

    public VirtualPadView(Context ctx) {
        super(ctx);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(4f);
        paint.setColor(Color.argb(160, 255, 255, 255));
        textPaint.setColor(Color.argb(200, 255, 255, 255));
        textPaint.setTextSize(36f);
        textPaint.setTextAlign(Paint.Align.CENTER);
        for (int i = 0; i < btnRects.length; i++) btnRects[i] = new RectF();
        setWillNotDraw(false);
    }

    public static float getOpacity(Context c) {
        SharedPreferences p = c.getSharedPreferences("fh2recomp", Context.MODE_PRIVATE);
        return p.getFloat("pad_opacity", 0.55f);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        float min = Math.min(w, h);
        stickR = min * 0.13f;
        stickCx = w * 0.14f;
        stickCy = h * 0.72f;
        float pw = w * 0.11f, ph = h * 0.30f;
        throttleRect.set(w - pw * 2 - 32, h - ph - 48, w - pw - 40, h - 48);
        brakeRect.set(w - pw - 24, h - ph - 48, w - 24, h - 48);
        float bw = w * 0.075f, bh = h * 0.10f;
        for (int i = 0; i < btnRects.length; i++) {
            float bx = w * 0.42f + (i % 3) * (bw + 16);
            float by = h * 0.62f + (i / 3) * (bh + 16);
            btnRects[i].set(bx, by, bx + bw, by + bh);
        }
    }

    @Override
    protected void onDraw(Canvas c) {
        float alpha = getOpacity(getContext());
        paint.setAlpha((int) (255 * alpha));
        textPaint.setAlpha((int) (255 * alpha));
        // steering base + knob
        c.drawCircle(stickCx, stickCy, stickR, paint);
        c.drawCircle(stickCx + stickDx * stickR, stickCy, stickR * 0.45f, paint);
        // pedals
        drawPedal(c, throttleRect, "ACE", throttle);
        drawPedal(c, brakeRect, "FRE", brake);
        // buttons
        for (int i = 0; i < btnRects.length; i++) {
            RectF r = btnRects[i];
            c.drawRoundRect(r, 16, 16, paint);
            c.drawText(btnLabels[i], r.centerX(), r.centerY() + 12, textPaint);
        }
    }

    private void drawPedal(Canvas c, RectF r, String label, float v) {
        c.drawRoundRect(r, 20, 20, paint);
        float fillH = r.height() * v;
        RectF fill = new RectF(r.left, r.bottom - fillH, r.right, r.bottom);
        Paint f = new Paint(paint);
        f.setStyle(Paint.Style.FILL);
        f.setAlpha(90);
        c.drawRoundRect(fill, 20, 20, f);
        c.drawText(label, r.centerX(), r.top - 8, textPaint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        int action = e.getActionMasked();
        int idx = e.getActionIndex();
        int id = e.getPointerId(idx);
        float x = e.getX(idx), y = e.getY(idx);

        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                if (inCircle(x, y, stickCx, stickCy, stickR * 1.6f) && stickPointer == -1) {
                    stickPointer = id;
                    stickDx = clamp((x - stickCx) / stickR, -1, 1);
                } else if (throttleRect.contains(x, y) && throttlePointer == -1) {
                    throttlePointer = id;
                    throttle = pedalVal(y, throttleRect);
                } else if (brakeRect.contains(x, y) && brakePointer == -1) {
                    brakePointer = id;
                    brake = pedalVal(y, brakeRect);
                } else {
                    for (int i = 0; i < btnRects.length; i++) {
                        if (btnRects[i].contains(x, y) && btnPointers[i] == -1) {
                            btnPointers[i] = id;
                            buttons |= btnFlags[i];
                            break;
                        }
                    }
                }
                break;
            case MotionEvent.ACTION_MOVE:
                for (int i = 0; i < e.getPointerCount(); i++) {
                    int pid = e.getPointerId(i);
                    float px = e.getX(i), py = e.getY(i);
                    if (pid == stickPointer) stickDx = clamp((px - stickCx) / stickR, -1, 1);
                    if (pid == throttlePointer) throttle = pedalVal(py, throttleRect);
                    if (pid == brakePointer) brake = pedalVal(py, brakeRect);
                }
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_CANCEL:
                if (id == stickPointer) { stickPointer = -1; stickDx = 0; }
                if (id == throttlePointer) { throttlePointer = -1; throttle = 0; }
                if (id == brakePointer) { brakePointer = -1; brake = 0; }
                for (int i = 0; i < btnPointers.length; i++) {
                    if (btnPointers[i] == id) { btnPointers[i] = -1; buttons &= ~btnFlags[i]; }
                }
                break;
        }
        push();
        invalidate();
        return true;
    }

    private void push() {
        // Triggers analógicos do 360 viram 0..1000 aqui.
        NativeBridge.nativePushDrivingInput(
                Math.round(stickDx * 1000f),
                Math.round(throttle * 1000f),
                Math.round(brake * 1000f),
                buttons);
    }

    public void reset() {
        stickDx = 0; throttle = 0; brake = 0; buttons = 0;
        push();
        invalidate();
    }

    private static boolean inCircle(float x, float y, float cx, float cy, float r) {
        float dx = x - cx, dy = y - cy;
        return dx * dx + dy * dy <= r * r;
    }

    private static float pedalVal(float y, RectF r) {
        return clamp(1f - (y - r.top) / r.height(), 0f, 1f);
    }

    private static float clamp(float v, float a, float b) {
        return Math.max(a, Math.min(b, v));
    }
}
