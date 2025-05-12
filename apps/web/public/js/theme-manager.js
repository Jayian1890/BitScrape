// Theme Manager for handling theme switching
const ThemeManager = {
    isDarkMode: true,

    init: function () {
        // Set initial theme
        this.applyTheme();

        // Add event listener for theme toggle button
        document.getElementById('theme-toggle').addEventListener('click', this.toggleTheme.bind(this));
    },

    // Toggle between light and dark themes
    toggleTheme: function () {
        this.isDarkMode = !this.isDarkMode;
        this.applyTheme();
    },

    // Apply the current theme
    applyTheme: function () {
        const body = document.body;

        if (this.isDarkMode) {
            // Apply dark theme
            body.classList.add('bg-gray-900');
            body.classList.add('text-white');
            body.classList.remove('bg-gray-100');
            body.classList.remove('text-gray-900');

            // Update theme toggle icon
            document.getElementById('theme-toggle').innerHTML = `
                <svg xmlns="http://www.w3.org/2000/svg" class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 3v1m0 16v1m9-9h-1M4 12H3m15.364 6.364l-.707-.707M6.343 6.343l-.707-.707m12.728 0l-.707.707M6.343 17.657l-.707.707M16 12a4 4 0 11-8 0 4 4 0 018 0z" />
                </svg>
            `;
        } else {
            // Apply light theme
            body.classList.add('bg-gray-100');
            body.classList.add('text-gray-900');
            body.classList.remove('bg-gray-900');
            body.classList.remove('text-white');

            // Update theme toggle icon
            document.getElementById('theme-toggle').innerHTML = `
                <svg xmlns="http://www.w3.org/2000/svg" class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M20.354 15.354A9 9 0 018.646 3.646 9.003 9.003 0 0012 21a9.003 9.003 0 008.354-5.646z" />
                </svg>
            `;
        }
    }
};
