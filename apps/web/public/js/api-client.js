// API Client for communicating with the backend
const APIClient = {
    baseUrl: '/api',

    init: function () {
        // Nothing to initialize
    },

    // Helper method for making API requests
    async request(endpoint, options = {}) {
        try {
            const url = `${this.baseUrl}${endpoint}`;
            const response = await fetch(url, options);

            if (!response.ok) {
                const errorText = await response.text();
                throw new Error(`API request failed: ${response.status} ${response.statusText} - ${errorText}`);
            }

            return await response.json();
        } catch (error) {
            console.error('API request error:', error);
            throw error;
        }
    },

    // Get system status
    async getStatus() {
        return this.request('/status');
    },

    // Get system statistics
    async getStatistics() {
        return this.request('/statistics');
    },

    // Start crawling
    async startCrawling() {
        return this.request('/crawling/start', {
            method: 'POST'
        });
    },

    // Stop crawling
    async stopCrawling() {
        return this.request('/crawling/stop', {
            method: 'POST'
        });
    },

    // Get nodes
    async getNodes(limit = 100, offset = 0) {
        return this.request(`/nodes?limit=${limit}&offset=${offset}`);
    },

    // Get infohashes
    async getInfohashes(limit = 100, offset = 0) {
        return this.request(`/infohashes?limit=${limit}&offset=${offset}`);
    },

    // Get metadata
    async getMetadata(limit = 100, offset = 0) {
        return this.request(`/metadata?limit=${limit}&offset=${offset}`);
    },

    // Get metadata by infohash
    async getMetadataByInfohash(infohash) {
        return this.request(`/metadata/${infohash}`);
    },

    // Get files for an infohash
    async getFiles(infohash) {
        return this.request(`/files/${infohash}`);
    },

    // Get peers for an infohash
    async getPeers(infohash) {
        return this.request(`/peers/${infohash}`);
    },

    // Get trackers for an infohash
    async getTrackers(infohash) {
        return this.request(`/trackers/${infohash}`);
    },

    // Search for metadata
    async searchMetadata(query, limit = 100, offset = 0) {
        return this.request(`/search?q=${encodeURIComponent(query)}&limit=${limit}&offset=${offset}`);
    }
};
