// Network Visualizer for 3D network visualization
const NetworkVisualizer = {
    scene: null,
    camera: null,
    renderer: null,
    nodes: [],
    links: [],
    nodeObjects: {},
    linkObjects: [],
    raycaster: null,
    mouse: null,

    init: function () {
        // Initialize Three.js scene
        this.initScene();

        // Create tooltip element
        this.createTooltip();

        // Start animation loop
        this.animate();

        // Add window resize handler
        window.addEventListener('resize', this.onWindowResize.bind(this));
    },

    // Create tooltip element
    createTooltip: function () {
        // Get container
        const container = document.getElementById('network-visualization');

        // Create tooltip element
        this.tooltip = document.createElement('div');
        this.tooltip.className = 'network-tooltip hidden';
        this.tooltip.innerHTML = '';

        // Add to container
        container.appendChild(this.tooltip);
    },

    // Initialize Three.js scene
    initScene: function () {
        // Get container
        const container = document.getElementById('network-visualization');

        // Create scene
        this.scene = new THREE.Scene();

        // Create camera
        this.camera = new THREE.PerspectiveCamera(
            60,
            container.clientWidth / container.clientHeight,
            0.1,
            1000
        );
        this.camera.position.z = 150;

        // Create renderer
        this.renderer = new THREE.WebGLRenderer({antialias: true, alpha: true});
        this.renderer.setSize(container.clientWidth, container.clientHeight);
        this.renderer.setClearColor(0x000000, 0);
        container.appendChild(this.renderer.domElement);

        // Create raycaster for mouse interaction
        this.raycaster = new THREE.Raycaster();
        this.mouse = new THREE.Vector2();

        // Add event listener for mouse movement
        container.addEventListener('mousemove', this.onMouseMove.bind(this));

        // Add ambient light
        const ambientLight = new THREE.AmbientLight(0xffffff, 0.5);
        this.scene.add(ambientLight);

        // Add directional light
        const directionalLight = new THREE.DirectionalLight(0xffffff, 0.5);
        directionalLight.position.set(0, 1, 1);
        this.scene.add(directionalLight);
    },

    // Update network visualization with new nodes
    updateNetwork: function (nodes) {
        // Clear existing objects
        this.clearNetwork();

        // Store nodes
        this.nodes = nodes;

        // Create node objects
        this.createNodeObjects();

        // Create links between nodes
        this.createLinks();
    },

    // Clear network visualization
    clearNetwork: function () {
        // Remove node objects
        for (const nodeId in this.nodeObjects) {
            this.scene.remove(this.nodeObjects[nodeId]);
        }
        this.nodeObjects = {};

        // Remove link objects
        for (const link of this.linkObjects) {
            this.scene.remove(link);
        }
        this.linkObjects = [];
    },

    // Create node objects
    createNodeObjects: function () {
        // Create geometry for nodes
        const geometry = new THREE.SphereGeometry(1, 16, 16);

        // Create nodes
        for (const node of this.nodes) {
            // Create material for node
            const material = new THREE.MeshPhongMaterial({
                color: this.getNodeColor(node),
                emissive: this.getNodeColor(node),
                emissiveIntensity: 0.3,
                specular: 0xffffff,
                shininess: 50
            });

            // Create mesh
            const mesh = new THREE.Mesh(geometry, material);

            // Set position
            const position = this.getNodePosition(node);
            mesh.position.set(position.x, position.y, position.z);

            // Store node data
            mesh.userData = {
                nodeId: node.node_id,
                nodeData: node
            };

            // Add to scene
            this.scene.add(mesh);

            // Store reference
            this.nodeObjects[node.node_id] = mesh;
        }
    },

    // Create links between nodes
    createLinks: function () {
        // Create links between nodes that are close to each other
        for (let i = 0; i < this.nodes.length; i++) {
            const nodeA = this.nodes[i];
            const posA = this.nodeObjects[nodeA.node_id].position;

            for (let j = i + 1; j < this.nodes.length; j++) {
                const nodeB = this.nodes[j];
                const posB = this.nodeObjects[nodeB.node_id].position;

                // Calculate distance
                const distance = posA.distanceTo(posB);

                // Create link if nodes are close
                if (distance < 40) {
                    // Create link
                    const link = this.createLink(posA, posB, distance);

                    // Add to scene
                    this.scene.add(link);

                    // Store reference
                    this.linkObjects.push(link);
                }
            }
        }
    },

    // Create a link between two positions
    createLink: function (posA, posB, distance) {
        // Create geometry
        const geometry = new THREE.BufferGeometry();

        // Set positions
        const positions = new Float32Array([
            posA.x, posA.y, posA.z,
            posB.x, posB.y, posB.z
        ]);
        geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));

        // Create material
        const material = new THREE.LineBasicMaterial({
            color: 0x4f46e5,
            transparent: true,
            opacity: Math.max(0.1, 1 - distance / 40)
        });

        // Create line
        return new THREE.Line(geometry, material);
    },

    // Get color for a node based on its properties
    getNodeColor: function (node) {
        if (node.is_responsive) {
            return 0x3b82f6; // Blue for responsive nodes
        } else {
            return 0xef4444; // Red for unresponsive nodes
        }
    },

    // Get position for a node based on its ID
    getNodePosition: function (node) {
        // Use node ID to generate a deterministic position
        const hash = this.hashCode(node.node_id);

        // Generate position within a sphere
        const radius = 50 + Math.random() * 30;
        const phi = Math.acos(-1 + (2 * ((hash >> 16) & 0xFFFF)) / 0xFFFF);
        const theta = 2 * Math.PI * ((hash & 0xFFFF) / 0xFFFF);

        const x = radius * Math.sin(phi) * Math.cos(theta);
        const y = radius * Math.sin(phi) * Math.sin(theta);
        const z = radius * Math.cos(phi);

        return {x, y, z};
    },

    // Simple hash function for strings
    hashCode: function (str) {
        let hash = 0;
        for (let i = 0; i < str.length; i++) {
            const char = str.charCodeAt(i);
            hash = ((hash << 5) - hash) + char;
            hash = hash & hash; // Convert to 32bit integer
        }
        return hash;
    },

    // Handle mouse movement
    onMouseMove: function (event) {
        // Get container
        const container = document.getElementById('network-visualization');

        // Calculate mouse position in normalized device coordinates
        const rect = container.getBoundingClientRect();
        this.mouse.x = ((event.clientX - rect.left) / container.clientWidth) * 2 - 1;
        this.mouse.y = -((event.clientY - rect.top) / container.clientHeight) * 2 + 1;

        // Store actual mouse position for tooltip positioning
        this.mouseX = event.clientX - rect.left;
        this.mouseY = event.clientY - rect.top;
    },

    // Show node tooltip
    showNodeTooltip: function (node) {
        if (!node || !this.tooltip) return;

        // Format node data
        const nodeId = this.truncateString(node.node_id, 16);
        const endpoint = node.endpoint ? node.endpoint.address + ':' + node.endpoint.port : 'Unknown';
        const status = node.is_responsive ? 'Active' : 'Inactive';
        const statusClass = node.is_responsive ? 'text-green-400' : 'text-red-400';

        // Format last seen time
        const lastSeen = new Date(node.last_seen * 1000);
        const timeAgo = this.formatTimeAgo(lastSeen);

        // Set tooltip content
        this.tooltip.innerHTML = `
            <div class="p-3">
                <div class="font-semibold mb-1">${nodeId}</div>
                <div class="text-sm text-gray-300">${endpoint}</div>
                <div class="text-sm mt-1">Status: <span class="${statusClass}">${status}</span></div>
                <div class="text-sm text-gray-400">Last seen: ${timeAgo}</div>
            </div>
        `;

        // Position tooltip near the mouse but ensure it stays within the container
        const container = document.getElementById('network-visualization');
        const tooltipWidth = this.tooltip.offsetWidth;
        const tooltipHeight = this.tooltip.offsetHeight;
        const containerWidth = container.offsetWidth;
        const containerHeight = container.offsetHeight;

        // Calculate position (offset slightly from cursor)
        let left = this.mouseX + 15;
        let top = this.mouseY + 15;

        // Ensure tooltip stays within container bounds
        if (left + tooltipWidth > containerWidth) {
            left = this.mouseX - tooltipWidth - 15;
        }
        if (top + tooltipHeight > containerHeight) {
            top = this.mouseY - tooltipHeight - 15;
        }

        // Set position
        this.tooltip.style.left = left + 'px';
        this.tooltip.style.top = top + 'px';

        // Show tooltip
        this.tooltip.classList.remove('hidden');
    },

    // Hide node tooltip
    hideNodeTooltip: function () {
        if (this.tooltip) {
            this.tooltip.classList.add('hidden');
        }
    },

    // Format time ago
    formatTimeAgo: function (date) {
        const now = new Date();
        const diffMs = now - date;
        const diffSec = Math.floor(diffMs / 1000);
        const diffMin = Math.floor(diffSec / 60);
        const diffHour = Math.floor(diffMin / 60);
        const diffDay = Math.floor(diffHour / 24);

        if (diffSec < 60) {
            return diffSec + ' seconds ago';
        } else if (diffMin < 60) {
            return diffMin + ' minutes ago';
        } else if (diffHour < 24) {
            return diffHour + ' hours ago';
        } else {
            return diffDay + ' days ago';
        }
    },

    // Handle window resize
    onWindowResize: function () {
        // Get container
        const container = document.getElementById('network-visualization');

        // Update camera
        this.camera.aspect = container.clientWidth / container.clientHeight;
        this.camera.updateProjectionMatrix();

        // Update renderer
        this.renderer.setSize(container.clientWidth, container.clientHeight);
    },

    // Create tooltip element
    createTooltip: function () {
        // Check if tooltip already exists
        if (document.getElementById('node-tooltip')) {
            return;
        }

        // Create tooltip element
        const tooltip = document.createElement('div');
        tooltip.id = 'node-tooltip';
        tooltip.className = 'fixed hidden z-50 bg-gray-800 text-white p-3 rounded-lg shadow-lg border border-gray-700 text-sm max-w-xs';
        tooltip.style.pointerEvents = 'none'; // Prevent tooltip from interfering with mouse events
        document.body.appendChild(tooltip);
    },

    // Show node tooltip
    showNodeTooltip: function (node, x, y) {
        const tooltip = document.getElementById('node-tooltip');
        if (!tooltip) {
            this.createTooltip();
        }

        // Format node data
        const nodeData = node.userData.nodeData;
        const nodeId = this.truncateString(nodeData.node_id, 16);
        const ip = nodeData.endpoint ? nodeData.endpoint.split(':')[0] : 'Unknown';
        const port = nodeData.endpoint ? nodeData.endpoint.split(':')[1] : 'Unknown';
        const status = nodeData.is_responsive ? 'Active' : 'Inactive';
        const statusClass = nodeData.is_responsive ? 'text-green-400' : 'text-red-400';

        // Set tooltip content
        tooltip.innerHTML = `
            <div>
                <div class="font-semibold mb-1">Node ID: <span class="text-blue-400">${nodeId}</span></div>
                <div>IP: ${ip}</div>
                <div>Port: ${port}</div>
                <div>Status: <span class="${statusClass}">${status}</span></div>
            </div>
        `;

        // Position tooltip
        tooltip.style.left = `${x + 15}px`;
        tooltip.style.top = `${y + 15}px`;

        // Show tooltip
        tooltip.classList.remove('hidden');
    },

    // Hide node tooltip
    hideNodeTooltip: function () {
        const tooltip = document.getElementById('node-tooltip');
        if (tooltip) {
            tooltip.classList.add('hidden');
        }
    },

    // Helper: Truncate string with ellipsis
    truncateString: function (str, maxLength) {
        if (!str) return '';
        if (str.length <= maxLength) return str;
        return str.substring(0, maxLength) + '...';
    },

    // Animation loop
    animate: function () {
        requestAnimationFrame(this.animate.bind(this));

        // Rotate camera around the scene
        const time = Date.now() * 0.0001;
        this.camera.position.x = Math.sin(time) * 150;
        this.camera.position.z = Math.cos(time) * 150;
        this.camera.lookAt(0, 0, 0);

        // Check for intersections with nodes
        this.raycaster.setFromCamera(this.mouse, this.camera);
        const intersects = this.raycaster.intersectObjects(Object.values(this.nodeObjects));

        // Reset all node sizes
        for (const nodeId in this.nodeObjects) {
            this.nodeObjects[nodeId].scale.set(1, 1, 1);
        }

        // Hide tooltip if no intersection
        if (intersects.length === 0) {
            this.hideNodeTooltip();
        }

        // Highlight intersected node and show tooltip
        if (intersects.length > 0) {
            const object = intersects[0].object;
            object.scale.set(1.5, 1.5, 1.5);

            // Show node info tooltip
            if (object.userData && object.userData.nodeData) {
                this.showNodeTooltip(object.userData.nodeData);
            }
        }

        // Render scene
        this.renderer.render(this.scene, this.camera);
    }
};
