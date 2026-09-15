package com.deivid22srk.fh2recomp;

import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;

/** Persisted user configuration + game-folder bookkeeping (SAF-first). */
public final class GameFiles {
    public static final String PREFS = "fh2recomp";
    public static final String KEY_TREE_URI = "tree_uri";
    public static final String KEY_GAME_PATH = "game_path";
    public static final String KEY_VALID_LABEL = "valid_label";
    public static final String KEY_RES_SCALE = "res_scale"; // 0.5..1.0
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
        float v = prefs(c).getFloat(KEY_RES_SCALE, 0.85f);
        return Math.max(0.5f, Math.min(1.0f, v)); // clamp legacy 1.5 values
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

    /**
     * Validates a SAF tree via DocumentsContract (framework-only, no AndroidX):
     * lists the tree root children looking for media/ or default.xex.
     * @return human-readable label (e.g. "media/ + default.xex") or null.
     */
    public static String validateTree(Context c, String uriStr) {
        if (uriStr == null || uriStr.isEmpty()) return null;
        Uri tree;
        try {
            tree = Uri.parse(uriStr);
        } catch (Exception e) {
            return null;
        }
        // Plain POSIX path: check directly.
        if (!"content".equals(tree.getScheme())) {
            java.io.File root = new java.io.File(uriStr);
            boolean media = new java.io.File(root, "media").isDirectory()
                    || new java.io.File(root, "Media").isDirectory();
            boolean xex = new java.io.File(root, "default.xex").isFile();
            if (media || xex) return root.getName() + (media ? " media/" : "") + (xex ? " default.xex" : "");
            return null;
        }
        Uri children;
        try {
            children = DocumentsContract.buildChildDocumentsUriUsingTree(
                    tree, DocumentsContract.getTreeDocumentId(tree));
        } catch (Exception e) {
            return null;
        }
        boolean media = false, xex = false;
        Cursor cur = null;
        try {
            cur = c.getContentResolver().query(children,
                    new String[]{DocumentsContract.Document.COLUMN_DISPLAY_NAME}, null, null, null);
            if (cur == null) return null;
            while (cur.moveToNext()) {
                String name = cur.getString(0);
                if ("media".equalsIgnoreCase(name)) media = true;
                if ("default.xex".equalsIgnoreCase(name)) xex = true;
            }
        } catch (Exception e) {
            return null;
        } finally {
            if (cur != null) cur.close();
        }
        if (!media && !xex) return null;
        StringBuilder sb = new StringBuilder();
        if (media) sb.append("media/");
        if (xex) { if (sb.length() > 0) sb.append(" + "); sb.append("default.xex"); }
        return sb.toString();
    }
}
