/**
 * BitScrape Application Logic
 * A Google-inspired, search-centric web UI for DHT crawling and torrent indexing.
 */

const API = {
    // Status & Stats
    getStatus: () => fetch('/api/status').then(r => r.json()),
    getStats: () => fetch('/api/statistics').then(r => r.json()),
    
    // Commands
    startCrawling: () => fetch('/api/crawling/start', { method: 'POST' }).then(r => r.json()),
    stopCrawling: () => fetch('/api/crawling/stop', { method: 'POST' }).then(r => r.json()),
    
    // Data Lists
    getNodes: (limit = 50, offset = 0) => fetch(`/api/nodes?limit=${limit}&offset=${offset}`).then(r => r.json()),
    getInfohashes: (limit = 50, offset = 0) => fetch(`/api/infohashes?limit=${limit}&offset=${offset}`).then(r => r.json()),
    getMetadata: (limit = 50, offset = 0) => fetch(`/api/metadata?limit=${limit}&offset=${offset}`).then(r => r.json()),
    
    // Search
    search: (query, limit = 50, offset = 0) => fetch(`/api/search?q=${encodeURIComponent(query)}&limit=${limit}&offset=${offset}`).then(r => r.json()),
    
    // Details
    getMetadataDetail: (hash) => fetch(`/api/metadata/${hash}`).then(r => r.json()),
    getFiles: (hash) => fetch(`/api/files/${hash}`).then(r => r.json()),
    getPeers: (hash) => fetch(`/api/peers/${hash}`).then(r => r.json()),
    getTrackers: (hash) => fetch(`/api/trackers/${hash}`).then(r => r.json()),
};

