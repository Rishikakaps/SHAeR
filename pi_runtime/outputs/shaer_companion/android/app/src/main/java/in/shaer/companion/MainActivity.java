package in.shaer.companion;

import android.app.Activity;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.InputType;
import android.util.Log;
import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.URI;
import java.net.URL;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

public class MainActivity extends Activity {
    private static final String PREFS = "shaer_connection";
    private static final String SERVICE_TYPE = "_shaer._tcp.";
    private static final int DEFAULT_PORT = 8775;
    private static final int API_TIMEOUT_MS = 1800;
    private final Handler main = new Handler(Looper.getMainLooper());
    private final ExecutorService discoveryExecutor = Executors.newFixedThreadPool(8);
    private final ExecutorService probeExecutor = Executors.newFixedThreadPool(24);
    private final AtomicBoolean cancelled = new AtomicBoolean(false);
    private final List<NativeDiscovery.Device> found = Collections.synchronizedList(new ArrayList<NativeDiscovery.Device>());
    private SharedPreferences preferences;
    private LinearLayout root;
    private TextView state;
    private LinearLayout devices;
    private LinearLayout advanced;
    private EditText hostInput;
    private EditText portInput;
    private WifiManager.MulticastLock multicastLock;
    private ValueCallback<Uri[]> fileChooserCallback;
    private static final int FILE_CHOOSER_REQUEST = 4107;

