// Data Manager for handling application data
const DataManager = {
    nodes: [],
    infohashes: [],
    metadata: [],

    init: function () {
        // Initialize data structures
    },

    // Set nodes data
    setNodes: function (nodes) {
        this.nodes = nodes;
    },

    // Set infohashes data
    setInfohashes: function (infohashes) {
        this.infohashes = infohashes;
    },

    // Set metadata data
    setMetadata: function (metadata) {
        this.metadata = metadata;
    },

    // Add a node
    addNode: function (node) {
        // Check if node already exists
        const index = this.nodes.findIndex(n => n.node_id === node.node_id);

        if (index !== -1) {
            // Update existing node
            this.nodes[index] = node;
        } else {
            // Add new node
            this.nodes.push(node);
        }
    },

    // Add an infohash
    addInfohash: function (infohash) {
        // Check if infohash already exists
        const index = this.infohashes.findIndex(ih => ih.info_hash === infohash.info_hash);

        if (index !== -1) {
            // Update existing infohash
            this.infohashes[index] = infohash;
        } else {
            // Add new infohash
            this.infohashes.push(infohash);
        }
    },

    // Add metadata
    addMetadata: function (metadata) {
        // Check if metadata already exists
        const index = this.metadata.findIndex(m => m.info_hash === metadata.info_hash);

        if (index !== -1) {
            // Update existing metadata
            this.metadata[index] = metadata;
        } else {
            // Add new metadata
            this.metadata.push(metadata);
        }

        // Update corresponding infohash
        const infohashIndex = this.infohashes.findIndex(ih => ih.info_hash === metadata.info_hash);
        if (infohashIndex !== -1) {
            this.infohashes[infohashIndex].has_metadata = true;
        }
    },

    // Get node count
    getNodeCount: function () {
        return this.nodes.length;
    },

    // Get infohash count
    getInfohashCount: function () {
        return this.infohashes.length;
    },

    // Get metadata count
    getMetadataCount: function () {
        return this.metadata.length;
    },

    // Get count for a specific data type
    getCount: function (type) {
        switch (type) {
            case 'nodes':
                return this.getNodeCount();

            case 'infohashes':
                return this.getInfohashCount();

            case 'metadata':
                return this.getMetadataCount();

            default:
                return 0;
        }
    },

    // Get nodes with pagination
    getNodes: function (limit, offset) {
        return this.nodes.slice(offset, offset + limit);
    },

    // Get infohashes with pagination
    getInfohashes: function (limit, offset) {
        return this.infohashes.slice(offset, offset + limit);
    },

    // Get metadata with pagination
    getMetadata: function (limit, offset) {
        return this.metadata.slice(offset, offset + limit);
    },

    // Search metadata by name
    searchMetadata: function (query, limit, offset) {
        if (!query) {
            return this.getMetadata(limit, offset);
        }

        const lowerQuery = query.toLowerCase();
        const results = this.metadata.filter(m =>
            m.name.toLowerCase().includes(lowerQuery)
        );

        return results.slice(offset, offset + limit);
    },

    // Get node by ID
    getNodeById: function (nodeId) {
        return this.nodes.find(n => n.node_id === nodeId);
    },

    // Get infohash by ID
    getInfohashById: function (infoHash) {
        return this.infohashes.find(ih => ih.info_hash === infoHash);
    },

    // Get metadata by infohash
    getMetadataByInfohash: function (infoHash) {
        return this.metadata.find(m => m.info_hash === infoHash);
    }
};
