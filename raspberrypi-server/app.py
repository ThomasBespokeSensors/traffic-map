#!/usr/bin/env python3
from flask import Flask, request, render_template_string, send_from_directory, jsonify, after_this_request
from datetime import datetime
import os

app = Flask(__name__)

# Configuration - using Docker volumes
LOG_FILE = '/data/logs/traffic-map.log'
FIRMWARE_FOLDER = '/data/firmware'
os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
os.makedirs(FIRMWARE_FOLDER, exist_ok=True)

# ============================================
# Web Interface
# ============================================

HTML_TEMPLATE = '''
<!DOCTYPE html>
<html>
<head>
    <title>Traffic Map Server</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 1200px; margin: 0 auto; }
        .card { background: white; padding: 20px; margin: 20px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #333; }
        h2 { color: #666; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }
        .upload-form { margin: 20px 0; }
        .upload-form input[type="file"] { padding: 10px; }
        .upload-form button { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }
        .upload-form button:hover { background: #45a049; }
        .message { padding: 10px; margin: 10px 0; border-radius: 4px; }
        .success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .firmware-list { list-style: none; padding: 0; }
        .firmware-list li { padding: 10px; margin: 5px 0; background: #f9f9f9; border-left: 4px solid #4CAF50; }
        .firmware-list a { color: #2196F3; text-decoration: none; }
        .firmware-list a:hover { text-decoration: underline; }
        .logs { background: #1e1e1e; color: #00ff00; padding: 15px; border-radius: 4px; max-height: 400px; overflow-y: scroll; font-family: monospace; font-size: 12px; white-space: pre-wrap; }
        .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }
        .stat-card { background: #f9f9f9; padding: 15px; border-radius: 4px; text-align: center; }
        .stat-card .number { font-size: 32px; font-weight: bold; color: #4CAF50; }
        .stat-card .label { color: #666; margin-top: 5px; }
        .refresh-btn { background: #2196F3; color: white; padding: 8px 16px; border: none; border-radius: 4px; cursor: pointer; margin: 10px 0; }
        .refresh-btn:hover { background: #0b7dda; }
        .delete-btn { background: #f44336; color: white; padding: 5px 10px; border: none; border-radius: 4px; cursor: pointer; font-size: 12px; float: right; }
        .delete-btn:hover { background: #da190b; }
    </style>
    <script>
        function refreshLogs() {
            location.reload();
        }
        setInterval(refreshLogs, 30000); // Auto-refresh every 30 seconds
    </script>
</head>
<body>
    <div class="container">
        <h1>🚦 Traffic Map Server</h1>
        
        <!-- Stats -->
        <div class="stats">
            <div class="stat-card">
                <div class="number">{{ stats.total_logs }}</div>
                <div class="label">Total Log Entries</div>
            </div>
            <div class="stat-card">
                <div class="number">{{ stats.firmware_count }}</div>
                <div class="label">Firmware Files</div>
            </div>
            <div class="stat-card">
                <div class="number">{{ stats.log_size_mb }}</div>
                <div class="label">Log Size (MB)</div>
            </div>
        </div>
        
        <!-- OTA Upload Section -->
        <div class="card">
            <h2>📤 Firmware Upload (OTA)</h2>
            {% if message %}
                <div class="message {{ message_type }}">{{ message }}</div>
            {% endif %}
            <form method="post" enctype="multipart/form-data" class="upload-form">
                <input type="file" name="firmware" accept=".bin" required>
                <button type="submit">Upload Firmware</button>
            </form>
            
            <h3>Available Firmware:</h3>
            {% if firmware_files %}
                <ul class="firmware-list">
                {% for file in firmware_files %}
                    <li>
                        <a href="/firmware/{{ file.name }}">{{ file.name }}</a>
                        <span style="color: #666; float: right;">{{ file.size }} | {{ file.date }}</span>
                    </li>
                {% endfor %}
                </ul>
            {% else %}
                <p style="color: #666;">No firmware files uploaded yet.</p>
            {% endif %}
        </div>
        
        <!-- Logs Section -->
        <div class="card">
            <h2>📝 Recent Logs</h2>
            <button onclick="refreshLogs()" class="refresh-btn">🔄 Refresh Logs</button>
            <div class="logs">{{ recent_logs }}</div>
        </div>
    </div>
</body>
</html>
'''

# ============================================
# Logging Endpoint
# ============================================

