from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.responses import HTMLResponse
import asyncio
import json
import uuid
from typing import Dict, Set
import logging

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI(title="Video Streaming Server")

# Store active publishers and viewers
publishers: Dict[str, WebSocket] = {}  # publisher_id -> websocket
viewers: Dict[str, Dict] = {}  # viewer_id -> {websocket, watching_publisher}
publisher_metadata: Dict[str, Dict] = {}  # publisher_id -> {name, status, etc}


class ConnectionManager:
    def __init__(self):
        self.active_connections: Dict[str, WebSocket] = {}

    async def connect(self, websocket: WebSocket, client_id: str):
        await websocket.accept()
        self.active_connections[client_id] = websocket
        logger.info(f"Client {client_id} connected")

    def disconnect(self, client_id: str):
        if client_id in self.active_connections:
            del self.active_connections[client_id]
            logger.info(f"Client {client_id} disconnected")

    async def send_personal_message(self, message: str, client_id: str):
        if client_id in self.active_connections:
            websocket = self.active_connections[client_id]
            await websocket.send_text(message)


manager = ConnectionManager()


@app.get("/")
async def get_homepage():
    """Live streaming viewer webpage"""
    html_content = """
    <!DOCTYPE html>
    <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Live Video Streaming</title>
            <style>
                * {
                    margin: 0;
                    padding: 0;
                    box-sizing: border-box;
                }
                
                body {
                    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
                    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                    min-height: 100vh;
                    padding: 20px;
                }
                
                .container {
                    max-width: 1200px;
                    margin: 0 auto;
                    background: rgba(255, 255, 255, 0.95);
                    border-radius: 20px;
                    box-shadow: 0 20px 40px rgba(0, 0, 0, 0.1);
                    overflow: hidden;
                }
                
                .header {
                    background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
                    color: white;
                    padding: 30px;
                    text-align: center;
                }
                
                .header h1 {
                    font-size: 2.5rem;
                    font-weight: 700;
                    margin-bottom: 10px;
                }
                
                .header p {
                    font-size: 1.1rem;
                    opacity: 0.9;
                }
                
                .main-content {
                    display: grid;
                    grid-template-columns: 1fr 300px;
                    gap: 30px;
                    padding: 30px;
                }
                
                .video-section {
                    display: flex;
                    flex-direction: column;
                }
                
                .video-container {
                    position: relative;
                    background: #000;
                    border-radius: 15px;
                    overflow: hidden;
                    box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
                    margin-bottom: 20px;
                }
                
                #videoFeed {
                    width: 100%;
                    height: auto;
                    display: block;
                    max-height: 500px;
                    object-fit: contain;
                }
                
                .video-placeholder {
                    width: 100%;
                    height: 400px;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    flex-direction: column;
                    color: #666;
                    font-size: 1.2rem;
                }
                
                .video-placeholder i {
                    font-size: 4rem;
                    margin-bottom: 20px;
                    opacity: 0.3;
                }
                
                .video-controls {
                    display: flex;
                    gap: 15px;
                    margin-bottom: 20px;
                }
                
                .btn {
                    padding: 12px 24px;
                    border: none;
                    border-radius: 10px;
                    font-size: 1rem;
                    font-weight: 600;
                    cursor: pointer;
                    transition: all 0.3s ease;
                    text-transform: uppercase;
                    letter-spacing: 0.5px;
                }
                
                .btn-primary {
                    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                    color: white;
                }
                
                .btn-primary:hover {
                    transform: translateY(-2px);
                    box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
                }
                
                .btn-secondary {
                    background: #f8f9fa;
                    color: #495057;
                    border: 2px solid #dee2e6;
                }
                
                .btn-secondary:hover {
                    background: #e9ecef;
                    border-color: #adb5bd;
                }
                
                .btn:disabled {
                    opacity: 0.6;
                    cursor: not-allowed;
                    transform: none !important;
                }
                
                .sidebar {
                    background: #f8f9fa;
                    border-radius: 15px;
                    padding: 25px;
                    height: fit-content;
                }
                
                .sidebar h3 {
                    color: #495057;
                    margin-bottom: 20px;
                    font-size: 1.3rem;
                }
                
                .publisher-list {
                    list-style: none;
                }
                
                .publisher-item {
                    background: white;
                    border-radius: 10px;
                    padding: 15px;
                    margin-bottom: 15px;
                    box-shadow: 0 2px 10px rgba(0, 0, 0, 0.05);
                    transition: all 0.3s ease;
                    cursor: pointer;
                }
                
                .publisher-item:hover {
                    transform: translateY(-2px);
                    box-shadow: 0 5px 20px rgba(0, 0, 0, 0.1);
                }
                
                .publisher-item.active {
                    background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
                    color: white;
                }
                
                .publisher-name {
                    font-weight: 600;
                    margin-bottom: 5px;
                }
                
                .publisher-info {
                    font-size: 0.9rem;
                    opacity: 0.8;
                }
                
                .status {
                    display: flex;
                    align-items: center;
                    gap: 10px;
                    margin-bottom: 20px;
                    padding: 15px;
                    border-radius: 10px;
                    font-weight: 500;
                }
                
                .status.connected {
                    background: #d4edda;
                    color: #155724;
                    border: 1px solid #c3e6cb;
                }
                
                .status.disconnected {
                    background: #f8d7da;
                    color: #721c24;
                    border: 1px solid #f5c6cb;
                }
                
                .status.connecting {
                    background: #fff3cd;
                    color: #856404;
                    border: 1px solid #ffeaa7;
                }
                
                .status-dot {
                    width: 10px;
                    height: 10px;
                    border-radius: 50%;
                    animation: pulse 2s infinite;
                }
                
                .status.connected .status-dot {
                    background: #28a745;
                }
                
                .status.disconnected .status-dot {
                    background: #dc3545;
                }
                
                .status.connecting .status-dot {
                    background: #ffc107;
                }
                
                @keyframes pulse {
                    0% { opacity: 1; }
                    50% { opacity: 0.5; }
                    100% { opacity: 1; }
                }
                
                .video-info {
                    background: #f8f9fa;
                    border-radius: 10px;
                    padding: 15px;
                    font-size: 0.9rem;
                }
                
                .video-info div {
                    margin-bottom: 5px;
                }
                
                @media (max-width: 768px) {
                    .main-content {
                        grid-template-columns: 1fr;
                    }
                    
                    .video-controls {
                        flex-direction: column;
                    }
                    
                    .header h1 {
                        font-size: 2rem;
                    }
                }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header">
                    <h1>🎥 Live Video Streaming</h1>
                    <p>Real-time video streams from connected cameras</p>
                </div>
                
                <div class="main-content">
                    <div class="video-section">
                        <div class="status disconnected" id="connectionStatus">
                            <div class="status-dot"></div>
                            <span>Disconnected from server</span>
                        </div>
                        
                        <div class="video-container">
                            <img id="videoFeed" style="display: none;" alt="Video Stream">
                            <div class="video-placeholder" id="videoPlaceholder">
                                <div style="font-size: 4rem; margin-bottom: 20px; opacity: 0.3;">📹</div>
                                <div>Select a camera stream to start watching</div>
                            </div>
                        </div>
                        
                        <div class="video-controls">
                            <button class="btn btn-primary" id="connectBtn" onclick="connectWebSocket()">
                                Connect to Server
                            </button>
                            <button class="btn btn-secondary" id="stopBtn" onclick="stopWatching()" disabled>
                                Stop Stream
                            </button>
                            <button class="btn btn-secondary" onclick="refreshPublishers()">
                                Refresh Cameras
                            </button>
                        </div>
                        
                        <div class="video-info" id="videoInfo" style="display: none;">
                            <div><strong>Camera:</strong> <span id="cameraName">-</span></div>
                            <div><strong>Device:</strong> <span id="deviceType">-</span></div>
                            <div><strong>Resolution:</strong> <span id="resolution">-</span></div>
                            <div><strong>FPS:</strong> <span id="fps">-</span></div>
                            <div><strong>Status:</strong> <span id="streamStatus">-</span></div>
                        </div>
                    </div>
                    
                    <div class="sidebar">
                        <h3>📡 Available Cameras</h3>
                        <ul class="publisher-list" id="publisherList">
                            <li style="text-align: center; color: #666; padding: 20px;">
                                No cameras detected<br>
                                <small>Connect to server to see available streams</small>
                            </li>
                        </ul>
                    </div>
                </div>
            </div>
            
            <script>
                let websocket = null;
                let currentPublisher = null;
                let publishers = [];
                
                function connectWebSocket() {
                    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
                    const wsUrl = `${protocol}//${window.location.host}/watch`;
                    
                    updateStatus('connecting', 'Connecting to server...');
                    
                    try {
                        websocket = new WebSocket(wsUrl);
                        
                        websocket.onopen = function(event) {
                            console.log('WebSocket connected');
                            updateStatus('connected', 'Connected to server');
                            document.getElementById('connectBtn').disabled = true;
                            requestPublisherList();
                        };
                        
                        websocket.onmessage = function(event) {
                            if (event.data instanceof Blob) {
                                // Handle binary data (video frame)
                                displayVideoFrame(event.data);
                            } else {
                                // Handle text data (control messages)
                                try {
                                    const message = JSON.parse(event.data);
                                    handleControlMessage(message);
                                } catch (e) {
                                    console.error('Failed to parse message:', e);
                                }
                            }
                        };
                        
                        websocket.onclose = function(event) {
                            console.log('WebSocket closed');
                            updateStatus('disconnected', 'Disconnected from server');
                            document.getElementById('connectBtn').disabled = false;
                            document.getElementById('stopBtn').disabled = true;
                            clearVideo();
                            clearPublisherList();
                        };
                        
                        websocket.onerror = function(error) {
                            console.error('WebSocket error:', error);
                            updateStatus('disconnected', 'Connection error');
                        };
                        
                    } catch (error) {
                        console.error('Failed to create WebSocket:', error);
                        updateStatus('disconnected', 'Failed to connect');
                    }
                }
                
                function handleControlMessage(message) {
                    switch (message.type) {
                        case 'publisher_list':
                            updatePublisherList(message.publishers);
                            break;
                        case 'publisher_selected':
                            if (message.status === 'success') {
                                document.getElementById('stopBtn').disabled = false;
                                document.getElementById('streamStatus').textContent = 'Streaming';
                                console.log('Now watching publisher:', message.publisher_id);
                            }
                            break;
                        case 'stopped_watching':
                            document.getElementById('stopBtn').disabled = true;
                            document.getElementById('streamStatus').textContent = 'Stopped';
                            clearVideo();
                            break;
                        case 'publisher_disconnected':
                            if (currentPublisher === message.publisher_id) {
                                clearVideo();
                                updateVideoInfo(null);
                                currentPublisher = null;
                            }
                            break;
                        case 'error':
                            console.error('Server error:', message.message);
                            alert('Error: ' + message.message);
                            break;
                    }
                }
                
                function displayVideoFrame(blob) {
                    const videoFeed = document.getElementById('videoFeed');
                    const videoPlaceholder = document.getElementById('videoPlaceholder');
                    
                    // Create object URL for the blob
                    const url = URL.createObjectURL(blob);
                    
                    // Clean up previous URL
                    if (videoFeed.src && videoFeed.src.startsWith('blob:')) {
                        URL.revokeObjectURL(videoFeed.src);
                    }
                    
                    videoFeed.src = url;
                    videoFeed.style.display = 'block';
                    videoPlaceholder.style.display = 'none';
                }
                
                function selectPublisher(publisherId) {
                    if (!websocket || websocket.readyState !== WebSocket.OPEN) {
                        alert('Please connect to server first');
                        return;
                    }
                    
                    currentPublisher = publisherId;
                    
                    const message = {
                        type: 'select_publisher',
                        publisher_id: publisherId
                    };
                    
                    websocket.send(JSON.stringify(message));
                    
                    // Update UI
                    const publisherItems = document.querySelectorAll('.publisher-item');
                    publisherItems.forEach(item => {
                        item.classList.remove('active');
                        if (item.dataset.publisherId === publisherId) {
                            item.classList.add('active');
                        }
                    });
                    
                    // Update video info
                    const publisher = publishers.find(p => p.id === publisherId);
                    updateVideoInfo(publisher);
                }
                
                function stopWatching() {
                    if (!websocket || websocket.readyState !== WebSocket.OPEN) return;
                    
                    const message = { type: 'stop_watching' };
                    websocket.send(JSON.stringify(message));
                    
                    // Update UI
                    const publisherItems = document.querySelectorAll('.publisher-item');
                    publisherItems.forEach(item => item.classList.remove('active'));
                    
                    clearVideo();
                    updateVideoInfo(null);
                    currentPublisher = null;
                }
                
                function requestPublisherList() {
                    if (!websocket || websocket.readyState !== WebSocket.OPEN) return;
                    
                    const message = { type: 'get_publishers' };
                    websocket.send(JSON.stringify(message));
                }
                
                function refreshPublishers() {
                    requestPublisherList();
                }
                
                function updatePublisherList(publisherList) {
                    publishers = publisherList;
                    const listElement = document.getElementById('publisherList');
                    
                    if (publisherList.length === 0) {
                        listElement.innerHTML = `
                            <li style="text-align: center; color: #666; padding: 20px;">
                                No cameras available<br>
                                <small>Waiting for camera connections...</small>
                            </li>
                        `;
                        return;
                    }
                    
                    listElement.innerHTML = publisherList.map(publisher => `
                        <li class="publisher-item" 
                            data-publisher-id="${publisher.id}"
                            onclick="selectPublisher('${publisher.id}')">
                            <div class="publisher-name">
                                ${publisher.metadata.name || 'Camera Stream'}
                            </div>
                            <div class="publisher-info">
                                ${publisher.metadata.device || 'Unknown Device'}<br>
                                ${publisher.metadata.resolution || 'Unknown Resolution'}
                                ${publisher.metadata.fps ? ` • ${publisher.metadata.fps} FPS` : ''}
                            </div>
                        </li>
                    `).join('');
                }
                
                function clearPublisherList() {
                    const listElement = document.getElementById('publisherList');
                    listElement.innerHTML = `
                        <li style="text-align: center; color: #666; padding: 20px;">
                            No cameras detected<br>
                            <small>Connect to server to see available streams</small>
                        </li>
                    `;
                    publishers = [];
                }
                
                function updateVideoInfo(publisher) {
                    const videoInfo = document.getElementById('videoInfo');
                    
                    if (!publisher) {
                        videoInfo.style.display = 'none';
                        return;
                    }
                    
                    document.getElementById('cameraName').textContent = publisher.metadata.name || 'Unknown';
                    document.getElementById('deviceType').textContent = publisher.metadata.device || 'Unknown';
                    document.getElementById('resolution').textContent = publisher.metadata.resolution || 'Unknown';
                    document.getElementById('fps').textContent = publisher.metadata.fps || 'Unknown';
                    document.getElementById('streamStatus').textContent = 'Connecting...';
                    
                    videoInfo.style.display = 'block';
                }
                
                function clearVideo() {
                    const videoFeed = document.getElementById('videoFeed');
                    const videoPlaceholder = document.getElementById('videoPlaceholder');
                    
                    if (videoFeed.src && videoFeed.src.startsWith('blob:')) {
                        URL.revokeObjectURL(videoFeed.src);
                    }
                    
                    videoFeed.src = '';
                    videoFeed.style.display = 'none';
                    videoPlaceholder.style.display = 'flex';
                }
                
                function updateStatus(status, message) {
                    const statusElement = document.getElementById('connectionStatus');
                    statusElement.className = `status ${status}`;
                    statusElement.querySelector('span').textContent = message;
                }
                
                // Auto-connect on page load
                window.addEventListener('load', function() {
                    setTimeout(connectWebSocket, 500);
                });
                
                // Handle page unload
                window.addEventListener('beforeunload', function() {
                    if (websocket) {
                        websocket.close();
                    }
                });
            </script>
        </body>
    </html>
    """
    return HTMLResponse(content=html_content)