const App = {
    state: {
        currentView: 'home', // home, results, detail, dashboard
        searchQuery: '',
        currentInfohash: null,
        stats: {},
        status: { running: false, crawling: false },
        logs: []
    },

    init() {
        console.log("BitScrape App Initialized");
        this.bindEvents();
        this.route();
        this.startBackgroundTasks();
    },

    bindEvents() {
        // Logo click -> Home
        document.querySelectorAll('.logo, .home-link').forEach(el => {
            el.addEventListener('click', (e) => {
                e.preventDefault();
                this.navigate('home');
            });
        });

        // Search links -> Results (Browse mode)
        document.querySelectorAll('.results-link').forEach(el => {
            el.addEventListener('click', (e) => {
                e.preventDefault();
                this.state.searchQuery = '';
                this.navigate('results');
            });
        });

        // Dashboard links -> Dashboard
        document.querySelectorAll('.dashboard-link').forEach(el => {
            el.addEventListener('click', (e) => {
                e.preventDefault();
                this.navigate('dashboard');
            });
        });

        // Home Search Interaction
        const searchInput = document.getElementById('search-input');
        const searchBtn = document.getElementById('search-btn');
        const luckyBtn = document.getElementById('lucky-btn');

        if (searchInput) {
            searchInput.addEventListener('keydown', (e) => {
                if (e.key === 'Enter') this.performSearch(searchInput.value);
            });
        }

        if (searchBtn) {
            searchBtn.addEventListener('click', () => this.performSearch(searchInput.value));
        }

        if (luckyBtn) {
            luckyBtn.addEventListener('click', () => {
                this.state.searchQuery = ''; // Browse all
                this.navigate('results');
            });
        }
    },

    route() {
        const hash = window.location.hash.substring(1);
        if (hash.startsWith('search/')) {
            this.state.searchQuery = decodeURIComponent(hash.substring(7));
            this.navigate('results');
        } else if (hash.startsWith('detail/')) {
            this.state.currentInfohash = hash.substring(7);
            this.navigate('detail');
        } else if (hash === 'dashboard') {
            this.navigate('dashboard');
        } else {
            this.navigate('home');
        }
    },

    navigate(view) {
        this.state.currentView = view;
        
        // Update URL hash without triggering route again
        if (view === 'home') window.location.hash = '';
        else if (view === 'results' && this.state.searchQuery) window.location.hash = `search/${encodeURIComponent(this.state.searchQuery)}`;
        else if (view === 'results') window.location.hash = 'results';
        else if (view === 'detail') window.location.hash = `detail/${this.state.currentInfohash}`;
        else if (view === 'dashboard') window.location.hash = 'dashboard';

        // Toggle UI views
        document.querySelectorAll('.view').forEach(el => el.classList.add('hidden'));
        const activeView = document.getElementById(`${view}-view`);
        if (activeView) activeView.classList.remove('hidden');

        // Update Active Nav
        document.querySelectorAll('nav a').forEach(el => el.classList.remove('active'));
        if (view === 'results') document.querySelector('.results-link')?.classList.add('active');
        if (view === 'dashboard') document.querySelector('.dashboard-link')?.classList.add('active');

        // Render view specific data
        this.renderView();
    },

    renderView() {
        switch (this.state.currentView) {
            case 'results': this.renderResults(); break;
            case 'detail': this.renderDetail(); break;
            case 'dashboard': this.renderDashboard(); break;
        }
    },

    performSearch(q) {
        this.state.searchQuery = q.trim();
        this.navigate('results');
    },

    async renderResults() {
        const container = document.getElementById('results-list');
        const statsEl = document.getElementById('results-stats');
        if (!container) return;

        container.innerHTML = '<div class="loading"><div class="spinner"></div></div>';
        
        try {
            let data;
            if (this.state.searchQuery) {
                data = await API.search(this.state.searchQuery);
                statsEl.textContent = `About ${data.length} results for "${this.state.searchQuery}"`;
            } else {
                data = await API.getMetadata(100);
                statsEl.textContent = `Showing latest ${data.length} metadata items`;
            }

            if (data.length === 0) {
                container.innerHTML = '<div class="card">No results found.</div>';
                return;
            }

            container.innerHTML = data.map(item => `
                <div class="result-item">
                    <div class="result-url">bitscrape://torrent/${item.info_hash}</div>
                    <h3 onclick="App.viewDetail('${item.info_hash}')">${this.escapeHtml(item.name || '(unnamed)')}</h3>
                    <div class="result-snippet">
                        Size: ${this.formatSize(item.total_size)} • 
                        Files: ${item.file_count} • 
                        Seen: ${this.formatDate(item.download_time)}
                    </div>
                </div>
            `).join('');
        } catch (err) {
            container.innerHTML = `<div class="card log-error">Error loading results: ${err.message}</div>`;
        }
    },

    async viewDetail(hash) {
        this.state.currentInfohash = hash;
        this.navigate('detail');
    },

    async renderDetail() {
        const hash = this.state.currentInfohash;
        const main = document.getElementById('detail-main');
        if (!main) return;

        main.innerHTML = '<div class="loading"><div class="spinner"></div></div>';

        try {
            const [meta, files, peers, trackers] = await Promise.all([
                API.getMetadataDetail(hash),
                API.getFiles(hash),
                API.getPeers(hash),
                API.getTrackers(hash)
            ]);

            main.innerHTML = `
                <div class="card">
                    <h2>${this.escapeHtml(meta.name || 'Unnamed Torrent')}</h2>
                    <table>
                        <tr><th>Infohash</th><td><code>${meta.info_hash}</code></td></tr>
                        <tr><th>Total Size</th><td>${this.formatSize(meta.total_size)}</td></tr>
                        <tr><th>File Count</th><td>${meta.file_count}</td></tr>
                        <tr><th>Piece Count</th><td>${meta.piece_count}</td></tr>
                        <tr><th>Comment</th><td>${this.escapeHtml(meta.comment || 'N/A')}</td></tr>
                        <tr><th>Downloaded</th><td>${this.formatDate(meta.download_time)}</td></tr>
                    </table>
                </div>

                <div class="card">
                    <h3>Files (${files.length})</h3>
                    <div style="max-height: 300px; overflow-y: auto;">
                        <table>
                            <thead><tr><th>Path</th><th>Size</th></tr></thead>
                            <tbody>
                                ${files.map(f => `<tr><td>${this.escapeHtml(f.path)}</td><td>${this.formatSize(f.size)}</td></tr>`).join('')}
                            </tbody>
                        </table>
                    </div>
                </div>

                <div class="card">
                    <h3>Peers (${peers.length})</h3>
                    <div style="max-height: 200px; overflow-y: auto;">
                        <table>
                            <thead><tr><th>IP</th><th>Port</th><th>Last Seen</th></tr></thead>
                            <tbody>
                                ${peers.map(p => `<tr><td>${p.ip}</td><td>${p.port}</td><td>${this.formatDate(p.last_seen)}</td></tr>`).join('')}
                            </tbody>
                        </table>
                    </div>
                </div>

                <div class="card">
                    <h3>Trackers (${trackers.length})</h3>
                    <div style="max-height: 200px; overflow-y: auto;">
                        <table>
                            <thead><tr><th>URL</th><th>Announces</th></tr></thead>
                            <tbody>
                                ${trackers.map(t => `<tr><td>${this.escapeHtml(t.url)}</td><td>${t.announce_count}</td></tr>`).join('')}
                            </tbody>
                        </table>
                    </div>
                </div>
            `;
        } catch (err) {
            main.innerHTML = `<div class="card log-error">Error loading detail: ${err.message}</div>`;
        }
    },

    async renderDashboard() {
        this.updateStats();
        this.loadNodes();
    },

    async updateStats() {
        try {
            const [status, stats] = await Promise.all([API.getStatus(), API.getStats()]);
            this.state.status = status;
            this.state.stats = stats;

            // Update Status Pill
            const pill = document.getElementById('crawling-status');
            if (pill) {
                pill.textContent = status.crawling ? 'Crawling Active' : 'Crawling Stopped';
                pill.className = status.crawling ? 'btn btn-primary' : 'btn';
            }

            // Update Counts
            document.getElementById('stat-nodes').textContent = stats['storage.node_count'] || '0';
            document.getElementById('stat-hashes').textContent = stats['storage.infohash_count'] || '0';
            document.getElementById('stat-metadata').textContent = stats['storage.metadata_count'] || '0';
            document.getElementById('stat-db-size').textContent = this.formatSize(parseInt(stats['storage.db_size_bytes'] || 0));

            // Add simulated log for demo feel
            if (status.crawling && Math.random() > 0.7) {
                this.addLog(`Discovered new nodes from DHT...`);
            }

        } catch (err) {
            console.error("Stats check failed", err);
        }
    },

    addLog(msg, isError = false) {
        const time = new Date().toLocaleTimeString();
        this.state.logs.unshift({ time, msg, isError });
        if (this.state.logs.length > 50) this.state.logs.pop();
        
        const container = document.getElementById('log-container');
        if (container) {
            container.innerHTML = this.state.logs.map(log => `
                <div class="log-entry ${log.isError ? 'log-error' : ''}">
                    <span class="log-time">[${log.time}]</span> ${this.escapeHtml(log.msg)}
                </div>
            `).join('');
        }
    },

    async loadNodes() {
        const tbody = document.getElementById('nodes-tbody');
        if (!tbody) return;

        try {
            const nodes = await API.getNodes(20);
            tbody.innerHTML = nodes.map(n => `
                <tr>
                    <td><code>${n.ip}</code></td>
                    <td>${n.port}</td>
                    <td>${n.last_rtt_ms}ms</td>
                    <td>${this.formatDate(n.last_seen)}</td>
                    <td>${n.is_responsive ? '<span style="color:var(--google-green)">Yes</span>' : '<span style="color:var(--google-red)">No</span>'}</td>
                </tr>
            `).join('');
        } catch (err) {
            tbody.innerHTML = `<tr><td colspan="5">Error: ${err.message}</td></tr>`;
        }
    },

    async toggleCrawling() {
        try {
            if (this.state.status.crawling) await API.stopCrawling();
            else await API.startCrawling();
            this.updateStats();
        } catch (err) {
            alert(`Failed to toggle crawling: ${err.message}`);
        }
    },

    startBackgroundTasks() {
        // Refresh stats and nodes periodically
        setInterval(() => {
            if (this.state.currentView === 'dashboard') {
                this.updateStats();
                this.loadNodes();
            } else {
                this.updateStats(); // Keep status updated for the header
            }
        }, 3000);
    },

    // Helpers
    formatSize(bytes) {
        if (!bytes) return '0 B';
        const units = ['B', 'KB', 'MB', 'GB', 'TB'];
        let i = 0;
        while (bytes >= 1024 && i < units.length - 1) {
            bytes /= 1024;
            i++;
        }
        return `${bytes.toFixed(1)} ${units[i]}`;
    },

    formatDate(ts) {
        if (!ts) return 'Never';
        const d = new Date(ts * 1000);
        return d.toLocaleString();
    },

    escapeHtml(str) {
        const div = document.createElement('div');
        div.textContent = str;
        return div.innerHTML;
    }
};

// Start the app when DOM is ready
document.addEventListener('DOMContentLoaded', () => App.init());
window.addEventListener('popstate', () => App.route());

// Export globally for inline onclicks if needed
window.App = App;
