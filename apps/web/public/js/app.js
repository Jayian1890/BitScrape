// Main application JavaScript file
document.addEventListener('DOMContentLoaded', function () {
    // Initialize modules
    ThemeManager.init();
    APIClient.init();
    UIController.init();
    DataManager.init();
    ChartManager.init();
    NetworkVisualizer.init();
    ParticleBackground.init();

    // Load initial data
    loadInitialData();

    // Set up WebSocket connection
    setupWebSocket();

    // Set up event listeners
    setupEventListeners();
});

// Load initial data from API
async function loadInitialData() {
    try {
        // Get system status
        const status = await APIClient.getStatus();
        UIController.updateStatus(status);

        // Get statistics
        const stats = await APIClient.getStatistics();
        UIController.updateStatistics(stats);

        // Load nodes data
        const nodes = await APIClient.getNodes();
        DataManager.setNodes(nodes);
        UIController.updateTable('nodes', nodes);

        // Initialize charts with data
        ChartManager.updateCharts(stats);

        // Initialize network visualization
        NetworkVisualizer.updateNetwork(nodes.slice(0, 50));
    } catch (error) {
        console.error('Error loading initial data:', error);
        UIController.showError('Failed to load initial data. Please try again later.');
    }
}

// Set up WebSocket connection for real-time updates
function setupWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;

    const socket = new WebSocket(wsUrl);

    socket.onopen = function () {
        console.log('WebSocket connection established');
    };

    socket.onmessage = function (event) {
        const data = JSON.parse(event.data);
        handleWebSocketMessage(data);
    };

    socket.onclose = function () {
        console.log('WebSocket connection closed');
        // Try to reconnect after 5 seconds
        setTimeout(setupWebSocket, 5000);
    };

    socket.onerror = function (error) {
        console.error('WebSocket error:', error);
    };
}

// Handle WebSocket messages
function handleWebSocketMessage(data) {
    switch (data.type) {
        case 'node_discovered':
            DataManager.addNode(data.node);
            UIController.updateStatistics({'storage.node_count': DataManager.getNodeCount()});
            break;

        case 'infohash_discovered':
            DataManager.addInfohash(data.infohash);
            UIController.updateStatistics({'storage.infohash_count': DataManager.getInfohashCount()});
            break;

        case 'metadata_downloaded':
            DataManager.addMetadata(data.metadata);
            UIController.updateStatistics({'storage.metadata_count': DataManager.getMetadataCount()});
            break;

        case 'statistics_update':
            UIController.updateStatistics(data.statistics);
            ChartManager.updateCharts(data.statistics);
            break;

        default:
            console.log('Unknown WebSocket message type:', data.type);
    }
}

// Set up event listeners
function setupEventListeners() {
    // Start/Stop buttons
    document.getElementById('start-button').addEventListener('click', async function () {
        try {
            const result = await APIClient.startCrawling();
            if (result.success) {
                UIController.updateStatus({running: true, crawling: true});
            }
        } catch (error) {
            console.error('Error starting crawler:', error);
            UIController.showError('Failed to start crawler. Please try again later.');
        }
    });

    document.getElementById('stop-button').addEventListener('click', async function () {
        try {
            const result = await APIClient.stopCrawling();
            if (result.success) {
                UIController.updateStatus({running: true, crawling: false});
            }
        } catch (error) {
            console.error('Error stopping crawler:', error);
            UIController.showError('Failed to stop crawler. Please try again later.');
        }
    });

    // Tab buttons
    document.getElementById('tab-nodes').addEventListener('click', function () {
        UIController.switchTab('nodes');
    });

    document.getElementById('tab-infohashes').addEventListener('click', function () {
        UIController.switchTab('infohashes');
    });

    document.getElementById('tab-metadata').addEventListener('click', function () {
        UIController.switchTab('metadata');
    });

    // Search input
    document.getElementById('search-input').addEventListener('input', function () {
        UIController.handleSearch(this.value);
    });

    // Pagination buttons
    document.getElementById('prev-page').addEventListener('click', function () {
        UIController.prevPage();
    });

    document.getElementById('next-page').addEventListener('click', function () {
        UIController.nextPage();
    });

    // Modal close button
    document.getElementById('close-modal').addEventListener('click', function () {
        UIController.closeModal();
    });
}