@app.get("/publishers")
async def get_publishers():
    """Get list of active publishers"""
    return {
        "publishers": [
            {
                "id": pub_id,
                "metadata": publisher_metadata.get(pub_id, {}),
                "status": "active"
            }
            for pub_id in publishers.keys()
        ]
    }


@app.websocket("/stream")
async def publisher_endpoint(websocket: WebSocket):
    """Endpoint for publishers to stream video"""
    publisher_id = str(uuid.uuid4())

    try:
        await websocket.accept()
        publishers[publisher_id] = websocket

        # Initialize publisher metadata
        publisher_metadata[publisher_id] = {
            "name": f"Publisher-{publisher_id[:8]}",
            "connected_at": asyncio.get_event_loop().time()
        }

        logger.info(f"Publisher {publisher_id} connected")

        # Notify all viewers about new publisher
        await broadcast_publisher_list()

        while True:
            # Receive video data from publisher
            data = await websocket.receive()

            if data["type"] == "websocket.receive":
                if "text" in data:
                    # Handle text messages (control messages)
                    try:
                        message = json.loads(data["text"])
                        if message.get("type") == "metadata":
                            # Update publisher metadata
                            publisher_metadata[publisher_id].update(
                                message.get("data", {}))
                            await broadcast_publisher_list()
                    except json.JSONDecodeError:
                        pass
                elif "bytes" in data:
                    # Handle binary data (video frames)
                    video_data = data["bytes"]
                    await broadcast_to_viewers(publisher_id, video_data)

    except WebSocketDisconnect:
        logger.info(f"Publisher {publisher_id} disconnected")
    except Exception as e:
        logger.error(f"Error with publisher {publisher_id}: {e}")
    finally:
        # Clean up publisher
        if publisher_id in publishers:
            del publishers[publisher_id]
        if publisher_id in publisher_metadata:
            del publisher_metadata[publisher_id]

        # Notify viewers about publisher removal
        await broadcast_publisher_list()

        # Disconnect viewers watching this publisher
        viewers_to_remove = []
        for viewer_id, viewer_info in viewers.items():
            if viewer_info.get("watching_publisher") == publisher_id:
                viewers_to_remove.append(viewer_id)

        for viewer_id in viewers_to_remove:
            await notify_viewer(viewer_id, {
                "type": "publisher_disconnected",
                "publisher_id": publisher_id
            })


