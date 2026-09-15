package com.deivid22srk.fh2recomp;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

/**
 * First-run setup: SAF folder picker + performance options.
 * No game assets are bundled; the user must supply their own legal dump.
 */
public class SetupActivity extends Activity {
    private static final int REQ_TREE = 1001;
    private TextView folderLabel;
    private SeekBar resSeek;
    private TextView resLabel;
    private Spinner fpsSpinner;
    private Spinner apiSpinner;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ScrollView sv = new ScrollView(this);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(48, 48, 48, 48);
        root.setGravity(Gravity.CENTER_HORIZONTAL);
        sv.addView(root);
        setContentView(sv);

        TextView title = new TextView(this);
        title.setText(R.string.setup_title);
        title.setTextSize(20);
        root.addView(title);

        TextView desc = new TextView(this);
        desc.setText(R.string.setup_desc);
        desc.setPadding(0, 16, 0, 16);
        root.addView(desc);

        folderLabel = new TextView(this);
        refreshFolderLabel();
        root.addView(folderLabel);

        Button pick = new Button(this);
        pick.setText(R.string.pick_folder);
        pick.setOnClickListener(v -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
            i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                    | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
            startActivityForResult(i, REQ_TREE);
        });
        root.addView(pick);

        resLabel = new TextView(this);
        root.addView(resLabel);
        resSeek = new SeekBar(this);
        resSeek.setMax(50); // 0.50..1.00 (matches native clamp)
        resSeek.setProgress(Math.round((GameFiles.getResScale(this) - 0.5f) * 100f));
        resSeek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar s, int p, boolean fromUser) {
                updateResLabel();
            }
            @Override public void onStartTrackingTouch(SeekBar s) {}
            @Override public void onStopTrackingTouch(SeekBar s) {
                float scale = 0.5f + s.getProgress() / 100f;
                GameFiles.prefs(SetupActivity.this).edit().putFloat(GameFiles.KEY_RES_SCALE, scale).apply();
            }
        });
        root.addView(resSeek);
        updateResLabel();

        TextView fpsT = new TextView(this);
        fpsT.setText("FPS alvo (mundo aberto = 30 recomendado)");
        root.addView(fpsT);
        fpsSpinner = new Spinner(this);
        ArrayAdapter<String> fpsAd = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, new String[]{"30", "60"});
        fpsAd.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        fpsSpinner.setAdapter(fpsAd);
        fpsSpinner.setSelection(GameFiles.getTargetFps(this) == 60 ? 1 : 0);
        root.addView(fpsSpinner);

        TextView apiT = new TextView(this);
        apiT.setText("GPU: Auto (Vulkan se disponível, senão GLES 3.1+)");
        root.addView(apiT);
        apiSpinner = new Spinner(this);
        ArrayAdapter<String> apiAd = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, new String[]{"Auto", "Vulkan", "OpenGL ES"});
        apiAd.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        apiSpinner.setAdapter(apiAd);
        apiSpinner.setSelection(GameFiles.getApi(this));
        root.addView(apiSpinner);

        Button start = new Button(this);
        start.setText(R.string.start_game);
        start.setOnClickListener(v -> {
            GameFiles.prefs(this).edit()
                    .putInt(GameFiles.KEY_TARGET_FPS, fpsSpinner.getSelectedItemPosition() == 1 ? 60 : 30)
                    .putInt(GameFiles.KEY_API, apiSpinner.getSelectedItemPosition())
                    .apply();
            if (!GameFiles.hasFolder(this)) {
                Toast.makeText(this, R.string.no_folder, Toast.LENGTH_LONG).show();
                // Still allow boot into status screen so CI smoke-tests pass.
            }
            startActivity(new Intent(this, MainActivity.class));
        });
        root.addView(start);

        TextView copy = new TextView(this);
        copy.setText(R.string.copyright);
        copy.setPadding(0, 24, 0, 0);
        copy.setGravity(Gravity.CENTER);
        root.addView(copy);
    }

    private void updateResLabel() {
        float scale = 0.5f + resSeek.getProgress() / 100f;
        if (scale > 1.0f) scale = 1.0f;
        resLabel.setText(String.format("Escala de resolução: %.2f (dinâmica, FBO em G-5)", scale));
    }

    private void refreshFolderLabel() {
        String u = GameFiles.getTreeUri(this);
        String p = GameFiles.getGamePath(this);
        String label = GameFiles.prefs(this).getString(GameFiles.KEY_VALID_LABEL, null);
        if ((u == null || u.isEmpty()) && (p == null || p.isEmpty())) {
            folderLabel.setText("Pasta: (não selecionada)");
        } else if (label != null) {
            folderLabel.setText("Pasta válida (" + label + ")");
        } else {
            folderLabel.setText("Pasta: " + (u != null && !u.isEmpty() ? u : p) + " (não validada)");
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQ_TREE && resultCode == RESULT_OK && data != null) {
            Uri uri = data.getData();
            if (uri != null) {
                try {
                    getContentResolver().takePersistableUriPermission(uri,
                            Intent.FLAG_GRANT_READ_URI_PERMISSION);
                } catch (Exception ignored) {}
                GameFiles.setTreeUri(this, uri);
                // Validate the tree NOW (media/ or default.xex at root).
                String label = GameFiles.validateTree(this, uri.toString());
                if (label != null) {
                    GameFiles.prefs(this).edit().putString(GameFiles.KEY_VALID_LABEL, label).apply();
                    try { NativeBridge.nativeSetAssetValidated(true, label); } catch (Throwable ignored) {}
                    Toast.makeText(this, "Pasta válida: " + label, Toast.LENGTH_LONG).show();
                } else {
                    GameFiles.prefs(this).edit().remove(GameFiles.KEY_VALID_LABEL).apply();
                    try { NativeBridge.nativeSetAssetValidated(false, ""); } catch (Throwable ignored) {}
                    Toast.makeText(this, "Pasta sem media/ nem default.xex — escolha a raiz do jogo.", Toast.LENGTH_LONG).show();
                }
                refreshFolderLabel();
            }
        }
    }
}
