// ===== MAIN JAVASCRIPT FOR NEO C++ WEBSITE =====

document.addEventListener('DOMContentLoaded', function() {
    // Initialize all components
    initializeNavigation();
    initializeTabs();
    initializeScrollEffects();
    initializeTypingAnimation();
    initializeParallax();
    initializeLazyLoading();
    initializeAnalytics();
});

// ===== NAVIGATION =====
function initializeNavigation() {
    const hamburger = document.querySelector('.hamburger');
    const navMenu = document.querySelector('.nav-menu');
    const navLinks = document.querySelectorAll('.nav-link');
    const navbar = document.querySelector('.navbar');

    // Mobile menu toggle
    if (hamburger && navMenu) {
        hamburger.addEventListener('click', function() {
            hamburger.classList.toggle('active');
            navMenu.classList.toggle('active');
            
            // Animate hamburger bars
            const bars = hamburger.querySelectorAll('.bar');
            bars.forEach((bar, index) => {
                if (hamburger.classList.contains('active')) {
                    if (index === 0) bar.style.transform = 'rotate(-45deg) translate(-5px, 6px)';
                    if (index === 1) bar.style.opacity = '0';
                    if (index === 2) bar.style.transform = 'rotate(45deg) translate(-5px, -6px)';
                } else {
                    bar.style.transform = 'none';
                    bar.style.opacity = '1';
                }
            });
        });
    }

    // Close mobile menu when clicking nav links
    navLinks.forEach(link => {
        link.addEventListener('click', () => {
            if (hamburger && navMenu) {
                hamburger.classList.remove('active');
                navMenu.classList.remove('active');
                
                // Reset hamburger bars
                const bars = hamburger.querySelectorAll('.bar');
                bars.forEach(bar => {
                    bar.style.transform = 'none';
                    bar.style.opacity = '1';
                });
            }
        });
    });

    // Smooth scrolling for navigation links
    document.querySelectorAll('a[href^="#"]').forEach(anchor => {
        anchor.addEventListener('click', function (e) {
            e.preventDefault();
            const target = document.querySelector(this.getAttribute('href'));
            if (target) {
                const offsetTop = target.offsetTop - 70; // Account for fixed navbar
                window.scrollTo({
                    top: offsetTop,
                    behavior: 'smooth'
                });
            }
        });
    });

    // Navbar scroll effect
    let lastScrollTop = 0;
    window.addEventListener('scroll', function() {
        const scrollTop = window.pageYOffset || document.documentElement.scrollTop;
        
        if (navbar) {
            if (scrollTop > 100) {
                navbar.style.background = 'rgba(255, 255, 255, 0.98)';
                navbar.style.boxShadow = '0 2px 20px rgba(0, 0, 0, 0.1)';
            } else {
                navbar.style.background = 'rgba(255, 255, 255, 0.95)';
                navbar.style.boxShadow = 'none';
            }
        }
        
        // Active navigation highlighting
        updateActiveNavigation();
        
        lastScrollTop = scrollTop;
    });
}

// Update active navigation based on scroll position
function updateActiveNavigation() {
    const sections = document.querySelectorAll('section[id]');
    const navLinks = document.querySelectorAll('.nav-link[href^="#"]');
    
    sections.forEach(section => {
        const sectionTop = section.offsetTop - 100;
        const sectionBottom = sectionTop + section.offsetHeight;
        const scrollPosition = window.pageYOffset;
        
        if (scrollPosition >= sectionTop && scrollPosition < sectionBottom) {
            const id = section.getAttribute('id');
            
            navLinks.forEach(link => {
                link.classList.remove('active');
                if (link.getAttribute('href') === `#${id}`) {
                    link.classList.add('active');
                }
            });
        }
    });
}

// ===== TABS FUNCTIONALITY =====
function initializeTabs() {
    const tabButtons = document.querySelectorAll('.tab-btn');
    const tabPanels = document.querySelectorAll('.tab-panel');

    tabButtons.forEach(button => {
        button.addEventListener('click', function() {
            const targetTab = this.getAttribute('data-tab');
            
            // Remove active class from all buttons and panels
            tabButtons.forEach(btn => btn.classList.remove('active'));
            tabPanels.forEach(panel => panel.classList.remove('active'));
            
            // Add active class to clicked button and corresponding panel
            this.classList.add('active');
            const targetPanel = document.getElementById(targetTab);
            if (targetPanel) {
                targetPanel.classList.add('active');
                
                // Trigger animation
                targetPanel.style.opacity = '0';
                targetPanel.style.transform = 'translateY(20px)';
                
                setTimeout(() => {
                    targetPanel.style.transition = 'all 0.3s ease';
                    targetPanel.style.opacity = '1';
                    targetPanel.style.transform = 'translateY(0)';
                }, 50);
            }
        });
    });
}