@app.websocket("/watch")
async def viewer_endpoint(websocket: WebSocket):
    """Enhanced WebSocket endpoint for web viewer"""
    viewer_id = str(uuid.uuid4())

    try:
        await websocket.accept()
        viewers[viewer_id] = {
            "websocket": websocket,
            "watching_publisher": None
        }

        logger.info(f"Web viewer {viewer_id} connected")

        # Send current publisher list to new viewer
        await send_publisher_list(viewer_id)

        while True:
            # Receive messages from viewer
            data = await websocket.receive_text()

            try:
                message = json.loads(data)
                message_type = message.get("type")

                if message_type == "select_publisher":
                    publisher_id = message.get("publisher_id")

                    if publisher_id in publishers:
                        viewers[viewer_id]["watching_publisher"] = publisher_id
                        await websocket.send_text(json.dumps({
                            "type": "publisher_selected",
                            "publisher_id": publisher_id,
                            "status": "success"
                        }))
                        logger.info(f"Web viewer {viewer_id} now watching publisher {
                                    publisher_id}")
                    else:
                        await websocket.send_text(json.dumps({
                            "type": "error",
                            "message": "Camera stream not found"
                        }))

                elif message_type == "stop_watching":
                    viewers[viewer_id]["watching_publisher"] = None
                    await websocket.send_text(json.dumps({
                        "type": "stopped_watching"
                    }))
                    logger.info(f"Web viewer {viewer_id} stopped watching")

                elif message_type == "get_publishers":
                    await send_publisher_list(viewer_id)

            except json.JSONDecodeError:
                await websocket.send_text(json.dumps({
                    "type": "error",
                    "message": "Invalid message format"
                }))

    except WebSocketDisconnect:
        logger.info(f"Web viewer {viewer_id} disconnected")
    except Exception as e:
        logger.error(f"Error with web viewer {viewer_id}: {e}")
    finally:
        # Clean up viewer
        if viewer_id in viewers:
            del viewers[viewer_id]