    @Override public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        preferences = getSharedPreferences(PREFS, MODE_PRIVATE);
        buildDiscoveryView();
        startDiscovery();
    }

    private void buildDiscoveryView() {
        root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(40, 48, 40, 40);
        root.setBackgroundColor(0xfff4f0e8);

        TextView title = text("SHAeR Companion", 28, 0xff1c1a17);
        root.addView(title, new LinearLayout.LayoutParams(-1, -2));
        TextView subtitle = text("Local-first companion", 15, 0xff6f655b);
        root.addView(subtitle, new LinearLayout.LayoutParams(-1, -2));
        state = text("Checking last known device...", 17, 0xff1c1a17);
        LinearLayout.LayoutParams stateParams = new LinearLayout.LayoutParams(-1, -2);
        stateParams.setMargins(0, 36, 0, 18);
        root.addView(state, stateParams);
        devices = new LinearLayout(this);
        devices.setOrientation(LinearLayout.VERTICAL);
        root.addView(devices, new LinearLayout.LayoutParams(-1, -2));

        Button retry = new Button(this);
        retry.setText("Retry discovery");
        retry.setOnClickListener(v -> startDiscovery());
        root.addView(retry, new LinearLayout.LayoutParams(-1, -2));

        Button advancedButton = new Button(this);
        advancedButton.setText("Advanced connection");
        advancedButton.setOnClickListener(v -> advanced.setVisibility(advanced.getVisibility() == View.VISIBLE ? View.GONE : View.VISIBLE));
        root.addView(advancedButton, new LinearLayout.LayoutParams(-1, -2));

        advanced = new LinearLayout(this);
        advanced.setOrientation(LinearLayout.VERTICAL);
        advanced.setVisibility(View.GONE);
        hostInput = new EditText(this);
        hostInput.setHint("IP address or hostname");
        hostInput.setSingleLine(true);
        hostInput.setInputType(InputType.TYPE_CLASS_TEXT);
        advanced.addView(hostInput, new LinearLayout.LayoutParams(-1, -2));
        portInput = new EditText(this);
        portInput.setHint("Port");
        portInput.setText(String.valueOf(DEFAULT_PORT));
        portInput.setSingleLine(true);
        portInput.setInputType(InputType.TYPE_CLASS_NUMBER);
        advanced.addView(portInput, new LinearLayout.LayoutParams(-1, -2));
        Button connect = new Button(this);
        connect.setText("Connect");
        connect.setOnClickListener(v -> connectManual());
        advanced.addView(connect, new LinearLayout.LayoutParams(-1, -2));
        root.addView(advanced, new LinearLayout.LayoutParams(-1, -2));

        ScrollView scroll = new ScrollView(this);
        scroll.addView(root);
        setContentView(scroll);
    }

    private TextView text(String value, int size, int color) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(size);
        view.setTextColor(color);
        return view;
    }

    private void startDiscovery() {
        cancelled.set(true);
        cancelled.set(false);
        found.clear();
        devices.removeAllViews();
        state.setText("Checking last known device...");
        String rememberedHost = preferences.getString("host", "");
        int rememberedPort = preferences.getInt("port", DEFAULT_PORT);
        discoveryExecutor.execute(() -> {
            if (!rememberedHost.isEmpty()) {
                NativeDiscovery.Device remembered = NativeDiscovery.verify(rememberedHost, rememberedPort, NativeDiscovery.Method.REMEMBERED);
                if (remembered != null && !cancelled.get()) {
                    choose(remembered);
                    return;
                }
            }
            postState("Searching local network...");
            NativeDiscovery.DiscoveryResult result = NativeDiscovery.discover(this, cancelled, probeExecutor);
            if (cancelled.get()) return;
            synchronized (found) {
                for (NativeDiscovery.Device device : result.devices) addUnique(device);
            }
            main.post(this::renderFound);
        });
    }

    private void addUnique(NativeDiscovery.Device candidate) {
        for (NativeDiscovery.Device existing : found) {
            if (existing.deviceId.equals(candidate.deviceId)) return;
        }
        found.add(candidate);
    }

    private void renderFound() {
        devices.removeAllViews();
        if (found.isEmpty()) {
            state.setText("No SHAeR device found. Check Wi-Fi and try again.");
            return;
        }
        state.setText(found.size() == 1 ? "SHAeR found. Connecting..." : "Multiple SHAeR devices found.");
        if (found.size() == 1) {
            choose(found.get(0));
            return;
        }
        for (NativeDiscovery.Device device : found) addDeviceButton(device);
    }

    private void addDeviceButton(NativeDiscovery.Device device) {
        Button button = new Button(this);
        button.setText(device.deviceName + "\n" + device.host + ":" + device.port + "  " + device.method.label);
        button.setOnClickListener(v -> choose(device));
        devices.addView(button, new LinearLayout.LayoutParams(-1, -2));
    }

    private void choose(NativeDiscovery.Device device) {
        if (cancelled.get()) return;
        cancelled.set(true);
        preferences.edit().putString("host", device.host).putInt("port", device.port).putString("device_id", device.deviceId).apply();
        state.setText("Connected to " + device.deviceName + "\n" + device.host + ":" + device.port);
        devices.removeAllViews();
        Button open = new Button(this);
        open.setText("Open SHAeR companion");
        open.setOnClickListener(v -> openCompanion(device));
        devices.addView(open, new LinearLayout.LayoutParams(-1, -2));
        openCompanion(device);
    }

    private void openCompanion(NativeDiscovery.Device device) {
        WebView web = new WebView(this);
        WebSettings settings = web.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
        web.setWebChromeClient(new WebChromeClient() {
            @Override public boolean onShowFileChooser(WebView view, ValueCallback<Uri[]> callback, FileChooserParams params) {
                if (fileChooserCallback != null) fileChooserCallback.onReceiveValue(null);
                fileChooserCallback = callback;
                Intent chooser = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                chooser.addCategory(Intent.CATEGORY_OPENABLE);
                chooser.setType("*/*");
                chooser.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
                startActivityForResult(chooser, FILE_CHOOSER_REQUEST);
                return true;
            }
        });
        web.loadUrl(device.baseUrl() + "/shaer_companion/");
        setContentView(web);
    }

    @Override protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != FILE_CHOOSER_REQUEST || fileChooserCallback == null) return;
        Uri[] results = null;
        if (resultCode == RESULT_OK && data != null) {
            ClipData clip = data.getClipData();
            if (clip != null && clip.getItemCount() > 0) {
                results = new Uri[clip.getItemCount()];
                for (int index = 0; index < clip.getItemCount(); index++) results[index] = clip.getItemAt(index).getUri();
            } else if (data.getData() != null) {
                results = new Uri[] { data.getData() };
            }
        }
        fileChooserCallback.onReceiveValue(results);
        fileChooserCallback = null;
    }

    private void connectManual() {
        String host = hostInput.getText().toString().trim();
        int port;
        try { port = Integer.parseInt(portInput.getText().toString().trim()); }
        catch (NumberFormatException error) { state.setText("Enter a valid port."); return; }
        state.setText("Verifying SHAeR...");
        discoveryExecutor.execute(() -> {
            NativeDiscovery.Device device = NativeDiscovery.verify(host, port, NativeDiscovery.Method.MANUAL);
            main.post(() -> {
                if (device == null) state.setText("That device did not identify itself as SHAeR.");
                else choose(device);
            });
        });
    }

    private void postState(String message) { main.post(() -> state.setText(message)); }

    @Override protected void onDestroy() {
        if (fileChooserCallback != null) {
            fileChooserCallback.onReceiveValue(null);
            fileChooserCallback = null;
        }
        cancelled.set(true);
        discoveryExecutor.shutdownNow();
        probeExecutor.shutdownNow();
        releaseMulticastLock();
        super.onDestroy();
    }

    private void releaseMulticastLock() {
        if (multicastLock != null && multicastLock.isHeld()) multicastLock.release();
        multicastLock = null;
    }

    static final class NativeDiscovery {
        enum Method { REMEMBERED("remembered"), MDNS("mDNS"), UDP("UDP"), SUBNET_SCAN("subnet scan"), MANUAL("manual");
            final String label; Method(String label) { this.label = label; }
        }

        static final class Device {
            final String deviceId, deviceName, host, apiVersion;
            final int port;
            final Method method;
            Device(String id, String name, String host, int port, String version, Method method) {
                this.deviceId = id; this.deviceName = name; this.host = host; this.port = port; this.apiVersion = version; this.method = method;
            }
            String baseUrl() { return "http://" + host + ":" + port; }
        }

        static final class DiscoveryResult { final List<Device> devices = Collections.synchronizedList(new ArrayList<>()); }

        static Device verify(String rawHost, int port, Method method) {
            String host = normalizeHost(rawHost);
            if (host.isEmpty() || port < 1 || port > 65535) return null;
            HttpURLConnection connection = null;
            try {
                URL url = new URL("http://" + host + ":" + port + "/api/v1/device/discovery");
                connection = (HttpURLConnection) url.openConnection();
                connection.setRequestMethod("GET");
                connection.setConnectTimeout(API_TIMEOUT_MS);
                connection.setReadTimeout(API_TIMEOUT_MS);
                connection.setUseCaches(false);
                int status = connection.getResponseCode();
                if (status != 200) {
                    Log.i("SHAeRDiscovery", "verify url=" + url + " status=" + status + " failure=http_status");
                    return null;
                }
                String body = read(connection.getInputStream());
                JSONObject root = new JSONObject(body);
                JSONObject data = root.optJSONObject("data");
                if (data == null) data = root;
                String name = data.optString("device_name", "");
                String firmware = data.optString("firmware_version", "");
                if (!root.optBoolean("ok", true) || name.isEmpty() || firmware.isEmpty()) {
                    Log.i("SHAeRDiscovery", "verify url=" + url + " failure=invalid_shaer_response body=" + body);
                    return null;
                }
                String id = data.optString("device_id", "shaer:" + name);
                String version = data.optString("api_version", data.optString("protocol_version", "1"));
                return new Device(id, name, host, port, version, method);
            } catch (Exception error) {
                Log.i("SHAeRDiscovery", "verify host=" + rawHost + ":" + port + " failure=" + error.getClass().getSimpleName() + " detail=" + error.getMessage());
                return null;
            } finally {
                if (connection != null) connection.disconnect();
            }
        }

        static DiscoveryResult discover(Context context, AtomicBoolean cancelled, ExecutorService probes) {
            DiscoveryResult result = new DiscoveryResult();
            WifiManager wifi = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            WifiManager.MulticastLock lock = null;
            if (wifi != null) {
                lock = wifi.createMulticastLock("shaer-discovery");
                lock.setReferenceCounted(false);
                lock.acquire();
            }
            try {
                discoverMdns(context, cancelled, probes, result);
                if (!cancelled.get()) discoverUdp(cancelled, probes, result);
                if (!cancelled.get()) {
                    Device hostname = verify("shaer.local", DEFAULT_PORT, Method.MDNS);
                    if (hostname != null) result.devices.add(hostname);
                }
                if (!cancelled.get()) discoverSubnet(context, cancelled, probes, result);
            } finally {
                if (lock != null && lock.isHeld()) lock.release();
            }
            return result;
        }

        private static void discoverMdns(Context context, AtomicBoolean cancelled, ExecutorService probes, DiscoveryResult result) {
            NsdManager manager = (NsdManager) context.getSystemService(Context.NSD_SERVICE);
            if (manager == null) return;
            CountDownLatch done = new CountDownLatch(1);
            Map<String, Device> local = Collections.synchronizedMap(new HashMap<>());
            NsdManager.DiscoveryListener listener = new NsdManager.DiscoveryListener() {
                @Override public void onDiscoveryStarted(String serviceType) {}
                @Override public void onDiscoveryStopped(String serviceType) { done.countDown(); }
                @Override public void onStartDiscoveryFailed(String serviceType, int errorCode) { done.countDown(); }
                @Override public void onStopDiscoveryFailed(String serviceType, int errorCode) { done.countDown(); }
                @Override public void onServiceLost(NsdServiceInfo serviceInfo) {}
                @Override public void onServiceFound(NsdServiceInfo serviceInfo) {
                    if (cancelled.get() || !SERVICE_TYPE.equals(serviceInfo.getServiceType())) return;
                    manager.resolveService(serviceInfo, new NsdManager.ResolveListener() {
                        @Override public void onResolveFailed(NsdServiceInfo serviceInfo, int errorCode) {}
                        @Override public void onServiceResolved(NsdServiceInfo resolved) {
                            InetAddress address = resolved.getHost();
                            if (address == null) return;
                            String host = address.getHostAddress();
                            probes.submit(() -> {
                                Device device = verify(host, resolved.getPort(), Method.MDNS);
                                if (device != null) local.put(device.deviceId, device);
                            });
                        }
                    });
                }
            };
            try {
                manager.discoverServices(SERVICE_TYPE, NsdManager.PROTOCOL_DNS_SD, listener);
                long deadline = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(4500);
                while (!cancelled.get() && System.nanoTime() < deadline) Thread.sleep(100);
                try { manager.stopServiceDiscovery(listener); } catch (Exception ignored) {}
                Thread.sleep(500);
                result.devices.addAll(local.values());
            } catch (Exception ignored) {
                try { manager.stopServiceDiscovery(listener); } catch (Exception ignoredAgain) {}
            }
        }

        private static void discoverSubnet(Context context, AtomicBoolean cancelled, ExecutorService probes, DiscoveryResult result) {
            ConnectivityManager connectivity = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            Network network = connectivity == null ? null : connectivity.getActiveNetwork();
            LinkProperties properties = network == null ? null : connectivity.getLinkProperties(network);
            if (properties == null) return;
            for (LinkAddress link : properties.getLinkAddresses()) {
                if (!(link.getAddress() instanceof Inet4Address)) continue;
                int prefix = link.getPrefixLength();
                if (prefix < 16 || prefix > 30) return;
                byte[] address = link.getAddress().getAddress();
                int ip = ((address[0] & 255) << 24) | ((address[1] & 255) << 16) | ((address[2] & 255) << 8) | (address[3] & 255);
                int mask = (int) (0xffffffffL << (32 - prefix));
                int start = (ip & mask) + 1;
                int end = (ip | ~mask) - 1;
                if (end - start + 1 > 512) {
                    // Keep the fallback bounded while covering the phone's
                    // local /24 slice on wider private networks.
                    start = (ip & 0xffffff00) + 1;
                    end = (ip & 0xffffff00) + 254;
                }
                int count = end - start + 1;
                CountDownLatch latch = new CountDownLatch(count);
                Map<String, Device> local = Collections.synchronizedMap(new HashMap<>());
                for (int candidate = start; candidate <= end && !cancelled.get(); candidate++) {
                    final String host = ((candidate >>> 24) & 255) + "." + ((candidate >>> 16) & 255) + "." + ((candidate >>> 8) & 255) + "." + (candidate & 255);
                    probes.submit(() -> {
                        try {
                            Device device = verify(host, DEFAULT_PORT, Method.SUBNET_SCAN);
                            if (device != null) local.put(device.deviceId, device);
                        } finally { latch.countDown(); }
                    });
                }
                try { latch.await(6500, TimeUnit.MILLISECONDS); } catch (InterruptedException interrupted) { Thread.currentThread().interrupt(); }
                result.devices.addAll(local.values());
                return;
            }
        }

        private static void discoverUdp(AtomicBoolean cancelled, ExecutorService probes, DiscoveryResult result) {
            try (DatagramSocket socket = new DatagramSocket()) {
                socket.setBroadcast(true);
                socket.setSoTimeout(1200);
                byte[] request = "SHAER_DISCOVER_V1".getBytes("UTF-8");
                socket.send(new DatagramPacket(request, request.length, new InetSocketAddress("255.255.255.255", 8776)));
                long deadline = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(1400);
                Map<String, Device> local = Collections.synchronizedMap(new HashMap<>());
                while (!cancelled.get() && System.nanoTime() < deadline) {
                    byte[] buffer = new byte[1024];
                    DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                    try { socket.receive(packet); } catch (java.net.SocketTimeoutException timeout) { break; }
                    try {
                        JSONObject response = new JSONObject(new String(packet.getData(), packet.getOffset(), packet.getLength(), "UTF-8"));
                        if (!"shaer".equals(response.optString("service"))) continue;
                        String host = response.optString("host", packet.getAddress().getHostAddress());
                        int port = response.optInt("port", DEFAULT_PORT);
                        probes.submit(() -> {
                            Device device = verify(host, port, Method.UDP);
                            if (device != null) local.put(device.deviceId, device);
                        });
                    } catch (JSONException ignored) {}
                }
                Thread.sleep(300);
                result.devices.addAll(local.values());
            } catch (Exception ignored) {}
        }

        private static String normalizeHost(String raw) {
            if (raw == null) return "";
            String host = raw.trim();
            if (host.startsWith("http://")) host = host.substring(7);
            if (host.startsWith("https://")) host = host.substring(8);
            int slash = host.indexOf('/');
            if (slash >= 0) host = host.substring(0, slash);
            int colon = host.lastIndexOf(':');
            if (colon > 0 && host.indexOf(':') == colon) host = host.substring(0, colon);
            return host;
        }

        private static String read(InputStream stream) throws Exception {
            StringBuilder body = new StringBuilder();
            BufferedReader reader = new BufferedReader(new InputStreamReader(stream));
            String line;
            while ((line = reader.readLine()) != null) body.append(line);
            return body.toString();
        }
    }
}
