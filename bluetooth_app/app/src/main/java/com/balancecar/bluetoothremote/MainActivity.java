package com.balancecar.bluetoothremote;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.pm.PackageManager;
import android.content.pm.ActivityInfo;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RadialGradient;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends Activity {
    private static final UUID SPP_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private static final long CONTROL_PERIOD_MS = 50L;
    private static final float MAX_SPEED = 3.0f;
    private static final float MAX_TURN = 2.0f;
    private static final float DEAD_ZONE = 0.08f;
    private static final int COLOR_BG = Color.rgb(17, 19, 23);
    private static final int COLOR_PANEL = Color.rgb(30, 35, 41);
    private static final int COLOR_PANEL_ALT = Color.rgb(24, 28, 34);
    private static final int COLOR_STROKE = Color.rgb(61, 68, 78);
    private static final int COLOR_SCREEN = Color.rgb(9, 11, 15);
    private static final int COLOR_TEXT = Color.rgb(234, 238, 246);
    private static final int COLOR_MUTED = Color.rgb(145, 149, 160);
    private static final int COLOR_CYAN = Color.rgb(142, 113, 255);
    private static final int COLOR_PURPLE = Color.rgb(122, 91, 227);
    private static final int COLOR_GREEN = Color.rgb(72, 224, 93);
    private static final int COLOR_RED = Color.rgb(226, 72, 55);

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final List<BluetoothDevice> devices = new ArrayList<>();

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothSocket socket;
    private OutputStream outputStream;
    private InputStream inputStream;
    private Thread receiveThread;
    private Spinner deviceSpinner;
    private TextView statusText;
    private TextView commandText;
    private TelemetryDisplayView telemetryView;
    private TextView speedValueText;
    private TextView turnValueText;
    private boolean running;
    private float currentSpeed;
    private float currentTurn;
    private float lastSentSpeed = Float.NaN;
    private float lastSentTurn = Float.NaN;

    private final Runnable controlTask = new Runnable() {
        @Override
        public void run() {
            sendControlTargets(false);
            handler.postDelayed(this, CONTROL_PERIOD_MS);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_FULLSCREEN |
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        setContentView(createUi());
        requestBluetoothPermission();
        loadPairedDevices();
        handler.postDelayed(controlTask, CONTROL_PERIOD_MS);
    }

    private View createUi() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(8), dp(8), dp(8), dp(8));
        root.setBackgroundColor(COLOR_BG);

        LinearLayout topBar = row();
        topBar.setGravity(Gravity.CENTER_VERTICAL);

        TextView title = new TextView(this);
        title.setText("Bluetooth");
        title.setTextSize(21);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setTextColor(COLOR_TEXT);
        topBar.addView(title, new LinearLayout.LayoutParams(dp(130), LinearLayout.LayoutParams.WRAP_CONTENT));

        statusText = new TextView(this);
        statusText.setText("未连接");
        statusText.setTextSize(13);
        statusText.setTextColor(COLOR_CYAN);
        topBar.addView(statusText, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 0.8f));

        deviceSpinner = new Spinner(this);
        deviceSpinner.setBackground(panelBackground(COLOR_PANEL_ALT));
        topBar.addView(deviceSpinner, new LinearLayout.LayoutParams(0, dp(44), 1.7f));

        topBar.addView(button("刷新", COLOR_PANEL_ALT, v -> loadPairedDevices()), smallButtonParams());
        topBar.addView(button("连接", COLOR_PURPLE, v -> connectSelectedDevice()), smallButtonParams());
        root.addView(topBar);

        LinearLayout main = row();
        main.setPadding(0, dp(8), 0, 0);

        LinearLayout leftPanel = panel("");
        TextView forwardLabel = padLabel("FORWARD");
        leftPanel.addView(forwardLabel);
        JoystickView speedStick = new JoystickView(this, "SPD");
        speedStick.setOnMoveListener((x, y, released) -> {
            float speed = applyDeadZone(-y) * MAX_SPEED;
            if (released) {
                speed = 0.0f;
            }
            setSpeedTarget(speed);
        });
        leftPanel.addView(speedStick, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));
        TextView backwardLabel = padLabel("BACKWARD");
        leftPanel.addView(backwardLabel);
        speedValueText = smallValueText("SPD 0.00");
        leftPanel.addView(speedValueText);
        main.addView(leftPanel, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1.05f));

        LinearLayout centerPanel = panel("");
        View topGlow = glowBar();
        centerPanel.addView(topGlow);
        telemetryView = new TelemetryDisplayView(this);
        centerPanel.addView(telemetryView, new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            0,
            1f));

        commandText = smallValueText("等待命令");
        commandText.setTextSize(13);
        commandText.setTextColor(COLOR_MUTED);
        centerPanel.addView(commandText);

        LinearLayout actionRow = row();
        actionRow.setGravity(Gravity.CENTER);
        actionRow.setPadding(0, 0, 0, 0);
        Button startButton = button("START", Color.rgb(13, 25, 19), v -> {
            running = true;
            sendCommand("RUN 1", true);
        });
        startButton.setTextColor(COLOR_GREEN);
        Button stopButton = button("EMERGENCY\nSTOP", Color.rgb(62, 20, 16), v -> {
            setSpeedTarget(0.0f);
            setTurnTarget(0.0f);
            running = false;
            sendCommand("RUN 0", true);
        });
        stopButton.setTextColor(Color.rgb(255, 110, 91));
        stopButton.setTextSize(12);
        stopButton.setLineSpacing(0.0f, 0.88f);
        actionRow.addView(startButton, actionButtonParams());
        actionRow.addView(stopButton, actionButtonParams());
        centerPanel.addView(actionRow, new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            dp(54)));
        main.addView(centerPanel, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1.2f));

        LinearLayout rightPanel = panel("");
        TextView turnLeftLabel = padLabel("TURN LEFT");
        rightPanel.addView(turnLeftLabel);
        JoystickView turnStick = new JoystickView(this, "TURN");
        turnStick.setOnMoveListener((x, y, released) -> {
            float turn = applyDeadZone(-x) * MAX_TURN;
            if (released) {
                turn = 0.0f;
            }
            setTurnTarget(turn);
        });
        rightPanel.addView(turnStick, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));
        TextView turnRightLabel = padLabel("TURN RIGHT");
        rightPanel.addView(turnRightLabel);
        turnValueText = smallValueText("TURN 0.00");
        rightPanel.addView(turnValueText);
        main.addView(rightPanel, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1.05f));

        root.addView(main, new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            0,
            1f));
        return root;
    }

    private void setSpeedTarget(float speed) {
        currentSpeed = speed;
        speedValueText.setText(String.format(Locale.US, "SPD %.2f", currentSpeed));
        sendControlTargets(true);
    }

    private void setTurnTarget(float turn) {
        currentTurn = turn;
        turnValueText.setText(String.format(Locale.US, "TURN %.2f", currentTurn));
        sendControlTargets(true);
    }

    private float applyDeadZone(float value) {
        if (Math.abs(value) < DEAD_ZONE) {
            return 0.0f;
        }
        return value;
    }

    private void sendControlTargets(boolean forceChanged) {
        if (!running) {
            return;
        }
        sendCommand(String.format(Locale.US, "CTL %.2f %.2f", currentSpeed, currentTurn), false);
        lastSentSpeed = currentSpeed;
        lastSentTurn = currentTurn;
    }

    private LinearLayout panel(String titleText) {
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(dp(14), dp(8), dp(14), dp(8));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1f);
        params.setMargins(dp(5), 0, dp(5), 0);
        panel.setLayoutParams(params);
        panel.setBackground(panelBackground(COLOR_PANEL));

        if (!titleText.isEmpty()) {
            TextView title = padLabel(titleText);
            panel.addView(title);
        }
        return panel;
    }

    private TextView padLabel(String text) {
        TextView view = new TextView(this);
        view.setText(text);
        view.setTextSize(13);
        view.setTextColor(COLOR_TEXT);
        view.setGravity(Gravity.CENTER);
        view.setTypeface(Typeface.DEFAULT_BOLD);
        view.setPadding(0, dp(4), 0, dp(4));
        return view;
    }

    private TextView smallValueText(String text) {
        TextView view = valueText(text);
        view.setTextSize(14);
        view.setTextColor(COLOR_TEXT);
        return view;
    }

    private TextView valueText(String text) {
        TextView view = new TextView(this);
        view.setText(text);
        view.setTextSize(16);
        view.setTextColor(COLOR_TEXT);
        view.setGravity(Gravity.CENTER);
        view.setPadding(0, dp(7), 0, dp(7));
        return view;
    }

    private View glowBar() {
        View view = new View(this);
        GradientDrawable drawable = new GradientDrawable(
            GradientDrawable.Orientation.LEFT_RIGHT,
            new int[] {
                Color.TRANSPARENT,
                COLOR_PURPLE,
                Color.rgb(185, 151, 255),
                COLOR_PURPLE,
                Color.TRANSPARENT
            });
        drawable.setCornerRadius(dp(4));
        view.setBackground(drawable);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(dp(86), dp(5));
        params.gravity = Gravity.CENTER_HORIZONTAL;
        params.setMargins(0, 0, 0, dp(8));
        view.setLayoutParams(params);
        return view;
    }

    private GradientDrawable panelBackground(int color) {
        GradientDrawable drawable = new GradientDrawable(
            GradientDrawable.Orientation.TOP_BOTTOM,
            new int[] { Color.rgb(39, 44, 51), color, Color.rgb(19, 22, 27) });
        drawable.setStroke(dp(1), COLOR_STROKE);
        drawable.setCornerRadius(dp(18));
        return drawable;
    }

    private GradientDrawable buttonBackground(int color) {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setColor(color);
        drawable.setStroke(dp(2), brighten(color));
        drawable.setCornerRadius(dp(6));
        return drawable;
    }

    private int brighten(int color) {
        int r = Math.min(255, (int)(Color.red(color) * 1.25f + 12));
        int g = Math.min(255, (int)(Color.green(color) * 1.25f + 12));
        int b = Math.min(255, (int)(Color.blue(color) * 1.25f + 12));
        return Color.rgb(r, g, b);
    }

    private Button button(String text, int color, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setText(text);
        button.setTextSize(14);
        button.setTextColor(COLOR_TEXT);
        button.setTypeface(Typeface.DEFAULT_BOLD);
        button.setAllCaps(false);
        button.setGravity(Gravity.CENTER);
        button.setIncludeFontPadding(false);
        button.setPadding(0, 0, 0, 0);
        button.setBackground(buttonBackground(color));
        button.setOnClickListener(listener);
        return button;
    }

    private LinearLayout row() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        return row;
    }

    private LinearLayout.LayoutParams smallButtonParams() {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(dp(74), dp(44));
        params.setMargins(dp(4), 0, 0, 0);
        return params;
    }

    private LinearLayout.LayoutParams actionButtonParams() {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0, dp(48), 1f);
        params.gravity = Gravity.CENTER_VERTICAL;
        params.setMargins(dp(5), 0, dp(5), 0);
        return params;
    }

    private int dp(int value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }

    private void requestBluetoothPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
            checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[] {
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.BLUETOOTH_SCAN
            }, 100);
        }
    }

    @SuppressLint("MissingPermission")
    private void loadPairedDevices() {
        if (bluetoothAdapter == null) {
            toast("此手机不支持蓝牙");
            return;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
            checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            requestBluetoothPermission();
            return;
        }

        devices.clear();
        List<String> names = new ArrayList<>();
        Set<BluetoothDevice> bondedDevices = bluetoothAdapter.getBondedDevices();
        for (BluetoothDevice device : bondedDevices) {
            devices.add(device);
            String name = device.getName();
            names.add((name == null ? "Unknown" : name) + "  " + device.getAddress());
        }
        if (names.isEmpty()) {
            names.add("未找到已配对设备");
        }
        deviceSpinner.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, names));
    }

    @SuppressLint("MissingPermission")
    private void connectSelectedDevice() {
        if (devices.isEmpty()) {
            toast("请先在系统蓝牙中配对 HC-05/HC-06");
            return;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
            checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            requestBluetoothPermission();
            return;
        }

        BluetoothDevice device = devices.get(deviceSpinner.getSelectedItemPosition());
        statusText.setText("正在连接 " + device.getName());
        new Thread(() -> {
            try {
                closeSocket();
                BluetoothSocket nextSocket = device.createRfcommSocketToServiceRecord(SPP_UUID);
                bluetoothAdapter.cancelDiscovery();
                nextSocket.connect();
                socket = nextSocket;
                outputStream = socket.getOutputStream();
                inputStream = socket.getInputStream();
                startReceiveThread();
                runOnUiThread(() -> statusText.setText("已连接 " + device.getName()));
            } catch (IOException e) {
                closeSocket();
                runOnUiThread(() -> {
                    statusText.setText("连接失败");
                    toast("连接失败，请确认模块已配对且未被其他APP占用");
                });
            }
        }).start();
    }

    private void startReceiveThread() {
        receiveThread = new Thread(() -> {
            try {
                BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream, StandardCharsets.UTF_8));
                String line;
                while ((line = reader.readLine()) != null) {
                    String received = line.trim();
                    runOnUiThread(() -> handleTelemetry(received));
                }
            } catch (IOException ignored) {
                runOnUiThread(() -> statusText.setText("连接已断开"));
            }
        });
        receiveThread.start();
    }

    private void handleTelemetry(String line) {
        if (!line.startsWith("OLED ")) {
            return;
        }
        String payload = line.substring(5);
        String[] parts = payload.split("\\|");
        if (telemetryView != null) {
            telemetryView.setTelemetry(
                parts.length > 0 ? parts[0] : "Temp: --.- C",
                parts.length > 1 ? parts[1] : "Humi: -- %",
                parts.length > 2 ? parts[2] : "Weight: -- g",
                parts.length > 3 ? parts[3] : "Raw: ----");
        }
    }

    private synchronized void sendCommand(String command, boolean showToastWhenDisconnected) {
        commandText.setText("发送: " + command);
        OutputStream out = outputStream;
        if (out == null) {
            if (showToastWhenDisconnected) {
                toast("蓝牙未连接");
            }
            return;
        }
        try {
            out.write((command + "\n").getBytes(StandardCharsets.UTF_8));
            out.flush();
        } catch (IOException e) {
            statusText.setText("发送失败，连接已断开");
            closeSocket();
        }
    }

    private synchronized void closeSocket() {
        try {
            if (outputStream != null) {
                outputStream.close();
            }
        } catch (IOException ignored) {
        }
        try {
            if (inputStream != null) {
                inputStream.close();
            }
        } catch (IOException ignored) {
        }
        try {
            if (socket != null) {
                socket.close();
            }
        } catch (IOException ignored) {
        }
        outputStream = null;
        inputStream = null;
        socket = null;
    }

    private void toast(String text) {
        Toast.makeText(this, text, Toast.LENGTH_SHORT).show();
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacks(controlTask);
        closeSocket();
        super.onDestroy();
    }

    private static final class JoystickView extends View {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final String label;
        private OnMoveListener listener;
        private float knobX;
        private float knobY;

        JoystickView(Activity activity, String label) {
            super(activity);
            this.label = label;
            setMinimumHeight(activity.getResources().getDisplayMetrics().densityDpi);
        }

        void setOnMoveListener(OnMoveListener listener) {
            this.listener = listener;
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            float width = getWidth();
            float height = getHeight();
            float cx = width * 0.5f;
            float cy = height * 0.53f;
            float radius = Math.min(width, height) * 0.38f;
            float knobRadius = radius * 0.31f;

            paint.setStyle(Paint.Style.FILL);
            paint.setShader(new RadialGradient(cx, cy, radius * 1.15f,
                new int[] { Color.rgb(78, 83, 91), Color.rgb(29, 33, 40), Color.rgb(8, 10, 13) },
                new float[] { 0.0f, 0.62f, 1.0f },
                Shader.TileMode.CLAMP));
            canvas.drawCircle(cx, cy, radius * 1.05f, paint);
            paint.setShader(null);

            RectF outer = new RectF(cx - radius, cy - radius, cx + radius, cy + radius);
            for (int i = 0; i < 4; i++) {
                paint.setStyle(Paint.Style.FILL);
                paint.setColor(i % 2 == 0 ? Color.rgb(31, 35, 42) : Color.rgb(25, 29, 36));
                canvas.drawArc(outer, -45.0f + i * 90.0f, 86.0f, true, paint);
            }

            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(3.0f);
            paint.setColor(Color.rgb(7, 9, 12));
            canvas.drawCircle(cx, cy, radius * 1.06f, paint);

            paint.setStrokeWidth(2.0f);
            paint.setColor(Color.rgb(87, 93, 102));
            canvas.drawCircle(cx, cy, radius * 0.96f, paint);

            paint.setStrokeWidth(3.0f);
            paint.setColor(COLOR_PURPLE);
            canvas.drawCircle(cx, cy, radius * 0.52f, paint);

            paint.setStrokeWidth(1.5f);
            paint.setColor(Color.rgb(46, 51, 59));
            canvas.drawCircle(cx, cy, radius * 0.72f, paint);

            paint.setStrokeWidth(3.0f);
            paint.setColor(Color.rgb(11, 13, 17));
            canvas.drawLine(cx - radius * 0.7f, cy - radius * 0.7f, cx + radius * 0.7f, cy + radius * 0.7f, paint);
            canvas.drawLine(cx + radius * 0.7f, cy - radius * 0.7f, cx - radius * 0.7f, cy + radius * 0.7f, paint);

            drawTriangle(canvas, cx, cy - radius * 0.64f, 0.0f, radius * 0.12f);
            drawTriangle(canvas, cx + radius * 0.64f, cy, 90.0f, radius * 0.12f);
            drawTriangle(canvas, cx, cy + radius * 0.64f, 180.0f, radius * 0.12f);
            drawTriangle(canvas, cx - radius * 0.64f, cy, 270.0f, radius * 0.12f);

            paint.setStyle(Paint.Style.FILL);
            float kx = cx + knobX * radius;
            float ky = cy + knobY * radius;
            paint.setColor(Color.rgb(9, 11, 15));
            canvas.drawCircle(kx, ky + knobRadius * 0.13f, knobRadius * 1.08f, paint);

            paint.setShader(new RadialGradient(kx - knobRadius * 0.25f, ky - knobRadius * 0.28f, knobRadius * 1.2f,
                new int[] { Color.rgb(126, 132, 140), Color.rgb(42, 45, 53), Color.rgb(7, 8, 11) },
                new float[] { 0.0f, 0.48f, 1.0f },
                Shader.TileMode.CLAMP));
            canvas.drawCircle(kx, ky, knobRadius, paint);
            paint.setShader(null);

            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(3.0f);
            paint.setColor(COLOR_PURPLE);
            canvas.drawCircle(kx, ky, knobRadius * 1.16f, paint);
            paint.setStrokeWidth(1.5f);
            paint.setColor(Color.rgb(198, 185, 255));
            canvas.drawCircle(kx, ky, knobRadius * 0.83f, paint);

            paint.setStyle(Paint.Style.FILL);
            paint.setColor(Color.rgb(237, 233, 249));
            paint.setTextAlign(Paint.Align.CENTER);
            paint.setTextSize(34.0f);
            paint.setTypeface(Typeface.DEFAULT_BOLD);
            canvas.drawText(label, cx, cy - radius - 10.0f, paint);
        }

        private void drawTriangle(Canvas canvas, float cx, float cy, float degrees, float size) {
            Path path = new Path();
            path.moveTo(0.0f, -size);
            path.lineTo(size * 0.82f, size * 0.72f);
            path.lineTo(-size * 0.82f, size * 0.72f);
            path.close();
            canvas.save();
            canvas.translate(cx, cy);
            canvas.rotate(degrees);
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(COLOR_PURPLE);
            canvas.drawPath(path, paint);
            canvas.restore();
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            float width = getWidth();
            float height = getHeight();
            float cx = width * 0.5f;
            float cy = height * 0.53f;
            float radius = Math.min(width, height) * 0.38f;

            if (event.getAction() == MotionEvent.ACTION_UP || event.getAction() == MotionEvent.ACTION_CANCEL) {
                knobX = 0.0f;
                knobY = 0.0f;
                if (listener != null) {
                    listener.onMove(0.0f, 0.0f, true);
                }
                invalidate();
                return true;
            }

            float dx = (event.getX() - cx) / radius;
            float dy = (event.getY() - cy) / radius;
            float length = (float)Math.sqrt(dx * dx + dy * dy);
            if (length > 1.0f) {
                dx /= length;
                dy /= length;
            }
            knobX = dx;
            knobY = dy;
            if (listener != null) {
                listener.onMove(knobX, knobY, false);
            }
            invalidate();
            return true;
        }
    }

    private interface OnMoveListener {
        void onMove(float x, float y, boolean released);
    }

    private static final class TelemetryDisplayView extends View {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final String[] labels = {"TEMPERATURE", "HUMIDITY", "WEIGHT", "RAW VALUE"};
        private final String[] icons = {"T", "H", "W", "A"};
        private final String[] values = {"--.- C", "-- %", "-- g", "----"};

        TelemetryDisplayView(Activity activity) {
            super(activity);
            setMinimumHeight(activity.getResources().getDisplayMetrics().densityDpi / 2);
        }

        void setTelemetry(String temp, String humi, String weight, String adc) {
            values[0] = valuePart(temp);
            values[1] = valuePart(humi);
            values[2] = valuePart(weight);
            values[3] = valuePart(adc);
            invalidate();
        }

        private String valuePart(String text) {
            int index = text.indexOf(':');
            if (index >= 0 && index + 1 < text.length()) {
                return text.substring(index + 1).trim();
            }
            return text;
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            float w = getWidth();
            float h = getHeight();
            float pad = Math.min(w, h) * 0.08f;
            RectF outer = new RectF(pad, pad * 0.5f, w - pad, h - pad * 0.5f);

            paint.setStyle(Paint.Style.FILL);
            paint.setColor(Color.rgb(4, 5, 8));
            canvas.drawRoundRect(outer, 14.0f, 14.0f, paint);

            RectF inner = new RectF(outer.left + 8.0f, outer.top + 8.0f, outer.right - 8.0f, outer.bottom - 8.0f);
            paint.setShader(new RadialGradient(inner.centerX(), inner.top, inner.width() * 0.9f,
                new int[] { Color.rgb(24, 28, 36), Color.rgb(10, 12, 17), Color.rgb(5, 6, 9) },
                new float[] { 0.0f, 0.55f, 1.0f },
                Shader.TileMode.CLAMP));
            canvas.drawRoundRect(inner, 8.0f, 8.0f, paint);
            paint.setShader(null);

            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(2.0f);
            paint.setColor(Color.rgb(58, 64, 76));
            canvas.drawRoundRect(inner, 8.0f, 8.0f, paint);

            float rowH = inner.height() / 4.0f;
            for (int i = 1; i < 4; i++) {
                float y = inner.top + rowH * i;
                paint.setStyle(Paint.Style.STROKE);
                paint.setStrokeWidth(1.0f);
                paint.setColor(Color.rgb(42, 47, 56));
                canvas.drawLine(inner.left + 10.0f, y, inner.right - 10.0f, y, paint);
            }

            for (int i = 0; i < 4; i++) {
                float cy = inner.top + rowH * (i + 0.5f);
                drawIcon(canvas, inner.left + 28.0f, cy, icons[i]);

                paint.setStyle(Paint.Style.FILL);
                paint.setShader(null);
                paint.setTextAlign(Paint.Align.LEFT);
                paint.setTypeface(Typeface.DEFAULT_BOLD);
                paint.setTextSize(24.0f);
                paint.setColor(Color.rgb(226, 230, 239));
                canvas.drawText(labels[i], inner.left + 58.0f, cy + 8.0f, paint);

                paint.setTextAlign(Paint.Align.RIGHT);
                paint.setTextSize(26.0f);
                paint.setColor(Color.rgb(171, 131, 255));
                canvas.drawText(values[i], inner.right - 22.0f, cy + 9.0f, paint);
            }
        }

        private void drawIcon(Canvas canvas, float cx, float cy, String text) {
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(Color.rgb(31, 34, 43));
            canvas.drawRoundRect(new RectF(cx - 12.0f, cy - 12.0f, cx + 12.0f, cy + 12.0f), 4.0f, 4.0f, paint);

            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(1.5f);
            paint.setColor(Color.rgb(171, 131, 255));
            canvas.drawRoundRect(new RectF(cx - 12.0f, cy - 12.0f, cx + 12.0f, cy + 12.0f), 4.0f, 4.0f, paint);

            paint.setStyle(Paint.Style.FILL);
            paint.setTextAlign(Paint.Align.CENTER);
            paint.setTypeface(Typeface.DEFAULT_BOLD);
            paint.setTextSize(17.0f);
            paint.setColor(Color.rgb(230, 222, 255));
            canvas.drawText(text, cx, cy + 6.0f, paint);
        }
    }
}