async def broadcast_to_viewers(publisher_id: str, video_data: bytes):
    """Broadcast video data to viewers watching specific publisher"""
    viewers_to_remove = []

    for viewer_id, viewer_info in viewers.items():
        if viewer_info.get("watching_publisher") == publisher_id:
            try:
                websocket = viewer_info["websocket"]
                await websocket.send_bytes(video_data)
            except Exception as e:
                logger.error(f"Failed to send data to viewer {viewer_id}: {e}")
                viewers_to_remove.append(viewer_id)

    # Remove disconnected viewers
    for viewer_id in viewers_to_remove:
        if viewer_id in viewers:
            del viewers[viewer_id]


async def broadcast_publisher_list():
    """Broadcast updated publisher list to all viewers"""
    publisher_list = {
        "type": "publisher_list",
        "publishers": [
            {
                "id": pub_id,
                "metadata": publisher_metadata.get(pub_id, {})
            }
            for pub_id in publishers.keys()
        ]
    }

    message = json.dumps(publisher_list)
    viewers_to_remove = []

    for viewer_id, viewer_info in viewers.items():
        try:
            websocket = viewer_info["websocket"]
            await websocket.send_text(message)
        except Exception as e:
            logger.error(f"Failed to send publisher list to viewer {
                         viewer_id}: {e}")
            viewers_to_remove.append(viewer_id)

    # Remove disconnected viewers
    for viewer_id in viewers_to_remove:
        if viewer_id in viewers:
            del viewers[viewer_id]


async def send_publisher_list(viewer_id: str):
    """Send publisher list to specific viewer"""
    if viewer_id in viewers:
        publisher_list = {
            "type": "publisher_list",
            "publishers": [
                {
                    "id": pub_id,
                    "metadata": publisher_metadata.get(pub_id, {})
                }
                for pub_id in publishers.keys()
            ]
        }

        try:
            websocket = viewers[viewer_id]["websocket"]
            await websocket.send_text(json.dumps(publisher_list))
        except Exception as e:
            logger.error(f"Failed to send publisher list to viewer {
                         viewer_id}: {e}")


async def notify_viewer(viewer_id: str, message: dict):
    """Send notification to specific viewer"""
    if viewer_id in viewers:
        try:
            websocket = viewers[viewer_id]["websocket"]
            await websocket.send_text(json.dumps(message))
        except Exception as e:
            logger.error(f"Failed to notify viewer {viewer_id}: {e}")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=4200)
