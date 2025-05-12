// UI Controller for managing the user interface
const UIController = {
    currentTab: 'nodes',
    currentPage: 0,
    itemsPerPage: 100,
    searchQuery: '',

    init: function () {
        // Initialize UI state
        this.updateStatus({running: false, crawling: false});
    },

    // Update system status display
    updateStatus: function (status) {
        const statusDot = document.getElementById('status-dot');
        const statusText = document.getElementById('status-text');
        const startButton = document.getElementById('start-button');
        const stopButton = document.getElementById('stop-button');

        if (status.crawling) {
            statusDot.classList.add('active');
            statusDot.classList.remove('bg-red-500');
            statusDot.classList.add('bg-green-500');
            statusText.textContent = 'Running';
            startButton.disabled = true;
            stopButton.disabled = false;
        } else {
            statusDot.classList.remove('active');
            statusDot.classList.remove('bg-green-500');
            statusDot.classList.add('bg-red-500');
            statusText.textContent = 'Stopped';
            startButton.disabled = false;
            stopButton.disabled = true;
        }
    },

    // Update statistics display
    updateStatistics: function (stats) {
        // Update node count
        if (stats['storage.node_count']) {
            document.getElementById('node-count').textContent = this.formatNumber(stats['storage.node_count']);
        }

        // Update infohash count
        if (stats['storage.infohash_count']) {
            document.getElementById('infohash-count').textContent = this.formatNumber(stats['storage.infohash_count']);
        }

        // Update metadata count
        if (stats['storage.metadata_count']) {
            document.getElementById('metadata-count').textContent = this.formatNumber(stats['storage.metadata_count']);
        }
    },

    // Switch between data tabs
    switchTab: function (tab) {
        this.currentTab = tab;
        this.currentPage = 0;

        // Update tab buttons
        document.getElementById('tab-nodes').classList.remove('bg-blue-600', 'hover:bg-blue-700');
        document.getElementById('tab-nodes').classList.add('bg-gray-700', 'hover:bg-gray-600');

        document.getElementById('tab-infohashes').classList.remove('bg-blue-600', 'hover:bg-blue-700');
        document.getElementById('tab-infohashes').classList.add('bg-gray-700', 'hover:bg-gray-600');

        document.getElementById('tab-metadata').classList.remove('bg-blue-600', 'hover:bg-blue-700');
        document.getElementById('tab-metadata').classList.add('bg-gray-700', 'hover:bg-gray-600');

        document.getElementById(`tab-${tab}`).classList.remove('bg-gray-700', 'hover:bg-gray-600');
        document.getElementById(`tab-${tab}`).classList.add('bg-blue-600', 'hover:bg-blue-700');

        // Load data for the selected tab
        this.loadTabData();
    },

    // Load data for the current tab
    loadTabData: async function () {
        try {
            let data;

            switch (this.currentTab) {
                case 'nodes':
                    data = await APIClient.getNodes(this.itemsPerPage, this.currentPage * this.itemsPerPage);
                    break;

                case 'infohashes':
                    data = await APIClient.getInfohashes(this.itemsPerPage, this.currentPage * this.itemsPerPage);
                    break;

                case 'metadata':
                    if (this.searchQuery) {
                        data = await APIClient.searchMetadata(this.searchQuery, this.itemsPerPage, this.currentPage * this.itemsPerPage);
                    } else {
                        data = await APIClient.getMetadata(this.itemsPerPage, this.currentPage * this.itemsPerPage);
                    }
                    break;
            }

            this.updateTable(this.currentTab, data);
        } catch (error) {
            console.error(`Error loading ${this.currentTab} data:`, error);
            this.showError(`Failed to load ${this.currentTab} data. Please try again later.`);
        }
    },

    // Update table with data
    updateTable: function (tab, data) {
        const tableHeader = document.getElementById('table-header');
        const tableBody = document.getElementById('table-body');

        // Clear existing content
        tableHeader.innerHTML = '';
        tableBody.innerHTML = '';

        // Create table headers based on tab
        let headers;

        switch (tab) {
            case 'nodes':
                headers = ['Node ID', 'IP', 'Port', 'Last Seen', 'Responses', 'Status'];
                break;

            case 'infohashes':
                headers = ['Infohash', 'First Seen', 'Last Seen', 'Nodes', 'Peers', 'Metadata'];
                break;

            case 'metadata':
                headers = ['Infohash', 'Name', 'Size', 'Files', 'Downloaded'];
                break;
        }

        // Add headers to table
        headers.forEach(header => {
            const th = document.createElement('th');
            th.textContent = header;
            tableHeader.appendChild(th);
        });

        // Add action column
        const actionTh = document.createElement('th');
        actionTh.textContent = 'Action';
        tableHeader.appendChild(actionTh);

        // Add data rows
        if (data && data.length > 0) {
            data.forEach(item => {
                const row = document.createElement('tr');

                switch (tab) {
                    case 'nodes':
                        this.createNodeRow(row, item);
                        break;

                    case 'infohashes':
                        this.createInfohashRow(row, item);
                        break;

                    case 'metadata':
                        this.createMetadataRow(row, item);
                        break;
                }

                tableBody.appendChild(row);
            });
        } else {
            // No data message
            const row = document.createElement('tr');
            const cell = document.createElement('td');
            cell.textContent = 'No data available';
            cell.colSpan = headers.length + 1;
            cell.className = 'text-center py-4';
            row.appendChild(cell);
            tableBody.appendChild(row);
        }

        // Update pagination info
        this.updatePagination(data ? data.length : 0);
    },

    // Create a row for a node
    createNodeRow: function (row, node) {
        // Node ID (truncated)
        const idCell = document.createElement('td');
        idCell.textContent = this.truncateString(node.node_id, 10);
        idCell.title = node.node_id;
        row.appendChild(idCell);

        // IP
        const ipCell = document.createElement('td');
        ipCell.textContent = node.ip;
        row.appendChild(ipCell);

        // Port
        const portCell = document.createElement('td');
        portCell.textContent = node.port;
        row.appendChild(portCell);

        // Last Seen
        const lastSeenCell = document.createElement('td');
        lastSeenCell.textContent = this.formatDate(node.last_seen);
        row.appendChild(lastSeenCell);

        // Responses
        const responsesCell = document.createElement('td');
        responsesCell.textContent = node.response_count;
        row.appendChild(responsesCell);

        // Status
        const statusCell = document.createElement('td');
        const statusBadge = document.createElement('span');
        statusBadge.className = `badge ${node.is_responsive ? 'badge-green' : 'badge-red'}`;
        statusBadge.textContent = node.is_responsive ? 'Active' : 'Inactive';
        statusCell.appendChild(statusBadge);
        row.appendChild(statusCell);

        // Action
        const actionCell = document.createElement('td');
        const detailsButton = document.createElement('button');
        detailsButton.className = 'bg-blue-600 hover:bg-blue-700 text-white font-bold py-1 px-3 rounded-lg text-sm transition-colors';
        detailsButton.textContent = 'Details';
        detailsButton.addEventListener('click', () => this.showNodeDetails(node));
        actionCell.appendChild(detailsButton);
        row.appendChild(actionCell);
    },

    // Create a row for an infohash
    createInfohashRow: function (row, infohash) {
        // Infohash (truncated)
        const idCell = document.createElement('td');
        idCell.textContent = this.truncateString(infohash.info_hash, 10);
        idCell.title = infohash.info_hash;
        row.appendChild(idCell);

        // First Seen
        const firstSeenCell = document.createElement('td');
        firstSeenCell.textContent = this.formatDate(infohash.first_seen);
        row.appendChild(firstSeenCell);

        // Last Seen
        const lastSeenCell = document.createElement('td');
        lastSeenCell.textContent = this.formatDate(infohash.last_seen);
        row.appendChild(lastSeenCell);

        // Node Count
        const nodeCountCell = document.createElement('td');
        nodeCountCell.textContent = infohash.node_count;
        row.appendChild(nodeCountCell);

        // Peer Count
        const peerCountCell = document.createElement('td');
        peerCountCell.textContent = infohash.peer_count;
        row.appendChild(peerCountCell);

        // Has Metadata
        const metadataCell = document.createElement('td');
        const metadataBadge = document.createElement('span');
        metadataBadge.className = `badge ${infohash.has_metadata ? 'badge-green' : 'badge-red'}`;
        metadataBadge.textContent = infohash.has_metadata ? 'Available' : 'Unavailable';
        metadataCell.appendChild(metadataBadge);
        row.appendChild(metadataCell);

        // Action
        const actionCell = document.createElement('td');
        const detailsButton = document.createElement('button');
        detailsButton.className = 'bg-blue-600 hover:bg-blue-700 text-white font-bold py-1 px-3 rounded-lg text-sm transition-colors';
        detailsButton.textContent = 'Details';
        detailsButton.addEventListener('click', () => this.showInfohashDetails(infohash));
        actionCell.appendChild(detailsButton);
        row.appendChild(actionCell);
    },

    // Create a row for metadata
    createMetadataRow: function (row, metadata) {
        // Infohash (truncated)
        const idCell = document.createElement('td');
        idCell.textContent = this.truncateString(metadata.info_hash, 10);
        idCell.title = metadata.info_hash;
        row.appendChild(idCell);

        // Name
        const nameCell = document.createElement('td');
        nameCell.textContent = metadata.name;
        row.appendChild(nameCell);

        // Size
        const sizeCell = document.createElement('td');
        sizeCell.textContent = this.formatSize(metadata.total_size);
        row.appendChild(sizeCell);

        // File Count
        const fileCountCell = document.createElement('td');
        fileCountCell.textContent = metadata.file_count;
        row.appendChild(fileCountCell);

        // Download Time
        const downloadTimeCell = document.createElement('td');
        downloadTimeCell.textContent = this.formatDate(metadata.download_time);
        row.appendChild(downloadTimeCell);

        // Action
        const actionCell = document.createElement('td');
        const detailsButton = document.createElement('button');
        detailsButton.className = 'bg-blue-600 hover:bg-blue-700 text-white font-bold py-1 px-3 rounded-lg text-sm transition-colors';
        detailsButton.textContent = 'Details';
        detailsButton.addEventListener('click', () => this.showMetadataDetails(metadata));
        actionCell.appendChild(detailsButton);
        row.appendChild(actionCell);
    },

    // Show node details in modal
    showNodeDetails: async function (node) {
        const modalTitle = document.getElementById('modal-title');
        const modalContent = document.getElementById('modal-content');

        modalTitle.textContent = `Node Details: ${this.truncateString(node.node_id, 10)}`;

        // Create content
        let content = `
            <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div>
                    <h4 class="text-lg font-semibold mb-2">Node Information</h4>
                    <p><strong>Node ID:</strong> ${node.node_id}</p>
                    <p><strong>IP Address:</strong> ${node.ip}</p>
                    <p><strong>Port:</strong> ${node.port}</p>
                    <p><strong>First Seen:</strong> ${this.formatDate(node.first_seen)}</p>
                    <p><strong>Last Seen:</strong> ${this.formatDate(node.last_seen)}</p>
                </div>
                <div>
                    <h4 class="text-lg font-semibold mb-2">Statistics</h4>
                    <p><strong>Ping Count:</strong> ${node.ping_count}</p>
                    <p><strong>Query Count:</strong> ${node.query_count}</p>
                    <p><strong>Response Count:</strong> ${node.response_count}</p>
                    <p><strong>Status:</strong> <span class="badge ${node.is_responsive ? 'badge-green' : 'badge-red'}">${node.is_responsive ? 'Active' : 'Inactive'}</span></p>
                </div>
            </div>
        `;

        modalContent.innerHTML = content;

        // Show modal
        document.getElementById('details-modal').classList.remove('hidden');
    },

    // Show infohash details in modal
    showInfohashDetails: async function (infohash) {
        const modalTitle = document.getElementById('modal-title');
        const modalContent = document.getElementById('modal-content');

        modalTitle.textContent = `Infohash Details: ${this.truncateString(infohash.info_hash, 10)}`;

        // Create content
        let content = `
            <div class="grid grid-cols-1 md:grid-cols-2 gap-4 mb-4">
                <div>
                    <h4 class="text-lg font-semibold mb-2">Infohash Information</h4>
                    <p><strong>Infohash:</strong> ${infohash.info_hash}</p>
                    <p><strong>First Seen:</strong> ${this.formatDate(infohash.first_seen)}</p>
                    <p><strong>Last Seen:</strong> ${this.formatDate(infohash.last_seen)}</p>
                </div>
                <div>
                    <h4 class="text-lg font-semibold mb-2">Statistics</h4>
                    <p><strong>Node Count:</strong> ${infohash.node_count}</p>
                    <p><strong>Peer Count:</strong> ${infohash.peer_count}</p>
                    <p><strong>Metadata:</strong> <span class="badge ${infohash.has_metadata ? 'badge-green' : 'badge-red'}">${infohash.has_metadata ? 'Available' : 'Unavailable'}</span></p>
                </div>
            </div>
        `;

        // If metadata is available, fetch and display it
        if (infohash.has_metadata) {
            try {
                const metadata = await APIClient.getMetadataByInfohash(infohash.info_hash);
                const files = await APIClient.getFiles(infohash.info_hash);

                content += `
                    <div class="border-t border-gray-600 pt-4 mt-4">
                        <h4 class="text-lg font-semibold mb-2">Metadata</h4>
                        <p><strong>Name:</strong> ${metadata.name}</p>
                        <p><strong>Size:</strong> ${this.formatSize(metadata.total_size)}</p>
                        <p><strong>Piece Count:</strong> ${metadata.piece_count}</p>
                        <p><strong>File Count:</strong> ${metadata.file_count}</p>
                        <p><strong>Download Time:</strong> ${this.formatDate(metadata.download_time)}</p>
                        ${metadata.comment ? `<p><strong>Comment:</strong> ${metadata.comment}</p>` : ''}
                    </div>
                `;

                if (files && files.length > 0) {
                    content += `
                        <div class="border-t border-gray-600 pt-4 mt-4">
                            <h4 class="text-lg font-semibold mb-2">Files (${files.length})</h4>
                            <div class="max-h-60 overflow-y-auto">
                                <ul class="file-list">
                    `;

                    files.forEach(file => {
                        content += `
                            <li class="flex justify-between">
                                <span class="truncate mr-4">${file.path}</span>
                                <span class="text-gray-400 whitespace-nowrap">${this.formatSize(file.size)}</span>
                            </li>
                        `;
                    });

                    content += `
                                </ul>
                            </div>
                        </div>
                    `;
                }
            } catch (error) {
                console.error('Error fetching metadata:', error);
                content += `
                    <div class="border-t border-gray-600 pt-4 mt-4">
                        <p class="text-red-500">Failed to load metadata. Please try again later.</p>
                    </div>
                `;
            }
        }

        modalContent.innerHTML = content;

        // Show modal
        document.getElementById('details-modal').classList.remove('hidden');
    },

    // Show metadata details in modal
    showMetadataDetails: async function (metadata) {
        const modalTitle = document.getElementById('modal-title');
        const modalContent = document.getElementById('modal-content');

        modalTitle.textContent = metadata.name;

        try {
            // Fetch files
            const files = await APIClient.getFiles(metadata.info_hash);

            // Create content
            let content = `
                <div class="grid grid-cols-1 md:grid-cols-2 gap-4 mb-4">
                    <div>
                        <h4 class="text-lg font-semibold mb-2">Metadata Information</h4>
                        <p><strong>Infohash:</strong> ${metadata.info_hash}</p>
                        <p><strong>Name:</strong> ${metadata.name}</p>
                        <p><strong>Size:</strong> ${this.formatSize(metadata.total_size)}</p>
                        <p><strong>Piece Count:</strong> ${metadata.piece_count}</p>
                        <p><strong>File Count:</strong> ${metadata.file_count}</p>
                    </div>
                    <div>
                        <h4 class="text-lg font-semibold mb-2">Additional Information</h4>
                        <p><strong>Download Time:</strong> ${this.formatDate(metadata.download_time)}</p>
                        ${metadata.comment ? `<p><strong>Comment:</strong> ${metadata.comment}</p>` : ''}
                    </div>
                </div>
            `;

            if (files && files.length > 0) {
                content += `
                    <div class="border-t border-gray-600 pt-4 mt-4">
                        <h4 class="text-lg font-semibold mb-2">Files (${files.length})</h4>
                        <div class="max-h-60 overflow-y-auto">
                            <ul class="file-list">
                `;

                files.forEach(file => {
                    content += `
                        <li class="flex justify-between">
                            <span class="truncate mr-4">${file.path}</span>
                            <span class="text-gray-400 whitespace-nowrap">${this.formatSize(file.size)}</span>
                        </li>
                    `;
                });

                content += `
                            </ul>
                        </div>
                    </div>
                `;
            }

            modalContent.innerHTML = content;
        } catch (error) {
            console.error('Error fetching files:', error);
            modalContent.innerHTML = `
                <div>
                    <h4 class="text-lg font-semibold mb-2">Metadata Information</h4>
                    <p><strong>Infohash:</strong> ${metadata.info_hash}</p>
                    <p><strong>Name:</strong> ${metadata.name}</p>
                    <p><strong>Size:</strong> ${this.formatSize(metadata.total_size)}</p>
                    <p><strong>Piece Count:</strong> ${metadata.piece_count}</p>
                    <p><strong>File Count:</strong> ${metadata.file_count}</p>
                    <p><strong>Download Time:</strong> ${this.formatDate(metadata.download_time)}</p>
                    ${metadata.comment ? `<p><strong>Comment:</strong> ${metadata.comment}</p>` : ''}
                </div>
                <div class="mt-4 text-red-500">
                    <p>Failed to load file information. Please try again later.</p>
                </div>
            `;
        }

        // Show modal
        document.getElementById('details-modal').classList.remove('hidden');
    },

    // Close modal
    closeModal: function () {
        document.getElementById('details-modal').classList.add('hidden');
    },

    // Update pagination controls
    updatePagination: function (itemCount) {
        const prevButton = document.getElementById('prev-page');
        const nextButton = document.getElementById('next-page');
        const showingText = document.getElementById('showing-text');

        // Update showing text
        const start = this.currentPage * this.itemsPerPage + 1;
        const end = Math.min(start + itemCount - 1, (this.currentPage + 1) * this.itemsPerPage);
        const total = DataManager.getCount(this.currentTab);

        showingText.textContent = `Showing ${start}-${end} of ${total}`;

        // Update button states
        prevButton.disabled = this.currentPage === 0;
        nextButton.disabled = end >= total;
    },

    // Go to previous page
    prevPage: function () {
        if (this.currentPage > 0) {
            this.currentPage--;
            this.loadTabData();
        }
    },

    // Go to next page
    nextPage: function () {
        const total = DataManager.getCount(this.currentTab);
        if ((this.currentPage + 1) * this.itemsPerPage < total) {
            this.currentPage++;
            this.loadTabData();
        }
    },

    // Handle search input
    handleSearch: function (query) {
        this.searchQuery = query;
        this.currentPage = 0;
        this.loadTabData();
    },

    // Show error message
    showError: function (message) {
        // TODO: Implement error notification
        console.error(message);
    },

    // Helper: Format number with commas
    formatNumber: function (number) {
        return number.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",");
    },

    // Helper: Format date
    formatDate: function (timestamp) {
        if (!timestamp) return 'N/A';

        const date = new Date(timestamp * 1000);
        return date.toLocaleString();
    },

    // Helper: Format file size
    formatSize: function (bytes) {
        if (bytes === 0) return '0 B';

        const units = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];
        const i = Math.floor(Math.log(bytes) / Math.log(1024));

        return parseFloat((bytes / Math.pow(1024, i)).toFixed(2)) + ' ' + units[i];
    },

    // Helper: Truncate string with ellipsis
    truncateString: function (str, maxLength) {
        if (str.length <= maxLength) return str;
        return str.substring(0, maxLength) + '...';
    }
};
