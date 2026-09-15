package com.deivid22srk.fh2recomp;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;

/** Persisted user configuration + game-folder bookkeeping (SAF-first). */
public final class GameFiles {
    public static final String PREFS = "fh2recomp";
    public static final String KEY_TREE_URI = "tree_uri";
    public static final String KEY_GAME_PATH = "game_path";
    public static final String KEY_RES_SCALE = "res_scale"; // 0.5..1.5
    public static final String KEY_TARGET_FPS = "target_fps"; // 30/60
    public static final String KEY_API = "gfx_api"; // 0=auto 1=vulkan 2=gles
    public static final String KEY_DRAW = "draw_dist";
    public static final String KEY_SHADOW = "shadow_q";

    private GameFiles() {}

    public static SharedPreferences prefs(Context c) {
        return c.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    public static void setTreeUri(Context c, Uri uri) {
        prefs(c).edit().putString(KEY_TREE_URI, uri.toString()).apply();
    }

    public static String getTreeUri(Context c) {
        return prefs(c).getString(KEY_TREE_URI, null);
    }

    public static void setGamePath(Context c, String path) {
        prefs(c).edit().putString(KEY_GAME_PATH, path).apply();
    }

    public static String getGamePath(Context c) {
        return prefs(c).getString(KEY_GAME_PATH, "");
    }

    public static float getResScale(Context c) {
        return prefs(c).getFloat(KEY_RES_SCALE, 0.85f);
    }

    public static int getTargetFps(Context c) {
        return prefs(c).getInt(KEY_TARGET_FPS, 30);
    }

    public static int getApi(Context c) {
        return prefs(c).getInt(KEY_API, 0);
    }

    public static boolean hasFolder(Context c) {
        String u = getTreeUri(c);
        String p = getGamePath(c);
        return (u != null && !u.isEmpty()) || (p != null && !p.isEmpty());
    }
}