// ===== SCROLL EFFECTS =====
function initializeScrollEffects() {
    // Intersection Observer for animations
    const observerOptions = {
        threshold: 0.1,
        rootMargin: '0px 0px -50px 0px'
    };

    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                entry.target.classList.add('animate-in');
                
                // Special handling for cards
                if (entry.target.classList.contains('feature-card') || 
                    entry.target.classList.contains('doc-card') || 
                    entry.target.classList.contains('community-card')) {
                    animateCard(entry.target);
                }
            }
        });
    }, observerOptions);

    // Observe elements for animation
    const animatedElements = document.querySelectorAll('.feature-card, .doc-card, .community-card, .layer, .section-header');
    animatedElements.forEach(el => observer.observe(el));
}

// Animate cards with stagger effect
function animateCard(card) {
    const cards = Array.from(card.parentElement.children);
    const index = cards.indexOf(card);
    
    setTimeout(() => {
        card.style.transform = 'translateY(0)';
        card.style.opacity = '1';
    }, index * 100);
}

// ===== TYPING ANIMATION =====
function initializeTypingAnimation() {
    const codeContent = document.querySelector('.code-content code');
    if (!codeContent) return;

    const originalText = codeContent.textContent;
    const lines = originalText.split('\n');
    let currentLine = 0;
    let currentChar = 0;
    
    codeContent.textContent = '';
    
    function typeLine() {
        if (currentLine >= lines.length) return;
        
        const line = lines[currentLine];
        
        if (currentChar < line.length) {
            codeContent.textContent += line[currentChar];
            currentChar++;
            setTimeout(typeLine, 50);
        } else {
            codeContent.textContent += '\n';
            currentLine++;
            currentChar = 0;
            setTimeout(typeLine, 200);
        }
    }
    
    // Start typing animation when hero is visible
    const heroObserver = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                setTimeout(typeLine, 1000);
                heroObserver.disconnect();
            }
        });
    });
    
    const hero = document.querySelector('.hero');
    if (hero) heroObserver.observe(hero);
}

// ===== PARALLAX EFFECTS =====
function initializeParallax() {
    const floatingElements = document.querySelectorAll('.floating-element');
    
    window.addEventListener('scroll', () => {
        const scrolled = window.pageYOffset;
        const parallax = scrolled * 0.5;
        
        floatingElements.forEach((element, index) => {
            const speed = 0.2 + (index * 0.1);
            element.style.transform = `translateY(${parallax * speed}px) rotate(${scrolled * 0.1}deg)`;
        });
    });
}

// ===== LAZY LOADING =====
function initializeLazyLoading() {
    const images = document.querySelectorAll('img[data-src]');
    
    const imageObserver = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                const img = entry.target;
                img.src = img.dataset.src;
                img.classList.remove('lazy');
                imageObserver.unobserve(img);
            }
        });
    });
    
    images.forEach(img => imageObserver.observe(img));
}

// ===== COPY TO CLIPBOARD =====
function copyToClipboard(text) {
    navigator.clipboard.writeText(text).then(() => {
        showNotification('Copied to clipboard!', 'success');
    }).catch(() => {
        showNotification('Failed to copy', 'error');
    });
}

// Add copy buttons to code blocks
document.querySelectorAll('pre code').forEach(codeBlock => {
    const copyButton = document.createElement('button');
    copyButton.className = 'copy-btn';
    copyButton.innerHTML = '<i class="fas fa-copy"></i>';
    copyButton.setAttribute('title', 'Copy to clipboard');
    
    copyButton.addEventListener('click', () => {
        copyToClipboard(codeBlock.textContent);
        copyButton.innerHTML = '<i class="fas fa-check"></i>';
        copyButton.style.color = '#00d4aa';
        
        setTimeout(() => {
            copyButton.innerHTML = '<i class="fas fa-copy"></i>';
            copyButton.style.color = '';
        }, 2000);
    });
    
    const pre = codeBlock.closest('pre');
    if (pre) {
        pre.style.position = 'relative';
        pre.appendChild(copyButton);
    }
});

// ===== NOTIFICATIONS =====
function showNotification(message, type = 'info') {
    const notification = document.createElement('div');
    notification.className = `notification ${type}`;
    notification.textContent = message;
    
    notification.style.cssText = `
        position: fixed;
        top: 100px;
        right: 20px;
        background: ${type === 'success' ? '#00d4aa' : type === 'error' ? '#e74c3c' : '#3498db'};
        color: white;
        padding: 12px 20px;
        border-radius: 8px;
        box-shadow: 0 4px 12px rgba(0,0,0,0.15);
        z-index: 10000;
        opacity: 0;
        transform: translateX(100%);
        transition: all 0.3s ease;
    `;
    
    document.body.appendChild(notification);
    
    // Animate in
    setTimeout(() => {
        notification.style.opacity = '1';
        notification.style.transform = 'translateX(0)';
    }, 100);
    
    // Animate out and remove
    setTimeout(() => {
        notification.style.opacity = '0';
        notification.style.transform = 'translateX(100%)';
        setTimeout(() => document.body.removeChild(notification), 300);
    }, 3000);
}

