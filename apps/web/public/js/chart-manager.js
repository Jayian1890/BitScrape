// Chart Manager for handling data visualization
const ChartManager = {
    discoveryChart: null,
    distributionChart: null,
    discoveryData: {
        nodes: [],
        infohashes: [],
        metadata: []
    },

    init: function () {
        this.initDiscoveryChart();
        this.initDistributionChart();
    },

    // Initialize discovery rate chart
    initDiscoveryChart: function () {
        const ctx = document.getElementById('discovery-chart').getContext('2d');

        this.discoveryChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: this.generateTimeLabels(12),
                datasets: [
                    {
                        label: 'Nodes',
                        data: Array(12).fill(0),
                        borderColor: 'rgba(59, 130, 246, 1)',
                        backgroundColor: 'rgba(59, 130, 246, 0.1)',
                        tension: 0.4,
                        fill: true
                    },
                    {
                        label: 'Infohashes',
                        data: Array(12).fill(0),
                        borderColor: 'rgba(139, 92, 246, 1)',
                        backgroundColor: 'rgba(139, 92, 246, 0.1)',
                        tension: 0.4,
                        fill: true
                    },
                    {
                        label: 'Metadata',
                        data: Array(12).fill(0),
                        borderColor: 'rgba(16, 185, 129, 1)',
                        backgroundColor: 'rgba(16, 185, 129, 0.1)',
                        tension: 0.4,
                        fill: true
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'top',
                        labels: {
                            color: 'white'
                        }
                    },
                    tooltip: {
                        mode: 'index',
                        intersect: false
                    }
                },
                scales: {
                    x: {
                        grid: {
                            color: 'rgba(255, 255, 255, 0.1)'
                        },
                        ticks: {
                            color: 'rgba(255, 255, 255, 0.7)'
                        }
                    },
                    y: {
                        grid: {
                            color: 'rgba(255, 255, 255, 0.1)'
                        },
                        ticks: {
                            color: 'rgba(255, 255, 255, 0.7)'
                        }
                    }
                },
                interaction: {
                    intersect: false,
                    mode: 'nearest'
                }
            }
        });
    },

    // Initialize distribution chart
    initDistributionChart: function () {
        const ctx = document.getElementById('distribution-chart').getContext('2d');

        this.distributionChart = new Chart(ctx, {
            type: 'doughnut',
            data: {
                labels: ['Nodes', 'Infohashes', 'Metadata'],
                datasets: [{
                    data: [0, 0, 0],
                    backgroundColor: [
                        'rgba(59, 130, 246, 0.8)',
                        'rgba(139, 92, 246, 0.8)',
                        'rgba(16, 185, 129, 0.8)'
                    ],
                    borderColor: [
                        'rgba(59, 130, 246, 1)',
                        'rgba(139, 92, 246, 1)',
                        'rgba(16, 185, 129, 1)'
                    ],
                    borderWidth: 1
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'top',
                        labels: {
                            color: 'white'
                        }
                    }
                },
                cutout: '70%'
            }
        });
    },

    // Update charts with new statistics
    updateCharts: function (stats) {
        // Update discovery data
        this.updateDiscoveryData(stats);

        // Update discovery chart
        this.updateDiscoveryChart();

        // Update distribution chart
        this.updateDistributionChart(stats);
    },

    // Update discovery data with new statistics
    updateDiscoveryData: function (stats) {
        // Get current counts
        const nodeCount = parseInt(stats['storage.node_count'] || '0');
        const infohashCount = parseInt(stats['storage.infohash_count'] || '0');
        const metadataCount = parseInt(stats['storage.metadata_count'] || '0');

        // Add to discovery data
        this.discoveryData.nodes.push(nodeCount);
        this.discoveryData.infohashes.push(infohashCount);
        this.discoveryData.metadata.push(metadataCount);

        // Keep only the last 12 data points
        if (this.discoveryData.nodes.length > 12) {
            this.discoveryData.nodes.shift();
            this.discoveryData.infohashes.shift();
            this.discoveryData.metadata.shift();
        }
    },

    // Update discovery chart with current data
    updateDiscoveryChart: function () {
        // Update chart data
        this.discoveryChart.data.datasets[0].data = this.discoveryData.nodes;
        this.discoveryChart.data.datasets[1].data = this.discoveryData.infohashes;
        this.discoveryChart.data.datasets[2].data = this.discoveryData.metadata;

        // Update chart labels
        this.discoveryChart.data.labels = this.generateTimeLabels(this.discoveryData.nodes.length);

        // Update chart
        this.discoveryChart.update();
    },

    // Update distribution chart with new statistics
    updateDistributionChart: function (stats) {
        // Get current counts
        const nodeCount = parseInt(stats['storage.node_count'] || '0');
        const infohashCount = parseInt(stats['storage.infohash_count'] || '0');
        const metadataCount = parseInt(stats['storage.metadata_count'] || '0');

        // Update chart data
        this.distributionChart.data.datasets[0].data = [nodeCount, infohashCount, metadataCount];

        // Update chart
        this.distributionChart.update();
    },

    // Generate time labels for the discovery chart
    generateTimeLabels: function (count) {
        const labels = [];
        const now = new Date();

        for (let i = count - 1; i >= 0; i--) {
            const time = new Date(now - i * 5 * 60 * 1000); // 5 minutes intervals
            labels.push(time.toLocaleTimeString([], {hour: '2-digit', minute: '2-digit'}));
        }

        return labels;
    }
};
