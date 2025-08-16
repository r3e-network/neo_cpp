/**
 * Neo C++ Website - Enhanced Main JavaScript
 * Modern, performant, and accessible interactions
 */

(function() {
    'use strict';

    // Performance optimization: Use requestAnimationFrame for smooth animations
    const raf = window.requestAnimationFrame || 
                window.webkitRequestAnimationFrame || 
                window.mozRequestAnimationFrame || 
                function(callback) { window.setTimeout(callback, 1000/60); };

    // Debounce function for performance
    function debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    }

    // Throttle function for scroll events
    function throttle(func, limit) {
        let inThrottle;
        return function(...args) {
            if (!inThrottle) {
                func.apply(this, args);
                inThrottle = true;
                setTimeout(() => inThrottle = false, limit);
            }
        };
    }

    // Smooth scroll with easing
    function smoothScrollTo(target, duration = 800) {
        const targetElement = document.querySelector(target);
        if (!targetElement) return;

        const targetPosition = targetElement.getBoundingClientRect().top + window.pageYOffset;
        const startPosition = window.pageYOffset;
        const distance = targetPosition - startPosition;
        let startTime = null;

        function ease(t, b, c, d) {
            t /= d / 2;
            if (t < 1) return c / 2 * t * t * t + b;
            t -= 2;
            return c / 2 * (t * t * t + 2) + b;
        }

        function animation(currentTime) {
            if (startTime === null) startTime = currentTime;
            const timeElapsed = currentTime - startTime;
            const run = ease(timeElapsed, startPosition, distance, duration);
            window.scrollTo(0, run);
            if (timeElapsed < duration) raf(animation);
        }

        raf(animation);
    }

    // Enhanced Navbar with intelligent hide/show
    class SmartNavbar {
        constructor() {
            this.navbar = document.getElementById('navbar');
            if (!this.navbar) return;

            this.lastScrollTop = 0;
            this.scrollThreshold = 5;
            this.hideOffset = 100;
            
            this.init();
        }

        init() {
            let ticking = false;
            
            const handleScroll = () => {
                if (!ticking) {
                    raf(() => {
                        this.updateNavbar();
                        ticking = false;
                    });
                    ticking = true;
                }
            };

            window.addEventListener('scroll', throttle(handleScroll, 100));
            this.updateNavbar(); // Initial check
        }

        updateNavbar() {
            const scrollTop = window.pageYOffset || document.documentElement.scrollTop;
            
            // Add scrolled class for styling
            if (scrollTop > 50) {
                this.navbar.classList.add('scrolled');
            } else {
                this.navbar.classList.remove('scrolled');
            }

            // Smart hide/show
            if (Math.abs(scrollTop - this.lastScrollTop) > this.scrollThreshold) {
                if (scrollTop > this.lastScrollTop && scrollTop > this.hideOffset) {
                    // Scrolling down & past offset
                    this.navbar.style.transform = 'translateY(-100%)';
                } else {
                    // Scrolling up or at top
                    this.navbar.style.transform = 'translateY(0)';
                }
                this.lastScrollTop = scrollTop;
            }
        }
    }

    // Intersection Observer for animations
    class AnimationObserver {
        constructor() {
            this.animatedElements = document.querySelectorAll('.animate-on-scroll');
            if (this.animatedElements.length === 0) return;

            this.init();
        }

        init() {
            const options = {
                root: null,
                rootMargin: '0px',
                threshold: 0.1
            };

            const observer = new IntersectionObserver((entries) => {
                entries.forEach(entry => {
                    if (entry.isIntersecting) {
                        entry.target.classList.add('animated');
                        // Optional: Stop observing after animation
                        observer.unobserve(entry.target);
                    }
                });
            }, options);

            this.animatedElements.forEach(element => {
                observer.observe(element);
            });
        }
    }

    // Parallax Effect for hero section
    class ParallaxEffect {
        constructor() {
            this.parallaxElements = document.querySelectorAll('.parallax');
            if (this.parallaxElements.length === 0) return;

            this.init();
        }

        init() {
            window.addEventListener('scroll', throttle(() => {
                this.updateParallax();
            }, 16)); // ~60fps
        }

        updateParallax() {
            const scrolled = window.pageYOffset;
            
            this.parallaxElements.forEach(element => {
                const speed = element.dataset.speed || 0.5;
                const yPos = -(scrolled * speed);
                
                raf(() => {
                    element.style.transform = `translate3d(0, ${yPos}px, 0)`;
                });
            });
        }
    }

    // Typing Effect for hero text
    class TypeWriter {
        constructor(element, texts, speed = 100) {
            this.element = element;
            this.texts = texts;
            this.speed = speed;
            this.textIndex = 0;
            this.charIndex = 0;
            this.isDeleting = false;
            
            if (this.element && this.texts.length > 0) {
                this.type();
            }
        }

        type() {
            const currentText = this.texts[this.textIndex];
            
            if (this.isDeleting) {
                this.element.textContent = currentText.substring(0, this.charIndex - 1);
                this.charIndex--;
            } else {
                this.element.textContent = currentText.substring(0, this.charIndex + 1);
                this.charIndex++;
            }

            let typeSpeed = this.speed;

            if (this.isDeleting) {
                typeSpeed /= 2; // Faster when deleting
            }

            if (!this.isDeleting && this.charIndex === currentText.length) {
                typeSpeed = 2000; // Pause at end
                this.isDeleting = true;
            } else if (this.isDeleting && this.charIndex === 0) {
                this.isDeleting = false;
                this.textIndex = (this.textIndex + 1) % this.texts.length;
                typeSpeed = 500; // Pause before typing new text
            }

            setTimeout(() => this.type(), typeSpeed);
        }
    }

    // Smooth Counter Animation
    class CounterAnimation {
        constructor() {
            this.counters = document.querySelectorAll('.counter');
            if (this.counters.length === 0) return;

            this.init();
        }

        init() {
            const options = {
                root: null,
                rootMargin: '0px',
                threshold: 0.5
            };

            const observer = new IntersectionObserver((entries) => {
                entries.forEach(entry => {
                    if (entry.isIntersecting && !entry.target.classList.contains('counted')) {
                        this.animateCounter(entry.target);
                        entry.target.classList.add('counted');
                    }
                });
            }, options);

            this.counters.forEach(counter => {
                observer.observe(counter);
            });
        }

        animateCounter(element) {
            const target = parseInt(element.dataset.target);
            const duration = parseInt(element.dataset.duration) || 2000;
            const increment = target / (duration / 16); // 60fps
            let current = 0;

            const updateCounter = () => {
                current += increment;
                
                if (current < target) {
                    element.textContent = Math.ceil(current).toLocaleString();
                    raf(updateCounter);
                } else {
                    element.textContent = target.toLocaleString();
                }
            };

            updateCounter();
        }
    }

    // Mobile Menu Handler
    class MobileMenu {
        constructor() {
            this.menuToggle = document.getElementById('mobile-menu-toggle');
            this.navMenu = document.querySelector('.nav-menu');
            this.menuOpen = false;
            
            if (this.menuToggle && this.navMenu) {
                this.init();
            }
        }

        init() {
            this.menuToggle.addEventListener('click', () => this.toggleMenu());
            
            // Close menu when clicking outside
            document.addEventListener('click', (e) => {
                if (this.menuOpen && 
                    !this.menuToggle.contains(e.target) && 
                    !this.navMenu.contains(e.target)) {
                    this.closeMenu();
                }
            });

            // Close menu on escape key
            document.addEventListener('keydown', (e) => {
                if (e.key === 'Escape' && this.menuOpen) {
                    this.closeMenu();
                }
            });

            // Close menu when clicking on nav links
            this.navMenu.querySelectorAll('.nav-link').forEach(link => {
                link.addEventListener('click', () => this.closeMenu());
            });
        }

        toggleMenu() {
            this.menuOpen ? this.closeMenu() : this.openMenu();
        }

        openMenu() {
            this.menuOpen = true;
            this.navMenu.classList.add('active');
            this.menuToggle.classList.add('active');
            this.menuToggle.setAttribute('aria-expanded', 'true');
            
            // Prevent body scroll
            document.body.style.overflow = 'hidden';
        }

        closeMenu() {
            this.menuOpen = false;
            this.navMenu.classList.remove('active');
            this.menuToggle.classList.remove('active');
            this.menuToggle.setAttribute('aria-expanded', 'false');
            
            // Restore body scroll
            document.body.style.overflow = '';
        }
    }

    // Scroll to Top Button
    class ScrollToTop {
        constructor() {
            this.button = document.getElementById('scroll-to-top');
            if (!this.button) {
                this.createButton();
            }
            
            this.init();
        }

        createButton() {
            this.button = document.createElement('button');
            this.button.id = 'scroll-to-top';
            this.button.className = 'scroll-top';
            this.button.innerHTML = '<i class="fas fa-arrow-up"></i>';
            this.button.setAttribute('aria-label', 'Scroll to top');
            document.body.appendChild(this.button);
        }

        init() {
            // Show/hide button based on scroll
            window.addEventListener('scroll', throttle(() => {
                if (window.pageYOffset > 300) {
                    this.button.classList.add('visible');
                } else {
                    this.button.classList.remove('visible');
                }
            }, 200));

            // Scroll to top on click
            this.button.addEventListener('click', () => {
                smoothScrollTo('body', 800);
            });
        }
    }

    // Code Copy Functionality
    class CodeCopy {
        constructor() {
            this.codeBlocks = document.querySelectorAll('pre');
            if (this.codeBlocks.length === 0) return;

            this.init();
        }

        init() {
            this.codeBlocks.forEach(block => {
                const wrapper = document.createElement('div');
                wrapper.className = 'code-wrapper';
                block.parentNode.insertBefore(wrapper, block);
                wrapper.appendChild(block);

                const button = document.createElement('button');
                button.className = 'copy-code-btn';
                button.innerHTML = '<i class="fas fa-copy"></i>';
                button.setAttribute('aria-label', 'Copy code');
                wrapper.appendChild(button);

                button.addEventListener('click', () => this.copyCode(block, button));
            });
        }

        async copyCode(block, button) {
            const code = block.textContent;
            
            try {
                await navigator.clipboard.writeText(code);
                this.showSuccess(button);
            } catch (err) {
                // Fallback for older browsers
                const textarea = document.createElement('textarea');
                textarea.value = code;
                textarea.style.position = 'fixed';
                textarea.style.opacity = '0';
                document.body.appendChild(textarea);
                textarea.select();
                document.execCommand('copy');
                document.body.removeChild(textarea);
                this.showSuccess(button);
            }
        }

        showSuccess(button) {
            const originalHTML = button.innerHTML;
            button.innerHTML = '<i class="fas fa-check"></i>';
            button.classList.add('copied');
            
            setTimeout(() => {
                button.innerHTML = originalHTML;
                button.classList.remove('copied');
            }, 2000);
        }
    }

    // Performance Monitor (Development only)
    class PerformanceMonitor {
        constructor() {
            if (window.location.hostname === 'localhost' || 
                window.location.hostname === '127.0.0.1') {
                this.monitor();
            }
        }

        monitor() {
            // Log performance metrics
            window.addEventListener('load', () => {
                const perfData = performance.getEntriesByType('navigation')[0];
                console.log('Page Load Performance:');
                console.log(`DNS: ${perfData.domainLookupEnd - perfData.domainLookupStart}ms`);
                console.log(`TCP: ${perfData.connectEnd - perfData.connectStart}ms`);
                console.log(`Request: ${perfData.responseStart - perfData.requestStart}ms`);
                console.log(`Response: ${perfData.responseEnd - perfData.responseStart}ms`);
                console.log(`DOM Processing: ${perfData.domComplete - perfData.domInteractive}ms`);
                console.log(`Total Load Time: ${perfData.loadEventEnd - perfData.fetchStart}ms`);
            });
        }
    }

    // Initialize all modules when DOM is ready
    function initializeApp() {
        // Core functionality
        new SmartNavbar();
        new AnimationObserver();
        new ParallaxEffect();
        new MobileMenu();
        new ScrollToTop();
        new CodeCopy();
        new CounterAnimation();

        // Typography effects
        const heroSubtitle = document.querySelector('.hero-subtitle-dynamic');
        if (heroSubtitle) {
            new TypeWriter(heroSubtitle, [
                'High-Performance Blockchain',
                'Production-Ready Implementation',
                'Enterprise-Grade Solution'
            ], 100);
        }

        // Smooth scrolling for anchor links
        document.querySelectorAll('a[href^="#"]').forEach(anchor => {
            anchor.addEventListener('click', function(e) {
                const href = this.getAttribute('href');
                if (href !== '#' && href !== '#0') {
                    e.preventDefault();
                    smoothScrollTo(href);
                }
            });
        });

        // Add loading class removal
        document.body.classList.add('loaded');

        // Performance monitoring (dev only)
        new PerformanceMonitor();

        // Log successful initialization
        console.log('%c🚀 Neo C++ Website Initialized', 
                    'color: #00e599; font-size: 16px; font-weight: bold;');
    }

    // Start when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initializeApp);
    } else {
        initializeApp();
    }

    // Export for potential external use
    window.NeoApp = {
        smoothScrollTo,
        debounce,
        throttle
    };

})();