@app.route('/log', methods=['POST'])
def log_message():
    """Receive log messages from ESP32"""
    try:
        data = request.get_json()
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        message = data.get('message', '')
        
        with open(LOG_FILE, 'a') as f:
            f.write(f"[{timestamp}] {message}\n")
        
        return jsonify({'status': 'ok'}), 200
    except Exception as e:
        print(f"Error logging: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

# ============================================
# OTA Endpoints
# ============================================

@app.route('/', methods=['GET', 'POST'])
def index():
    """Main web interface"""
    message = None
    message_type = 'success'
    
    # Handle firmware upload
    if request.method == 'POST':
        file = request.files.get('firmware')
        if file and file.filename.endswith('.bin'):
            filepath = os.path.join(FIRMWARE_FOLDER, file.filename)
            file.save(filepath)
            message = f"✅ Uploaded {file.filename} successfully! ESP32 will auto-update on next check."
            message_type = 'success'
        else:
            message = "❌ Please upload a .bin file"
            message_type = 'error'
    
    # Get firmware files with details
    firmware_files = []
    if os.path.exists(FIRMWARE_FOLDER):
        for filename in os.listdir(FIRMWARE_FOLDER):
            if filename.endswith('.bin'):
                filepath = os.path.join(FIRMWARE_FOLDER, filename)
                stat = os.stat(filepath)
                firmware_files.append({
                    'name': filename,
                    'size': f"{stat.st_size / 1024:.1f} KB",
                    'date': datetime.fromtimestamp(stat.st_mtime).strftime('%Y-%m-%d %H:%M')
                })
    firmware_files.sort(key=lambda x: x['date'], reverse=True)
    
    # Get recent logs (last 100 lines)
    recent_logs = ""
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, 'r') as f:
            lines = f.readlines()
            recent_logs = ''.join(lines[-100:])  # Last 100 lines
    
    # Calculate stats
    stats = {
        'total_logs': len(open(LOG_FILE).readlines()) if os.path.exists(LOG_FILE) else 0,
        'firmware_count': len(firmware_files),
        'log_size_mb': f"{os.path.getsize(LOG_FILE) / 1024 / 1024:.2f}" if os.path.exists(LOG_FILE) else "0.00"
    }
    
    return render_template_string(
        HTML_TEMPLATE,
        message=message,
        message_type=message_type,
        firmware_files=firmware_files,
        recent_logs=recent_logs,
        stats=stats
    )

@app.route('/firmware/<filename>')
def download_firmware(filename):
    """Serve firmware file for ESP32 OTA"""
    return send_from_directory(FIRMWARE_FOLDER, filename)

@app.route('/firmware/latest')
def latest_firmware():
    """Get the most recent firmware file and delete after serving"""

    if not os.path.exists(FIRMWARE_FOLDER):
        return jsonify({'error': 'No firmware available'}), 404

    files = [f for f in os.listdir(FIRMWARE_FOLDER) if f.endswith('.bin')]

    if not files:
        return jsonify({'error': 'No firmware available'}), 404

    # Get newest firmware
    latest = max(
        [os.path.join(FIRMWARE_FOLDER, f) for f in files],
        key=os.path.getmtime
    )

    filename = os.path.basename(latest)

    @after_this_request
    def remove_file(response):
        try:
            os.remove(latest)
            print(f"Deleted firmware after OTA: {filename}")
        except Exception as e:
            print(f"Could not delete firmware: {e}")

        return response

    return send_from_directory(FIRMWARE_FOLDER, filename)

# ============================================
# API Status Endpoint
# ============================================

@app.route('/status')
def status():
    """API endpoint for ESP32 to check server status"""
    return jsonify({
        'status': 'online',
        'timestamp': datetime.now().isoformat()
    })

# ============================================
# Health Check (for Docker)
# ============================================

@app.route('/health')
def health():
    """Health check endpoint for Docker"""
    return jsonify({'status': 'healthy'}), 200

# ============================================
# Main
# ============================================

if __name__ == '__main__':
    print("=" * 50)
    print("Traffic Map Server Starting...")
    print("=" * 50)
    print(f"Web Interface: http://0.0.0.0:5000")
    print(f"Logs: {LOG_FILE}")
    print(f"Firmware: {FIRMWARE_FOLDER}")
    print("=" * 50)
    app.run(host='0.0.0.0', port=5000, debug=False)