// ===== PERFORMANCE METRICS =====
function initializeAnalytics() {
    // Track page load time
    window.addEventListener('load', () => {
        const loadTime = performance.now();
        console.log(`Page loaded in ${Math.round(loadTime)}ms`);
        
        // Track to analytics if available
        if (typeof gtag !== 'undefined') {
            gtag('event', 'page_load_time', {
                value: Math.round(loadTime),
                custom_parameter: 'neo_cpp_website'
            });
        }
    });
    
    // Track scroll depth
    let maxScroll = 0;
    window.addEventListener('scroll', () => {
        const scrollPercent = Math.round((window.scrollY / (document.body.scrollHeight - window.innerHeight)) * 100);
        if (scrollPercent > maxScroll) {
            maxScroll = scrollPercent;
            
            // Track milestones
            if (maxScroll >= 25 && maxScroll < 50) {
                trackEvent('scroll_depth', '25_percent');
            } else if (maxScroll >= 50 && maxScroll < 75) {
                trackEvent('scroll_depth', '50_percent');
            } else if (maxScroll >= 75 && maxScroll < 90) {
                trackEvent('scroll_depth', '75_percent');
            } else if (maxScroll >= 90) {
                trackEvent('scroll_depth', '90_percent');
            }
        }
    });
}

// Generic event tracking
function trackEvent(action, label, value) {
    if (typeof gtag !== 'undefined') {
        gtag('event', action, {
            event_label: label,
            value: value
        });
    }
    console.log(`Event tracked: ${action} - ${label}${value ? ` (${value})` : ''}`);
}

// ===== BUTTON INTERACTIONS =====
document.addEventListener('click', (e) => {
    // Track button clicks
    if (e.target.classList.contains('btn')) {
        const buttonText = e.target.textContent.trim();
        trackEvent('button_click', buttonText);
    }
    
    // Track external links
    if (e.target.tagName === 'A' && e.target.hostname !== window.location.hostname) {
        trackEvent('external_link', e.target.href);
    }
});

// ===== KEYBOARD NAVIGATION =====
document.addEventListener('keydown', (e) => {
    // Close mobile menu with Escape key
    if (e.key === 'Escape') {
        const hamburger = document.querySelector('.hamburger');
        const navMenu = document.querySelector('.nav-menu');
        
        if (hamburger && navMenu && navMenu.classList.contains('active')) {
            hamburger.classList.remove('active');
            navMenu.classList.remove('active');
            
            const bars = hamburger.querySelectorAll('.bar');
            bars.forEach(bar => {
                bar.style.transform = 'none';
                bar.style.opacity = '1';
            });
        }
    }
    
    // Copy code with Ctrl/Cmd + C when focused on code block
    if ((e.ctrlKey || e.metaKey) && e.key === 'c' && e.target.tagName === 'CODE') {
        copyToClipboard(e.target.textContent);
        showNotification('Code copied to clipboard!', 'success');
    }
});

// ===== THEME DETECTION =====
function detectTheme() {
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    const theme = prefersDark ? 'dark' : 'light';
    document.documentElement.setAttribute('data-theme', theme);
    return theme;
}

// Initialize theme
detectTheme();

// Listen for theme changes
window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', detectTheme);

// ===== FORM VALIDATION (if forms are added later) =====
function validateEmail(email) {
    const re = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    return re.test(email);
}

// ===== SEARCH FUNCTIONALITY (if search is added later) =====
function initializeSearch() {
    const searchInput = document.querySelector('#search-input');
    const searchResults = document.querySelector('#search-results');
    
    if (!searchInput || !searchResults) return;
    
    let searchTimeout;
    
    searchInput.addEventListener('input', (e) => {
        clearTimeout(searchTimeout);
        const query = e.target.value.trim();
        
        if (query.length < 2) {
            searchResults.innerHTML = '';
            return;
        }
        
        searchTimeout = setTimeout(() => {
            performSearch(query);
        }, 300);
    });
}

// ===== ERROR HANDLING =====
window.addEventListener('error', (e) => {
    console.error('JavaScript error:', e.error);
    // Track error if analytics available
    trackEvent('javascript_error', e.error.message);
});

// ===== CSS LOADING DETECTION =====
function checkCSSLoaded() {
    const testElement = document.createElement('div');
    testElement.className = 'css-test';
    testElement.style.cssText = 'position: absolute; visibility: hidden;';
    document.body.appendChild(testElement);
    
    const styles = window.getComputedStyle(testElement);
    const cssLoaded = styles.fontFamily.includes('Inter');
    
    document.body.removeChild(testElement);
    
    if (!cssLoaded) {
        console.warn('CSS may not have loaded properly');
        // Fallback styles or retry logic here
    }
    
    return cssLoaded;
}

// Check CSS loading
setTimeout(checkCSSLoaded, 1000);

// ===== EXPORT FOR TESTING =====
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        validateEmail,
        trackEvent,
        copyToClipboard,
        showNotification
    };
}