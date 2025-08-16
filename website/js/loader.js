/**
 * Premium Loading Screen
 */

(function() {
    'use strict';

    // Create loading screen HTML
    const loaderHTML = `
        <div id="loading-screen" class="loading-screen">
            <div class="loader-content">
                <div class="loader-logo">
                    <svg viewBox="0 0 100 100" width="100" height="100">
                        <defs>
                            <linearGradient id="logo-gradient" x1="0%" y1="0%" x2="100%" y2="100%">
                                <stop offset="0%" style="stop-color:#00e599;stop-opacity:1" />
                                <stop offset="100%" style="stop-color:#00b377;stop-opacity:1" />
                            </linearGradient>
                        </defs>
                        <polygon points="50,15 80,35 80,65 50,85 20,65 20,35" 
                                 fill="url(#logo-gradient)" 
                                 class="loader-polygon"/>
                    </svg>
                </div>
                <div class="loader-text">Neo C++</div>
                <div class="loader-progress">
                    <div class="loader-progress-bar"></div>
                </div>
                <div class="loader-status">Initializing blockchain...</div>
            </div>
        </div>
    `;

    // Add loading screen styles
    const loaderStyles = `
        <style>
            .loading-screen {
                position: fixed;
                top: 0;
                left: 0;
                right: 0;
                bottom: 0;
                background: linear-gradient(135deg, #0a0a0a 0%, #1a1a2e 100%);
                z-index: 10000;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 1;
                transition: opacity 0.5s ease-out;
            }

            .loading-screen.fade-out {
                opacity: 0;
                pointer-events: none;
            }

            .loader-content {
                text-align: center;
            }

            .loader-logo {
                width: 100px;
                height: 100px;
                margin: 0 auto 20px;
                animation: float 3s ease-in-out infinite;
            }

            .loader-polygon {
                animation: rotate 3s linear infinite;
                transform-origin: center;
            }

            @keyframes float {
                0%, 100% { transform: translateY(0); }
                50% { transform: translateY(-20px); }
            }

            @keyframes rotate {
                from { transform: rotate(0deg); }
                to { transform: rotate(360deg); }
            }

            .loader-text {
                font-size: 2rem;
                font-weight: 800;
                background: linear-gradient(135deg, #00e599 0%, #00b377 100%);
                -webkit-background-clip: text;
                -webkit-text-fill-color: transparent;
                background-clip: text;
                margin-bottom: 30px;
                letter-spacing: 2px;
            }

            .loader-progress {
                width: 200px;
                height: 4px;
                background: rgba(255, 255, 255, 0.1);
                border-radius: 2px;
                margin: 0 auto 20px;
                overflow: hidden;
            }

            .loader-progress-bar {
                height: 100%;
                background: linear-gradient(90deg, #00e599, #00b377);
                border-radius: 2px;
                width: 0;
                animation: progress 2s ease-out forwards;
            }

            @keyframes progress {
                to { width: 100%; }
            }

            .loader-status {
                color: rgba(255, 255, 255, 0.6);
                font-size: 0.875rem;
                animation: pulse 1.5s ease-in-out infinite;
            }

            @keyframes pulse {
                0%, 100% { opacity: 0.6; }
                50% { opacity: 1; }
            }
        </style>
    `;

    // Insert loader into page
    document.head.insertAdjacentHTML('beforeend', loaderStyles);
    document.body.insertAdjacentHTML('afterbegin', loaderHTML);

    // Loading messages
    const messages = [
        'Initializing blockchain...',
        'Loading smart contracts...',
        'Connecting to network...',
        'Verifying consensus...',
        'Starting Neo C++...'
    ];

    let messageIndex = 0;
    const statusElement = document.querySelector('.loader-status');
    
    // Change loading messages
    const messageInterval = setInterval(() => {
        messageIndex = (messageIndex + 1) % messages.length;
        if (statusElement) {
            statusElement.textContent = messages[messageIndex];
        }
    }, 500);

    // Hide loader when page is fully loaded
    window.addEventListener('load', () => {
        setTimeout(() => {
            const loader = document.getElementById('loading-screen');
            if (loader) {
                loader.classList.add('fade-out');
                clearInterval(messageInterval);
                
                setTimeout(() => {
                    loader.remove();
                    document.body.classList.add('loaded');
                }, 500);
            }
        }, 2000); // Minimum loading time for effect
    });

    // Fallback: Hide loader after max time
    setTimeout(() => {
        const loader = document.getElementById('loading-screen');
        if (loader && !loader.classList.contains('fade-out')) {
            loader.classList.add('fade-out');
            clearInterval(messageInterval);
            setTimeout(() => {
                loader.remove();
                document.body.classList.add('loaded');
            }, 500);
        }
    }, 5000);

})();